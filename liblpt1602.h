#include <cstddef>
#include <stdexcept>
#include <sys/io.h>
#pragma once

class lpt1602{	
	public:
		enum entryMode: unsigned char{	
			curLsTxtNs = 0,
			curNsTxtRs = 1,
			curRsTxtNs = 2,
			curNsTxtLs = 3
		};

		constexpr lpt1602(size_t lptAddr = 0x378): dataAddr(lptAddr), 
												   statAddr(lptAddr + 1), 
												   ctrlAddr(lptAddr + 2){	
			if(ioperm(dataAddr, 3, 1))
				throw std::runtime_error("Permission denied");
		};

		int init();
		int write(char char2write);
		int wrChr(char chr);
		int wrCmd(char cmd);
		int puts(const char* str2write);
		int curPos(size_t row, size_t column);
		int clear();
		int home();
		int curMode(bool cur, bool blk);
		int entMode(lpt1602::entryMode mode);
		int tgDsp(bool on);
	private:
		size_t dataAddr;
		size_t statAddr;
		size_t ctrlAddr;
};
