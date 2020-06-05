#include "liblpt1602.h"

int main(void){	
	lpt1602 npt(0x378);
	npt.init();
	npt.curPos(1, 1);
	npt.puts("Hello World");
	npt.curPos(1,0);
	npt.puts("SdtElectronics");
	return 0;
}
