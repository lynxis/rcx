/* H8/300 data types */

typedef unsigned char      byte;
typedef unsigned short int word;
typedef short int          int16;
typedef unsigned short int uint16;



/* LCD routines */

void lcd_refresh (void)
{ asm("jsr 0x27c8      ; call refresh
      ");
}

void lcd_clear (void)
{ asm("jsr 0x27ac      ; call clear
       jsr 0x27c8      ; call refresh
      ");
} 


#define LCD_STANDING         0x3006     /* standing figure         */
#define LCD_WALKING          0x3007     /* walking figure          */
#define LCD_SENSOR_0_VIEW    0x3008     /* sensor 0 view selected  */
#define LCD_SENSOR_0_ACTIVE  0x3009     /* sensor 0 active         */
#define LCD_SENSOR_1_VIEW    0x300a     /* sensor 1 view selected  */
#define LCD_SENSOR_1_ACTIVE  0x300b     /* sensor 1 active         */
#define LCD_SENSOR_2_VIEW    0x300c     /* sensor 2 view selected  */
#define LCD_SENSOR_2_ACTIVE  0x300d     /* sensor 2 active         */
#define LCD_MOTOR_0_VIEW     0x300e     /* motor 0 view selected   */
#define LCD_MOTOR_0_REV      0x300f     /* motor 0 backward arrow  */
#define LCD_MOTOR_0_FWD      0x3010     /* motor 0 forward arrow   */
#define LCD_MOTOR_1_VIEW     0x3011     /* motor 1 view selected   */
#define LCD_MOTOR_1_REV      0x3012     /* motor 1 backward arrow  */
#define LCD_MOTOR_1_FWD      0x3013     /* motor 1 forward arrow   */
#define LCD_MOTOR_2_VIEW     0x3014     /* motor 2 view selected   */
#define LCD_MOTOR_2_REV      0x3015     /* motor 2 backward arrow  */
#define LCD_MOTOR_2_FWD      0x3016     /* motor 2 forward arrow   */
#define LCD_DATALOG          0x3018     /* datalog indicator       */
#define LCD_DOWNLOAD         0x3019     /* download in progress    */
#define LCD_UPLOAD           0x301a     /* upload in progress      */ 
#define LCD_BATTERY          0x301b     /* battery low             */
#define LCD_RANGE_SHORT      0x301c     /* short range indicator   */
#define LCD_RANGE_LONG       0x301d     /* long range indicator    */
#define LCD_ALL              0x3020     /* all segments            */


void lcd_show_icon (uint16 icon)
{ asm("mov.w r0,r6
       jsr 0x1b62      ; call show_icon
       jsr 0x27c8      ; call refresh
      "
      );
}

void lcd_hide_icon (uint16 icon)
{ asm("mov.w r0,r6
       jsr 0x1e4a      ; call show_icon
       jsr 0x27c8      ; call refresh
      "
      );
}

void lcd_show_number ( int16 format, int16 value, int16 scalecode)
{
   asm("push r2
        push r1
        mov.w r0,r6
        jsr 0x1ff2      ; call show_number
        adds #2,r7      ; adjust stack 
        adds #2,r7
        jsr 0x27c8      ; call refresh
       " 
       );
}

/* Shows integer value as four decimal digits in the LCD left of 
 * the LEGO man. This means that the range is [-9999, 9999]. 
 * Negative values are show with a leading minussign.
 */
void lcd_show_int16(int16 r)
{ 
  lcd_show_number(0x3001, r, 0x3002);
}


void lcd_show_digit(int16 r)
{ 
  lcd_show_number(0x3017, r, 0);
}



/* RCX_Reset: 
 * Reset RCX through an indirect jump through the reset vector at address 0
 */
void RCX_Reset(void)
{ 
  asm (" jmp @@0");  
}

char *RCX_string="Do you byte, when I knock?";
