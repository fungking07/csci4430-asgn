#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h> // required by "netfilter.h"
#include <arpa/inet.h> // required by ntoh[s|l]()
#include <signal.h> // required by SIGINT
#include <string.h> // required by strerror()
#include <sys/time.h> // required by gettimeofday()
#include <time.h> // required by nanosleep()
#include <errno.h> // required by errno
#include <pthread.h>
#include <netinet/ip.h>        // required by "struct iph"
#include <netinet/tcp.h>    // required by "struct tcph"
#include <netinet/udp.h>    // required by "struct udph"
#include <netinet/ip_icmp.h>    // required by "struct icmphdr"
#include "entry.c"        //for entry table

extern "C" {
#include <linux/netfilter.h> // required by NF_ACCEPT, NF_DROP, etc...
#include <libnetfilter_queue/libnetfilter_queue.h>
}

#include "checksum.h"   //for checksum

#define BUF_SIZE 1500

//global variable
char *public_ip;
char *subnet_mask;
char *internal_ip;
int bucket_size;
int fill_rate;
unsigned int local_mask;
unsigned int publicIP;
int port_used[2001]={0};

int findport()
{
  int i;
  for(i = 10000;i <= 12000; i++)
  {
    if(port_used[i-10000]==0)
    {
      port_used[i-10000]=1;
      return i;
    }
  }
  return -1;
}

static int Callback(struct nfq_q_handle *myQueue, struct nfgenmsg *msg,
    struct nfq_data *pkt, void *cbData) {
  // Get the id in the queue
  unsigned int id = 0;

  struct nfqnl_msg_packet_hdr *header;
  if (header = nfq_get_msg_packet_hdr(pkt)) {
    id = ntohl(header->packet_id);
  }

  // Access IP Packet
  unsigned char *pktData;
  int ip_pkt_len = nfq_get_payload(pkt, &pktData);
  struct iphdr *ipHeader = (struct iphdr *)pktData;
  
  //Drop non-UDP packet
  if (ipHeader->protocol != IPPROTO_UDP) {
    printf("Wrong protocol\n");
    return nfq_set_verdict(myQueue, id, NF_DROP, 0, NULL);
  }

  //Get IP addr
  uint32_t src_ip = ntohl(ipHeader->saddr);
  uint32_t dest_ip=ntohl(ipHeader->daddr);

  //Access UDP Packet
  struct udphdr *udph=(struct udphrd*)(((char*)ipHeader) + ipHeader->ihl*4);

  //Get snd'r/recv'er Port
  uint16_t src_port=ntohs(udph->source);
  uint16_t dest_port=ntohs(udph->dest);

  //handle mask
  int mask_int = atoi(subnet_mask);
  unsigned int local_mask= 0xffffffff << (32 - mask_int);
  struct in_addr tmp;
  inet_aton(internal_ip, &tmp);
  uint32_t local_network = ntohl(tmp.s_addr) & local_mask;

  //check in or out bound
  if((src_ip & local_mask) == local_network)
  {
    //outbound
    struct Entry *finder=search_for_outbound(src_port,src_ip);
    unsigned int new_ip;
    unsigned int new_port;
    if(finder == NULL)
    {
      //entry not found, create new one
      unsigned int trans_port = findport();
      if(trans_port == -1)
      {
        printf("no available port any more!\nbye bye!\nsee you!\ndont come back!\n:-P\n");
        exit(-1);
      }
      new_ip = public_ip;
      new_port = trans_port;
      struct Entry tmp;
      tmp.src_ip = src_ip;
      tmp.src_port = src_port;
      tmp.tran_port = trans_port;
      tmp.tran_ip = publicIP;
      insert(tmp);
    }
    else
    {
      //entry found
      new_ip=publicIP;
      new_port=finder->tran_port;
    }
    //do translation
    ipHeader->sadr = htonl(new_ip);
    udph->source = htons(new_port);

    //re-calculate checksum
    udph->check=udp_checksum(pktData);
    ipHeader->check=ip_checksum(pktData);

    //this line may be rewrite for multi thread
    return nfq_set_verdict(myQueue, id, NF_ACCEPT, ip_pkt_len, pktData);
  }
  else
  {
    //inbound
    
  }
  
}

int main(int argc, char** argv) {
   
  //Check arguments
  if(argc != 6){
    fprintf(stderr, "Usage sudo ./nat <IP> <LAN> <MASK> <bucket size> <fill rate>");
    exit(1);
  }

  // Get a queue connection handle from the module
  struct nfq_handle *nfqHandle;
  if (!(nfqHandle = nfq_open())) {
    fprintf(stderr, "Error in nfq_open()\n");
    exit(-1);
  }

  // Unbind the handler from processing any IP packets
  if (nfq_unbind_pf(nfqHandle, AF_INET) < 0) {
    fprintf(stderr, "Error in nfq_unbind_pf()\n");
    exit(1);
  }

  // Install a callback on queue 0
  struct nfq_q_handle *nfQueue;
  if (!(nfQueue = nfq_create_queue(nfqHandle,  0, &Callback, NULL))) {
    fprintf(stderr, "Error in nfq_create_queue()\n");
    exit(1);
  }
  // nfq_set_mode: I want the entire packet 
  if(nfq_set_mode(nfQueue, NFQNL_COPY_PACKET, BUF_SIZE) < 0) {
    fprintf(stderr, "Error in nfq_set_mode()\n");
    exit(1);
  }

  struct nfnl_handle *netlinkHandle;
  netlinkHandle = nfq_nfnlh(nfqHandle);

  int fd;
  fd = nfnl_fd(netlinkHandle);

  int res;
  char buf[BUF_SIZE];

  //Key in the arguments
  public_ip = argv[1];
  internal_ip = argv[2];
  subnet_mask = argv[3];
  bucket_size = atoi(argv[4]);
  fill_rate = atoi(argv[5]);
  
  //handle global var (char)public ip to (unsigned int)publicIP
  inet_aton(public_ip,&publicIP);
  publicIP = ntohl(publicIP);


  while ((res = recv(fd, buf, sizeof(buf), 0)) && res >= 0) {
    check_time();
    nfq_handle_packet(nfqHandle, buf, res);
  }

  nfq_destroy_queue(nfQueue);
  nfq_close(nfqHandle);
  return 0;
}