/*
 * screenUtils.c
 *
 *  Created on: 7 févr. 2017
 *      Author: clementbrocard
 *
 *  @see http://www.osdever.net/bkerndev/Docs/printing.htm
 *
 */
#define VID_BEGIN 0xB8000 //1st address of the video memory
#define VID_END 0xB8FA0 //last address of the video memory
#define VID_WIDTH 80 //Nb char of a line in terminal
#define VID_HEIGHT 25 //Nb lines

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

unsigned short *position; //Current text pointer position
char cursor_x;
char cursor_y = 10; //Cursor coords (Set it to 10 so the cursor is in the middle of the screen
char attr = 0x07; //Video attributes, here we set foreground to light grey http://wiki.osdev.org/Text_UI#Colours


static __inline __attribute__((always_inline, no_instrument_function))
void outb(uint16_t port, uint8_t data) {
  __asm __volatile("outb %0,%w1" : : "a" (data), "d" (port));
}

void screen_init(){
	position = (unsigned short *) VID_BEGIN; //First position of the video buffer in memory
	//screen_clear(); //Fill the buffer with blanks to remove boot prints
}


/*
 * See:
 *   http://wiki.osdev.org/Printing_To_Screen
 *   http://wiki.osdev.org/Text_UI
 *
 * Note this example will always write to the top
 * line of the screen.
 * Note it assumes a color screen 0xB8000.
 * It also assumes the screen is in text mode,
 * and page-0 is displayed.
 * Screen is 80x25 (80 characters, 25 lines).
 *
 * For more info:
 *   http://wiki.osdev.org/VGA_Resources
 */
void write_string( int colour, const char *string ) {
    volatile char *video = (volatile char*)0xB8000;
    while( *string != 0 ) {
        *video++ = *string++;
        *video++ = colour;
    }
}


char *write_string_end( int colour, const char *string, char position ) {
    volatile char *video = (volatile char*) position;
    while( *string != 0 ) {
        *video++ = *string++;
        *video++ = colour;
    }
    return video;
}

volatile char* screen_write_string(int color, char* string, volatile char* pos){
	while(*string!=0){
		*pos++ = *string++;
		*pos++ = color;
	}
	return pos;
}

volatile char* screen_write_char(int color, char c, volatile char* pos){
	*pos++ = c;
	*pos++ = color;
	return pos;
}


// Scrolls 1 line
void scroll(){
	char *videoStart = (volatile char*) 0xB8000;
	int i = 0;
	for (i = 0; i < (VID_HEIGHT-1) * 2*VID_WIDTH; i++) {
		*videoStart++ = *(videoStart + 159);
	}
	for (i = 0; i < 2*VID_WIDTH; i++) {
		videoStart = screen_write_char(0x07, ' ', videoStart);
	}

}

void putchar(unsigned char c){
	switch (c) {
		case 0x8: //Backspace
			//Move cursor x--
			if(cursor_x > 0){
				cursor_x--;
			}
			break;
		case '\n': //Next line
			cursor_x = 0;
			cursor_y++;
			break;

		unsigned char *where;
		//Every char greater than space is printable
		if(c>=' '){
			where = position + (cursor_y * 80 + cursor_x);
			*where++ = c;
			*where++ = 0x2a;
			cursor_x++;
		}

		//Handle end of the line
		if(cursor_x == 80){
			cursor_x = 0; //Reset cursor position
			cursor_y++; //Move to the a new line
		}

		//TODO Scroll
		//TODO Move cursor
	}
}

// http://wiki.osdev.org/Text_Mode_Cursor
void move_cursor(int x, int y){
	unsigned short position=(y*VID_WIDTH) + x;
	// cursor LOW port to vga INDEX register
	outb(0x3D4, 0x0F);
	outb(0x3D5, (unsigned char)(position&0xFF));
	// cursor HIGH port to vga INDEX register
	outb(0x3D4, 0x0E);
	outb(0x3D5, (unsigned char )((position>>8)&0xFF));
}
