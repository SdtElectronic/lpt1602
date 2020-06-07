#include <sys/io.h>
#include <unistd.h>
#include "liblpt1602.h"

int lpt1602::init(){	
	outb( inb(ctrlAddr) & 0xDF ,ctrlAddr);
	//config data pins as output
	outb( inb(ctrlAddr) | 0x08 ,ctrlAddr);
	//RS is made high: control (register select)
	write(0x0f);
	usleep(1000);
	write(0x01);
	usleep(1000);
	write(0x38);
	usleep(1000);
	write(0x38);
	usleep(1000);
	write(0x38);
	write(0x08);
	write(0x01);
	write(0x06);
	write(0x0c);
	return 0;
}

int lpt1602::write(char char2write){	
	outb(char2write, dataAddr);
	outb(inb(ctrlAddr) | 0x01, ctrlAddr); /* Set Strobe*/
	usleep(2000);
	outb(inb(ctrlAddr) & 0xFE, ctrlAddr); /* Reset Strobe*/
	usleep(2000);
	return 0;
}

int lpt1602::wrCmd(char cmd){	
	outb( inb(ctrlAddr) | 0x08, ctrlAddr);
	write(cmd);
	return 0;
}

int lpt1602::wrChr(char chr){	
	outb( inb(ctrlAddr) & 0xF7, ctrlAddr);
	//RS=low: data
	write(chr);
	return 0;
}

int lpt1602::puts(const char* str2write){	
	outb( inb(ctrlAddr) & 0xF7, ctrlAddr);
	//RS=low: data
	while(*str2write)
		write(*(str2write++));
	return 0;
}

int lpt1602::curPos(size_t row, size_t column){	
	if(row==2) column+=0x40;
	/* Add these if you are using LCD module with 4 columns
	 * if(row==2) column+=0x14;
	 * if(row==3) column+=0x54;
	 */
	wrCmd(0x80 | column);
	return 0;
}

int lpt1602::clear(){	
	wrCmd(0x01);
	return 0;
}

int lpt1602::entMode(lpt1602::entryMode mode){	
	/*
	 * if you dont call this function, entry mode sets to
	 * 2 by default.
	 * mode: 0 - cursor left shift, no text shift
	 * 1 - no cursor shift, text right shift
	 * 2 - cursor right shift, no text shift
	 * 3 - no cursor shift, text left shift
	 */
	wrCmd(0x04 + (mode%4));
	return 0;
}

int lpt1602::curMode(bool cur, bool blk){	
	/*
	 * set cursor:
	 * 0 - no cursor, no blink
	 * 1 - only blink, no cursor
	 * 2 - only cursor, no blink
	 * 3 - both cursor and blink
	 */
	wrCmd( 0x0c + (((cur<<1)|blk)%4));
	return 0;
}

int lpt1602::tgDsp(bool on){	
	wrCmd(8 + (on << 3));
	if(on){
		outb(0x00, ctrlAddr);
		this->init();
		return 0;
	}
	outb(0xff, ctrlAddr);
	return 0;
}
