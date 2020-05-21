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
struct Entry *search(unsigned int port);
void insert(struct Entry new_one);
void showTable();

#endif