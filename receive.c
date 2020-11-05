#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>

#define BUFFER_SIZE 1600
#define ETHERTYPE 0x0806
#define REPLY 0X0002



int main(int argc, char *argv[])
{
	int fd;
	unsigned char buffer[BUFFER_SIZE];
	unsigned char *data;
	struct ifreq ifr;
	char ifname[IFNAMSIZ];
	char orig_ip[] = {192, 168, 15, 131};

	if (argc != 2) {
		printf("Usage: %s iface\n", argv[0]);
		return 1;
	}
	strcpy(ifname, argv[1]);

	/* Cria um descritor de socket do tipo RAW */
	fd = socket(PF_PACKET,SOCK_RAW, htons(ETH_P_ALL));
	if(fd < 0) {
		perror("socket");
		exit(1);
	}

	/* Obtem o indice da interface de rede */
	strcpy(ifr.ifr_name, ifname);
	if(ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
		perror("ioctl");
		exit(1);
	}

	/* Obtem as flags da interface */
	if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0){
		perror("ioctl");
		exit(1);
	}

	/* Coloca a interface em modo promiscuo */
	ifr.ifr_flags |= IFF_PROMISC;
	if(ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
		perror("ioctl");
		exit(1);
	}

	printf("Esperando pacotes ... \n");


	unsigned char *reply;
	while (1) {
		unsigned char mac_dst[6];
		unsigned char mac_src[6];
		unsigned char ip_src[4];
		unsigned char ip_dst[4];
		short int ethertype;
		short int oprecieve;

		/* Recebe pacotes */
		if (recv(fd,(char *) &buffer, BUFFER_SIZE, 0) < 0) {
			perror("recv");
			close(fd);
			exit(1);
		}
        
		/* Copia o conteudo do cabecalho Ethernet */
		memcpy(mac_dst, buffer, sizeof(mac_dst));
		memcpy(mac_src, buffer+sizeof(mac_dst), sizeof(mac_src));
		memcpy(ðertype, buffer+sizeof(mac_dst)+sizeof(mac_src), sizeof(ethertype));
		ethertype = ntohs(ethertype);
		reply = (buffer+sizeof(mac_dst)+sizeof(mac_src)+sizeof(ethertype));

		memcpy(&oprecieve,reply+6, sizeof(oprecieve));
		oprecieve = ntohs(oprecieve);
		
		memcpy(ip_src, reply+14, sizeof(ip_src));

		memcpy(ip_dst, reply+24, sizeof(ip_dst));
		
		if ((ethertype == ETHERTYPE && oprecieve == 2)
			&& (131 == ip_dst[3]))
			//&& (orig_ip[0] == ip_dst[0] && orig_ip[1] == ip_dst[1] && orig_ip[2] == ip_dst[2] && orig_ip[3] == ip_dst[3])) 
		{
			printf("IP: %d.%d.%d.%d\t", ip_src[0], ip_src[1], ip_src[2], ip_src[3]);
			printf("MAC origem:  %02x:%02x:%02x:%02x:%02x:%02x\n", 
                        mac_src[0], mac_src[1], mac_src[2], mac_src[3], mac_src[4], mac_src[5]);
			printf("\n");
		}
	}

	close(fd);
	return 0;
}


