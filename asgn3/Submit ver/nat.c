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
#include <linux/netfilter.h> // required by NF_ACCEPT, NF_DROP, etc...
#include <libnetfilter_queue/libnetfilter_queue.h>

#include "checksum.h"   //for checksum
#include "entry.h"        //for entry table

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
int num_token;
double prev_time;
double curr_time;
struct nfq_data *pkt_buff[10]={NULL};    //to store packet data for multi thread
struct nfq_q_handle *myqueue[10]={NULL};  //to store queue info
pthread_mutex_t mutex;
pthread_mutex_t bucket;

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

void check_port()
{
  int i;
  struct Entry *finder;
  for(i = 10000;i <= 12000; i++)
  {
    if(port_used[i-10000]==1)
    {
      unsigned int port= (unsigned int)i;
      finder=search_for_inbound(port);
      if(finder == NULL)
        port_used[i-10000]=0;
    }
  }
}

static int Callback(struct nfq_q_handle *myQueue, struct nfgenmsg *msg,
    struct nfq_data *pkt, void *cbData) {
  //check available
  int i;
  int flag=-1;
  for(i=0;i<10;i++)
  {
    if(pkt_buff[i]==NULL)
    {
      pthread_mutex_lock(&mutex);
      printf("Callback loop i=%d\n",i);
      //we have a place to handle, store to buffer
      flag=i;
      pkt_buff[i]=pkt; //(struct nfq_data*) malloc(sizeof(struct nfq_data));
      myqueue[i]=myQueue;
      pthread_mutex_unlock(&mutex);
      break;
    }
  }
  if(flag == -1)
  {
    printf("no enough user space, DROP!\n");
    unsigned int id = 0;

    struct nfqnl_msg_packet_hdr *header;
    if ((header = nfq_get_msg_packet_hdr(pkt))) {
      id = ntohl(header->packet_id);
    }
    return nfq_set_verdict(myQueue, id, NF_DROP, 0, NULL);
  }
  else
  {
    return 1;
  }
  
}

double get_time(){ // in millisecond
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec * 1000 + (t.tv_usec) / 1000;
}

int consume_token(){
  pthread_mutex_lock(&bucket);
  curr_time = get_time();
  int time_diff = ((int)(curr_time - prev_time) / 1000);
  if(time_diff){
    num_token += fill_rate * time_diff;
    prev_time += 1000 * time_diff;
    curr_time = get_time();
    if(num_token >= bucket_size){
      num_token = bucket_size;
      prev_time = get_time();
    }
  }
  pthread_mutex_unlock(&bucket);
  if(num_token > 0){
    num_token--;
    return 1;
  }
  return 0;
}

void get_token()[
  while(!consume_token()){
    if(nanosleep(&tim1, &tim2) < 0){
      printf("ERROR: nanosleep() system call failed!\n");
      exit(1);
    }
  }
]

void *handle_thread()
{
  printf("handle_thread on\n");
  //thread func to handle
  while(1)
  {
    //use loop to handle all
    int i;
    for(i=0;i<10;i++)
    {
      if(pkt_buff[i]!=NULL)
      {
        pthread_mutex_lock(&mutex);
        printf("handle thread , got pkt in i=%d\n",i);
        //get the pkt data and reset buff to NULL
        struct nfq_data *pkt=pkt_buff[i];
        pkt_buff[i]=NULL;
        struct nfq_q_handle *myQueue=myqueue[i];
        myqueue[i]=NULL;
        pthread_mutex_unlock(&mutex);
        // Get the id in the queue
        unsigned int id = 0;

        struct nfqnl_msg_packet_hdr *header;
        if ((header = nfq_get_msg_packet_hdr(pkt))) {
          id = ntohl(header->packet_id);
        }

        // Access IP Packet
        unsigned char *pktData;
        int ip_pkt_len;
        ip_pkt_len = nfq_get_payload(pkt, &pktData);
        struct iphdr *ipHeader;
        ipHeader = (struct iphdr *)pktData;
        
        //Drop non-UDP packet
        if (ipHeader->protocol != IPPROTO_UDP) {
          printf("Wrong protocol\n");
          //this line may be rewrite for multi thread
          nfq_set_verdict(myQueue, id, NF_DROP, 0, NULL);
          continue;
        }

        //Get IP addr
        uint32_t src_ip = ntohl(ipHeader->saddr);
        uint32_t dest_ip=ntohl(ipHeader->daddr);

        //Access UDP Packet
        struct udphdr *udph;
        udph=(struct udphdr*)(((char*)ipHeader) + ipHeader->ihl*4);

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
            new_ip = publicIP;
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
            //refresh time count
            finder->time=time(NULL);
          }
          //do translation
          ipHeader->saddr = htonl(new_ip);
          udph->source = htons(new_port);
          //printf("Outbound:dest/src port after trans: %u / %u\n",(unsigned int)ntohs(udph->dest),(unsigned int)(ntohs(udph->source)));

          //re-calculate checksum
          udph->check=udp_checksum(pktData);
          ipHeader->check=ip_checksum(pktData);

          //this line may be rewrite for multi thread
          nfq_set_verdict(myQueue, id, NF_ACCEPT, ip_pkt_len, pktData);
        }
        else
        {
          //inbound
          struct Entry *finder = search_for_inbound(dest_port);
          if(finder != NULL)
          {
            //entry found
            //refresh time count
            finder->time=time(NULL);
            //do translation
            ipHeader->daddr = htonl(finder->src_ip);
            udph->dest = htons(finder->src_port);

            //printf("Inbound:dest/src port after trans: %u / %u\n",(unsigned int)ntohs(udph->dest),(unsigned int)(ntohs(udph->source)));

            //recalculate checksum
            udph->check = udp_checksum(pktData);
            ipHeader->check = ip_checksum(pktData);

            //this line may be rewrite for multi thread
            nfq_set_verdict(myQueue, id, NF_ACCEPT, ip_pkt_len, pktData);
            printf("end one pkt\n");
          }
          else
          {
            //404 NOT FOUND drop it
            //this line may be rewrite for multi thread
            nfq_set_verdict(myQueue, id, NF_DROP, 0, NULL);
          }
        }
        printf("end one pkt\n");
      }
    }

    //take a short break
    usleep(10);
  }
  pthread_exit(NULL);
}

int main(int argc, char** argv) {
   
  //Check arguments
  if(argc != 6){
    fprintf(stderr, "Usage sudo ./nat <IP> <LAN> <MASK> <bucket size> <fill rate>");
    exit(1);
  }

  //initialize mutex
  pthread_mutex_init(&mutex,NULL);
  pthread_mutex_init(&bucket, NULL);

  // Get a queue connection handle from the module
  struct nfq_handle *nfqHandle;
  if (!(nfqHandle = nfq_open())) {
    printf("Error in nfq_open()\n");
    exit(-1);
  }

  // Unbind the handler from processing any IP packets
  if (nfq_unbind_pf(nfqHandle, AF_INET) < 0) {
    printf("Error in nfq_unbind_pf()\n");
    exit(1);
  }

  // Install a callback on queue 0
  struct nfq_q_handle *nfQueue;
  if (!(nfQueue = nfq_create_queue(nfqHandle,  0, &Callback, NULL))) {
    printf("Error in nfq_create_queue()\n");
    exit(1);
  }
  // nfq_set_mode: I want the entire packet 
  if(nfq_set_mode(nfQueue, NFQNL_COPY_PACKET, BUF_SIZE) < 0) {
    printf("Error in nfq_set_mode()\n");
    exit(-1);
  }

  struct nfnl_handle *netlinkHandle;
  netlinkHandle = nfq_nfnlh(nfqHandle);

  //Key in the arguments
  public_ip = argv[1];
  internal_ip = argv[2];
  subnet_mask = argv[3];
  bucket_size = atoi(argv[4]);
  fill_rate = atoi(argv[5]);

  //handle global var (char)public ip to (unsigned int)publicIP
  struct in_addr temp;
  inet_aton(public_ip,&temp);
  publicIP = ntohl(temp.s_addr);
  int fd;
  fd = nfnl_fd(netlinkHandle);

  int res;
  char buf[BUF_SIZE];

  //create a thread to to handle
  pthread_t handle;
  if(pthread_create(&handle,NULL,handle_thread, NULL))
  {
    printf("Fail to create handle_thread!\n");
    exit(-1);
  }

  printf("start receiving\n");

  prev_time = get_time();
  num_token = bucket_size;

  struct timespec tim1, tim2;
  tim1.tv_sec = 0;
  tim1.tv_nsec = 5000;

  while((res = recv(fd, buf, sizeof(buf), 0)) && res >= 0){
      get_token();
      check_time();
      check_port();
      nfq_handle_packet(nfqHandle, buf, res);
  }
  pthread_mutex_destroy(&mutex);
  pthread_mutex_destroy(&bucket);
  nfq_destroy_queue(nfQueue);
  nfq_close(nfqHandle);


  return 0;
}