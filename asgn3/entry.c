#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include "entry.h"
#include <time.h>

#define ONE_SEC ((clock_t)1000);

struct Entry *head = NULL;
struct Entry *current = NULL;

//insert a new entry to entry table
void insert(struct Entry new_one)
{
    struct Entry *entry=(struct Entry*) malloc(sizeof(struct Entry));
    entry->in_ip=new_one->in_ip;
    entry->in_port=new_one->in_port;
    entry->out_port=new_one->out_port;
    entry->time=clock();
    entry->next=head;
    head=entry;
}

struct Entry *search(unsigned int port)
{
    
}