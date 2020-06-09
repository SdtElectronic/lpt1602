#include "liblpt1602.h"

#include <stdio.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <string.h> /* for strncpy */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <ctime>
#include <string>
#include <iomanip>
#include <array>
#include <thread>
#include <sstream>

/* status monitor screen */
int cpuPg(std::string);
int netPg(std::string);
int memPg(std::string);
int timPg(std::string);

constexpr std::array<int(*)(std::string), 4> cbArr{cpuPg, memPg, netPg, timPg};

/* helper function */
unsigned int getXbytes(std::string interface, std::string xDir);
std::string getXunit(double &xSpd);

/* exit flag */
bool ext = 0;

/* Units */
constexpr unsigned int KiB = 1<<10;
constexpr unsigned int MiB = 1<<20;
constexpr unsigned int GiB = 1<<30;

lpt1602 npt;

/* User defined characters, stored in CGRAM */
constexpr std::array<char, 8> upArr{0x00, 0x00, 0x04, 0x0a, 0x11, 0x00, 0x00, 0x00};
constexpr std::array<char, 8> dnArr{0x00, 0x00, 0x11, 0x0a, 0x04, 0x00, 0x00, 0x00};
constexpr std::array<char, 8> cDeg {0x02, 0x05, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};

int cpuPg(std::string dumb){	
	struct sysinfo info;
	const int ret = sysinfo(&info);	
	std::stringstream cpu;
	/* 1min, 5min, 15min Load average */
	cpu <<" "<<std::fixed<<std::setprecision(2)
		<<static_cast<double>(info.loads[0])/(1 << SI_LOAD_SHIFT)<<' '
		<<static_cast<double>(info.loads[1])/(1 << SI_LOAD_SHIFT)<<' '
		<<static_cast<double>(info.loads[2])/(1 << SI_LOAD_SHIFT);
	npt.clear();
	npt.curPos(1, 0);
	npt.puts(cpu.str().c_str());
	/* Core1. core2 temperature */
	float systemp, tmp1, tmp2;
	FILE *thermal;
	int n;
	thermal = fopen("/sys/bus/platform/devices/coretemp.0/hwmon/hwmon1/temp2_input", "r");
	n = fscanf(thermal, "%f", &tmp1);
	fclose(thermal);
	thermal = fopen("/sys/bus/platform/devices/coretemp.0/hwmon/hwmon1/temp3_input", "r");
	n = fscanf(thermal, "%f", &tmp2);
	fclose(thermal);
	std::stringstream tem;
	tem <<" "<<std::fixed<<std::setprecision(1)
		<<tmp1/1000<<(char)0x02<<'C'<<"  "<<tmp2/1000<<(char)0x02<<'C';
	npt.curPos(2, 0);
	npt.puts(tem.str().c_str());
	return 0;
}

int memPg(std::string dumb){	
	struct sysinfo info;
	const int ret = sysinfo(&info);	
	std::stringstream tsk;
	/* Running procs */
	tsk <<"   "<<std::setw(4)<<info.procs<<"procs";
	npt.clear();
	npt.curPos(1, 0);
	npt.puts(tsk.str().c_str());
	std::stringstream mem;
	/* Memory utilization and percentage */
	double usd = static_cast<double>(info.totalram - info.freeram);
	double utPer = usd/static_cast<double>(info.totalram)*100;
	std::string uUnt = getXunit(usd);
	mem <<"Mem "<<std::fixed<<std::setprecision(1)<<usd<<uUnt<<' '<<utPer<<'%';
	npt.curPos(2, 0);
	npt.puts(mem.str().c_str());
	return 0;
}

int timPg(std::string dumb){	
	time_t now = time(0);
	tm *ltm = localtime(&now);
	std::stringstream ntm;
	/* Present time */
	ntm <<' '<<std::setw(2)<<std::setfill('0')<<ltm->tm_year % 100<<'/'
		<<std::setw(2)<<std::setfill('0')<<ltm->tm_mon<<'/'
		<<std::setw(2)<<std::setfill('0')<<ltm->tm_mday<<' '
		<<std::setw(2)<<std::setfill('0')<<ltm->tm_hour<<':'
		<<std::setw(2)<<std::setfill('0')<<ltm->tm_min;
	npt.clear();
	npt.curPos(1, 0);
	npt.puts(ntm.str().c_str());
	/* Uptime */
	struct sysinfo info;
	const int ret = sysinfo(&info);
	std::stringstream utm;
	time_t tdf = now - static_cast<time_t>(info.uptime);
	tm *ptm = gmtime(&tdf);
	tdf = static_cast<time_t>(info.uptime);
	tm *upt = gmtime(&tdf);
	utm <<std::setw(2)<<std::setfill(' ')<<ltm->tm_mon -  ptm->tm_mon<<'M'
		<<std::setw(2)<<std::setfill(' ')<<ltm->tm_mday - ptm->tm_mday<<'D'
		<<std::setw(2)<<std::setfill(' ')<<upt->tm_hour<<'h'
		<<std::setw(2)<<std::setfill(' ')<<upt->tm_min<<'m'
		<<std::setw(2)<<std::setfill(' ')<<upt->tm_sec<<'s';
	npt.curPos(2, 0);
	npt.puts(utm.str().c_str());
	return 0;
}

int netPg(std::string interface = std::string{"eth0"}){	
	int fd;
	struct ifreq ifr;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	   /* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;
	   /* Get IP address by interface name */
	strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);
	npt.clear();
	npt.curPos(1, 0);
	std::string ipAddr(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	int padd = (16 - ipAddr.size())/2;                              /* Centeral alignment */ 
	npt.puts((std::string(padd, ' ') + ipAddr).c_str());
	/* TX and RX traffic */
	const int rxTmp = getXbytes(interface, std::string("rx_bytes"));
	const int txTmp = getXbytes(interface, std::string("tx_bytes"));
	usleep(50000);
	double rxSpd = (getXbytes(interface, std::string("rx_bytes")) - rxTmp)/0.05;
	double txSpd = (getXbytes(interface, std::string("tx_bytes")) - txTmp)/0.05;
	std::string txUnit = getXunit(txSpd);
	std::string rxUnit = getXunit(rxSpd);
	std::stringstream spd;
	spd <<(char)0x03<<std::left<<std::fixed<<std::setprecision(2)<<txSpd<<txUnit;
	npt.curPos(2, 0);
	npt.puts(spd.str().c_str());
	spd.str("");
	spd	<<(char)0x04<<std::setprecision(2)<<rxSpd<<rxUnit;
	npt.curPos(2, 8);
	npt.puts(spd.str().c_str());
	return 0;
}

/* Select the proper unit for data size and convert the size accordingly */
std::string getXunit(double &xSpd){
	std::array<std::string, 4> xUnit{std::string("B"), std::string("K"), 
									 std::string("M"), std::string("G")};
	size_t unitInd = 0;
	while(unitInd != 4){
		if(xSpd > KiB){	
			++unitInd;
			xSpd /= KiB;
		}else{	
			break;
		};
	}
	return xUnit[unitInd];
}

unsigned int getXbytes(std::string interface, std::string xDir){
	unsigned int byt;
	FILE *spd;
	int n;
	spd = fopen((std::string("/sys/class/net/") 
					+ interface 
					+ std::string("/statistics/").c_str()
					+ xDir).c_str(), "r");
	n = fscanf(spd, "%d", &byt);
	fclose(spd);
	return byt;
}

/* thread to display all status one time */
bool thd(){	
	/* Record next screen to display, persists after interrupt */
	static size_t tsk = 0;
	npt.tgDsp(1);										/* Turn on the LCD */
	do{													/* Go through all the screens */	
		if(ext) return (ext = false);					/* clear exit flag */
		cbArr[tsk]("eth0");								/* change it to wanted interface */
		sleep(3);										/* how long each screen last */
		npt.clear();
	}while(++tsk != cbArr.size());
	tsk = 0;											/* reset screen index */
	npt.tgDsp(0);										/* Turn off the LCD */
	return 0;
}

int main(void){	
	npt.init();
	/* define user characters */
	npt.defCh(0x58, upArr);	
	npt.defCh(0x60, dnArr);
	npt.defCh(0x50, cDeg);
	/* run once after start */
	thd();
	while(1){	
		if(((inb(0x378 + 1)^0x80) >> 3) & (1 << 3)){	
			usleep(200000);
			if(((inb(0x378 + 1)^0x80) >> 3) & (1 << 3)){	
				usleep(200000);
				if(!(((inb(0x378 + 1)^0x80) >> 3) & (1 << 3))){	
					ext = 1;
					std::thread td(thd);
					td.detach();
				}
			}
		}
	}
	return 0;
}
