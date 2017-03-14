/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Author: Olivier Gruber (olivier dot gruber at acm dot org)
*/

#define VID_BEGIN 0xB8000 //1st address of the video memory
#define VID_END 0xB8FA0 //last address of the video memory
#define VID_WIDTH 80 //Nb char of a line in terminal
#define VID_HEIGHT 25 //Nb lines

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

#define COM1 ((uint16_t)0x3f8)
#define COM2 ((uint16_t)0x2f8)

static __inline __attribute__((always_inline, no_instrument_function))
uint8_t inb(uint16_t port) {
  uint8_t data;
  __asm __volatile("inb %w1,%0" : "=a" (data) : "d" (port));
  return data;
}

static __inline __attribute__((always_inline, no_instrument_function))
void outb(uint16_t port, uint8_t data) {
  __asm __volatile("outb %0,%w1" : : "a" (data), "d" (port));
}


static void serial_init(uint16_t port) {

  outb(port+1,0x00);    // Disable all interrupts
  outb(port+3,0x80);    // Enable DLAB (set baud rate divisor)

  outb(port+0,0x01);  // Set divisor to 1 (lo byte) 115200 baud
  outb(port+1,0x00);  //                0 (hi byte)

  outb(port+3,0x03);    // 8 bits, no parity, one stop bit
  outb(port+2,/*0xC7*/ 0x00);    // Enable FIFO, clear them, with 14-byte threshold
  outb(port+4,/*0x0B*/ 0x08);    // IRQs enabled, RTS/DSR set

  // outb(port+1,0x0d); // enable all intrs but writes
}



static
void serial_write_char(uint16_t port, char c) {
  while ((inb(port + 5) & 0x20) == 0);
  outb(port,c);
}

static
void serial_write_string(uint16_t port, const unsigned char *s) {
  while(*s != '\0') {
    serial_write_char(port,*s);
    s++;
  }
}

char serial_read(uint16_t port) {
   while ((inb(port + 5) & 1) == 0);
   return inb(port);
}



int i=0;
void kputchar(int c, void *arg) {
  serial_write_char(COM1,c);
}

void kmain(void) {

	volatile char *pos = (volatile char*) VID_BEGIN;
	int yPos = 0;
	int xPos = 0;
	int xLimit = 0;

	//write_string("Bienvenue sur la console");
	pos = screen_write_string(0x2A, ">", pos);

	serial_init(COM1);
	serial_write_string(COM1,"\n\rBienvenue.\n\r");
	serial_write_char(COM1,'>');
	move_cursor(yPos, xPos + 1);

	while(1) {
		unsigned char c;
		c=serial_read(COM1);

		if(c==13){ //Carriage return
			serial_write_char(COM1, '\r');
			serial_write_char(COM1, '\n');
			serial_write_char(COM1, '>');
			yPos++;

			//Checks if we are at the end of the screen
			if(yPos > VID_HEIGHT-1){
				scroll();
				pos -= 2*(xPos+1);
			}else{
				pos += 2*(79-xPos);
			}
			pos = screen_write_string(0x2A, ">", pos);
			xPos = 0;
			xLimit = 0;

			int tempX, tempY;
			if(yPos > VID_HEIGHT-1){
				tempY = VID_HEIGHT-1;
			}else{
				tempY = yPos;
			}
			if(xPos == 79){
				tempX = xPos;
			}else{
				tempX = xPos+1;
			}
			move_cursor(tempX, tempY);
		}else if(c==27){
			c=serial_read(COM1);
			if(c==91){ //It's an arrow
				c=serial_read(COM1);
				switch (c) {
					case 68: //Left arrow
						if(xPos > 0){
							xPos--;
							pos -= 2;
						}
						break;
					case 67: //Right arrow
						if(xPos < xLimit){
							xPos++;
							pos += 2;

						}
						break;
				}

				int tempX, tempY;
				if(yPos > VID_HEIGHT-1){
					tempY = VID_HEIGHT-1;
				}else{
					tempY = yPos;
				}
				if(xPos == 79){
					tempX = xPos;
				}else{
					tempX = xPos+1;
				}
				move_cursor(tempX, tempY);
			}

		}else if(xPos < VID_WIDTH-1){ //Limit to 1 line
			serial_write_char(COM1, c);
			pos = screen_write_char(0x2A, c, pos);
			xPos++;
			if(xPos > xLimit) xLimit = xPos; //For use with the right arrow, prevents moving more than we wrote
			int tempX, tempY;
			if(yPos > VID_HEIGHT-1){
				tempY = VID_HEIGHT-1;
			}else{
				tempY = yPos;
			}
			if(xPos == 79){
				tempX = xPos;
			}else{
				tempX = xPos+1;
			}
			move_cursor(tempX, tempY);
		}
	}

}
