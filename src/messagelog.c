#include "messagelog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_log(struct log* log){
    log->loglen = 0;
    log->head = NULL;
    log->tail = NULL;
}

struct message* add_log(struct log* log, char* msg){
    struct message* newMsg = malloc(sizeof(struct message));
    strcpy(newMsg->msg, msg);
    newMsg->next = NULL;
    newMsg->last = NULL;
    
    if(log->tail == NULL){
        log->head = newMsg;
        log->tail = newMsg;
    }else if(log->head == log->tail){
        log->tail = newMsg;

        log->head->next = newMsg;
        newMsg->last = log->head;
    }else{
        newMsg->last = log->tail;
        log->tail->next = newMsg;

        log->tail = newMsg;
    }
    log->loglen++;

    if(log->loglen > MAXLOG){
        struct message* head = log->head;
        log->head = log->head->next;
        log->loglen--;

        free(head);
    }

    printf("[DEBUG] Current log len: %d\n", log->loglen);

    return newMsg;
}