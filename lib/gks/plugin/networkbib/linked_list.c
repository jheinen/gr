#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "linked_list.h"


node_t* new_list(void) {
    node_t* new_node = malloc(sizeof(node_t));
    new_node->next = NULL;
    new_node->val = NULL;
    return new_node;
}

int generate_number(node_t * head){
  printf("In generate_number\n");
    /*node_t* current = head;
    int size = list_size(head);
    for (int i = 0; i< size; i++){
        if (((struct val_data*)current->val)->id > i){
            return i;
        }
        else{
            current = current->next;
        }
    }
    return size;*/
     static DATALENGTH number = 0;
     number++;
     return number-1;
}

int push(node_t ** head, void* val) {

    struct val_data* push_val;
    int size = list_size(*head);

    push_val = (struct val_data*)malloc(sizeof(struct val_data));
    push_val->id = size;
    push_val->data = val;
    node_t * current = *head;
    int size_of_list = size;

    if(size_of_list == 0){
        current->val = (void*)push_val;
        current->next = NULL;
        return size;
    }
     /*Hinten einfuegen*/

        while(current->next!= NULL){
            current = current->next;
        }
        /*  add a new value */
        node_t* next = (node_t*)malloc(sizeof(node_t));
        next->val = (void*)push_val;
        next->next= NULL;
        current->next = next;
        return size;

}
void* pop(node_t ** head) { /*removes the head*/
    void* retval;
    //node_t * next_node;
    if (*head == NULL) {
        return NULL;
    }
    retval = ((struct val_data*)(*head)->val)->data;
    if((*head)->next == NULL){
        (*head)->val = NULL;
        return retval;
    }
    *head = (*head)->next;
    return retval;
}
void* remove_by_index(node_t ** head, int n) { /*removes by list_index*/
    int i = 0;
    void* retval = NULL;
    node_t * current = *head;
    node_t * temp_node = NULL;

    if (n == 0) {
        return pop(head);
    }

    for (i = 0; i < n-1; i++) {
        if (current->val== NULL) {
            return NULL;
        }
        current = current->next;
    }

    temp_node = current->next;
    retval = ((struct val_data*)(temp_node->val))->data;
    current->next = temp_node->next;
    free(temp_node);

    return retval;
}

void* remove_by_id(node_t ** head, int id){

    node_t * current = *head;
    if (((struct val_data*)current->val)->id == id) {
      struct val_data* tmp = (struct val_data*)(*head)->val;
      if ((*head)->next != NULL){
        struct val_data* tmp = (struct val_data*)((*head)->next)->val;
      }
      void* ret = pop(head);
      //tmp = (struct val_data*)(*head)->val;
      //printf("Head ID after pop: %d\n", tmp->id);
      //printf("%d\n", tmp== NULL);
        return ret;
    }
    void* retval = NULL;
    node_t * temp_node = NULL;
    int current_id = 0;
    int i;
    for (i = 0; i < list_size(*head); i++) {/*iterate over list and search for id*/

        temp_node = current->next;
        current_id = ((struct val_data*)(temp_node->val))->id;
        if (current_id == id){ /*if current element should be removed*/
            //printf("%d Verbindung soll geloescht werden\n", i);
            retval = ((struct val_data*)(temp_node->val))->data;
            current->next = temp_node->next;
            temp_node->val = NULL;
            free(temp_node);
            break;
        }
        else{
            current = current->next;
        }
    }
    return retval;
}

void* get_by_index(node_t * head, int index){ /*0 indexed*/

    node_t * current = head;
    int i = 0;
    void* retval = NULL;

    if (index == 0){
        return ((struct val_data*)(current->val))->data;
    }

    for (i = 0; i < index; i++) {
        if (current->val == NULL) {
            return NULL;
        }
        current = current->next;
    }
    retval = ((struct val_data*)(current->val))->data;
    return retval;
}

void* get_by_id(node_t * head, int id){
    int i=0; /*list size index*/
    void* retval = NULL;
    node_t * temp_node = head;
    int current_id = 0;
    int size_of_list = list_size(head);
    for (i = 0; i < size_of_list; i++) { /*iterate over list and search for id*/
        current_id = ((struct val_data*)(temp_node->val))->id;
        if (current_id == id){
            retval = ((struct val_data*)(temp_node->val))->data;
        }
        else{
            temp_node = temp_node->next;
        }
    }
    return retval;
}

int list_size(node_t * head){
    node_t * current = head;
    int size = 0;
    /*if (head == NULL){
      printf("head == NULL\n");
    }*/
    if (head->val == NULL){
        return size;
    }
    size++;
    while (current->next != NULL){
        //printf("%d: In list size, Adresse: %p\n", size, ((struct val_data*)(current->val))->data);
        size ++;
        current = current->next;
    }
    //printf("%d: In list size, Adresse: %p\n", size, ((struct val_data*)(current->val))->data);
    return size;
}

/*checks if element with id is in list*/
int list_contains_id(node_t* head, int id){

    //printf("aufgerufen mit: %d\n", id);
    //printf("ID des ersten Eintrags: %d\n", ((struct val_data*)(head->val))->id);
    //printf("ID des zweiten Eintrags: %d\n", ((struct val_data*)(head->next->val))->id);

    node_t* current = head;
    if(current->val == NULL){
        return 0;
    }
    if(((struct val_data*)(current->val))->id == id){
        return 1;
    }
    while(current->next != NULL){
        if (((struct val_data*)(current->val))->id == id){
            return 1;
        }
        //printf("Current ID: %d\n", ((struct val_data*)(current->val))->id);
        current = current->next;
        if (((struct val_data*)(current->val))->id == id){
            return 1;
        }
    }
    return 0;
}
int push_with_lowest_id(node_t ** head, void* val, node_t * id_list){

      struct val_data* push_val;
      //int size = list_size(*head);

      node_t * current = *head;
      int id = get_and_set_lowest_id(id_list);
      push_val = (struct val_data*)malloc(sizeof(struct val_data));
      push_val->id = id;
      push_val->data = val;
      int size_of_list = list_size(*head);

      node_t* push_data = (node_t*)malloc(sizeof(node_t));
      push_data->val = (void*)push_val;
      push_data->next= NULL;

      if(size_of_list == 0){
          current->val = (void*)push_val;
          current->next = NULL;
          return id;
      }

       /*ziwschen anderen Eintraegen einfuegen*/

          while(current->next!= NULL){
              if (((struct val_data*)current->next->val)->id > id){
                /*insert push_val between current and current->next*/
                push_data->next = current->next;
                current->next = push_data;
                return id;
              }
              current = current->next;
          }
          /*  add a new value */

          current->next = push_data;
          return id;
}

  /*returns lowest free id*/
  int get_and_set_lowest_id(node_t* id_list){

    /*node_t* current2 = id_list;
    int laaenge = list_size(id_list);
    printf("Suche minimale ID, laenge: %d\n", list_size(id_list));
    int k = 0;
    if (laaenge != 0){
    for(int i=0; i < laaenge; i++){
      int* d = (int*)(current2->val);
      printf("Platz %d, current2 id: %d\n", k, *d);
      current2 = current2->next;
      k++;
      }
    }*/

    //printf("In der neuen loesch Funktion\n");
    int ret_id = 0;
    struct is_used* new_entry = malloc(sizeof(struct is_used));
    new_entry->used = 1;

    node_t* current = id_list;

    if (current->val == NULL){ /*no entry*/
      /*create first entry*/
      current->val = new_entry;
      /*printf("case 1, returned id: %d\n",  ret_id);
      printf("current->val: %d\n", *(int*)current->val);*/
      return ret_id;

    }
    else{
      while(current->next != NULL){
        current = current->next;
        ret_id += 1;
        if (*(int*)current->val == 0){ /*id ret_val is free*/
          current->val = new_entry;
          printf("case 2, returned id: %d\n",  ret_id);
          return ret_id;
        }
      }
      if (*(int*)current->val == 0){ /*id ret_val is free*/
        current->val = new_entry;
        printf("case 3, returned id: %d\n",  ret_id);
        return ret_id;
      }
      else{ /*create new element*/
        node_t* append_node = malloc(sizeof(node_t));
        append_node->next = NULL;
        append_node->val = new_entry;
        current->next = append_node;
        return ret_id+1;
      }
    }
  }

void* remove_by_id_and_free_id(node_t ** head, node_t* id_list, int id){

  node_t* current = *head;
  node_t * temp_node = *head;
  node_t* current2 = id_list;
  struct is_used* used;
  void* ret_data;
  int i;

  /*case remove first element*/
  if (((struct val_data*)current->val)->id == id) {
      for(i = 0; i < id; i++){
        current2 = current2->next;
        }
        used = current2->val;
        used->used = 0;
      return pop(head);
    }

  for (i = 0; i< id; i++){
    if(current->next!= NULL){
      current = current->next;
    }
    if(((struct val_data*)current->val)->id == id ){
      /*remove*/
      ret_data = ((struct val_data*)current->val)->data;
      int j;
      for(j = 0; j < id; j++){
        current2 = current2->next;
      }
      used = current2->val;
      used->used = 0;
      /*remove node*/
      temp_node->next = current->next;
      free(current);
      return ret_data;
    }

    temp_node= temp_node->next;

  }
  return NULL;
}

int id_in_list(node_t* head, DATALENGTH id){
  printf("in id in liste\n");
  node_t* current = head;
  struct val_data* tmp = NULL;
  printf("vor if\n");
  int size = list_size(head);
  printf("laenge erhaltgne\n");
  if (size == 0){
    return 0;
  }
  printf("nach erstem if\n");
  while(current->next != NULL){
    tmp = ((struct val_data*)current)->data;
    if ( tmp->id == id){
      return 1;
    }
    current = current->next;
  }
  printf("nach hauptteil\n");
  if ( tmp->id == id){
    return 1;
  }
  return 0;
}


void print_pop_list(node_t * head){
    int id;
    //int value;
    node_t* current = head;

    while(current->val != NULL){
        //value = *(int*)((struct val_data*)current->val)->data;
        id = ((struct val_data*)current->val)->id;
        printf("Id: %d\n",  id);
        if (current->next != NULL){
            current = current->next;
        }
        else{
          break;
        }
    }
    //value = *(int*)((struct val_data*)current->val)->data;
    //id = ((struct val_data*)current->val)->id;
    //printf("Listenwert: %d, id: %d\n", value, id);
}
