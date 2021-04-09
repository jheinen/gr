#ifndef LIST
#define LIST
#define DATALENGTH unsigned int

struct val_data{
    int id;
    void* data;
};

typedef struct node {
    void* val;
    struct node * next;
} node_t;

struct is_used{
  int used;
};

/*creates and returns a Pointer to a new node Object*/
node_t *new_list(void);

/*creates new Element with the val param at End of List*/
int push(node_t ** head, void* val);

/*removes the head Element*/
void* pop(node_t ** head);

/*removes Element by given Index*/
void* remove_by_index(node_t ** head, int n);

/*removes Element by given id*/
void* remove_by_id(node_t ** head, int id);

/*returns Element at index*/
void* get_by_index(node_t * head, int index);

/*returns Element by id*/
void* get_by_id(node_t * head, int id);

/*returns the size of the List*/
int list_size(node_t * head);

/*returns 1 or 0*/
int list_contains_id(node_t* head, int id);

/*pushes element and uses given id as id*/
int push_with_lowest_id(node_t ** head, void* val, node_t * id_list);

/*gets lowest id wich is free*/
int get_lowest_id(node_t* id_list);

/*returns lowest free id*/
int get_and_set_lowest_id(node_t* head);

/*removes element with given id and unsets id in id_list*/
void* remove_by_id_and_free_id(node_t ** head, node_t* id_list, int id);

/*checks if id in list, return 1 if TRUE, else 0*/
int id_in_list(node_t* head, DATALENGTH id);

void print_pop_list(node_t * head);

#endif
