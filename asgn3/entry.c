#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "entry.h"
#include <time.h>

struct Entry *head = NULL;

//insert a new entry to entry table
void insert(struct Entry new_one)
{
    struct Entry *entry=(struct Entry*) malloc(sizeof(struct Entry));
    entry->src_ip=new_one.src_ip;
    entry->src_port=new_one.src_port;
    entry->tran_port=new_one.tran_port;
    entry->tran_ip=new_one.tran_ip;
    entry->time=time(NULL);
    entry->next=head;
    head=entry;
}

struct Entry *search(unsigned int port)
{
    struct Entry *current = NULL;
    if(head==NULL)
        return NULL;
    current=head;
    while(current != NULL)
    {
        if(current->tran_port == port)
            return current;
        current=current->next;
    }
    return NULL;
}

void check_time()
{
    struct Entry *current = head;
    struct Entry *prev;
    time_t now=time(NULL);
    double duration;
    if(head!=NULL)
    {
        while(current !=NULL)
        {
            duration=difftime(now,current->time);
            if(duration >= 10.0)
            {
                //last one time out
                if(current->next == NULL)
                {
                    //also is the only one
                    if(head == current)
                    {
                        free(current);
                        head=NULL;
                        current=NULL;
                    }
                    else
                    {
                        free(current);
                        prev->next=NULL;
                        current=NULL;
                    }
                }
                //not the tail
                else
                {
                    //cur is head
                    if(head==current)
                    {
                        head=current->next;
                        free(current);
                        current=head;
                    }
                    //cur is in not head and tail
                    else
                    {
                        prev->next=current->next;
                        free(current);
                        current=prev->next;
                    }
                }
            }
            else
            {
                prev=current;
                current=prev->next;
            }
        }
    }
}

void showTable()
{
    printf("NAT table:\n");
    printf("  source IP - Port  |  translated IP - Port\n");

    struct Entry *cur=head;
    while(cur != NULL)
    {
        struct in_addr tmp;
        tmp.s_addr = htonl(cur->src_ip);
        printf(" (%s , %d) ",(char*)inet_ntoa(tmp),cur->src_port);
        tmp.s_addr = htonl(cur->tran_ip);
        printf(" (%s , %d) ",(char*)inet_ntoa(tmp),cur->tran_port);
        printf("\n");
        cur=cur->next;
    }
    printf("\n\n");
}