#include "connection_library.h"

//int client_port = 7017; /*TODO send via connection*/
char* client_ip = "127.0.0.1"; /*TODO send via connection*/

struct message* new_message(){
  struct message* new_message = malloc(sizeof(struct message));
  new_message->how_much_sent = 0;
  new_message->fully_sent = 0;
  return new_message;
}

/*returns new response message with equal socket and number*/
struct message* get_response_message(struct message* message, DATALENGTH size, void* data, int use_memcopy){
  struct message* response_message = new_message();
  response_message->scon = message->scon;
  response_message->number = message->number;
  if (use_memcopy == USE_MEMCOPY){
    void* datapointer = malloc(size);
    memcpy(datapointer, data, size);
    response_message->data = datapointer;
  }
  else if (use_memcopy == DONT_USE_MEMCOPY){
    response_message->data = data;
  }
  response_message->datalength = size;
  return response_message;
}

/*Adds message user wants to send, to queue_to_network*/
void message_send(struct message* message){
  //TODO send_data_optimiert(message->scon->socket, message->data,
  //message->datalength, message->number, 0);
  /*struct queue_q *queue_to_network = message->scon->queue_to_network;
  int pipe_to_network = message->scon->pipe_to_network;
  pthread_mutex_lock(&message->scon->connection_mutex);
  queue_enqueue(queue_to_network, message);
  pthread_mutex_unlock(&message->scon->connection_mutex);

  char ch = 1;
  if (write(pipe_to_network, &ch, 1) < 1){
    printf("couldnt write into pipe(message send)\n");
    exit(1);
  }
  printf("Ende message send\n"); */
  push_message_to_sending_list(message->scon->messages_to_be_send, message, 8);
  char ch = 1;
  int pipe_to_network = message->scon->pipe_to_network;

  if (write(pipe_to_network, &ch, 1) < 1){
    printf("couldnt write into pipe(message send)\n");
    exit(1);
  }
}

/*sends single message */
void push_message_to_sending_list(struct list_plus_size* messages_to_be_send, struct message* message, int kind){

  struct message_kind* message_kind = malloc(sizeof(struct message_kind));
  message_kind->message = message;
  message_kind->kind = kind;
  push(messages_to_be_send, message_kind);
}

void send_message_list(struct single_connection* act_con){
  //printf("in send message list\n");
  /*iterate over list of messages to send*/
  int message_index;
  int message_count = act_con->messages_to_be_send->size;
  //printf("Anzahl der Nachrichten in messages_to_be_send: %d\n", message_count);
  /*for (message_index=0; message_index < message_count; message_index++){
    pthread_mutex_lock(&act_con->connection_mutex);
    struct message_kind* message_and_kind = get_by_index(act_con->messages_to_be_send, message_index);
    pthread_mutex_unlock(&act_con->connection_mutex);
    struct message* act_message = message_and_kind->message;
  }*/
  int to_delete_index[message_count];
  for (message_index=0; message_index < message_count; message_index++){
    to_delete_index[message_index] = -1;
  }

  for (message_index = 0; message_index < message_count; message_index++){
    /*send some data from every message*/
    struct message_kind* message_and_kind = get_by_index(act_con->messages_to_be_send, message_index);
    struct message* act_message = message_and_kind->message;
    /*send some data from message*/
    if (message_and_kind->kind < 8){ /*remove message from list, one send_data_optimiert call is enough to send whole message*/
      //message_and_kind = remove_by_index(&(act_con->messages_to_be_send), message_index);
      to_delete_index[message_index] = message_index;
    }
    int ret_val;
    if ((ret_val = send_data_optimiert(act_con->socket, act_message, message_and_kind->kind)) < 0){
      printf("Versenden der Daten hat nicht funktioniert, kind:%d \n", message_and_kind->kind);
      perror("Error occurred while trying to send some data\n");
      if(message_and_kind->kind == 8){
        act_message->fully_sent = -1;
        printf("act_message->fully_sent gesetzt\n");
      }
    }
    /*check if message is sent completely and possibly remove byte from User Pipeline*/
    if (ret_val == 3){ /*message sent completely*/

      int pipe_from_user = act_con->pipe_from_user;
      char ch;
      int piperesult = read(pipe_from_user, &ch, 1);
      if (piperesult < 0){
        perror("Could not read from connection pipeline\n");
        exit(1);
      }
      to_delete_index[message_index] = message_index;
      //message_and_kind = remove_by_index(&(act_con->messages_to_be_send), message_index-tmp);
    }
  }
  /*Delete messages*/
  int tmp = 0;
  for(message_index = 0; message_index < message_count; message_index++){
    if (to_delete_index[message_index] != -1){
      remove_by_index(act_con->messages_to_be_send, message_index-tmp);
      tmp += 1;
    }
  }
}

int delete_socket_from_list(struct list_plus_size* sockets_to_be_deleted, int connection_id){
  /*Reconnection successfull, delete socket in sockets_to_be_deleted*/
  int stbd_size = list_size(sockets_to_be_deleted);
  struct connection_id_and_socket* stbd_element;
  int stbd_element_int;
  for (stbd_element_int = 0; stbd_element_int < stbd_size; stbd_element_int ++){
    stbd_element = get_by_id(sockets_to_be_deleted, stbd_element_int);
    if (stbd_element->connection_id == connection_id){
#ifdef _WIN32
    closesocket(stbd_element->socket);
    WSACleanup();
#else
      close(stbd_element->socket);
#endif
      remove_by_id(sockets_to_be_deleted, stbd_element_int);
      printf("Socket von Verbindung : %d geschlossen\n", stbd_element->connection_id);
      return 1;
    }
  }
  return 0;
}

void clear_messagelist_and_set_status(struct single_connection* scon,
  DATALENGTH status) {
    printf("In clear Messagelist and set Status, status: %d\n", status);
    struct list_plus_size* tmp = scon->messages_beeing_received;
    void* tmp_message;
    while(list_size(tmp) != 0){
      //tmp = tmp->next;
      tmp_message = pop(tmp);
      free(tmp_message);
      if(tmp == NULL){break;}
    }
    scon->messages_beeing_received->list->val = NULL;
    scon->messages_beeing_received->list->next = NULL;
    scon->last_interaction = time(NULL);
    scon->status = status;
  }

DATALENGTH receive_connection(struct context_object* context, struct single_connection* act_con, DATALENGTH* recv_status){

  DATALENGTH recvd;
  struct message* rcv_message = new_message();
  rcv_message->scon = act_con;
  recvd = receive_data_optimiert(act_con, recv_status, &rcv_message);
    //if (*recv_status == 8 && rcv_message->datalength == rcv_message->how_much_sent){
    //    printf("Adresse der rcv_message: %p\n", rcv_message);
    //}
  /*recv cases*/
  if (*recv_status == 0){
    printf("Verbindung mit id: %d soll geschlossen werden \n", act_con->connection_id);
    clear_messagelist_and_set_status(act_con, 2);
    return 0;
  }
  if (*recv_status == 1){
    /*recvd part of a mesage*/
    act_con->last_interaction = time(NULL);
    return 0;
  }
  if (*recv_status == 2){
    printf("Client default port received\n");
    act_con->last_interaction = time(NULL);
    return 0;
  }
  if (*recv_status == 3){
    /*Client Port recvd*/
    act_con->last_interaction = time(NULL);
    return 0;
  }
  if (*recv_status == 4){
    //printf("Recieved Connection ID from Server\n");
    act_con->last_interaction = time(NULL);
    return 0;
  }
  if(*recv_status == 5){
    /*return value of recv_data = index of connection that should be reconnected*/
    return recvd;
  }
  if(*recv_status == 6){
    return recvd;
  }
  if(*recv_status == 7){
    /*Reconnection successfull, delete socket in sockets_to_be_deleted*/
    /*recd contains connection_id which is needed to remove */
    act_con->last_interaction = time(NULL);
    return recvd;
  }

  if (*recv_status == 8 && rcv_message->datalength == rcv_message->how_much_sent){
    /* full message received succesfull*/
    act_con->last_interaction = time(NULL);
    int existing_callback = 0;
    struct  id_callback* id_callback;
    pthread_mutex_lock (&bibmutex);
    for (int i=0; i< list_size(context->callbacks); i++){
      id_callback = (struct id_callback*)get_by_index(context->callbacks, i);
      if (id_callback->id == rcv_message->number){
        id_callback = (struct id_callback*)remove_by_index(context->callbacks, i);
        void (*handle_message_func)(struct message* response_message) = id_callback->callback;
        free(id_callback);
        handle_message_func(rcv_message);
        existing_callback = 1;
      }
    }
    pthread_mutex_unlock (&bibmutex);

    if (act_con->handle_message_func != NULL){
      act_con->handle_message_func(rcv_message);
      existing_callback = 1;
      return 8;
    }
    if (existing_callback == 0){
      struct queue_q* to_context_queue = act_con->queue_from_network;
      pthread_mutex_lock(&act_con->connection_mutex);
      queue_enqueue(to_context_queue, rcv_message);
      pthread_mutex_unlock(&act_con->connection_mutex);
      char ch = 1;
      if (write(act_con->pipe_to_user, &ch, 1) < 0){
        perror("Could not write data into Pipeline\n");
        exit(1);
      }
    }
  }
  if(*recv_status == 9){
    /*part of message received*/
    /*pass*/
  }
  return 0;
}

static void delete_fd_from_pollset(int fd, int* nfds, struct pollfd fds[]){
  printf("in delete from pollset\n");
  int con_index;
  for(con_index = 1; con_index < *nfds; con_index++){
    if (fd == fds[con_index].fd){
      printf("delete_fd_from_pollset fd: %d\n", fd);
      fds[con_index].fd = fds[*nfds-1].fd;
      fds[con_index].revents = fds[(*nfds)-1].revents;
      /*unset fd[nfds-1].revent */
      fds[*nfds-1].revents = 0;
      *nfds = *nfds-1;
      con_index--;
    }
  }
}

void try_reconnection(struct single_connection* act_con, int* nfds, struct pollfd fds[], struct list_plus_size* sockets_to_be_deleted){
  {
    /*if client 0, else if server 1*/
    struct sockaddr_in serverAddr;
    /*try reconnection*/

    /*create new connection socket*/
    int newsocket = socket(AF_INET, SOCK_STREAM, 0);
    if (newsocket < 0){
      perror("error creating new socket for client connection\n");
    }
    fcntl(newsocket, F_SETFL, O_NONBLOCK);
    /*configure serverAddress*/
    if(act_con->type == 0){
      /*connect to server*/
      serverAddr.sin_port = htons(act_con->target_address->server_port);
      serverAddr.sin_addr.s_addr = inet_addr(act_con->target_address->server_ip);
    }
    else if(act_con->type == 1){
      /*connect to client*/
      serverAddr.sin_port = htons(act_con->target_address->client_port);
      serverAddr.sin_addr.s_addr = inet_addr(act_con->target_address->client_ip);
    }
    serverAddr.sin_family = AF_INET;
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
    int connectret = -2147483648;
    struct timespec begin, end;
    clock_gettime(CLOCK_REALTIME, &begin);
    while (connectret < 0 ){
      /*perror("Error printed by perror");*/
      connectret = connect(newsocket, (struct sockaddr *) &serverAddr,
      sizeof(serverAddr));
      if (errno == EALREADY || errno == EINPROGRESS){
        clock_gettime(CLOCK_REALTIME, &end);
        long seconds = end.tv_sec - begin.tv_sec;
        long nanoseconds = end.tv_nsec - begin.tv_nsec;
        double elapsed = seconds + nanoseconds*1e-9;
        if (elapsed > 1.0){
          printf("Could not Connect to %s\n", act_con->target_address->server_ip);
          delete_fd_from_pollset(act_con->pipe_from_user, nfds,  fds);
          act_con->status = 2; /*delete connection*/
          break;
        }
      }
      else if (errno == EISCONN){
        printf("CONNECT SUCCESSFULL\n");
        int tmp_socket = act_con->socket;
        act_con->socket = newsocket;
        delete_fd_from_pollset(tmp_socket, nfds,  fds);
        /*send id, the server used for the connection to the server*/
        struct message* message = new_message();
        message->scon = act_con;
        /*tell connnection partner that connnection already existed*/
        //send_data_optimiert(act_con->socket, message, 5);
        //push_message_to_sending_list(act_con->messages_to_be_send, message, 5);
        send_data_optimiert(act_con->socket, message, 5);
        /*if server did not find the connection, resend clients con_id*/
        //send_data_optimiert(newsocket, message, 3);
        if(act_con->type == 0){
          //push_message_to_sending_list(act_con->messages_to_be_send, message, 3);
          send_data_optimiert(act_con->socket, message, 3);
        }
        else if(act_con->type ==  1){
          //push_message_to_sending_list(act_con->messages_to_be_send, message, 4);
          send_data_optimiert(act_con->socket, message, 4);
        }
        free(message);
        /*now remove the old socket and replace it with the new one*/
        struct connection_id_and_socket* con_id_and_socket = malloc(sizeof(struct connection_id_and_socket));
        con_id_and_socket->connection_id = act_con->connection_id;
        con_id_and_socket->socket = tmp_socket;
        push(sockets_to_be_deleted, con_id_and_socket);
        //close(tmp_socket);

        act_con->status = 0; /*succses*/
        fds[*nfds].fd = newsocket;
        fds[*nfds].events = POLLIN;
        *nfds = (*nfds) + 1;
        break;
      }
      else{
        printf("couldnt reconnect to Server, close connection\n");
        act_con->status = 2;
      }
    }
    act_con->last_interaction = time(NULL);

  }
}

struct single_connection* new_connection_struct(){
  /*returns a new connection*/

  struct single_connection* scon = (struct single_connection*)malloc(
    sizeof(struct single_connection));
    scon->queue_to_network = NULL;
    scon->queue_from_network = NULL;
    scon->status = 0;
    scon->current_messages = new_list();
    pthread_mutex_t connection_mutex;
    pthread_mutex_init(&connection_mutex, NULL);
    scon->connection_mutex = connection_mutex;
    scon->handle_message_func = NULL;
    scon->last_interaction = time(NULL);
    scon->target_address = malloc(sizeof(struct target_address));
    scon->target_address->server_ip = NULL;
    scon->target_address->client_ip = NULL;
    scon->client_id_set = 0;
    scon->server_id_set = 0;
    scon->messages_to_be_send = new_list();
    scon->messages_beeing_received = new_list();
    scon->can_be_deleted = 1;
    return scon;
  }

  void delete_connection(struct single_connection* scon, struct list_plus_size* context_connections,
    struct list_plus_size* server_connections, struct list_plus_size* connection_ids, int* nfds, struct pollfd fds[]){

      printf("delete connection Aufruf, connection id: %d\n", scon->connection_id);
      printf("scon->socket: %d\n", scon->socket);
      pthread_mutex_lock(&bibmutex);
      if(scon->queue_to_network != NULL){
        queue_destroy(&(scon->queue_to_network));
        scon->queue_to_network = NULL;
      }
      if (scon->socket != 0){
        if(scon->socket >= 0){
          #ifdef _WIN32
              closesocket(stbd_element->socket);
              WSACleanup();
              #else
                close(scon->socket);
              #endif
            }
        printf("geschlossenes Socket: %d\n", scon->socket);
        delete_fd_from_pollset(scon->socket, nfds, fds);
        scon->socket = 0;
        /*close from user and to network pipe*/
        delete_fd_from_pollset(scon->pipe_from_user, nfds, fds);
        printf("delete fd from pollset: pipe_from_user: %d\n", scon->pipe_from_user);
        close(scon->pipe_from_user);
        close(scon->pipe_to_network);
      }
      if (queue_empty(scon->queue_from_network) == TRUE){
        queue_destroy(&(scon->queue_from_network));
        /*find connection index in pollset*/
        /*remove connection*/
        close(scon->pipe_from_network);
        close(scon->pipe_to_user);
        if(context_connections != NULL){
          remove_by_id_and_free_id(context_connections, connection_ids, scon->connection_id);
        }
        if (server_connections != NULL){
          remove_by_id_and_free_id(server_connections, connection_ids, scon->connection_id);
        }
        free(scon->target_address);
        free(scon);
      }
      pthread_mutex_unlock(&bibmutex);
    }

      /*Creates a new Networkthread, that manages all single connections*/
      static void* create_network_thread(void* args){
        int activity; /*select return value, if written in filedescriptor*/
        char ch; /*aingle char to read from pipeline*/
        struct sockaddr_in serverAddr; /*struct to define the Adress of the
        other Application*/
        /*context Object with every single connection*/
        struct context_object* context = (struct context_object*) args;
        /*queue that receives connection-requests from clients*/
        struct queue_q* client_queue = context->to_network_queue;
        /*Pipeline which informs NetworkThread that new data in queue*/
        int context_pipeline = context->connection_request_pipe_read;
        /*checks if write and read is succesfull*/
        /*pipeline to user*/
        int to_context_pipeline = context->connection_response_pipe_write;

        /*manage connection ids*/
        struct list_plus_size* connection_ids = new_list();

        /*timeout for poll, 10 sec*/
        time_t timeout = 10*1000;

        /*time for Heartbeating*/
        time_t last_heartbeat;
        last_heartbeat = time(NULL);

        int type = AF_INET; /*IPv4*/
        int protocol = SOCK_STREAM;

        //time_t limit = context->limit; /*closes connection if no iteraction fot the time of limit*/
        //time_t max_time_reconnect = 3; /*time in which library trys to reconnect*/

        /*list to save watch activity of Communication Partners*/
        //node_t* interactions = new_list();
        //node_t* client_interactions = new_list();

        /*list of sockets that will be deleted in the future*/
        struct list_plus_size* sockets_to_be_deleted = new_list();

        struct pollfd fds[10000]; /*space for filedescriptors*/
        int nfds = 1; /*number of file descriptors to watch*/
        fds[0].fd = context_pipeline;
        fds[0].events = POLLIN;

        int piperesult;

        /*endless loop, waiting for connection-wishes, data to send, or network-data*/
        while(1){
          /*delete connections if Client closed*/
          int client_con;
          for (client_con = 0; client_con < list_size(context->connections); client_con++){
            struct single_connection* act_con =
            get_by_index(context->connections, client_con);
            if (act_con->status == 2 && act_con->can_be_deleted == 1){
              delete_connection(act_con, (context->connections),
                NULL, connection_ids, &nfds, fds);
            }
            else if (act_con->status == 2 && act_con->can_be_deleted == 0){
              //printf("Else fall Status 2 aber kann nicht loeschen\n");
            }
            else if(act_con->status != 2){
              //printf("NT: vor poll, Soll nicht geloescht werden\n");
            }
              //printf("try reconnection\n");
              //try_reconnection(act_con, &nfds, fds, sockets_to_be_deleted);
          }
          int p;
          for (p = 0; p< list_size(context->servers); p++){
            struct single_server* act_server = (struct single_server*)
            get_by_index(context->servers, p);
            //printf("Anfang der Schleife, Laenge server_connections vorher: %d\n", list_size(act_server->connections));
            //int act_server_socket = act_server->serversocket;
            int connection;
            for (connection = 0; connection < list_size(act_server->connections); connection++){
              struct single_connection* act_con =
              get_by_index(act_server->connections, connection);
              if (act_con->status == 2 && act_con->can_be_deleted == 1){
                printf("rufe delete auf, type der Verbindung: %d\n", act_con->type);
                printf("server connections == NULL: %d\n", act_server->connections == NULL);
                delete_connection(act_con, NULL,
                  (act_server->connections), connection_ids, &nfds, fds);
                    printf("nach delete, Anzahl der Verbindungen context: %d\n", list_size(context->connections));
                //node_t* server_connections = act_server->connections;
              }
              else if (act_con->status == 4){
                //try_reconnection(act_con, &nfds, fds, sockets_to_be_deleted);
                }
                else if (act_con->status == 2 && act_con->can_be_deleted == 0){
                  printf("Else fall, kann nicht loeschen\n");
                }
              }
            }
              // for (int pollzahl=0; pollzahl < nfds; pollzahl ++){
              //   printf("Aktueller FD: %d\n", fds[pollzahl].fd);
              // }

          //for (int testmp = 0; testmp < nfds; testmp++){
          //  printf("FD Nummer: %d\n", fds[testmp].fd);
          //  printf("Revents & POLLIN: %d\n", fds[testmp].revents & POLLIN);
          //}
          //printf("nfds: %d\n", nfds);
          //for(int tmpp=0; tmpp<1000001; tmpp++){
          //  if (tmpp == 100000 || tmpp == 200000 || tmpp == 300000 || tmpp == 400000 || tmpp == 800000){
          //    printf("tmp: %d\n", tmpp);
          //  }
          //}
          activity = poll(fds, nfds, timeout);
          /*printf("Poll ausgeloest, Anzahl FDs: %d\n", nfds);*/
          int fd;
          for (fd = 0; fd <nfds; fd ++){
            //printf("Aktuell gecheckter Filedescriptor: %d\n", fds[fd].fd);
            //printf("Filedescriptor events: %d\n", fds[fd].events);
            //printf("Filedescriptor Revents: %d\n", fds[fd].revents);
            /*if (fds[fd].revents & POLLIN){
              printf("Der Filedeskriptor mit der Nummer: %d pollt\n", fds[fd].fd);
            }*/
          }
          /*sleep(1);*/
          /*for (int ls=0; ls <list_size(context->servers); ls++){
          struct single_server* act_servers = get_by_index(context->servers, ls);
          for (int connections = 0; connections < list_size(act_servers->connections);
          connections++){
          struct single_connection* act_connection = get_by_index(act_servers->connections, connections);
          printf("2 Connection id und Socket Adresse: %d und %p\n", act_connection->connection_id, &act_connection->socket);
        }
      }*/
      /*for(int conn_index = 0; conn_index < nfds; conn_index++){
        if (fds[conn_index].revents & POLLIN){
          printf("fds[con_index].fd: %d, Pollt: %d\n", fds[conn_index].fd, fds[conn_index].revents & POLLIN);
        }
      }*/
      /*for (int tes=1; tes< nfds; tes++){
      if (fds[tes].revents & POLLIN ){
    }
  } */
  /*TODO Heartbeat aktivieren*/
  if (time(NULL)-last_heartbeat > context->diff){ /*send Heartbeat message about every connection*/
    printf("Soll Heartbeat verschicken\n");
    /*iterate over every connection and check if it should be closed*/
    /*TODO remove*/struct single_server* ss = get_by_index(context->servers, 0);
    printf("time for Heartbeat, Anzahl Client und Server Verbingungen: %d und %d\n", list_size(context->connections), list_size(ss->connections));
    int j;
    for (j=0; j<list_size(context->connections); j++){
      struct single_connection* act_con =
      (struct single_connection*)get_by_index(context->connections, j);
      time_t last_interaction = act_con->last_interaction;
      if (time(NULL) - last_interaction > context->limit && act_con->status != 2){
        /*client connection lost -> try to reconnect, if not possible -> close*/
        act_con->status = 4; /*try reconnection*/
        try_reconnection(act_con, &nfds, fds, sockets_to_be_deleted);
      }
      else{
        /*send heartbeat*/
        if (act_con->status == 0 || act_con->status == 1){
          struct message* empty_message = NULL; /*no new struct needed, to send Heartbeat*/
          //send_data_optimiert(act_con->socket, (struct message*)&empty_message, 1);
          push_message_to_sending_list(act_con->messages_to_be_send, empty_message, 1);
        }
        else{
        }
      }
    }
      int l;
      for (l=0; l <list_size(context->servers); l++){
        /*get server at index l*/
        struct single_server* act_server = get_by_index(context->servers, l);
        int connection;
        for (connection = 0; connection < list_size(act_server->connections);
        connection++){
          struct single_connection* act_con =
            get_by_index(act_server->connections, connection);
          time_t last_interaction = act_con->last_interaction;
          if (time(NULL)- last_interaction > context->limit && act_con->status != 2){
            /*connection lost -> close it*/
            printf("Server hat Clientverbindung verloren, versuche Verbindung neu aufzubauen\n");
            printf("Versuche Verbindung zu Client auf Port: %d \n", act_con->target_address->client_port);
            act_con->status = 4; /*try reconnection*/
            printf("Setzte Status auf 4\n");
            try_reconnection(act_con, &nfds, fds, sockets_to_be_deleted);
          }
          else{ /*send heartbeat*/
            void* empty_message = NULL;
            //send_data_optimiert(act_con->socket, empty_message, 1);
            printf("Pushe Heartbeat\n");
            push_message_to_sending_list(act_con->messages_to_be_send, empty_message, 1);
          }
        }
      }
      last_heartbeat = time(NULL);
    }

    //printf("NT: Hinter Loeschungen\n");

    if(fds[0].revents & POLLIN){ /*new wish from user*/
      piperesult = read(context_pipeline, &ch, 1);
      if (piperesult < 0){
        perror("Could not read from connection pipeline\n");
        exit(1);
      }

      if (ch == 1){ /*new connection wish*/
        int connection_id;
        int success = 0;
        pthread_mutex_lock (&bibmutex);
        struct connection_wish* connection_wish =
        (struct connection_wish*) queue_dequeue(client_queue);
        pthread_mutex_unlock (&bibmutex);
        int pipe_from_user = connection_wish->pipe_from_user;
        int pipe_to_user = connection_wish->pipe_to_user;
        int pipe_to_network = connection_wish->pipe_to_network;
        int pipe_from_network = connection_wish->pipe_from_network;
        struct queue_q* request_queue = connection_wish->request_queue;
        struct queue_q* response_queue = connection_wish->response_queue;
        int server_port = connection_wish->server_port;
        char* server_ip = connection_wish->server_ip;
        int client_port = connection_wish->client_port;
        char* client_ip = connection_wish->client_ip;

#ifdef _WIN32
        SOCKET sock;
        WORD wVersionRequested;
        WSADATA wsaData;
        wVersionRequested = MAKEWORD (1, 1);
        if (WSAStartup (wVersionRequested, &wsaData) != 0)
            error_exit( "Fehler beim Initialisieren von Winsock");
        else
            printf("Winsock initialisiert\n");
#else
        int sock;
#endif
        int clientSocket = socket(type, protocol, 0);
        if (clientSocket < 0){
          perror("error creating Serverside socket\n");
        }
        fcntl(clientSocket, F_SETFL, O_NONBLOCK);
        /*configure serverAddress*/
        serverAddr.sin_family = type;
        serverAddr.sin_port = htons(server_port);
        serverAddr.sin_addr.s_addr = inet_addr(server_ip);
        memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
        int connectret = -2147483648;
        struct timespec begin, end;
        clock_gettime(CLOCK_REALTIME, &begin);
        while (connectret < 0 ){
          connectret = connect(clientSocket, (struct sockaddr *) &serverAddr,
          sizeof(serverAddr));
          clock_gettime(CLOCK_REALTIME, &end);
          long seconds = end.tv_sec - begin.tv_sec;
          long nanoseconds = end.tv_nsec - begin.tv_nsec;
          double elapsed = seconds + nanoseconds*1e-9;

          if (errno == EALREADY || errno == EINPROGRESS){
            if (elapsed > 0.2){
              printf("Could not Connect to %s\n", server_ip);
              int connection_error_id = -1;
              pthread_mutex_lock (&bibmutex);
              queue_enqueue(context->to_context_queue, (void*)(long)connection_error_id);
              pthread_mutex_unlock (&bibmutex);
              int connection_response_pipe = context->connection_response_pipe_write;
              ch = 1;
              if (write(connection_response_pipe, &ch, 1) <1){
                printf("error returning connection number\n");
              }
              break;
            }
          }
          else if (errno == EISCONN){
            success = 1;
            break;
          }
          else if (elapsed > 0.2){
            //printf("errno %d\n", errno);
            /*return failed attempt to Client*/
            connection_id = -1;
            pthread_mutex_lock (&bibmutex);
            queue_enqueue(context->to_context_queue, (void*)(long)connection_id);
            pthread_mutex_unlock (&bibmutex);
            int connection_response_pipe = context->connection_response_pipe_write;
            ch = 1;
            if (write(connection_response_pipe, &ch, 1) <1){
              printf("error write, returning connection number\n");
              exit(1);
            }
            #ifdef _WIN32
                closesocket(stbd_element->socket);
                WSACleanup();
            #else
                close(clientSocket);
            #endif
            close(pipe_from_user);
            close(pipe_to_user);
            close(pipe_to_network);
            close(pipe_from_network);
            free(request_queue);
            free(response_queue);
            break;
          }
        }
        if (success == 1){
          struct single_connection* scon = new_connection_struct();
          scon->target_address->server_port = server_port;
          scon->target_address->server_ip = server_ip;
          scon->target_address->client_port = client_port;
          scon->target_address->client_ip = client_ip;
          /*assigning all necessary compononets to connection*/
          scon->socket = clientSocket;
          scon->queue_from_network = response_queue;
          scon->queue_to_network = request_queue;
          scon->pipe_to_network = pipe_to_network;
          scon->pipe_from_network = pipe_from_network;
          scon->pipe_to_user = pipe_to_user;
          scon->pipe_from_user = pipe_from_user;
          /*insert new connection to connection list and get_id*/
          //int connection_id = push((&context->connections), (void*)scon);
          connection_id = push_with_lowest_id((context->connections), (void*)scon, connection_ids);
          printf("Id der neu erzeugten Verbindung: %d\n", connection_id);
          scon->connection_id = connection_id;
          scon->client_id = connection_id;
          scon->client_id_set = 1;
          scon->handle_message_func = NULL;
          scon->type = 0; /*client connection*/
          /*put new connection in set of watched filedescriptors*/
          fds[nfds].fd = clientSocket;
          fds[nfds].events = POLLIN;
          fds[nfds+1].fd = pipe_from_user;
          fds[nfds+1].events = POLLIN;
          nfds+=2;

          /*send client port number to server*/
          struct message* message = malloc(sizeof(struct message));
          message->scon = scon;
          push_message_to_sending_list(scon->messages_to_be_send, message, 2);
          push_message_to_sending_list(scon->messages_to_be_send, message, 3);
          //free(message);
          pthread_mutex_lock (&bibmutex);
          queue_enqueue(context->to_context_queue, (void*)(long)connection_id);
          pthread_mutex_unlock (&bibmutex);
          int connection_response_pipe = context->connection_response_pipe_write;
          ch = 1;
          if (write(connection_response_pipe, &ch, 1) <1){
            printf("error returning connection number\n");
            exit(1);
          }
        }
      }
      else if (ch == 2){ /*remove one single connection*/
        pthread_mutex_lock (&bibmutex);
        int id =  ((int)(long)queue_dequeue(client_queue));
        pthread_mutex_unlock (&bibmutex);
        if (list_contains_id(context->connections, id) == 1){
          struct single_connection* scon = get_by_id((context->connections), id);
          delete_fd_from_pollset(scon->socket, &nfds,  fds);
          delete_fd_from_pollset(scon->pipe_from_user, &nfds,  fds);
          #ifdef _WIN32
              closesocket(stbd_element->socket);
              WSACleanup();
          #else
                close(scon->socket);
          #endif
          printf("Socket %d geschlossen\n", scon->socket);
          free(scon->queue_to_network);
          scon->status = 2;
        }
        else{
          printf("Connection to remove is not in connections\n");
        }
      }
      else if (ch == 3){ /*remove all connections*/
        struct single_connection* scon;
        while(list_size(context->connections) != 0){
          scon = (struct single_connection*)pop((context->connections));
          int scon_index;
          for(scon_index = 1; scon_index < nfds; scon_index++){
            if (scon->socket == fds[scon_index].fd ){
              fds[scon_index].fd = fds[nfds-1].fd;
              nfds--;
            }
          }
          #ifdef _WIN32
              closesocket(stbd_element->socket);
              WSACleanup();
          #else
                close(scon->socket);
          #endif
          printf("Socket %d geschlossen\n", scon->socket);
          scon->status = 2;
        }
      }
      if (ch == 4){ /*create new server*/
        #ifdef SO_REUSEADDR
        int socket_opt;
        #endif
        struct sockaddr_in serverAddr;
        /*get server arguments from queue*/
        pthread_mutex_lock (&bibmutex);
        struct server_configuration* server_conf =
        queue_dequeue(context->to_network_queue);
        pthread_mutex_unlock (&bibmutex);
        struct single_server* new_server = (struct single_server*)
        malloc(sizeof(struct single_server));
        new_server->connections = new_list();
        new_server->handle_message_func = server_conf->handle_message_func;
        int server_socket = socket(server_conf->type, server_conf->protocol, 0);
        if (server_socket < 0){
          perror("error creating new serversocket\n");
          exit(1);
        }
        #ifdef SO_REUSEADDR
        socket_opt = 1;
        if (setsockopt(server_socket,SOL_SOCKET, SO_REUSEADDR,
          (char *)&socket_opt, sizeof(socket_opt)) < 0)
          {
            perror("setting socket options failed");
          }
          #endif
          new_server->serversocket = server_socket;
          /*bind server socket to adress*/
          serverAddr.sin_family = server_conf->type;
          serverAddr.sin_port = htons(server_conf->port);
          serverAddr.sin_addr.s_addr = htonl(inet_addr(server_conf->accepted_clients));
          memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
          if (bind(server_socket, (struct sockaddr *) &serverAddr,
          sizeof(serverAddr)) < 0 ){
            perror("error bind\n");
            exit(1);
          }
          if (listen(server_socket, 10) == -1){
            perror("Listen failed\n");
            exit(1);
          }
          else{
          }
          //fcntl(server_socket, F_SETFL, O_NONBLOCK);
          /*push new server to context server list*/
          printf("in new server\n");
          int new_server_id = push_with_lowest_id(context->servers, (void*)new_server, connection_ids);
          new_server->id = new_server_id;
          /*put server socket in pollset*/
          fds[nfds].fd = server_socket;
          fds[nfds].events = POLLIN;
          nfds++;
          pthread_mutex_lock (&bibmutex);
          queue_enqueue(context->to_context_queue, (void*)(long)new_server_id);
          pthread_mutex_unlock (&bibmutex);
          char ret = 1;
          piperesult = write(to_context_pipeline, &ret, 1);
          if (piperesult < 0){
            printf("NT: could not write in pipe to context(create server)\n");
          }
          free(server_conf);
        }
      } /*End case context pipeline wish from user*/
        //printf("NT: hinter Service\n");
      /*check connections*/
      //printf("NT: Anzahl Context Connections: %d\n", list_size(context->connections));
      //printf("NT: Anzahl der FDs: %d\n", nfds);
      int j;
      for (j=0; j<list_size(context->connections); j++){
        struct single_connection* act_con =
        (struct single_connection*)get_by_index(context->connections, j);
        //printf("Die aktuelle connection hat ID: %d\n", act_con->connection_id);
        /*get socket and from user pipeline from actuel connection*/
        int act_pipe = act_con->pipe_from_user;
        int act_sock = act_con->socket;
        /*check if socket or pipeline in pollset*/
        int q;
        for (q = 1; q < nfds; q++){
          if(((fds[q].fd == act_pipe) && fds[q].revents & POLLIN)){
            /*at least one message in queue to be send*/
            struct queue_q* from_client_queue = act_con->queue_to_network;
            /*iterate over every message in queue to be sended and add it to list*/
            while (queue_length(from_client_queue) != 0){
              /*get data*/
              struct size_number_data* to_send_data_optimiert;
              pthread_mutex_lock (&act_con->connection_mutex);
              struct message* to_send_message = queue_dequeue(from_client_queue);
              pthread_mutex_unlock (&act_con->connection_mutex);
              push_message_to_sending_list(act_con->messages_to_be_send, to_send_message, 8);
            }
          }
          else{
          }
          if (list_size(act_con->messages_to_be_send) != 0){
            /*iterate and send data over every single message*/
            send_message_list(act_con);
          }
          if ((fds[q].fd == act_sock && fds[q].revents & POLLIN)){
            //     /*receive message over connection*/
            if (act_con->status == 0 && act_sock == fds[q].fd
              && (fds[q].revents & POLLIN)){
                act_con->status = 1;
            }
            if (act_con->status == 1 && act_sock == fds[q].fd
              && (fds[q].revents & POLLIN)){
              /*receive data over actuel connection*/
              DATALENGTH recv_status;
              DATALENGTH recvd_bytes = 0;
              DATALENGTH recvd = 0;
              /*receive messages until everything is received or
              recvd bytes passes 1000 Bytes*/
              recvd = receive_connection(context, act_con, &recv_status);
              recvd_bytes += recvd;
              if(recv_status == 5){
                perror("Unexepted case\n");
                exit(1);
              }
              if(recv_status == 6){
                perror("TODO Unimplemented case, server wants to reconnect\n");
                exit(1);
              }
              if(recv_status == 7){
                act_con->last_interaction = time(NULL);
                /*Reconnection successfull, delete old open socket*/
                int stbd_size = list_size(sockets_to_be_deleted);
                struct connection_id_and_socket* stbd_element;
                int stbd_element_int;
                for (stbd_element_int = 0; stbd_element_int < stbd_size; stbd_element_int++){
                  stbd_element = get_by_index(sockets_to_be_deleted, stbd_element_int);
                  if ((stbd_element->connection_id) == recvd){
                    stbd_element = remove_by_index(sockets_to_be_deleted, stbd_element_int);
                    #ifdef _WIN32
                        closesocket(stbd_element->socket);
                        WSACleanup();
                    #else
                          close(stbd_element->socket);
                    #endif
                    //free(stbd_element);
                    break;
                  }
                }
              }
            }
          }
          else{
            //printf("NT: Socket war nicht am pollen\n");
          }
        }
      } /*End context connections*/
      //printf("NT: hinter context connections\n");
      int l;
      for (l=0; l <list_size(context->servers); l++){
        /*get server at index l*/
        struct single_server* act_server = get_by_index(context->servers, l);
        /*get serversocket*/
        int act_server_socket = act_server->serversocket;
        int r;
        for (r = 1; r < nfds; r++){
          if (fds[r].fd == act_server_socket && fds[r].revents & POLLIN){
              /*new connection wish via serversocket, accept and add to watched fds*/
              struct sockaddr_in client; /*information about client*/
              socklen_t addr_size = sizeof(client); /*size of sockaddr_in client*/
              /*Threadpool has space => accept connection_request*/
              int new_connection_socket = accept(act_server_socket,
                (struct sockaddr *) &client, &addr_size);
                printf("nach accept, neues Socket: %d\n", new_connection_socket);
              struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&client;
              struct in_addr ipAddr = pV4Addr->sin_addr;
              char str[20];
              char* client_ip;
              inet_ntop( AF_INET, &ipAddr, str, INET_ADDRSTRLEN);
              int client_port_tmp = ntohs (client.sin_port);
              if (strstr(str, "0.0.0.0") != NULL) {
                client_ip = "127.0.0.1";
              }
              else{
                client_ip = str;
              }

                /*set to Nonblocking*/
                //fcntl(new_connection_socket, F_SETFL, O_NONBLOCK);
              struct single_connection* scon = new_connection_struct();
              /*create connection Pipes*/
              int pipe_network[2]; /*Pipe*/
              int pipe_user[2]; /*Pipe*/
              piperesult = pipe(pipe_network);
              if (piperesult < 0){
                perror("Failed to create Network-Context-Pipeline\n");
                exit(1);
              }
              piperesult = pipe(pipe_user);
              if (piperesult < 0){
                perror("Failed to create Network-Context-Pipeline\n");
                exit(1);
              }
              struct target_address* target_address = malloc(sizeof(struct target_address));
                //target_address->client_port = client_port;
                target_address->client_ip = client_ip;
                target_address->server_ip = NULL;
                scon->target_address = target_address;
                scon->queue_to_network = queue_new();
                scon->queue_from_network = queue_new();
                scon->pipe_to_network = pipe_user[1];
                scon->pipe_from_network = pipe_network[0];
                scon->pipe_to_user = pipe_network[1];
                scon->pipe_from_user = pipe_user[0];
                fds[nfds].fd = scon->pipe_from_user;
                fds[nfds].events = POLLIN;
                nfds++;
                scon->socket = new_connection_socket;
                scon->connection_id = push_with_lowest_id(
                act_server->connections,(void*)scon, connection_ids);
                printf("Server connection id: %d\n", scon->connection_id);
                scon->type = 1; /*server connection*/
                //printf("ID, Socket und Pipe der neuen Verbindung: %d, %d und %d\n",
                //scon->connection_id, scon->socket, scon->pipe_from_user);
                scon->server_id = scon->connection_id;
                struct message* message = new_message();
                message->scon = scon;
                push_message_to_sending_list(scon->messages_to_be_send, message, 4);
                //send_data_optimiert(scon->socket, message,4);
                free(message);
                if (act_server->handle_message_func != NULL){
                  scon->handle_message_func = act_server->handle_message_func;
                }
                else{
                  scon->handle_message_func = NULL;
                }
                fds[nfds].fd = new_connection_socket;
                fds[nfds].events = POLLIN;
                nfds++;
                //printf("Ende accept\n");
                //printf("Anzahl der context cons: %d\n", list_size(context->connections));
                //printf("Anzahl der server cons: %d\n", list_size(act_server->connections));
              }
            }
            /*check if server connections in pollset*/
            int connection;
            for (connection = 0; connection < list_size(act_server->connections);
              connection++){
              struct single_connection* act_con =
                get_by_index(act_server->connections, connection);
              int s;
              for (s = 1; s < nfds; s++){
                if (act_con->pipe_from_user == fds[s].fd && (fds[s].revents & POLLIN)){
                  /*at least one message in queue to be send*/
                  struct queue_q* from_client_queue = act_con->queue_to_network;
                  /*iterate over every message in queue to be sended and add it to list*/
                  while (queue_length(from_client_queue) != 0){
                    /*get data*/
                    pthread_mutex_lock (&act_con->connection_mutex);
                    struct message* to_send_message = queue_dequeue(from_client_queue);
                    pthread_mutex_unlock (&act_con->connection_mutex);
                    push_message_to_sending_list(act_con->messages_to_be_send, to_send_message, 8);
                  }
                  if (list_size(act_con->messages_to_be_send) != 0){
                    /*iterate and send data over every single message*/
                    send_message_list(act_con);
                  }
                }
                if (act_con->status == 0 && act_con->socket == fds[s].fd
                  && (fds[s].revents & POLLIN)){
                  act_con->status = 1;
                }
                if(act_con->status != 0 && act_con->socket == fds[s].fd
                  && (fds[s].revents & POLLIN)){ /*receive data*/
                  /*receive data*/
                  DATALENGTH recv_status;
                  DATALENGTH recvd_bytes = 0;
                  /*one non blocking recv call*/
                  recvd_bytes += receive_connection(context, act_con, &recv_status);
                  if(recv_status == 5){
                    /*return value of recv_data = index of connection that should be reconnected*/
                    printf("NT: RECONNECTION\n");
                    printf("NT: suche nach einer Verbindung mit id: %d\n", recvd_bytes);
                    int found = 0;
                    struct single_connection* con;
                    if (list_contains_id(act_server->connections, recvd_bytes)){
                      con = get_by_id(act_server->connections, recvd_bytes);
                      printf("gefunden\n");
                      found = 1;
                    }
                    else if (list_contains_id(context->connections, recvd_bytes)){
                      con = get_by_id(context->connections, recvd_bytes);
                      printf("gefunden\n");
                      if (act_server->handle_message_func != NULL){
                        con->handle_message_func = NULL;
                      }
                      found = 1;
                    }
                    else{
                      /*not found*/
                      for (int cindex=0; cindex < list_size(act_server->connections); cindex ++){
                        struct single_connection* tmp_con = get_by_index(act_server->connections, cindex);
                      }
                    }
                    if (found == 1){
                      delete_fd_from_pollset(con->socket, &nfds, fds);
                      /*inform connection partner that he can close his socket*/
                      struct message* message = new_message();
                      message->scon = con;
                      send_data_optimiert(act_con->socket, message, 7);
                      free(message);
                      #ifdef _WIN32
                          closesocket(stbd_element->socket);
                          WSACleanup();
                      #else
                            close(con->socket);
                      #endif
                      con->socket = act_con->socket;
                      if (queue_length(act_con->queue_from_network)!= 0){
                        queue_merge(&(con->queue_from_network), act_con->queue_from_network);
                      }
                      act_con->socket = -1;
                      clear_messagelist_and_set_status(act_con, 2);
                      con->status = 0;
                    }
                  }
                  if(recv_status == 6){
                    /*pass*/
                  }
                  if(recv_status == 7){
                    /*Reconnection successfull, delete old open socket*/
                    int stbd_size = list_size(sockets_to_be_deleted);
                    struct connection_id_and_socket* stbd_element;
                    int stbd_element_int;
                    for (stbd_element_int = 0; stbd_element_int < stbd_size; stbd_element_int ++){
                      stbd_element = get_by_id(sockets_to_be_deleted, stbd_element_int);
                      if ((stbd_element->connection_id) == recvd_bytes){
                        printf("socket gefunden, lösche es\n");
                        #ifdef _WIN32
                            closesocket(stbd_element->socket);
                            WSACleanup();
                        #else
                              close(stbd_element->socket);
                        #endif
                        remove_by_id(sockets_to_be_deleted, stbd_element_int);
                        printf("NT: Socket von Verbindung : %d geschlossen\n", stbd_element->connection_id);
                      }
                    }
                  }
                }/*end recv message case*/
              }
            }
          }
          //printf("Hinter Server Connections\n");
        } /*End While True*/
          return NULL;
      } /*End Function*/
      /*creates Communication Object with empty connection list and NetworkThread*/
      struct context_object* init_context(int default_port,
      void (*handle_message_func)(struct message* message), time_t diff, time_t limit){
      //FILE *fptr;
      //fptr = fopen("/Users/peters/Desktop/grnb/gr/lib/gks/quartz/log_output.txt","w");
      //log_add_fp(fptr, 0);
      //log_trace("Anfang init context\n");
      pthread_t network_thread_number;
      int pipe_to_network[2]; /*pipeline to inform NetworkThread about new connection-wish*/
      int pipe_from_network[2]; /*pipeline to receive date from Networkthread*/
      int piperesult;
      /*init mutex for secure queue access*/
      pthread_mutex_init(&bibmutex, NULL);
      /*creates pipeline*/
      piperesult = pipe(pipe_to_network);
      if (piperesult < 0){
        perror("Failed to create Network-Context-Pipeline\n");
        exit(1);
      }
      piperesult = pipe(pipe_from_network);
      if (piperesult < 0){
        perror("Failed to create NetworkThread-Context-Pipeline\n");
        exit(1);
      }
      /*allocates memory for communication Object*/
      struct context_object* context = (struct context_object*)
        malloc(sizeof(struct context_object));
      context->servers = new_list();
      context->connections = new_list();
      context->callbacks = new_list();
      context->to_network_queue = queue_new();
      context->to_context_queue = queue_new();
      context->connection_request_pipe_read = pipe_to_network[0];
      context->connection_response_pipe_write = pipe_from_network[1];
      context->connection_request_pipe_write = pipe_to_network[1];
      context->connection_response_pipe_read = pipe_from_network[0];
      context->request_number = 0;
      context->diff = diff;
      context->limit = limit;
      pthread_create(&network_thread_number, NULL, create_network_thread, (void*)context);
      char* accepted_clients = "0.0.0.0";
      create_server(context, default_port, accepted_clients, handle_message_func);
      return context;
    }

    /*set heartbeat interval*/
    void context_set_heartbeat(struct context_object* context, time_t heartbeat, time_t limit){
      context->diff = heartbeat;
      context->limit = limit;
    }

  /*Method to create a new Connection to a Server, identified, by ip and port*/
  int new_connection(struct context_object* context, char* server_ip,
    int server_port, char* client_ip, int client_port){

    int fd1[2]; /*Filedescriptor for Threadcommunication*/
    int fd2[2]; /*Filedescriptor for Threadcommunication*/
    struct queue_q *request_queue; /*Client_to_Networkthread*/
    struct queue_q *response_queue;/*Networkthread_to_Client*/
    struct connection_wish* connection_wish; /*Arguments for Network Thread*/
    int piperesult; /*contains success of creating a Pipeline*/
    char ch = 1; /*1 -> new connection*/
    fd_set readfds;/*contains watched pipeline*/
    /*Using contexts client queue to transfer connection_wish*/
    //struct queue_q *client_queue = context->to_network_queue;
    request_queue = queue_new(); /*Request_to_Server*/
    response_queue = queue_new(); /*Response_from_Server*/
    /*Creating Pipelines*/
    piperesult = pipe(fd1);
    if (piperesult < 0){
      perror("Failed to create Pipeline 1\n");
      exit(1);
    }
    piperesult = pipe(fd2);
    if (piperesult < 0){
      perror("Failed to Create Pipeline 2\n");
      exit(1);
    }
    /*Preparing Arguments for Network Thread to create connection*/
    connection_wish = (struct connection_wish*)malloc(
    sizeof(struct connection_wish));
    connection_wish->request_queue = request_queue;
    connection_wish->response_queue = response_queue;
    connection_wish->pipe_from_user = fd1[0];
    connection_wish->pipe_to_user = fd2[1];
    connection_wish->pipe_to_network = fd1[1];
    connection_wish->pipe_from_network = fd2[0];
    connection_wish->server_ip = server_ip;
    connection_wish->server_port = server_port;
    connection_wish->client_ip = client_ip;
    connection_wish->client_port = client_port;
    /*connection wish to NetworkThread via Queue*/
    pthread_mutex_lock (&bibmutex);
    queue_enqueue(context->to_network_queue, (void*)connection_wish);
    if (write(context->connection_request_pipe_write, &ch, 1) < 0){
      printf("MT: Could not write data into Pipeline\n");
    }
    pthread_mutex_unlock (&bibmutex);
    /*wait for connection_id*/
    FD_ZERO(&readfds); /*clear the filedescriptor set*/
    FD_SET(context->connection_response_pipe_read, &readfds); /*insert pipeline*/
    int max_fd = (context->connection_response_pipe_read) +1;
    /*wait till connection is alive*/
    int activity = select(max_fd, &readfds, NULL, NULL, NULL);
    if (activity < 0 ){
      printf("MT: error select\n");
      exit(1);
    }
    /*clears pipeline*/
    if (read(context->connection_response_pipe_read, &ch, 1) < 0){
      printf("could not read from Netzwerkthread Pipeline\n");
      exit(1);
    }
    pthread_mutex_lock (&bibmutex);
    int connection_id = (int)queue_dequeue(context->to_context_queue);
    pthread_mutex_unlock (&bibmutex);
    free(connection_wish);
    return connection_id;
  }
  /*send data to specific connection, identified by connection_id*/
  int nb_send_message(struct context_object* context,void* data, DATALENGTH size,
    int connection_id, void (*handle_message_func)(struct message* response_message), int wt, int timeout){
      /*param kind specifies if send should not wait, wait till sent, or wait till
      an answer is there */
      int piperesult;
      char ch = 2;
      /*checks if connection is avaiable*/
      pthread_mutex_lock(&bibmutex);
      int connection_found = list_contains_id(context->connections, connection_id);
      struct single_connection* sin_con;
      if (connection_found == 1){
        //printf("Verbindung gefunden (recv, Clientverbindung)\n");
        sin_con = get_by_id(context->connections, connection_id);
        if (sin_con->status == 2){
          pthread_mutex_unlock(&bibmutex);
          return -1;
        }
        sin_con->can_be_deleted = 0;
        pthread_mutex_unlock(&bibmutex);
      }
      if (connection_found == 0){
        /*iterate over all servers and search for connnenction*/
        int server;
        for (server = 0; server < list_size(context->servers); server++){
          struct single_server* act_server = get_by_id(context->servers, server);
          connection_found = list_contains_id(act_server->connections, connection_id);
          if (connection_found == 1){
            //printf("Verbindung gefunden(recv, Serververbindung)\n");
            struct single_connection* sin_con
              = get_by_id(act_server->connections, connection_id);
              if (sin_con->status == 2){
                pthread_mutex_unlock(&bibmutex);
                return -1;
              }
              sin_con->can_be_deleted = 0;
              pthread_mutex_unlock(&bibmutex);
            }
          }
        }
        if (connection_found == 0){
          printf("Connection not found, could not send Data\n");
          pthread_mutex_unlock(&bibmutex);
          return -1;
        }
      struct queue_q* to_network_queue = sin_con->queue_to_network;
      int pipe_to_network = sin_con->pipe_to_network;
      /*create message struct */
      struct message* to_send_message = new_message();
      to_send_message->number = context->request_number;
      context->request_number+=1; /*TODO Request number generation*/
      to_send_message->data = data;
      to_send_message->datalength = size;
      if (handle_message_func != NULL){
        struct id_callback* id_callback = malloc(sizeof(struct id_callback));
        id_callback->id = to_send_message->number;
        id_callback->callback = handle_message_func;
        pthread_mutex_lock (&bibmutex);
        push(context->callbacks, id_callback);
        pthread_mutex_unlock (&bibmutex);
      }
      /*put to-send data in queue*/
      pthread_mutex_lock (&bibmutex);
      queue_enqueue(to_network_queue, (void*)to_send_message);
      pthread_mutex_unlock (&bibmutex);
      /*inform Networkthread that data should be send*/
      piperesult = write(pipe_to_network, &ch, 1);
      if (piperesult < 0){
        printf("could not write in pipe to network(send)\n");
        sin_con->can_be_deleted = 1;
        return -1;
      }
      if (wt == 0){ /*sending and dont wait for acknowledgement*/
        sin_con->can_be_deleted = 1;
        return 1;
      }
      if (wt == 2){
        struct timespec begin, end;
        clock_gettime(CLOCK_REALTIME, &begin);
        clock_gettime(CLOCK_REALTIME, &end);
        long seconds = end.tv_sec - begin.tv_sec;
        long nanoseconds = end.tv_nsec - begin.tv_nsec;
        double elapsed = seconds + nanoseconds*1e-9;
        int eins = 1;
        while (elapsed < timeout){
          pthread_mutex_lock(&bibmutex);
          if (to_send_message->fully_sent == 1){
            sin_con->can_be_deleted = 1;
            pthread_mutex_unlock(&bibmutex);
            return 1;
          }
          else if(to_send_message->fully_sent == -1){
            sin_con->can_be_deleted = 1;
            pthread_mutex_unlock(&bibmutex);
            printf("-1 !! nicht vollständig gesendet, gebe -1 zurueck\n");
            return -1;
          }
          else{
            pthread_mutex_unlock(&bibmutex);
            usleep(2000);
            clock_gettime(CLOCK_REALTIME, &end);
            seconds = end.tv_sec - begin.tv_sec;
            nanoseconds = end.tv_nsec - begin.tv_nsec;
            elapsed = seconds + nanoseconds*1e-9;
          }
        }
      }
      sin_con->can_be_deleted = 1;
      return -1;
    }
    /*returns 0 if data could not be readed, else data points to data*/
    /*wt speciefies if recv should be blocking till data can be recvd*/
    /*blocking if synchron message*/
    int nb_recv_message(struct context_object* context, int connection_id, struct message** message, int wt, int timeout){
      /*check if connection is avaiable in context connections*/
      //struct single_connection* sin_con;
      pthread_mutex_lock(&bibmutex);

      int connection_found = list_contains_id(context->connections, connection_id);
      struct single_connection* sin_con;
      if (connection_found == 1){
        //printf("Verbindung gefunden (recv, Clientverbindung)\n");
        sin_con = get_by_id(context->connections, connection_id);
        sin_con->can_be_deleted = 0;
        pthread_mutex_unlock(&bibmutex);
      }
      if (connection_found == 0){
      /*iterate over all servers and search for connnenction*/
      int server;
      for (server = 0; server < list_size(context->servers); server++){
        struct single_server* act_server = get_by_id(context->servers, server);
        connection_found = list_contains_id(act_server->connections, connection_id);
        if (connection_found == 1){
          //printf("Verbindung gefunden(recv, Serververbindung)\n");
          struct single_connection* sin_con
            = get_by_id(act_server->connections, connection_id);
            sin_con->can_be_deleted = 0;
            pthread_mutex_unlock(&bibmutex);
          }
        }
      }
  if (connection_found == 0){
    printf("Connection not found, could not receive Data\n");
    return -1;
  }

  if (wt == 0){
    if (queue_empty(sin_con->queue_from_network)){
      //printf("queue empty, no message found\n");
      sin_con->can_be_deleted = 1;
      return 0; /*no new answer receiveable*/
    }
    else { /*at least one entry from queue*/
      //printf("queue not empty, len: %d\n", queue_length(from_network_queue));
      struct message* return_message = (struct message*) queue_dequeue(sin_con->queue_from_network);
      *message = return_message;
      sin_con->can_be_deleted = 1;
      return return_message->datalength;
    }
  }
  if (wt == 1){ /*blocking case*/
    /*try for timmeout seconds*/
    struct timespec begin, end;
    clock_gettime(CLOCK_REALTIME, &begin);
    clock_gettime(CLOCK_REALTIME, &end);
    long seconds = end.tv_sec - begin.tv_sec;
    long nanoseconds = end.tv_nsec - begin.tv_nsec;
    double elapsed = seconds + nanoseconds*1e-9;
    while (elapsed < timeout){
      pthread_mutex_lock(&bibmutex);
      if (!queue_empty(sin_con->queue_from_network)){
        struct message* return_message = (struct message*) queue_dequeue(sin_con->queue_from_network);
        pthread_mutex_unlock(&bibmutex);
        *message = return_message;
        sin_con->can_be_deleted = 1;
        return return_message->datalength;
      }
      else{
        pthread_mutex_unlock(&bibmutex);
        usleep(1000);
        clock_gettime(CLOCK_REALTIME, &end);
        seconds = end.tv_sec - begin.tv_sec;
        nanoseconds = end.tv_nsec - begin.tv_nsec;
        elapsed = seconds + nanoseconds*1e-9;
      }
    }
    sin_con->can_be_deleted = 1;
    return -1;
  }
  sin_con->can_be_deleted = 1;
  return -1;
}

  /*closes one single connection by id*/
  int close_connection(struct context_object* context, int connection_id){
    char ch = 2; /*ch -> remove connection*/
    int piperesult;
    int to_network_pipe = context->connection_request_pipe_write;
    struct queue_q* to_network_queue = context->to_network_queue;
    pthread_mutex_lock (&bibmutex);
    piperesult = write(to_network_pipe, &ch, 1);
    if (piperesult < 0){
      printf("could not write in context pipeline\n");
      exit(1);
    }
    queue_enqueue(to_network_queue, (void*)(long)connection_id);
    pthread_mutex_unlock (&bibmutex);
    return 1;
  }
  /*closes every single connection from one context object*/
  void close_all_connections(struct context_object* context){
    char ch = 3; /*3->remove all connections*/
    int piperesult;
    int to_network_pipe = context->connection_request_pipe_write;
    //struct queue_q* to_network_queue = context->to_network_queue;
    /*informs NetworkThreat, that connections should be removed*/
    piperesult = write(to_network_pipe, &ch, 1);
    if (piperesult < 0){
      printf("could not write in context pipeline\n");
      exit(1);
    }
  }
  int create_server(struct context_object* context, int port, char* accepted_clients,
    void (*handle_message_func)(struct message* message)){
    fd_set readfds;
    char ch = 4;
    int piperesult;
    struct server_configuration* server_conf = (struct server_configuration*)
    malloc(sizeof(struct server_configuration));
    server_conf->type = AF_INET;
    server_conf-> protocol = SOCK_STREAM;
    server_conf->port = port;
    server_conf->accepted_clients = accepted_clients;
    server_conf->handle_message_func = handle_message_func;
    pthread_mutex_lock (&bibmutex);
    /*server configurations to Network Thread*/
    queue_enqueue(context->to_network_queue, (void*)server_conf);
    pthread_mutex_unlock (&bibmutex);
    piperesult = write(context->connection_request_pipe_write, &ch, 1);
    if (piperesult < 0){
      printf("could not write in pipe to network(create server)\n");
      return -1; /*sending data was */
    }
    /*wait for server id*/
    FD_ZERO(&readfds); /*clear the filedescriptor set*/
    FD_SET(context->connection_response_pipe_read, &readfds); /*insert pipe*/
    int max_fd = (context->connection_response_pipe_read) +1;
    /*wait till server is created*/
    int activity = select(max_fd, &readfds, NULL, NULL, NULL);
    if (activity < 0){
      perror(NULL);
      printf("errno %d\n", errno);
      printf("error select\n");
      exit(1);
    }
    piperesult = read(context->connection_response_pipe_read, &ch, 1);
    if (piperesult < 0){
      printf("could not read from Netzwerkthread Pipeline(create Server)\n");
      exit(1);
    }
    pthread_mutex_lock (&bibmutex);
    /*get server id from queue*/
    int server_id_ptr = (int)(long)queue_dequeue(context->to_context_queue);
    pthread_mutex_unlock (&bibmutex);
    if (server_id_ptr < 0){
      printf("couldnt create server\n");
      exit(1);
    }
    return server_id_ptr;
  }

  int connection_active(struct context_object* context, int connection_id){
    if (list_contains_id(context->connections, connection_id)){
        printf("Ende der neuen Funktion, Verbindung gefunden\n");
      return 1;
    }
    else{
      int server;
      for (server = 0; server < list_size(context->servers); server++){
        struct single_server* act_server = get_by_id(context->servers, server);
        if (list_contains_id(act_server->connections, connection_id) == 1){
          printf("Ende der neuen Funktion, Verbindung gefunden\n");
          return 1;
        }
      }
    }
          printf("Ende der neuen Funktion, Verbindung nicht gefunden\n");
    return 0;
  }
