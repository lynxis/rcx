/*
 *  asm_rcx.lds
 *
 *  GNU ld script for mapping an assembled object module into
 *  an S-record represented absolute H8/300 program. The sections of
 *  the object module is mapped into the memory of the RCX starting at
 *  the load address 0x8000. The length of the program should not exceed
 *  0x6fff because the address 0xf000 is used as device register.
 *  The program entry point is __start. 
 *
*/

OUTPUT_FORMAT(srec)
OUTPUT_ARCH(h8300)
ENTRY(__start)
MEMORY
{
    mem : ORIGIN = 0x8000, LENGTH = 0x6fff
}
SECTIONS
{
    .text : {
              *(.text)
    } > mem 

    .data : {
              *(.data)
    } > mem
}
