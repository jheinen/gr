#ifndef CONLIB
#define CONLIB


#define DATALENGTH unsigned int
#define USE_MEMCOPY 0
#define DONT_USE_MEMCOPY 1

#include <unistd.h>
#include <pthread.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>

#include "queue.h"
#include "linked_list.h"
#include "library.h"

#ifdef _WIN32
/* Headerfiles für Windows */
#include <winsock.h>
#include <io.h>

#else
/* UNIX/Linux */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

struct message{
    DATALENGTH number;
    void* data;
    DATALENGTH datalength;
    int how_much_sent;
    struct single_connection* scon;
    int fully_sent;
};

struct size_number_data{
    DATALENGTH size;
    DATALENGTH request_number;
    void* data;
};

struct connection_id_and_socket{
    DATALENGTH connection_id;
    DATALENGTH socket;
};

struct message_kind{
  int kind;
  struct message* message;
};

struct connection_wish{

    int pipe_from_user; /*Pipeline to communicate between Threads*/
    int pipe_to_user; /*Pipeline to communicate between Threads*/
    int pipe_to_network; /*Pipeline to communicate between Threads*/
    int pipe_from_network; /*Pipeline to communicate between Threads*/
    struct queue_q *request_queue; /*Queue which contains request data*/
    struct queue_q *response_queue; /*Queue which contains response data*/
    int server_port; /**/
    char* server_ip; /*Server Adress to connect with*/
    int client_port; /**/
    char* client_ip; /**/
};

struct server_configuration{
    const char* server_ip; /*Server Adress to connect with*/
    int type; /* Type of Communication, IPv4 for example*/
    int protocol; /*TCP or UDP*/
    int port; /*Server Port to connect with*/
    char* accepted_clients;
    void (*handle_message_func)(struct message*); /*function which handles incoming request*/
};

struct single_server{
    int id; /*id to adress the server*/
    int serversocket; /*sockets to achieve the server*/
    struct list_plus_size* connections; /*contains list of serverconnections*/
    void (*handle_message_func)(struct message*);/*function to handle all connections*/
    char* server_ip;
};

struct network_thread_args{
    struct queue_q* network_queue;
    int connection_pipe;
};

struct target_address{
    DATALENGTH server_port;
    DATALENGTH client_port;
    char* server_ip;
    char* client_ip;
};

struct single_connection{ /*struct which represents one single Connection*/
    int socket; /*contains number of socket*/
    int connection_id; /*to adress connection*/
    struct queue_q *queue_to_network; /* queue which contains requests*/
    struct queue_q *queue_from_network; /*queue which contains responses*/
    int pipe_to_network; /*pipeline to inform NetworkThread about new data*/
    int pipe_from_network; /*contains data if answer for synch message is there*/
    int pipe_to_user; /*contains data if answer for synch message is there*/
    int pipe_from_user; /*pipeline to inform Networkthread about new data*/
    int status; /*shows if connection is used by thread 1: is used, 2: not active, should be closed soon*/
    pthread_mutex_t connection_mutex; /*mutex to lock conection to other Threads*/
    void (*handle_message_func)(struct message*); /*Callback to handle incoming messages*/
    time_t last_interaction; /*the last time data was received from this connection*/
    struct target_address* target_address; /*information about target address*/
    DATALENGTH client_id; /*id client uses*/
    int client_id_set; /*boolean*/
    DATALENGTH server_id; /*id server uses*/
    int server_id_set; /*boolean*/
    struct list_plus_size* current_messages; /*message which is currently send/received*/
    struct list_plus_size* messages_to_be_send; /*messages that arent fully sent*/
    struct list_plus_size* messages_beeing_received; /*messages that arent fully received*/
};

struct context_object{
    struct list_plus_size* connections; /*Linked List with single Connections as Element*/
    struct list_plus_size* servers; /*list of different servers, belong to this context*/
    struct queue_q *to_network_queue; /*queue which receives connection-requests*/
    struct queue_q *to_context_queue; /*queue which responses connection-requests*/
    int connection_request_pipe_read; /*pipe between context and Networkthread*/
    int connection_request_pipe_write; /*pipe between context and Networkthread*/
    int connection_response_pipe_read; /*pipe between context and Networkthread*/
    int connection_response_pipe_write; /*pipe between context and Networkthread*/
    DATALENGTH request_number; /*needed to generate request numbers*/
    time_t diff;   /*time for Hartbeat*/
    time_t limit; /*closes connection after no reply for limit seconds*/
};


static pthread_mutex_t bibmutex; /*secure Access to queues*/

/*return pointer to new struct message*/
struct message* new_message();

/*returns new response message with equal socket and number than given message*/
struct message* get_response_message(struct message* message, DATALENGTH size, void* data, int use_memcopy);

void push_message_to_sending_list(struct list_plus_size* messages_to_be_send, struct message* message, int kind);

/*sends some data from every message, that should be send*/
void send_message_list(struct single_connection* act_con);
/*API: send message with data*/

int delete_socket_from_list(struct list_plus_size* sockets_to_be_deleted, int connection_id);
/**/

DATALENGTH receive_connection(struct single_connection* act_con, DATALENGTH* status_pointer);
/**/

void message_send(struct message* message);

/*deletes single filedescriptor from filedescriptor list*/
static void delete_fd_from_pollset(int fd, int* nfds, struct pollfd fds[]);

/*trys to reconnect connection after communication partner didnt answer heartbeat*/
void try_reconnection(struct single_connection* act_con, int* nfds, struct pollfd fds[],
   struct list_plus_size* sockets_to_be_deleted, int to_client_or_server);

/*deletes connection if from network queue is empty */
void delete_connection(struct single_connection* scon, struct list_plus_size* context_connections,
    struct list_plus_size* server_connections, struct list_plus_size* connection_ids, int* nfds, struct pollfd fds[]);

/*removes all current messages from a connection*/
void clear_messagelist_and_set_status(struct single_connection* scon,
  DATALENGTH status);

/*Networkt Thread*/
static void* create_network_thread(void* args);

/*returns a new context object*/
struct context_object* init_context(int default_port,
                                    void (*handle_message_func)(struct message* message), time_t diff, time_t limit);

/*set heartbeat interval and max time interval*/
void context_set_heartbeat(struct context_object* context, time_t heartbeat, time_t limit);

/*trys to open a new connection */
int new_connection(struct context_object* context, char* server_ip,
                   int server_port, char* client_ip, int client_port);

/*sends message about connection with id=id*/
int nb_send_message(struct context_object* context, void*data, DATALENGTH size,
                 int connection_id, int wt, int timeout);

/*trys to recv message, waits if wt set to 1*/
int nb_recv_message(struct context_object* context, int connection_id,
                 struct message** rcv_message , int wt, int timeout);

/*closes one single connection by id*/
int close_connection(struct context_object* context, int connection_id);

/*closes every single connection from one context object*/
void close_all_connections(struct context_object* context);

/*creates new Server with given params at one context object*/
int create_server(struct context_object* context, int port, char* accepted_clients,
                  void (*handle_message_func)(struct message* message));

#endif
