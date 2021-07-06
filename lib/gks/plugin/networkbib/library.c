#include "library.h"

void assert_unsigned_int_size(){
    switch(1) {
        case 0:
            break;
        case (sizeof(unsigned int) == 4):
            break;
    }
}

uint64_t fntohll(uint64_t value){
    int num = 42;
     if (*(char *)&num == 42) {  //if little-Endian System
     uint32_t high_part = htonl((uint32_t)(value >> 32));
     uint32_t low_part = htonl((uint32_t)(value & 0xFFFFFFFFLL));
     return (((uint64_t)low_part) << 32) | high_part;
     }
     else{
    return value;
    }
}

uint64_t fhtonll(uint64_t value) {
    int num = 42;
     if (*(char *)&num == 42) {
       uint32_t high_part = htonl((uint32_t)(value >> 32));
       uint32_t low_part = htonl((uint32_t)(value & 0xFFFFFFFFLL));
       return (((uint64_t)low_part) << 32) | high_part;
     } else {
       return value;
    }
}

int send_data_optimiert(int socket, struct message* message, int kind){
  /*protocol in one send call*/

  /*switch (kind){
    case 1: printf("1,send Info: Heartbeat wird verschickt\n");break;
    case 2: printf("2,send Info: Clientport wird verschickt\n");break;
    case 3: printf("3,send Info: Client connection id wird verschickt\n");break;
    case 4: printf("4,send Info: Server connection id wird verschickt\n");break;
    case 5: printf("5,send Info: Client schickt Server_id zum Wiederaufbau\n");break;
    case 6: printf("6,send Info: Server schickt Client_id zum Wiederaufbau\n");break;
    case 7: printf("7,send Info: Server sagt Client, dass altes Socket geloescht werden kann\n");break;
    case 8: printf("8, will daten verschicken\n");
  }*/
  int n = 1;
  struct timespec begin, end;
  char oneByte;
  DATALENGTH to_send;
  char protocol[(2+2*sizeof(DATALENGTH))];

  if(kind >= 0 && kind <= 8){
    oneByte = (char)kind;
    if (kind == 0){oneByte = 8;}
  }
  else{
    printf("send wrong kind\n");
    exit(1);
  }
  protocol[0] = oneByte;
  size_t sent = 0;
  int tmp = 0;
  if (kind == 2){
    /*send client port to reconnect in case of lost client*/
    DATALENGTH client_port = message->scon->target_address->client_port;
    client_port = ntohl(client_port);
    memcpy(&protocol[1], &client_port, sizeof(DATALENGTH));
  }
  else if (kind == 3 || kind == 4 || kind == 5 || kind == 6 || kind == 7){
    /*send clinetside connection_id*/

    DATALENGTH connection_id;
    if (kind == 3 || kind == 6 || kind == 7){
      connection_id = message->scon->client_id;
    }
    if (kind == 4 || kind == 5){
        connection_id = message->scon->server_id;
    }
    connection_id = ntohl(connection_id);
    memcpy(&protocol[1], &connection_id, sizeof(DATALENGTH));
  }
  else if (kind == 8){
    DATALENGTH size = message->datalength;
    DATALENGTH request_number = message->number;
    char first_part;
    request_number = ntohl(request_number);
    memcpy(&protocol[1], &request_number, sizeof(DATALENGTH));
    protocol[1] = request_number;
    if (message->how_much_sent == 0){
      first_part = 1;
    }
    else{
      first_part = 0;
    }
    protocol[(1+sizeof(DATALENGTH))] = first_part;
    if(first_part == 1){
      DATALENGTH size_in_no = ntohl(size);
      memcpy(&protocol[1+sizeof(DATALENGTH)+1], &size_in_no, sizeof(DATALENGTH));
    }
  }
  to_send = sizeof(protocol);
  clock_gettime(CLOCK_REALTIME, &begin);
  while(sent != 10){
    tmp = send(socket, &protocol+sent, to_send-sent, 0);
    if (tmp >= 0){
        sent += tmp;
    }
    else{
      clock_gettime(CLOCK_REALTIME, &end);
      long seconds = end.tv_sec - begin.tv_sec;
      long nanoseconds = end.tv_nsec - begin.tv_nsec;
      double elapsed = seconds + nanoseconds*1e-9;
      if (elapsed > 0.5){
        return -1;
      }
    }
  }
  if (kind ==8){
      void* datapointer = message->data;
      DATALENGTH max_bytes = 60000;
      int from_where = message->how_much_sent;
      DATALENGTH size = message->datalength;
      /*send data:*/
      if (max_bytes > size-from_where){
          to_send = size-from_where;
      }
      else{
          to_send = max_bytes;
      }
      /*send data*/
      sent = 0;
      tmp = 0;
      clock_gettime(CLOCK_REALTIME, &begin);
      while(sent < to_send){
        tmp = send(socket, datapointer+from_where+sent, to_send-sent, 0);
        if(tmp >= 0){
          sent += tmp;
        }
        else{
          clock_gettime(CLOCK_REALTIME, &end);
          long seconds = end.tv_sec - begin.tv_sec;
          long nanoseconds = end.tv_nsec - begin.tv_nsec;
          double elapsed = seconds + nanoseconds*1e-9;
          if (elapsed > 0.5){
            printf("Zeit abgelaufen, hat nicht geklappt\n");
            return -1;
          }
        }
      }
      message->how_much_sent += sent;
      if (message->how_much_sent == message->datalength){
        /*Message sent completly*/
        message->fully_sent = 1;
        return 3;
      }

  }
  return 0;
}

DATALENGTH receive_data_optimiert(struct single_connection* scon, DATALENGTH* recv_status,
   struct message** rcv_message){

    char oneByte;
    DATALENGTH recvd;
    DATALENGTH size = 0;
    DATALENGTH number = 0;
    //DATALENGTH result = 0;
    char first_part;
    void* buffer;
    DATALENGTH datasize;
    DATALENGTH max_bytes = 60000;
    DATALENGTH to_receive;
    DATALENGTH recv_ret = 0;
    DATALENGTH connection_id;
    DATALENGTH request_number;
    int socket;
    socket = scon->socket;
    recvd = 0;
    char protocol[(2+2*sizeof(DATALENGTH))];

    while (recvd < sizeof(protocol)){
        recv_ret = recv(socket, (&protocol + recvd), sizeof(protocol)-recvd, 0);
        if (recv_ret == 0){
          printf("Null Byte empfangen => schliesse Verbindung\n");
          printf("Null Byte Socket: %d\n", scon->socket);
            *recv_status = 0; /*connection closed*/
            return 0;
        }
        if (!(recv_ret < 0 || recv_ret > max_bytes)){
          recvd += recv_ret;
        }
        recv_ret = 0;
    }
    oneByte = protocol[0];
    if (oneByte > 8){perror("wrong Message Status(oneByte)\n"); exit(1);}
    *recv_status = oneByte;
    /*switch (oneByte){
    //case 8: printf("send Info: Nachrichtenanteil wird verschickt\n");break;
      case 1: printf("oneByte: 1\n");break;
      case 2: printf("oneByte: 2\n");break;
      case 3: printf("oneByte: 3\n");break;
      case 4: printf("oneByte: 4\n");break;
      case 5: printf("oneByte: 5\n");break;
      case 6: printf("oneByte: 6\n");break;
      case 7: printf("oneByte: 7\n");break;
      case 8: printf("oneByte: 8\n");break;
    }*/
    if (oneByte == 0){
      /*close connection*/
      return 0;
    }
    else if(oneByte == 1){
      printf("Heartbeat empfangen\n");
      /*Heartbeat "Message*/
      return 0;
    }
    else if(oneByte == 2){
      /*client default port received*/
      DATALENGTH client_port;
      memcpy(&client_port, &protocol[1], sizeof(DATALENGTH));
      //DATALENGTH client_port = *client_port_pointer;
      client_port = htonl(client_port);
      scon->target_address->client_port = client_port;
      printf("Empfangener Port: %d\n", client_port);
      return 0;
    }
    else if (oneByte == 3  || oneByte == 4 || oneByte == 5 || oneByte == 6 || oneByte == 7){
      /*receive id the connection partner uses*/
      memcpy(&connection_id, &protocol[1], sizeof(DATALENGTH));
      connection_id = htonl(connection_id);
      printf("Empfangene connection_id: %d\n", connection_id);
      if (oneByte == 3){
        if (scon->client_id_set == 0){
          scon->client_id_set = 1;
          scon->client_id = connection_id;
        }
        /*printf("server hat id von client erhalten\n");*/
        return connection_id;
      }
      if (oneByte == 4){
        if (scon->server_id_set == 0){
          scon->server_id_set = 1;
          scon->server_id = connection_id;
        }
        /*printf("client hat id von server erhalten, id = %d\n", connection_id);*/
        return connection_id;
      }
      if (oneByte == 5){
        /*Reconnection*/
        /*printf("5: empfangene connection id: %d\n", connection_id);*/
        return connection_id;
      }
      if (oneByte == 6){
        /*Reconnection*/
        return connection_id;
      }
      if(oneByte == 7){
        /*Socket can be closed*/
        return connection_id;
      }
    }
    if (oneByte == 8){
      memcpy(&request_number, &protocol[1], sizeof(DATALENGTH));
      request_number = htonl(request_number);
      first_part = protocol[1+sizeof(DATALENGTH)];
      memcpy(&size, &protocol[1 + sizeof(DATALENGTH) +1], sizeof(DATALENGTH));
      size = htonl(size);
      int index = 0;
      if (first_part == 1){
          (*(rcv_message))->datalength = size;
          buffer = (void*)malloc(size);
          (*(rcv_message))->data = buffer;
      }

      else if (first_part == 0){ /*not the first part of the message*/
        node_t* tmp = scon->messages_beeing_received->list;
        struct message* tmp_message;
        while(tmp->val != NULL){
          tmp_message = get_by_index(scon->messages_beeing_received, index);
          if (tmp_message->number == (*(rcv_message))->number){
              /*message found, use it instead of given rcv_message*/
            free(*(rcv_message));
            *(rcv_message) = tmp_message;
            break;
          }
          if (tmp->next == NULL){
            perror("no message found\n");
            exit(1);
          }
          else{
            tmp = tmp->next;
            index++;
          }
        }
      }
      /*receive data*/
      datasize = (*(rcv_message))->datalength;
      int from_where = (*(rcv_message))->how_much_sent;
      //printf("maxbytes: %d, datasize-from_where: %d\n", max_bytes, datasize-from_where);
      if (max_bytes > datasize-from_where){
          to_receive = datasize-from_where;
      }
      else{
          to_receive = max_bytes;
      }
      //printf("to_receive: %d\n", to_receive);
      recvd = 0;
      /*one recv call*/
      if (to_receive > max_bytes){
        printf("param grösser als max_bytes\n");
        exit(1);}
        //printf("Empfange: %d Bytes\n", to_receive);
        //printf("Message size: %d\n", (*(rcv_message))->datalength);
        int ret = 0;
        while (recvd != to_receive){ /*receive size of data*/
          ret = recv(socket, (*(rcv_message))->data + (*(rcv_message))->how_much_sent + recvd, to_receive-recvd, 0);
          if (ret > 0 && ret <= to_receive){
            recvd += ret;
          }
          if(ret == to_receive){
            break;
          }
          ret = 0;
        }
        //printf("Empfangene Byteanzahl:%d\n", recvd);
        if(recvd == -1){
            *recv_status = 1;
            return 0;
          }
          (*(rcv_message))->how_much_sent += recvd;

          if((*(rcv_message))->datalength == (*(rcv_message))->how_much_sent){
          *recv_status = 8;
              //printf("Nachricht vollständig emfangen\n");
              if (first_part == 0){
              //printf("Entferne aus liste\n");
              remove_by_index((scon->messages_beeing_received), index);
            }
          //printf("Nachricht vollstaendig empfangen, recv message data: %s\n",
           //(*(rcv_message))->data);
          //printf("Adresse der rcv_message: %p\n", *(rcv_message));
      }
      else{
        if (first_part == 1){/*message not sent completely*/
        /*push message to messages beeing received*/
          push((scon->messages_beeing_received), *(rcv_message));
        }

      }
      return recvd;
    }
  return -1;
}

int send_data(int socket, struct message* message, int kind){
  int n = 1;
  struct timespec begin, end;
 //little endian if true
//if(*(char *)&n == 1) {printf("little Endian\n");}
//else{printf("Big Endian\n");}

/*kind 8 -> normal message, kind 1 -> heartbeat, kind 2, send client port*/
/*kind 3 -> send client connecetion id kind 4 -> send serverside connection id */

  //printf("\nin send: kind = %d ", kind);
  /*switch (kind){
    case 1: printf("send Info: Heartbeat wird verschickt\n");break;
    case 2: printf("send Info: Clientport wird verschickt\n");break;
    case 3: printf("send Info: Client connection id wird verschickt\n");break;
    case 4: printf("send Info: Server connection id wird verschickt\n");break;
    case 5: printf("send Info: Client schickt Server_id zum Wiederaufbau\n");break;
    case 6: printf("send Info: Server schickt Client_id zum Wiederaufbau\n");break;
    case 7: printf("send Info: Server sagt Client, dass altes Socket geloescht werden kann\n");break;
    //case 8: printf("8, will daten verschicken\n");
  }*/

  char oneByte;

  if(kind >= 0 && kind <= 8 ){
    oneByte = (char)kind;
    if (kind == 0){oneByte = 8;}
  }
  else{
    printf("send wrong kind\n");
    exit(1);
  }
//printf("Vor One Byte versenden\n");
    /*send oneByte to tell Receiver which kind of message*/
    size_t sent = 0;
    int tmp = 0;
    clock_gettime(CLOCK_REALTIME, &begin);
    /*FR: absolute time get current*/
    while (sent != 1){
      tmp = send(socket, (const void*)&oneByte, 1, 0);
        if (tmp >= 0){
          sent += tmp;
        }
        else{
          clock_gettime(CLOCK_REALTIME, &end);
          long seconds = end.tv_sec - begin.tv_sec;
          long nanoseconds = end.tv_nsec - begin.tv_nsec;
          double elapsed = seconds + nanoseconds*1e-9;
          if (elapsed > 0.5){
            //printf("Sende Aufruf war nicht erfoglreich\n");
            if(kind == 1){
              printf("Heartbeat hat nicht geklappt\n");
            }
            return -1;
          }
        }
    }
    //printf("One Byte versendet\n");

    /*cases*/
    if (kind == 1){
      printf("Heartbeat case vor return\n");
      /*Heartbeat Signal sent, end of function*/
      return 0;
    }
    else if (kind == 2){
      /*send client port to reconnect in case of lost client*/
      DATALENGTH client_port = message->scon->target_address->client_port;
      client_port = ntohl(client_port);
      sent = 0;
      tmp = 0;
      //clock_gettime(CLOCK_REALTIME, &begin);
      while (sent != sizeof(client_port)){
        tmp = send(socket, (&client_port+sent), sizeof(client_port)-sent, 0);
        if (tmp >= 0){
            sent += tmp;
        }
        else{
          clock_gettime(CLOCK_REALTIME, &end);
          long seconds = end.tv_sec - begin.tv_sec;
          long nanoseconds = end.tv_nsec - begin.tv_nsec;
          double elapsed = seconds + nanoseconds*1e-9;
          if (elapsed > 0.5){
            //printf("Sende Aufruf war nicht erfoglreich\n");
            return -1;
          }
        }
      }
      return 0;
    }
    else if (kind == 3 || kind == 4 || kind == 5 || kind == 6 || kind == 7){
      /*send clinetside connection_id*/

      DATALENGTH connection_id;
      if (kind == 3 || kind == 6 || kind == 7){
        connection_id = message->scon->client_id;
      }
      if (kind == 4 || kind == 5){
          connection_id = message->scon->server_id;
      }
      connection_id = ntohl(connection_id);
      sent = 0;
      tmp = 0;
      //clock_gettime(CLOCK_REALTIME, &begin);
      while (sent != sizeof(connection_id)){
        tmp = send(socket, &connection_id+sent, sizeof(connection_id)-sent, 0);
        if (tmp >= 0){
          sent += tmp;
        }
        else{
          clock_gettime(CLOCK_REALTIME, &end);
          long seconds = end.tv_sec - begin.tv_sec;
          long nanoseconds = end.tv_nsec - begin.tv_nsec;
          double elapsed = seconds + nanoseconds*1e-9;
          if (elapsed > 0.5){
            //printf("Sende Aufruf war nicht erfoglreich\n");
            return -1;
          }
        }
      }
      return 0;
    }
    else if (kind == 8){
      //printf("Send data im 8ter Fall\n");
      //printf("send kind = 8, socket: %d, Nummmer: %d \n", socket, message->number);
      /*send piece of message*/

      //printf("message length: %d, how how_much_sent: %d, Nummer:%d \n", message->datalength, message->how_much_sent, message->number);
      //char* test = (char*)(message->data);
      /*switch((*test)){
       case 5:
        printf("in send, sende GKSTERM_FUNTION_IS_RUNNING\n");
        break;
      }*/
      void* datapointer = message->data;
      int from_where = message->how_much_sent;
      DATALENGTH size = message->datalength;
      DATALENGTH request_number = message->number;
      DATALENGTH max_bytes = 60000;
      DATALENGTH to_send;
      /*send request number*/
      request_number = ntohl(request_number);
      sent = 0;
      tmp = 0;
      //clock_gettime(CLOCK_REALTIME, &begin);
      while (sent != sizeof(request_number)){
        tmp = send(socket, ((void*) &request_number)+sent, sizeof(request_number)-sent, 0);
        if (tmp >= 0){
            sent += tmp;
        }
        else{
          clock_gettime(CLOCK_REALTIME, &end);
          long seconds = end.tv_sec - begin.tv_sec;
          long nanoseconds = end.tv_nsec - begin.tv_nsec;
          double elapsed = seconds + nanoseconds*1e-9;
          if (elapsed > 0.5){
            return -1;
          }
        }
      }
      if (from_where == 0){ /*First part of message*/
          sent = 0;
          char first_part = 1;
          while(sent != 1){
              sent += send(socket, (const void*)&first_part, 1, 0);
          }
          /*send size for allocation*/
          DATALENGTH size_in_no = ntohl(size);
          //printf("Laenge der Nachricht: %u\n", size);
          sent = 0;
          tmp = 0;
          //clock_gettime(CLOCK_REALTIME, &begin);
          while(sent != sizeof(size_in_no)){ /* sends the size for allocating*/
            tmp = send(socket, ((void*)&size_in_no)+sent,sizeof(size)-sent, 0);
            if (tmp >= 0){
              sent += tmp;
            }
            else{
              clock_gettime(CLOCK_REALTIME, &end);
              long seconds = end.tv_sec - begin.tv_sec;
              long nanoseconds = end.tv_nsec - begin.tv_nsec;
              double elapsed = seconds + nanoseconds*1e-9;
              if (elapsed > 0.5){
                //printf("8ter Fall, Sende Aufruf war nicht erfoglreich\n");
                return -1;
              }
            }
          }
              //printf("8ter Fall Size versendet\n");
          //printf("Size gesendet, size: %d\n", size_in_no);
          //printf("Bytes gesendet: %lu\n", sent);
      }

      else if (from_where != 0){
       /*not the first part of the message*/
          sent = 0;
          tmp = 0;
          //clock_gettime(CLOCK_REALTIME, &begin);
          char first_part = 0;
          //printf("sende first part: %d\n", first_part);
          while(sent != 1){
            tmp = send(socket, (const void*)&first_part, 1, 0);
            if (tmp >= 0){
                sent += tmp;
            }
            else{
              clock_gettime(CLOCK_REALTIME, &end);
              long seconds = end.tv_sec - begin.tv_sec;
              long nanoseconds = end.tv_nsec - begin.tv_nsec;
              double elapsed = seconds + nanoseconds*1e-9;
              if (elapsed > 0.5){
                //printf("8ter Fall, Sende Aufruf war nicht erfoglreich\n");
                return -1;
              }
            }
          }
          //printf("8ter Fall not first part versendet\n");
      }
      /*now send part of message*/
      //printf("maxbytes: %d, size-from_where: %d\n", max_bytes, size-from_where);
      if (max_bytes > size-from_where){
          to_send = size-from_where;
      }
      else{
          to_send = max_bytes;
      }
      /*send data*/
      //printf("versende: %u Bytes\n", to_send);
      sent = 0;
      tmp = 0;
      //clock_gettime(CLOCK_REALTIME, &begin);
      while(sent < to_send){
        tmp = send(socket, datapointer+from_where+sent, to_send-sent, 0);
        if(tmp >= 0){
          sent += tmp;
        }
        else{
          clock_gettime(CLOCK_REALTIME, &end);
          long seconds = end.tv_sec - begin.tv_sec;
          long nanoseconds = end.tv_nsec - begin.tv_nsec;
          double elapsed = seconds + nanoseconds*1e-9;
          if (elapsed > 0.5){
            printf("Zeit abgelaufen, hat nicht geklappt\n");
            return -1;
          }
        }
      }
      //printf("gesendete Bytes: %lu\n", sent);
      //printf("message how much sent alt: %d\n", message->how_much_sent);
      //printf("message how much sent neu: %lu\n", message->how_much_sent+sent);
      //printf("8ter Fall, daten versendet\n");
      //printf("versendet\n");
      /*char* tmp = malloc(to_send+1);
       memcpy(tmp, datapointer+from_where, to_send);
       char* tmp2 = "";
       memcpy(tmp + to_send, tmp2, 1);*/
      message->how_much_sent += sent;
      if (message->how_much_sent == message->datalength){
        /*Message sent completly*/
        //printf("Nachricht vollstaendig versendet\n");
        message->fully_sent = 1;
        return 3;
      }
      //printf("Teil der Nachricht versendet\n");
      return sent;
  }
  return 0;
}

DATALENGTH receive_data(struct single_connection* scon, DATALENGTH* recv_status,
   struct message** rcv_message){

    char oneByte;
    DATALENGTH recvd;
    DATALENGTH size = 0;
    DATALENGTH number = 0;
    //DATALENGTH result = 0;
    char first_part;
    void* buffer;
    DATALENGTH datasize;
    DATALENGTH max_bytes = 60000;
    DATALENGTH to_receive;
    DATALENGTH recv_ret = 0;
    int socket;
    socket = scon->socket;
    recvd = 0;

    while (recvd != 1){ /*receive Heartbeat status*/
        recv_ret = recv(socket, (void*)&oneByte, 1, 0);
        if (!(recv_ret < 0 || recv_ret > max_bytes)){
          recvd += recv_ret;
        }
        recv_ret = 0;
        if (recvd == 0){
          printf("Null Byte empfangen => schliesse Verbindung\n");
          printf("Null Byte Socket: %d\n", scon->socket);
            *recv_status = 0; /*connection closed*/
            return 0;
        }
      if (oneByte > 8){perror("wrong Message Status(oneByte)\n"); exit(1);}
    }
    *recv_status = oneByte;
    //printf("in recv, One Byte: %d\n", oneByte);
    /*switch (oneByte){
      //case 8: printf("send Info: Nachrichtenanteil wird verschickt\n");break;
      case 1: printf("recvd: 1\n");break;
      case 2: printf("recvd: 2\n");break;
      case 3: printf("recvd: 3\n");break;
      case 4: printf("recvd: 4\n");break;
      case 5: printf("recvd: 5\n");break;
      case 6: printf("recvd: 6\n");break;
      case 7: printf("recvd: 7\n");break;
      case 8: printf("recvd: 8\n");break;
    }*/
    if (oneByte == 0){
      /*close connection*/
      return 0;
    }
    else if(oneByte == 1){
      printf("Heartbeat empfangen\n");
      /*Heartbeat "Message*/
      return 0;
    }
    else if(oneByte == 2){
      /*client default port received*/
      DATALENGTH client_port;
      recvd = 0;
      while (recvd != sizeof(DATALENGTH)){
        recv_ret += recv(socket, &client_port+recvd, sizeof(DATALENGTH)-recvd, 0);
        if (!(recv_ret < 0 || recv_ret > max_bytes)){
          recvd += recv_ret;
        }
        recv_ret = 0;
      }
      client_port = htonl(client_port);
      scon->target_address->client_port = client_port;
      return 0;
    }
    else if (oneByte == 3  || oneByte == 4 || oneByte == 5 || oneByte == 6 || oneByte == 7){
      /*receive id the connection partner uses*/
      DATALENGTH connection_id;
      recvd = 0;
      while(recvd != sizeof(DATALENGTH)){
          recv_ret += recv(socket, &connection_id+recvd, sizeof(DATALENGTH)-recvd, 0);
          if (!(recv_ret < 0 || recv_ret > max_bytes)){
            recvd += recv_ret;
          }
          recv_ret = 0;
      }
      connection_id = htonl(connection_id);
      if (oneByte == 3){
        if (scon->client_id_set == 0){
          scon->client_id_set = 1;
          scon->client_id = connection_id;
        }
        /*printf("server hat id von client erhalten\n");*/
      }
      if (oneByte == 4){
        if (scon->server_id_set == 0){
          scon->server_id_set = 1;
          scon->server_id = connection_id;
          /*printf("client hat id von server erhalten, id = %d\n", connection_id);*/
        }
      }
      if (oneByte == 5){
        /*Reconnection*/
        /*printf("5: empfangene connection id: %d\n", connection_id);*/
        return connection_id;
      }
      if (oneByte == 6){
        /*Reconnection*/
        return connection_id;
      }
      if(oneByte == 7){
        /*Socket can be closed*/
        return connection_id;
      }
      return 0;
    }
    if(oneByte == 8){

      /*receive request number*/
      recvd = 0;
      while (recvd != sizeof(DATALENGTH)){ /*receive request number*/
          recv_ret = recv(socket, ((void*)(&number)+recvd), sizeof(DATALENGTH)-recvd, 0);
          if (!(recv_ret < 0 || recv_ret > max_bytes)){
            recvd += recv_ret;
          }
          recv_ret = 0;
      }
      number = htonl(number);
      (*(rcv_message))->number = number;
      //printf("Empfangene Nummer: %d\n", number);
      recvd = 0;
      while(recvd != 1){ /*check if first part of message*/
          recv_ret = recv(socket, (void*)&first_part, 1, 0);
          if (!(recv_ret < 0 || recv_ret > max_bytes)){
            recvd += recv_ret;
          }
          recv_ret = 0;
      }
      int index = 0;
      if (first_part == 1){
        //printf("first part of message, request nummer: %d\n", number);
          /*recv datalength to allocate memory*/
          recvd = 0;
          while (recvd != sizeof(DATALENGTH)){ /*receive size of data*/
              recv_ret = recv(socket, ((void*)&size + recvd), sizeof(DATALENGTH)-recvd, 0);
              if (!(recv_ret < 0 || recv_ret > max_bytes)){
                recvd += recv_ret;
              }
              recv_ret = 0;
          }
          //printf("Message size, recvd: %d\n", recvd);
          size = htonl(size);
          //printf("Message Size: %u\n", size);
          (*(rcv_message))->datalength = size;
          /*allocating memory*/
          buffer = (void*)malloc(size);
          (*(rcv_message))->data = buffer;
      }

      else if (first_part == 0){ /*not the first part of the message*/
        //printf("not first part\n");
        //printf("Laenge Messages beeing received: %d\n", list_size(scon->messages_beeing_received));
        node_t* tmp = scon->messages_beeing_received->list;
        struct message* tmp_message;
        while(tmp->val != NULL){
          tmp_message = get_by_index(scon->messages_beeing_received, index);
          if (tmp_message->number == (*(rcv_message))->number){
              /*message found, use it instead of given rcv_message*/
            free(*(rcv_message));
            *(rcv_message) = tmp_message;
            break;
          }
          if (tmp->next == NULL){
            perror("no message found\n");
            exit(1);
          }
          else{
            tmp = tmp->next;
            index++;
          }
        }
      }

      datasize = (*(rcv_message))->datalength;
      int from_where = (*(rcv_message))->how_much_sent;
      //printf("maxbytes: %d, datasize-from_where: %d\n", max_bytes, datasize-from_where);
      if (max_bytes > datasize-from_where){
          to_receive = datasize-from_where;
      }
      else{
          to_receive = max_bytes;
      }
      //printf("to_receive: %d\n", to_receive);
      recvd = 0;
      /*one recv call*/
      if (to_receive > max_bytes){
        printf("param grösser als max_bytes\n");
        exit(1);}
        //printf("Empfange: %d Bytes\n", to_receive);
        //printf("Message size: %d\n", (*(rcv_message))->datalength);
        int ret = 0;
        while (recvd != to_receive){ /*receive size of data*/
          ret = recv(socket, (*(rcv_message))->data + (*(rcv_message))->how_much_sent + recvd, to_receive-recvd, 0);
          if (ret > 0 && ret <= to_receive){
            recvd += ret;
          }
          if(ret == to_receive){
            break;
          }
          ret = 0;
        }
        //printf("Empfangene Byteanzahl:%d\n", recvd);
        if(recvd == -1){
            *recv_status = 1;
            return 0;
          }
          (*(rcv_message))->how_much_sent += recvd;

          if((*(rcv_message))->datalength == (*(rcv_message))->how_much_sent){
          *recv_status = 8;
              //printf("Nachricht vollständig emfangen\n");
              if (first_part == 0){
              //printf("Entferne aus liste\n");
              remove_by_index((scon->messages_beeing_received), index);
            }
          //printf("Nachricht vollstaendig empfangen, recv message data: %s\n",
           //(*(rcv_message))->data);
          //printf("Adresse der rcv_message: %p\n", *(rcv_message));
      }
      else{
        if (first_part == 1){/*message not sent completely*/
        /*push message to messages beeing received*/
          push((scon->messages_beeing_received), *(rcv_message));
        }

      }
      return recvd;
    }

    return 0;
}
