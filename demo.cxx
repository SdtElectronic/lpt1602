#include "liblpt1602.h"

int main(void){	
	lpt1602 npt(0x378);
	npt.lcd_init();
	npt.lcd_goto(1,1);
	npt.lcd_puts("Hello World");
	npt.lcd_goto(1,0);
	npt.lcd_puts("SdtElectronics");
	return 0;
}
