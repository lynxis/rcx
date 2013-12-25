/*
 *  RCX_Request_Reply.c
 *
 *  A program to communicate with the RCX Executive from a UNIX system.
 *  A simple request/reply protocol is used e.g. to ask if the RCX 
 *  is running or to download a program from the UNIX system. 
 *
 *  Under UNIX systems like IRIX, Linux, and Solaris, this program compiles
 *  with gcc RCX_Request_Reply.c -o RCX_Request_Reply.
 *
 *  Set DEFAULT_RCX_IR to the name of the RS232 port connected to the 
 *  infrared transmitter/receiver. Set the RCX_IR environment variable 
 *  to override DEFAULT_RCX_IR.
 *
 *  To obtained a detailed knowledge of the different protocol layers,
 *  insert calls to the routine print_sequence in the routine send_receive
 *  to watch the packing and unpacking of requests and replies.
 *
 *
 *  Acknowledgements:
 *  
 *  Kekoa Proudfoot (kekoa@graphics.stanford.edu) has provided almost all
 *  the information needed to write this program at:
 *  
 *     http://graphics.stanford.edu/~kekoa/rcx/
 *
 *  He has also written and documented a program, send.c, similar in 
 *  functionality to the program RCX_Request_Reply.c. Minor parts of the 
 *  code in RCX_Request_Reply.c have been copied from send.c.
 *------------------------------------------------------------------------  
 */

#include <stdio.h>      /* printf                                        */
#include <stdlib.h>     /* strtol, getenv, exit                          */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>      /* open, close, read, write                      */

#include <termios.h>    /* termios, and related routines like tcsetattr 
                           and related constants like B2400.             */
#include <string.h>     /* memset                                        */

/*------------------------------------------------------------------------ 
 * RCX infrared routines. 
 *
 * RCX_IR_open:  returns a file descriptor for read/write access to the
 *               RS232 port connected to the RCX infrared 
 *               transmitter/receiver.
 * RCX_IR_close: close usage of the file descriptor.
 *------------------------------------------------------------------------
 */

#define DEFAULT_RCX_IR   "/dev/term/a"    /* Solaris name of serial port */
      
/* Linux - COM1 port is /dev/ttyS0 */
/* SGI port is          /dev/ttyd2 */

int RCX_IR_open()
{
    int fd;
    char * RCX_IR_Name;
    struct termios ios;

    RCX_IR_Name = getenv("RCX_IR");
    if (RCX_IR_Name == NULL) RCX_IR_Name = DEFAULT_RCX_IR;

    fd = open(RCX_IR_Name, O_RDWR);
    if ( fd < 0) {
       printf("Open Infraread failed. Name of RCX_IR = %s. \n", RCX_IR_Name);
       exit(1);
    }

    if (!isatty(fd)) {
       close(fd);
       printf("RCX_IR = %s is not the name of a serial port.\n", RCX_IR_Name);
       exit(1);
    }

    /* Allocate memory for ios and initialize with zeroes. 
       Set serial port mode to 8 bit characters (bytes), odd parity, and local 
       (no modem) with read enabled. Set input and output baud rate to 
       2400 bit/sec.
    */
    memset(&ios, 0, sizeof(ios));
    ios.c_cflag = CREAD | CLOCAL | CS8 | PARENB | PARODD;
    cfsetispeed(&ios, B2400);
    cfsetospeed(&ios, B2400);

    /* Non-canonical mode input processing: A read is satisfied as soon as 
       a single byte is received or the read timer expires. If no byte is 
       received within 0.1 seconds after the read is initiated, the read 
       returns with zero bytes.
    */
    ios.c_cc[VTIME] = 1; /* read timer in units of 0.1 sec. */
    ios.c_cc[VMIN]  = 0; /* return after one byte is received. */

    if (tcsetattr(fd, TCSANOW, &ios) == -1) {
	printf("tcsetattr failed.\n");
	exit(1);
    }

    return fd;
}

void RCX_IR_close(int fd)
{
    close(fd);
}


/*-----------------------------------------------------------------------
 * Definition of bytesequence as an array of bytes together with 
 * the length of the sequence.
 * 
 * print_sequence: 
 * A routine to print a bytesequence as hexadecimal represented 
 * byte values. Each line starts with a hexadecimal represented 
 * byte index of the first byte on the line. Bytes are grouped with 
 * spacing between groups.
 *-----------------------------------------------------------------------
 */
typedef unsigned char    byte ;

#define MAXSIZE          4096
struct byteseq_t         { byte data[MAXSIZE];
                          int bytecount; 
                          };
typedef struct byteseq_t bytesequence;

#define LINE_SIZE   16
#define GROUP_SIZE   4

void print_sequence(bytesequence bs)
{
    int i, j, w;

    for (i = 0; i < bs.bytecount; i += w) {
       w = bs.bytecount - i;
       if (w > LINE_SIZE)
	   w = LINE_SIZE;
       printf("%04x: ", i);
       for (j = 0; j < w; j++) {
	   printf("%02x ", bs.data[i+j]);
	   if ((j + 1) % GROUP_SIZE == 0)
	       printf(" ");
       }
       printf("\n");
    }
}


/*------------------------------------------------------------------------
 * Request/reply protocol:
 * 
 * A request/reply packet is a packet with the format:
 *
 *   A header of two bytes:  0x55 0xff.
 *   A sequence of request/reply bytes.
 *   A checksum: The checksum is calculated as the 
 *   sum of the request/reply bytes modulo 8 bit.
 *
 * The result of sending a request to the RCX is either a correct received 
 * reply consisting of a sequence of bytes or some error has occurred 
 * during the transmission of request/reply. 
 * 
 *------------------------------------------------------------------------
 */
typedef bytesequence     packet;
typedef bytesequence     request;

enum result_t  { REPLY_OK, 
                 NO_ECHO, SHORT_ECHO, BAD_ECHO, ECHO_OK_NO_RESPONSE, 
                 ECHO_OK, BAD_LENGTH, BAD_HEADER, BAD_BIT_COMPLEMENT, 
                 BAD_CHECKSUM 
                };

typedef enum result_t    result;
struct  reply_t          { bytesequence bs;
                           result  res; 
                          };

typedef struct reply_t   reply;

/*-------------------------------------------------------------------------
 * build_packet:
 * Adds a header of two bytes and a checksum to a request to form 
 * a request packet.
 *-------------------------------------------------------------------------
 */
packet build_packet(request req)
{   
    packet pac;
    int i; 
    byte check_sum;

    check_sum = 0;

    pac.data[0] = 0x55;
    pac.data[1] = 0xff;
    for (i = 0; i < req.bytecount ; i++){
	pac.data[2+i] =  req.data[i];
	check_sum    +=  req.data[i];
    }
    pac.data[2+i] = check_sum;
    pac.bytecount = 3 + req.bytecount;

    return pac;
}

/*---------------------------------------------------------------------------
 * check_reply:
 * Checks if a packet received has the correct format.
 * Returns a possible reply together with a result indicating 
 * the success of receiving a reply or the course of failure.
 *---------------------------------------------------------------------------
 */
reply check_reply(reply rep_pac)
{
    int i;
    byte sum;
    reply rep;

    /* Check if the number of bytes received is 
       at least 2 ( length of header ) + 1 ( lenght of checksum ) 
    */
    if ( rep_pac.bs.bytecount < 3 )
       rep.res = BAD_LENGTH; 
    else {
       /* Check header */
       if ((rep_pac.bs.data[0] != 0x55) || (rep_pac.bs.data[1] != 0xff))
          rep.res = BAD_HEADER;
       else {
	  /* Calculate checksum of the bytes excluding the checksum byte.
             Copy reply bytes of packet  to a reply.
          */
          sum = 0;
          for ( i = 0; i < rep_pac.bs.bytecount-3; i++){ 
             sum           += rep_pac.bs.data[2+i];
             rep.bs.data[i] = rep_pac.bs.data[2+i];
          }
          /* Check checksum */
          if (rep_pac.bs.data[rep_pac.bs.bytecount-1] != sum)
             rep.res = BAD_CHECKSUM;
          else 
             rep.res = REPLY_OK;
          rep.bs.bytecount = rep_pac.bs.bytecount-3;
       }
    }
    return rep;
}

/*---------------------------------------------------------------------------
 * IR protocol:
 *
 * Each byte in a packet except the first byte, is sent as two bytes, 
 * the byte itself and its bit-complement. This is to balance the number of
 * zeroes and ones in the infrared transmission of a bytesequence.
 *
 * The IR transmitter/receiver on the UNIX system echoes the bytesequence
 * sent. When a byte sequence has been received it is checked for a correct 
 * echo.
 *---------------------------------------------------------------------------
 */

typedef bytesequence IR_packet;
typedef reply        IR_reply;

/*--------------------------------------------------------------------------
 * build_IR_packet:
 * Build a packet ready for IR transmission.
 *--------------------------------------------------------------------------
 */
IR_packet build_IR_packet(packet pac)
{  
   int i, j;
   IR_packet IR_pac;

   IR_pac.data[0] = pac.data[0];
   for (i = 1, j = 1; i < pac.bytecount; i++, j = j+2){
       IR_pac.data[j]   =  pac.data[i];
       IR_pac.data[j+1] = ~pac.data[i];
   }
   IR_pac.bytecount = j;

   return IR_pac;
}

/*-------------------------------------------------------------------------- 
 * check_and_remove_bit_complements:
 * A routine that checks bytepairs in an IR_reply for bit-complement 
 * equality starting with the second byte. Copy the first byte and 
 * the first byte of each bytepair into a reply which is returned.
 *--------------------------------------------------------------------------
 */
reply check_and_remove_bit_complements(IR_reply IR_rep)
{ 
   reply rep;
   int i, j;
      
   rep.bs.data[0] = IR_rep.bs.data[0];
   for (i = 1, j = 1; (i < IR_rep.bs.bytecount-1); i = i+2, j++){
      if (IR_rep.bs.data[i] != (byte)~IR_rep.bs.data[i+1])
         break;
      rep.bs.data[j] = IR_rep.bs.data[i];
   }
   if ( i < IR_rep.bs.bytecount )
      rep.res = BAD_BIT_COMPLEMENT;
   else 
      rep.res = REPLY_OK;
   rep.bs.bytecount = j;

   return rep;
}

/*-------------------------------------------------------------------------
 * Routines to send/receive bytesequences on the RS232 port.
 *
 * send_IR_packet: sends a bytesequence
 *
 * receive_bytes:  reads single bytes until a read timer of 0.1 s
 *                 has expired. This is taken as the end-of-sequence.
 *                 The bytes are returned as a bytesequence.
 *-----------------------------------------------------------------------
 */
void send_IR_packet(int fd, IR_packet pac)
{   
    if (write(fd, pac.data, pac.bytecount)!= pac.bytecount) {
	printf("Error in write.\n");
	exit(1);
    } 
}

bytesequence receive_bytes(int fd)
{

    bytesequence bs;
    int count, received; 

    received = 0;
    do {
       count = read(fd, &bs.data[received], 1);
       /* count= 1, one character received, 
          count= 0, timeout, end of bytesequence, 
          count=-1, error. 
       */
       if ( count == -1) {
          printf("Error in read.\n");
          exit(1);
       }
       received = received + count; 
    } while ( count != 0);

    bs.bytecount = received;
    return bs;
}

/*-----------------------------------------------------------------------
 * check_and_remove_echo:
 * A routine that checks the first part of a bytesequence for echo
 * from the IR transmitter/receiver. The IR packet sent is compared 
 * with the first part of the bytesequence received. The echo is 
 * removed and the rest of the bytes is returned as a reply with
 * a result indicating the success or failure of the echo comparison.
 *-----------------------------------------------------------------------
 */
IR_reply check_and_remove_echo(bytesequence bs, IR_packet IR_pac)
{    
    int i, j;
    IR_reply IR_rep;

    /* Empty bytesequence returned if echo not ok.*/
    IR_rep.bs.bytecount = 0;

    /* Check echo from IR device */
    if ( bs.bytecount == 0 )
       IR_rep.res = NO_ECHO;
    else {
    if ( bs.bytecount < IR_pac.bytecount )
       IR_rep.res = SHORT_ECHO;
    else {
       /* Check bytes in echo */
       for (i = 0; ((i < IR_pac.bytecount) && 
                    (bs.data[i] == IR_pac.data[i])); i++)
       ;
       if ( i < IR_pac.bytecount ) 
          IR_rep.res = BAD_ECHO;
       else 
          if ( bs.bytecount == IR_pac.bytecount )
             IR_rep.res = ECHO_OK_NO_RESPONSE;
          else {
             IR_rep.res = ECHO_OK;
             /* Copy the rest of the bytesequence to IR_rep. */
             for (i = 0, j = IR_pac.bytecount; j < bs.bytecount; i++,j++)
                IR_rep.bs.data[i] = bs.data[j];
             IR_rep.bs.bytecount = i;
          }
       }
    }
    return IR_rep;
}


/*-----------------------------------------------------------------------
 * assemble_request: 
 * Assembles a request from the program arguments. Each argument 
 * is interpreted as a two digit hexadecimal represented byte. The bytes 
 * and the number of bytes assembled are returned. No error control.
 *-----------------------------------------------------------------------
 */
request assemble_request(int argc, char * argv[])
{   request req;
    int i;

    for (i = 1; i < argc; i++)
        req.data[i-1] = strtol(argv[i], NULL, 16);
    req.bytecount = argc - 1;

    return req;
} 

/*-----------------------------------------------------------------------
 * send_receive:
 * Transformes a request into an IR_packet which is written to the 
 * RS232 port and hopefully received by the RCX Executive.
 * Then the routine blocks until a sequence of bytes has been received 
 * on the RS232 port. This sequence is checked for echo from the IR
 * transmitter/receiver, and the echo is removed from the bytesequence.
 * The resulting bytesequence is checked for bit-complemented bytes,
 * and the bit-complements are removed. The remaining bytes are checked 
 * against the packet format and a possible reply is returned together 
 * with a result indicating the success of receiving a reply or the course 
 * of failure.
 *-----------------------------------------------------------------------
 */      
reply send_receive(request req)
{   
    int fd; 
    packet       req_pac;
    IR_packet    IR_req_pac;
    bytesequence bs;
    IR_reply     IR_rep_pac;
    reply        rep_pac;
    reply        rep; 

    fd = RCX_IR_open();

    req_pac    = build_packet(req);
    IR_req_pac = build_IR_packet(req_pac);
    send_IR_packet(fd, IR_req_pac);
    bs         = receive_bytes(fd);
    IR_rep_pac = check_and_remove_echo(bs,IR_req_pac);
    if ( IR_rep_pac.res != ECHO_OK )
       rep.res = IR_rep_pac.res;
    else {
       rep_pac = check_and_remove_bit_complements(IR_rep_pac);
       if ( rep_pac.res != REPLY_OK )
          rep.res = rep_pac.res;
       else
          rep  = check_reply(rep_pac);
    }
    
    RCX_IR_close(fd);

    return rep;
}


void print_result(reply rep)
{
    switch ( rep.res ) {
    case REPLY_OK:            print_sequence(rep.bs);            break;
    case NO_ECHO:             printf("No echo.\n");              break;
    case BAD_ECHO:            printf("Bad echo.\n");             break;
    case ECHO_OK_NO_RESPONSE: printf("Echo ok. No response.\n"); break;
    case ECHO_OK:             printf("Echo ok.\n");              break;  
    case BAD_LENGTH:          printf("Bad length.\n");           break;
    case BAD_HEADER:          printf("Bad header.\n");           break;
    case BAD_BIT_COMPLEMENT:  printf("Bad bit complement.\n");   break;
    case BAD_CHECKSUM:        printf("Bad checksum.\n");         break;
    default: break;
    }
}


int main (int argc, char * argv[]) {

    request req;
    reply   rep;

    /* Print usage if no arguments. */
    if (argc == 1) {
	printf("usage: %s byte [byte ...]\n", argv[0]);
	exit(1);
    }

    /* Assemble request for the RCX Executive from the program arguments. */
    req = assemble_request(argc,argv);

    /* Send request and receive reply. */
    rep = send_receive(req);

    /* Print result of sending a request to the RCX Executive. */
    print_result(rep);

    exit(0);
}

