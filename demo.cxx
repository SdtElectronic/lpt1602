#include "liblpt1602.h"
#include <stdio.h>

#include <sys/sysinfo.h>
#include <unistd.h>
/*
#include <netdb.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <linux/if_link.h>
*/
#include <string.h> /* for strncpy */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string>
#include <iomanip>
#include <array>
#include <thread>
#include <sstream>

int cpuPg(std::string);
int netPg(std::string);
int memPg(std::string);
unsigned int getXbytes(std::string interface, std::string xDir);
std::string getXunit(double &xSpd);

bool ext = 0;
constexpr unsigned int KiB = 1<<10;
constexpr unsigned int MiB = 1<<20;
constexpr unsigned int GiB = 1<<30;
lpt1602 npt;
constexpr std::array<int(*)(std::string), 3> cbArr{cpuPg, memPg, netPg};

constexpr std::array<char, 8> upArr{0x00, 0x00, 0x04, 0x0a, 0x11, 0x00, 0x00, 0x00};
constexpr std::array<char, 8> dnArr{0x00, 0x00, 0x11, 0x0a, 0x04, 0x00, 0x00, 0x00};
constexpr std::array<char, 8> cDeg {0x16, 0x09, 0x08, 0x08, 0x08, 0x09, 0x06, 0x00};

int cpuPg(std::string dumb){	
	struct sysinfo info;
	const int ret = sysinfo(&info);	
	std::stringstream cpu;
	cpu <<" "<<std::fixed<<std::setprecision(2)
		<<static_cast<double>(info.loads[0])/(1 << SI_LOAD_SHIFT)<<' '
		<<static_cast<double>(info.loads[1])/(1 << SI_LOAD_SHIFT)<<' '
		<<static_cast<double>(info.loads[2])/(1 << SI_LOAD_SHIFT);
	npt.clear();
	npt.curPos(1, 0);
	npt.puts(cpu.str().c_str());
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
	tem <<"  "<<std::fixed<<std::setprecision(1)
		<<tmp1/1000<<(char)0x03<<"  "<<tmp2/1000<<(char)0x03;
	npt.curPos(2, 0);
	npt.puts(tem.str().c_str());
	return 0;
}

int memPg(std::string dumb){	
	struct sysinfo info;
	const int ret = sysinfo(&info);	
	std::stringstream mem;
	mem <<"Mem "<<std::fixed<<std::setprecision(1)
		<<static_cast<double>(info.totalram - info.freeram)/(MiB)<<'/'
		<<static_cast<double>(info.totalram)/(MiB);
	npt.curPos(2, 0);
	npt.puts(mem.str().c_str());
	return 0;
}

int netPg(std::string interface = std::string{"eth0"}){	
	/*
	struct ifaddrs *ifaddr, *ifa;
	int family, s, n;
	char host[NI_MAXHOST];
	if (getifaddrs(&ifaddr) == -1) {	           
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}
	*/
	/* Walk through linked list, maintaining head pointer so we
	 * can free list later */
	/*
	for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {	
		if (ifa->ifa_addr == NULL || std::string(ifa->ifa_name) != interface)
			continue;
		family = ifa->ifa_addr->sa_family;
		if (family == AF_PACKET && ifa->ifa_data != NULL) {	    
	        struct rtnl_link_stats *stats = (struct rtnl_link_stats *)ifa->ifa_data;
			printf("\t\ttx_packets = %10u; rx_packets = %10u\n"
			"\t\ttx_bytes   = %10u; rx_bytes   = %10u\n",
			stats->tx_packets, stats->rx_packets,
	        stats->tx_bytes, stats->rx_bytes);
			unsigned int txp = stats->tx_bytes;
			unsigned int rxp = stats->rx_bytes;
			usleep(500000);
			double txSpd = (stats->tx_bytes - txp)/0.5;
			double rxSpd = (stats->rx_bytes - rxp)/0.5;
			std::stringstream spd;
			spd<<"TX: "<<std::fixed<<std::setprecision(9)<<txSpd<<"Bps";
			npt.clear();
			npt.curPos(1, 0);
			npt.puts(spd.str().c_str());
		}
	}
	npt.clear();
	npt.curPos(1, 0);
	*/
	int fd;
	struct ifreq ifr;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	   /* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;
	    /* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);
	npt.clear();
	npt.curPos(1, 0);
	npt.puts(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	const int rxTmp = getXbytes(interface, std::string("rx_bytes"));
	const int txTmp = getXbytes(interface, std::string("tx_bytes"));
	usleep(50000);
	double rxSpd = (getXbytes(interface, std::string("rx_bytes")) - rxTmp)/0.05;
	double txSpd = (getXbytes(interface, std::string("tx_bytes")) - txTmp)/0.05;
	std::string txUnit = getXunit(txSpd);
	std::string rxUnit = getXunit(rxSpd);
	std::stringstream spd;
	spd <<(char)0x58<<std::fixed<<std::setprecision(1)<<std::setw(5)<<std::setfill(' ')
		<<txSpd<<txUnit
		<<' '<<(char)0x60<<std::setprecision(2)<<std::setw(6)<<std::setfill(' ')
		<<rxSpd<<rxUnit;
	npt.curPos(2, 0);
	npt.puts(spd.str().c_str());
	return 0;
}

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

bool thd(){	
	static size_t tsk = 0;
	npt.tgDsp(1);
	do{	
		if(ext) return (ext = false);					/*clear exit flag*/
		cbArr[tsk]("eth0");
		sleep(3);
		npt.clear();
	}while(++tsk != cbArr.size());
	tsk = 0;
	npt.tgDsp(0);
	return 0;
}

int main(void){	
	npt.init();
	npt.defCh(0x58, upArr);
	npt.defCh(0x60, dnArr);
	npt.defCh(0x50, cDeg);
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
