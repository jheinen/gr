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
    /*int num = 42;
     //if (*(char *)&num == 42) {  if little-Endian System
     //uint32_t high_part = htonl((uint32_t)(value >> 32));
     //uint32_t low_part = htonl((uint32_t)(value & 0xFFFFFFFFLL));
     //return (((uint64_t)low_part) << 32) | high_part;
     //}
     //else{*/
    return value;
    //}
}

uint64_t fhtonll(uint64_t value) {
    /*int num = 42;
     if (*(char *)&num == 42) {
     uint32_t high_part = htonl((uint32_t)(value >> 32));
     uint32_t low_part = htonl((uint32_t)(value & 0xFFFFFFFFLL));
     return (((uint64_t)low_part) << 32) | high_part;
     } else {*/
    return value;
    //}
}


int send_data(int socket, struct message* message, int kind){

/*kind 8 -> normal message, kind 1 -> heartbeat, kind 2, send client port*/
/*kind 3 -> send client connnecetion id kind 4 -> send serverside connection id */

  //printf("\nin send: kind = %d ", kind);
  /*switch (kind){
    case 8: printf("send Info: Nachrichtenanteil wird verschickt\n");break;
    case 1: printf("send Info: Heartbeat wird verschickt\n");break;
    case 2: printf("send Info: Clientport wird verschickt\n");break;
    case 3: printf("send Info: Client connection id wird verschickt\n");break;
    case 4: printf("send Info: Server connection id wird verschickt\n");break;
    case 5: printf("send Info: Client schickt Server_id zum Wiederaufbau\n");break;
    case 6: printf("send Info: Server schickt Client_id zum Wiederaufbau\n");break;
    case 7: printf("send Info: Server sagt Client, dass altes Socket geloescht werden kann\n");break;
  }*/

  char oneByte;

  if(kind >= 0 && kind <= 8 ){
    oneByte = (char)kind;
    if (kind == 0){oneByte = 8;}
  }
  else{printf("send wrong kind\n");exit(1);}

    /*send oneByte to tell Receiver which kind of message*/
    size_t sent = 0;
    while (sent != 1){
        sent += send(socket, (const void*)&oneByte, 1, 0);
    }

    /*cases*/
    if (kind == 1){
      /*Heartbeat Signal sent, end of function*/
      return 0;
    }
    else if (kind == 2){
      /*send client port to reconnect in case of lost client*/
      DATALENGTH client_port = message->scon->target_address->client_port;
      client_port = fhtonll(client_port);
      sent = 0;
      while (sent != sizeof(client_port)){
          sent += send(socket, (&client_port+sent), sizeof(client_port)-sent, 0);
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
        printf("Kind = 4 (oder5), sende id: %d\n", message->scon->server_id);
        connection_id = message->scon->server_id;
      }
      sent = 0;
      while (sent != sizeof(connection_id)){
          sent += send(socket, &connection_id+sent, sizeof(connection_id)-sent, 0);
      }
      return 0;
    }
    else if (kind == 8){
      /*send piece of message*/

      //printf("message length: %d, how how_much_sent: %d, Nummer:%d \n", message->datalength, message->how_much_sent, message->number);
      char* test = (char*)(message->data);
      /*switch((*test)){
       case 5:
        printf("in send, sende GKSTERM_FUNTION_IS_RUNNING\n");
        break;
      }*/
      void* datapointer = message->data;
      int from_where = message->how_much_sent;
      DATALENGTH size = message->datalength;
      DATALENGTH request_number = message->number;
      DATALENGTH max_bytes = 900;
      DATALENGTH to_send;
      /*send request number*/
      sent = 0;
      while (sent != sizeof(request_number)){
          sent += send(socket, ((void*) &request_number)+sent, sizeof(request_number)-sent, 0);
      }
      if (from_where == 0){ /*First part of message*/

          sent = 0;
          char first_part = 1;
          //printf("Erster Teil der Nachricht\n");
          //printf("sende first part: %d\n", first_part);
          while(sent != 1){
              sent += send(socket, (const void*)&first_part, 1, 0);
          }

          /*send size for allocation*/
          DATALENGTH size_in_no = fhtonll(size);
          //printf("Laenge der Nachricht: %u\n", size);
          sent = 0;
          while(sent != sizeof(size_in_no)){ /* sends the size for allocating*/
            //printf("sende size: %d\n", size_in_no);
              sent += send(socket, ((void*)&size_in_no)+sent,sizeof(size)-sent, 0);
          }
          //printf("Size gesendet, size: %d\n", size_in_no);
          //printf("Bytes gesendet: %lu\n", sent);
      }

      else if (from_where != 0){ /*not the first part of the message*/
        //printf("Nicht der erste Part der Nachricht\n");
          sent = 0;
          char first_part = 0;
          //printf("sende first part: %d\n", first_part);
          while(sent != 1){
              sent += send(socket, (const void*)&first_part, 1, 0);
          }
      }
      /*now send part of message*/
      sent = 0;

      if (max_bytes > size-from_where){
          to_send = size-from_where;
      }
      else{
          to_send = max_bytes;
      }
      /*send data*/
      //printf("versende: %u Bytes\n", to_send);
      sent = send(socket, datapointer+from_where, to_send, 0);
      //printf("versendet\n");
      /*char* tmp = malloc(to_send+1);
       memcpy(tmp, datapointer+from_where, to_send);
       char* tmp2 = "";
       memcpy(tmp + to_send, tmp2, 1);*/

      message->how_much_sent += sent;
      if (message->how_much_sent == message->datalength){
        /*Message sent completly*/
        message->fully_sent = 1;
        return 3;
      }
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
    DATALENGTH max_bytes = 900;
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
            *recv_status = 0; /*connection closed*/
            return 0;
        }
      if (oneByte > 8){perror("wrong Message Status(oneByte)\n"); exit(1);}
    }
    *recv_status = oneByte;
    //printf("in recv, One Byte: %d\n", oneByte);
    if (oneByte == 0){
      /*close connection*/
      return 0;
      }
    else if(oneByte == 1){
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
      if (oneByte == 3){
        scon->client_id = connection_id;
        //printf("server hat id von client erhalten\n");
      }
      if (oneByte == 4){
        if (scon->server_id_set == 0){
          scon->server_id_set = 1;
          scon->server_id = connection_id;
          //printf("client hat id von server erhalten, id = %d\n", connection_id);
        }
      }
      if (oneByte == 5){
        /*Reconnection*/
        printf("5: empfangene connection id: %d\n", connection_id);
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
          //printf("vor message size, recvd: %d\n", recvd);
          size = fntohll(size);
          //printf("Message Size: %u\n", size);
          (*(rcv_message))->datalength = size;
          /*allocating memory*/
          buffer = (void*)malloc(size);
          (*(rcv_message))->data = buffer;
      }

      else if (first_part == 0){ /*not the first part of the message*/
        //printf("not first part\n");
        //printf("Laenge Messages beeing received: %d\n", list_size(scon->messages_beeing_received));
        node_t* tmp = scon->messages_beeing_received;
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
      if (max_bytes > datasize-from_where){
          to_receive = datasize-from_where;
      }
      else{
          to_receive = max_bytes;
      }

      recvd = 0;
      /*one recv call*/
      if (to_receive > max_bytes){
        printf("param grösser als max_bytes\n");
        exit(1);}
        //printf("Empfange: %d Bytes\n", to_receive);
        //printf("Message size: %d\n", (*(rcv_message))->datalength);
        int ret = 0;
        while (recvd != to_receive){ /*receive size of data*/
          ret = recv(socket, (*(rcv_message))->data + (*(rcv_message))->how_much_sent, to_receive, 0);
          if (ret > 0 && ret <= to_receive){
            recvd += ret;
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
              remove_by_index(&(scon->messages_beeing_received), index);
            }
          //printf("Nachricht vollstaendig empfangen, recv message data: %s\n",
           //(*(rcv_message))->data);
          //printf("Adresse der rcv_message: %p\n", *(rcv_message));
      }
      else{
        if (first_part == 1){/*message not sent completely*/
        /*push message to messages beeing received*/
          push(&(scon->messages_beeing_received), *(rcv_message));
        }

      }
      return recvd;
    }

    return 0;
}
