#ifndef LIB
#define LIB

#define SEND_CLIENT_ID 3
#define SEND_SERVER_ID 4
#define SEND_RECONNECTION_SERVER_ID 5
#define SEND_RECONNECTION_CLIENT_ID 6


#include <sys/types.h>
#include <arpa/inet.h>

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include"connection_library.h"


struct message;
struct single_connection;

void assert_unsigned_int_size();

void assert_custom_uint64_t_size();

uint64_t fntohll(uint64_t value);

uint64_t fhtonll(uint64_t value);

int send_data(int socket, struct message* message, int kind);

DATALENGTH receive_data(struct single_connection* scon, DATALENGTH* recv_status, struct message** rcv_message);

#endif
