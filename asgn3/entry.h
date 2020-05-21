#ifndef __TABLE__

#define __TABLE__

struct Entry{
    uint32_t in_ip;
    uint16_t in_port;
    uint16_t out_port;
    int time;
    struct Entry *next;
};

void check_time();
struct Entry *search(unsigned int port);
void insert(struct Entry new_one);

#endif