#include <cstddef>
#include <stdexcept>
#include <sys/io.h>

class lpt1602{	
	public:
		enum entryMode: unsigned char{	
			curLsTxtNs = 0,
			curNsTxtRs = 1,
			curRsTxtNs = 2,
			curNsTxtLs = 3
		};

		constexpr lpt1602(size_t lptAddr): dataAddr(lptAddr), statAddr(lptAddr + 1), ctrlAddr(lptAddr + 2){	
			if(ioperm(dataAddr, 3, 1))
				throw std::runtime_error("Permission denied");
		};

		int lcd_init();
		int lcd_write(char char2write);
		int lcd_putch(char char2write);
		int lcd_puts(const char* str2write);
		int lcd_goto(size_t row, size_t column);
		int lcd_clear();
		int lcd_home();
		int lcd_cursor(bool cur, bool blk);
		int lcd_entry_mode(lpt1602::entryMode mode);
	private:
		size_t dataAddr;
		size_t statAddr;
		size_t ctrlAddr;
};
