/*
 *  RCX_Download.c
 *
 *  Download an S-record program to the RCX and starts it.
 *
 *
 *  Under UNIX systems like IRIX, Linux, and Solaris, this program compiles
 *  with gcc RCX_Download.c -o RCX_Download.
 *
 *  Set DEFAULT_RCX_IR to the name of the RS232 port connected to the 
 *  infrared transmitter/receiver. Set the RCX_IR environment variable 
 *  to override DEFAULT_RCX_IR.
 *
 *  Acknowledgements:
 *  
 *  Kekoa Proudfoot (kekoa@graphics.stanford.edu) has provided almost all
 *  the information needed to write this program at:
 *  
 *     http://graphics.stanford.edu/~kekoa/rcx/
 *
 *  He has also written and documented a program, firmdl.c, similar in 
 *  functionality to the program RCX_Download.c. Minor parts of the 
 *  code in RCX_Download.c have been copied from firmdl.c.
 *------------------------------------------------------------------------  
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include <string.h>

/*
 *  RCX routines.
 */

/*
Definition of bytesequence as an array of bytes together with the length of the sequence.
*/

#define MAXSIZE       4096
typedef unsigned char byte ;

struct byteseq_t { byte data[MAXSIZE];
                   int bytecount; 
                 };
typedef struct byteseq_t bytesequence;

/* 
Print byte sequence as hexadecimal represented byte values. Each
line starts with a hexadecimal represented byte index of the first
byte on the line. Bytes are grouped with spacing between groups.
*/

#define LINE_SIZE   16
#define GROUP_SIZE  4

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

typedef bytesequence     message;

enum result_types { OK, 
                    NO_ECHO, BAD_ECHO, ECHO_OK_NO_RESPONSE, ECHO_OK,  
                    BAD_LENGTH, BAD_HEADER, BAD_PARITY, BAD_CHECKSUM, 
                    BAD_ANSWER};
struct answer_t   { bytesequence bs;
                    int  result; };

typedef struct answer_t  answer;


/* 
Assemble message from arguments. Each argument is interpreted as a two 
digit hexadecimal represented byte. The bytes and the number of bytes 
assembled are returned. No error control.
*/
message assemble_message(int argc, char * argv[])
{  message m;
   int i;

    for ( i = 1; i < argc; i++)
        m.data[i-1] = strtol(argv[i], NULL, 16);
    m.bytecount = argc - 1;

    return m;
} 

     

/*
Send message to RCX and return answer received. 
*/

/* RS232 routines. */
 

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>

#define DEFAULT_RCX_IR   "/dev/term/a"    /* Solaris name of serial port */
      
/* Linux - COM1 port is /dev/ttyS0 */
/* SGI port is          /dev/ttyd2 */
/* Solaris port is      /dev/term/a */

int IR_open()
{
    int fd;
    char * IR_Name;
    struct termios ios;

    if (( IR_Name = getenv("RCX_IR")) == NULL)
        IR_Name = DEFAULT_RCX_IR;

    if ((fd = open(IR_Name, O_RDWR)) < 0) {
	printf("Open Infraread failed. Name of RCX_IR = %s. \n", IR_Name);
	exit(1);
    }

    if (!isatty(fd)) {
	close(fd);
	printf("IR = %s is not the name of a serial port.\n", IR_Name);
	exit(1);
    }

    memset(&ios, 0, sizeof(ios));
    ios.c_cflag = CREAD | CLOCAL | CS8 | PARENB | PARODD;
    cfsetispeed(&ios, B2400);
    cfsetospeed(&ios, B2400);

    ios.c_cc[VTIME] = 1;
    ios.c_cc[VMIN]  = 0;

    if (tcsetattr(fd, TCSANOW, &ios) == -1) {
	perror("tcsetattr");
	exit(1);
    }

    return fd;
}

void IR_close(int fd)
{
    close(fd);
}



/* An RCX message consists of a sequence of bytes with the format:

   A message header with three bytes:  0x55 0xff 0x00;

   A sequence of data bytes, each byte is send as two bytes, the byte 
   itself and its bitcomplement;

   A check sum and its bitcomplemnt. The check sum is defined as the 
   sum of the data bytes modulo 8 bit.          
*/

message build_RCX_message(message m)
{   message RCX_m;
    int i,j; 
    byte check_sum;

    check_sum = 0;
    RCX_m.data[0] = 0x55;
    RCX_m.data[1] = 0xff;
    RCX_m.data[2] = 0x00;
    for ( i = 0, j = 3; i < m.bytecount ; i++, j=j+2){
	RCX_m.data[j]   =  m.data[i];
	RCX_m.data[j+1] = ~m.data[i];
	check_sum      +=  m.data[i];
    }
    RCX_m.data[j]   =  check_sum;
    RCX_m.data[j+1] = ~check_sum;
    RCX_m.bytecount = j + 2;

    return RCX_m;

}

void send_message(int fd, message m)
{   
    if (write(fd, m.data, m.bytecount)!= m.bytecount) {
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
       if ((count = read(fd, &bs.data[received], 1)) == -1) {
          printf("Error in read.\n");
          exit(1);
       }
       received = received + count; 
    } while ( count != 0);

    bs.bytecount = received;
    return bs;
}



answer find_answer(bytesequence bs, message m)
{
    int i, j, a_index;
    byte sum;
    answer a;

    /* Check echo from IR device */
    if ( bs.bytecount == 0 )
       a.result = NO_ECHO;
    else {
    if ( bs.bytecount < m.bytecount )
       a.result = BAD_ECHO;
    else {
       /* Check bytes in echo */
       for ( i = 0; ( (i < m.bytecount) && (bs.data[i] == m.data[i])); i++)
       ;
       if ( i < m.bytecount ) 
          a.result = BAD_ECHO;
       else 
          if ( bs.bytecount == m.bytecount )
             a.result = ECHO_OK_NO_RESPONSE;
          else {
             a.result = ECHO_OK;
             a_index = m.bytecount;
          }
       }
    }

    /* Check response if echo is ok and the number of bytes received is 
       at least 3 ( length of header ) + 2 ( lenght of checksum ) 
    */
    if ( (a.result != ECHO_OK) || ((bs.bytecount - a_index) < 5) )
       { if (a.result == ECHO_OK) a.result = BAD_LENGTH; }
    else {
       /* Check header */
       if ( (bs.data[a_index]   != 0x55) ||
            (bs.data[a_index+1] != 0xff) ||
            (bs.data[a_index+2] != 0x00)
          )
          a.result = BAD_HEADER;
       else {
	 /* Check bytepairs for bitcomplement equality. 
            Copy first byte of each pair to answer.
            Calculate checksum of the bytes (first byte of each byte pair)
            excluding the checksum byte.
         */
         sum = 0;
         for ( i = a_index+3, j = 0; 
               ( (i < bs.bytecount-2) && (bs.data[i] == (byte)~bs.data[i+1]) ); 
               i=i+2, j=j+1) 
         {
             sum += bs.data[i];
             a.bs.data[j] = bs.data[i];
         }
         if ( i < (bs.bytecount-2) )
            a.result = BAD_PARITY;
	 else  
	    /* Check checksum */
            if ( (bs.data[bs.bytecount-2] != (byte)~bs.data[bs.bytecount-1]) ||
                 (bs.data[bs.bytecount-2] != sum)
               )
               a.result = BAD_CHECKSUM;
            else {
               a.result = OK;
               a.bs.bytecount = j;
            }
       }
    }
    return a;
}

answer send_receive(int fd, message m)
{   
    bytesequence bs;
    answer a;
    int i;

    m = build_RCX_message(m);

    i=0;
    do {
       send_message(fd,m);
       bs = receive_bytes(fd);
       a  = find_answer(bs,m);
       i++;
    } while ((a.result !=OK) && (i < 5));
    
    return a;
}

void print_answer(answer a)
{
    switch ( a.result ) {
    case OK:                  print_sequence(a.bs);              break;
    case NO_ECHO:             printf("No echo.\n");              break;
    case BAD_ECHO:            printf("Bad echo.\n");             break;
    case ECHO_OK_NO_RESPONSE: printf("Echo ok. No response.\n"); break;
    case ECHO_OK:             printf("Echo ok.\n");              break;  
    case BAD_LENGTH:          printf("Bad length.\n");           break;
    case BAD_HEADER:          printf("Bad header.\n");           break;
    case BAD_PARITY:          printf("Bad parity.\n");           break;
    case BAD_CHECKSUM:        printf("Bad checksum.\n");         break;
    default: break;
    }
}

/* Delete firmware */
int delete_firmware(int fd)
{   
    message m;
    answer  a;

    m.bytecount = 6;
    m.data[0]   = 0x65;
    m.data[1]   = 1;
    m.data[2]   = 3;
    m.data[3]   = 5;
    m.data[4]   = 7;
    m.data[5]   = 11;

    a = send_receive(fd, m);

    if ( (a.result == OK) && (a.bs.bytecount != 1))
       a.result = BAD_ANSWER;
    
    return a.result;
}

/* Start firmware download */
int start_firmware_download(int fd, int image_start, int check_sum)
{
    message m;
    answer  a;

    m.bytecount = 6;
    m.data[0]   = 0x75;
    m.data[1]   = (image_start >> 0) & 0xff;
    m.data[2]   = (image_start >> 8) & 0xff;
    m.data[3]   = (check_sum >> 0) & 0xff;
    m.data[4]   = (check_sum >> 8) & 0xff;
    m.data[5]   = 0;

    a = send_receive(fd, m);

    if ( (a.result == OK) && (a.bs.bytecount != 2))
       a.result = BAD_ANSWER;
    
    return a.result;
}

/* Transfer data */

#define IMAGE_START   0x8000
#define IMAGE_LEN     0x4c00
#define IMAGE_END     (IMAGE_START + IMAGE_LEN)
#define TRANSFER_SIZE 0xc8

int transfer_data( int fd, byte * image, int length)
{
    message m;
    answer  a;
    int addr, sequence_number, size, i;
    byte check_sum;

    addr = 0;
    sequence_number= 1;
    do {
	size = length - addr;
        /* Toggle bit 3 of command byte as bit 0 of the block sequence number */
	m.data[0] = 0x45 | ((sequence_number & 1) << 3);
	if (size > TRANSFER_SIZE)
	    size = TRANSFER_SIZE;
	else
	    /* Last block has sequence number equal to 0 */
	    sequence_number = 0;
	m.data[1] = sequence_number;
	m.data[2] = sequence_number >> 8;
	m.data[3] = size;
	m.data[4] = size >> 8;
	memcpy(&m.data[5], &image[addr], size);
        check_sum = 0;
	for (i = 0; i < size; i++)
	    check_sum += m.data[5 + i];
	m.data[size + 5] = check_sum;
        m.bytecount = size + 5 + 1;
        
        a = send_receive(fd, m);

        if ( (a.result == OK) && (a.bs.bytecount != 2))
           a.result = BAD_ANSWER;

        addr += size; sequence_number++;
    } while ( (addr < length) && (a.result == OK)); 

    return a.result;
}

/* Unlock firmware */
int unlock_firmware(int fd)
{
    message m;
    answer  a;

    m.bytecount = 6;
    m.data[0] = 0xa5;
    m.data[1] = 76;
    m.data[2] = 69;
    m.data[3] = 71;
    m.data[4] = 79;
    m.data[5] = 174;

    a = send_receive(fd, m);

    if ((a.result == OK) && (a.bs.bytecount != 26))
       a.result = BAD_ANSWER;

    return a.result;
}

char *progname;




/* S-record routines */

/* srec.h */

typedef struct {
    unsigned char type;
    unsigned long addr;
    unsigned char count;
    unsigned char data[32];
} srec_t;

#define S_OK               0
#define S_NULL            -1
#define S_INVALID_HDR     -2
#define S_INVALID_CHAR    -3
#define S_INVALID_TYPE    -4
#define S_TOO_SHORT       -5
#define S_TOO_LONG        -6
#define S_INVALID_CKSUM   -7

extern int srec_decode(srec_t *srec, char *line);
extern int srec_encode(srec_t *srec, char *line);

/* srec.c */

static signed char ctab[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,  -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,  -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,  -1,-1,-1,-1,-1,-1,-1,-1,
     0, 1, 2, 3, 4, 5, 6, 7,   8, 9,-1,-1,-1,-1,-1,-1,
     0,10,11,12,13,14,15,-1,  -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,  -1,-1,-1,-1,-1,-1,-1,-1,
     0,10,11,12,13,14,15,-1,  -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,  -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,  -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,  -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,  -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,  -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,  -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,  -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,  -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,  -1,-1,-1,-1,-1,-1,-1,-1,
};

static int ltab[10] = {4,4,6,8,0,4,0,8,6,4};

#define C1(l,p)  (ctab[l[p]])
#define C2(l,p)  ((C1(l,p)<<4)|C1(l,p+1))

int
srec_decode(srec_t *srec, char *_line)
{
    int len, pos = 0, count, alen, sum = 0;
    unsigned char *line = (unsigned char *)_line;

    if (!srec || !line)
	return S_NULL;

    for (len = 0; line[len]; len++)
	if (line[len] == '\n' || line[len] == '\r')
	    break;

    if (len < 4)
	return S_INVALID_HDR;

    if (line[0] != 'S')
	return S_INVALID_HDR;

    for (pos = 1; pos < len; pos++) {
	if (C1(line, pos) < 0)
	    return S_INVALID_CHAR;
    }

    srec->type = C1(line, 1);
    count = C2(line, 2);

    if (srec->type > 9)
	return S_INVALID_TYPE;
    alen = ltab[srec->type];
    if (alen == 0)
	return S_INVALID_TYPE;
    if (len < alen + 6 || len < count * 2 + 4)
	return S_TOO_SHORT;
    if (count > 37 || len > count * 2 + 4)
	return S_TOO_LONG;

    sum += count;

    len -= 4;
    line += 4;

    srec->addr = 0;
    for (pos = 0; pos < alen; pos += 2) {
	unsigned char value = C2(line, pos);
	srec->addr = (srec->addr << 8) | value;
	sum += value;
    }

    len -= alen;
    line += alen;

    for (pos = 0; pos < len - 2; pos += 2) {
	unsigned char value = C2(line, pos);
	srec->data[pos / 2] = value;
	sum += value;
    }

    srec->count = count - (alen / 2) - 1;

    sum += C2(line, pos);

    if ((sum & 0xff) != 0xff)
	return S_INVALID_CKSUM;

    return S_OK;
}

int
srec_encode(srec_t *srec, char *line)
{
    int alen, count, sum = 0, pos;

    if (srec->type > 9)
	return S_INVALID_TYPE;
    alen = ltab[srec->type];
    if (alen == 0)
	return S_INVALID_TYPE;

    line += sprintf(line, "S%d", srec->type);

    if (srec->count > 32)
	return S_TOO_LONG; 
    count = srec->count + (alen / 2) + 1;
    line += sprintf(line, "%02X", count);
    sum += count;

    while (alen) {
	int value;
	alen -= 2;
	value = (srec->addr >> (alen * 4)) & 0xff;
	line += sprintf(line, "%02X", value);
	sum += value;
    }

    for (pos = 0; pos < srec->count; pos++) {
	line += sprintf(line, "%02X", srec->data[pos]);
	sum += srec->data[pos];
    }

    sprintf(line, "%02X\n", (~sum) & 0xff);

    return S_OK;
}


#ifdef  FORCE_NO_ZERO_PADDING
#define STRIP_ZEROS   1
#else
#define STRIP_ZEROS   0
#endif

int main(int argc, char * argv[])
{
    unsigned char image[IMAGE_LEN];
    char buf[256];
    FILE *file;
    srec_t srec;
    int line = 0;
    unsigned short cksum = 0;
    int addr, index, size, i;
    int fd;
    int length = 0;
    int strip = STRIP_ZEROS;
    unsigned short image_start = IMAGE_START;

    progname = argv[0];

    if (argc != 2) {
	fprintf(stderr, "usage: %s filename\n", argv[0]);
	exit(1);
    }

    if ((file = fopen(argv[1], "r")) == NULL) {
	fprintf(stderr, "%s: failed to open\n", argv[1]);
	exit(1);
    }

    /* Build an image of the srecord data */

    memset(image, 0, sizeof(image));

    while (fgets(buf, sizeof(buf), file)) {
	int error;
	line++;
	/* Skip blank lines */
	for (i = 0; buf[i]; i++)
	    if (buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\n' &&
		buf[i] != '\r')
		break;
	if (!buf[i])
	    continue;
	if ((error = srec_decode(&srec, buf)) < 0) {
	    char *errstr = NULL;
	    switch (error) {
	    case S_NULL: errstr = "null string error"; break;
	    case S_INVALID_HDR: errstr = "invalid header"; break;
	    case S_INVALID_CHAR: errstr = "invalid character"; break;
	    case S_INVALID_TYPE: errstr = "invalid type"; break;
	    case S_TOO_SHORT: errstr = "line too short"; break;
	    case S_TOO_LONG: errstr = "line too line"; break;
	    case S_INVALID_CKSUM: break; /* ignore these */
	    default: errstr = "unknown error"; break;
	    }
	    if (errstr) {
		fprintf(stderr, "%s: %s on line %d\n", argv[1], errstr, line);
		exit(1);
	    }
	}
	if (srec.type == 0) {
	    if (srec.count == 16)
		if (!strncmp(srec.data, "?LIB_VERSION_L00", 16))
		    strip = 1;
	}
	else if (srec.type == 1) {
	    if (srec.addr < IMAGE_START || srec.addr + srec.count > IMAGE_END){
		fprintf(stderr, "%s: address out of bounds on line %d\n",
			argv[1], line);
		exit(1);
	    }
	    if (!strip && (srec.addr + srec.count - IMAGE_START > length))
		length = srec.addr + srec.count - IMAGE_START;
	    memcpy(&image[srec.addr - IMAGE_START], &srec.data, srec.count);
	}
	else if (srec.type == 9) {
	    if (srec.addr < IMAGE_START || srec.addr > IMAGE_END) {
		fprintf(stderr, "%s: address out of bounds on line %d\n",
			argv[1], line);
		exit(1);
	    }
	    image_start = srec.addr;
	}
    }

    /* Find image length */



    if (strip) {
	for (length = IMAGE_LEN - 1; length >= 0 && image[length]; length--);
	length++;
    }

    if (length == 0) {
      fprintf(stderr, "%s: image contains no data\n", argv[1]);
	exit(1);
    }

    /* Checksum it */

    for (i = 0; i < length; i++)
	cksum += image[i];

    /* Open the serial port */
    fd = IR_open();

    /* Delete firmware */
    if (delete_firmware(fd) != OK) 
	fprintf(stderr, "%s: EnterDownloadMode failed.\n", progname);
    else
    /* Start firmware download */
    if (start_firmware_download(fd,image_start, cksum) != OK) 
	fprintf(stderr, "%s: BeginDownload failed.\n", progname);
    else
    /* Transfer data */
    if (transfer_data(fd,image,length) != OK) 
        fprintf(stderr, "%s: DownloadBlock failed.\n", progname);
    else
    /* Unlock firmware */
    if (unlock_firmware(fd) != OK) 
	fprintf(stderr, "%s: RunProgram failed.\n", progname);


    IR_close(fd);

    exit(0);
}
