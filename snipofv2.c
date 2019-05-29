#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <pcap.h>
#include <errno.h>
#include <unistd.h>

//Create RAW socket
int sock;

struct pseudo_hdr
{
    uint32_t src_ip;
    uint32_t dst_ip;
    uint8_t reserved;
    uint8_t protocol;
    uint16_t tcp_len;

    struct tcphdr tcph;
};

uint16_t chksum(const uint16_t *ptr, int len){
    uint32_t sum;
    uint16_t oddbyte;
    uint16_t answer;

    sum = 0;
    while(len > 1){
        sum+=*ptr++;
        len-=2;
    }
    if(len == 1){
        oddbyte = 0;
        *((uint8_t *) &oddbyte) = *(uint8_t*)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;

    return(answer);
}

uint16_t tcpCS(const struct iphdr *iph, const struct tcphdr *tcph, uint16_t tcp_len){
    struct pseudo_hdr psh;
    psh.src_ip = iph->saddr;
    psh.dst_ip = iph->daddr;
    psh.reserved = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_len = htons(tcp_len);

    memcpy(&psh.tcph, tcph, sizeof(struct tcphdr));
    return (chksum((uint16_t *)&psh, sizeof(struct pseudo_hdr)));
}

int tcpPacket(struct iphdr *_iph){

    struct tcphdr *_tcph = (struct tcphdr*) (((uint8_t *) _iph) + _iph->ihl*4);
    //Setting the address and variables
    u_int16_t src_port = _tcph->dest;
    u_int16_t dst_port = _tcph->source;
    uint32_t src_addr = _iph->daddr;
    uint32_t dst_addr = _iph->saddr;
    uint32_t seq = _tcph->ack_seq;
    uint32_t ack = _tcph->seq+1;

    //Read the target ip addr, and print out for double check
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(_iph->daddr), ip_str, INET_ADDRSTRLEN);
    printf("dest=%s", ip_str);
    printf(":%u", ntohs(_tcph->dest));

    struct sockaddr_in sin;
    struct pseudo_hdr psh;
    
    //Define a new packet based on common MTU
    char *packet = malloc(1500);
    struct iphdr *iph = (struct iphdr*) packet;
    struct tcphdr *tcph = (struct tcphdr*) (packet + sizeof(struct ip));

    //Setting up socket
    sin.sin_family = AF_INET;
    //sin.sin_port = htons(dst_port);
    sin.sin_port = dst_port;
    //sin.sin_addr.s_addr = inet_addr(target);
    sin.sin_addr.s_addr = dst_addr;

    //clear the packet buffer
    memset(packet, 0, 1500);

    /* if something is set to 0, it is not necessary write them out. */
    //Fill IP header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct ip) + sizeof(struct tcphdr);
    iph->id = htons(1551);
    iph->ttl = 255; //Set to max because we can
    iph->protocol = IPPROTO_TCP;
    iph->check = 0; /* when set to 0, the kernel will calculate for you */
    //iph->saddr = inet_addr(src_ip);
    iph->saddr = src_addr;
    iph->daddr = sin.sin_addr.s_addr;
    //Calculate IP checksum
    //iph->check = chksum((uint16_t *) packet, iph->tot_len >> 1); 

    //Fill TCP header
    tcph->source = src_port;
    tcph->dest = dst_port;
    //tcph->seq = htonl(12345);
    tcph->seq = seq;
    //tcph->ack_seq = htonl(54321);
    tcph->ack_seq = ack;
    tcph->doff = 5; /* No TCP options */
    tcph->fin = 0;
    tcph->syn = 0; /* This is synchronization packet */
    tcph->rst = 1; /* This is reset packet */
    tcph->psh = 0;
    tcph->ack = 1;
    tcph->urg = 0;
    tcph->window = htons(3660);
    tcph->urg_ptr = 0;
    //Calculate the TCP checksum
    tcph->check = 0; 
    tcph->check = tcpCS(iph, tcph, 20);

    //Tell kernel that we construct the header
    int _one = 1;
    const int *one = &_one;
    if(setsockopt(sock, IPPROTO_IP, IP_HDRINCL, one, sizeof(_one)) < 0){
        printf("Error setting socket! Error number : %d . Error message : %s \n", errno, strerror(errno));
        exit(0);
    }

    //Send out the packet
    for(int i=0;i<3;i++){
        if(sendto(sock, packet, iph->tot_len, 0, (struct sockaddr *)&sin, sizeof(sin)) < 0){
            printf("Error sending packet\n");
        }
        else{
            printf(".");
        }
    }

    tcph->ack = 0;
    tcph->window = htons(5218);
    tcph->ack_seq = 0;
    tcph->check = 0;
    tcph->check = tcpCS(iph, tcph, 20);
    
    if(sendto(sock, packet, iph->tot_len, 0, (struct sockaddr *)&sin, sizeof(sin)) < 0){
        printf("Error sending packet\n");
    }
    else{
        printf(".\n");
    }
    

    return 1;
}

void gPacket(u_char *args, const struct pcap_pkthdr *header, const u_char *packet){
    //printf("Received Packet!");

    struct ether_header *eth = (struct ether_header *)packet;
    struct iphdr *iph = (struct iphdr*) (packet + sizeof(struct ether_header));

    tcpPacket(iph);
}

int main(){
    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];
    struct bpf_program fp;
    char filter_exp[] = "tcp and src host 10.0.2.15";
    bpf_u_int32 net;

    sock = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
    if(sock <= 0){
        printf("Error init socket!");
    }

    // Step 1: Open live pcap session on NIC with interface name
	handle = pcap_open_live("enp0s3", BUFSIZ, 1, 1000, errbuf);

    // Step 2: Compile filter_exp into BPF psuedo-code 
	pcap_compile(handle, &fp, filter_exp, 0, net); 
	pcap_setfilter(handle, &fp);

    // Step 3: Capture packets 
	pcap_loop(handle, -1, gPacket, NULL);

    pcap_close(handle); //Close the handle 
    close(sock);
	return 0;
}