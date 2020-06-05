#include <sys/io.h>
#include <unistd.h>
#include "liblpt1602.h"

int lpt1602::lcd_init(){	
	outb( inb(ctrlAddr) & 0xDF ,ctrlAddr);
	//config data pins as output
	outb( inb(ctrlAddr) | 0x08 ,ctrlAddr);
	//RS is made high: control (register select)
	lcd_write(0x0f);
	usleep(5000);
	lcd_write(0x01);
	usleep(5000);
	lcd_write(0x38);
	usleep(5000);
	lcd_write(0x38);
	usleep(5000);
	lcd_write(0x38);
	lcd_write(0x08);
	lcd_write(0x01);
	lcd_write(0x06);
	lcd_write(0x0c);
	return 0;
}

int lpt1602::lcd_write(char char2write){	
	outb(char2write, dataAddr);
	outb(inb(ctrlAddr) | 0x01, ctrlAddr); /* Set Strobe*/
	usleep(2000);
	outb(inb(ctrlAddr) & 0xFE, ctrlAddr); /* Reset Strobe*/
	usleep(2000);
	return 0;
}

int lpt1602::lcd_putch(char char2write){	
	outb( inb(ctrlAddr) & 0xF7, ctrlAddr);
	//RS=low: data
	lcd_write(char2write);
	return 0;
}

int lpt1602::lcd_puts(const char* str2write){	
	outb( inb(ctrlAddr) & 0xF7, ctrlAddr);
	//RS=low: data
	while(*str2write)
		lcd_write(*(str2write++));
	return 0;
}

int lpt1602::lcd_goto(size_t row, size_t column){	
	 outb( inb(ctrlAddr) | 0x08, ctrlAddr);
	if(row==2) column+=0x40;
	/* Add these if you are using LCD module with 4 columns
	 * if(row==2) column+=0x14;
	 * if(row==3) column+=0x54;
	 */
	lcd_write(0x80 | column);
	return 0;
}

int lpt1602::lcd_clear(){	
	outb( inb(ctrlAddr) | 0x08, ctrlAddr);
	lcd_write(0x01);
	return 0;
}

int lpt1602::lcd_entry_mode(lpt1602::entryMode mode){	
	/*
	 * if you dont call this function, entry mode sets to
	 * 2 by default.
	 * mode: 0 - cursor left shift, no text shift
	 * 1 - no cursor shift, text right shift
	 * 2 - cursor right shift, no text shift
	 * 3 - no cursor shift, text left shift
	 */
	outb( inb(ctrlAddr) | 0x08, ctrlAddr);
	lcd_write(0x04 + (mode%4));
	return 0;
}

int lpt1602::lcd_cursor(bool cur, bool blk){	
	/*
	 * set cursor:
	 * 0 - no cursor, no blink
	 * 1 - only blink, no cursor
	 * 2 - only cursor, no blink
	 * 3 - both cursor and blink
	 */
	outb(  inb(ctrlAddr) | 0x08, ctrlAddr);
	lcd_write( 0x0c + (((cur<<1)|blk)%4));
	return 0;
}
