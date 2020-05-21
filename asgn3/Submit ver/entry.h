#ifndef __TABLE__

#define __TABLE__

struct Entry{
    uint32_t src_ip;
    uint16_t src_port;
    uint32_t tran_ip;
    uint16_t tran_port;
    time_t time;
    struct Entry *next;
};

void check_time();
struct Entry *search_for_inbound(unsigned int port);
struct Entry *search_for_outbound(unsigned int port,unsigned int ip);
void insert(struct Entry new_one);
void showTable();

#endif