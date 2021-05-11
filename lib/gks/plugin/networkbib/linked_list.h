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

struct list_plus_size{
  node_t* list;
  DATALENGTH size;
};

/*creates and returns a Pointer to a new node Object*/
struct list_plus_size* new_list(void);

/*creates new Element with the val param at End of List*/
int push(struct list_plus_size* list_plus_size, void* val);

/*removes the head Element*/
void* pop(struct list_plus_size* list_plus_size); /*removes the head*/

/*removes Element by given Index*/
void* remove_by_index(struct list_plus_size* list_plus_size, int n);

/*removes Element by given id*/
void* remove_by_id(struct list_plus_size* list_plus_size, int id);

/*returns Element at index*/
void* get_by_index(struct list_plus_size* list_plus_size, int index);

/*returns Element by id*/
void* get_by_id(struct list_plus_size*, int id);

/*returns the size of the List*/
int list_size(struct list_plus_size* list_plus_size);

/*returns 1 or 0*/
int list_contains_id(struct list_plus_size* list_plus_size, int id);

/*pushes element and uses given id as id*/
int push_with_lowest_id(struct list_plus_size* list_plus_size, void* val, struct list_plus_size * id_list_);

/*gets lowest id wich is free*/
int get_lowest_id(node_t* id_list);

/*returns lowest free id*/
int get_and_set_lowest_id(node_t* head);

/*removes element with given id and unsets id in id_list*/
void* remove_by_id_and_free_id(struct list_plus_size* list_plus_size, struct list_plus_size* id_list_, int id);

/*checks if id in list, return 1 if TRUE, else 0*/
int id_in_list(struct list_plus_size* list_plus_size, DATALENGTH id);

void print_pop_list(node_t * head);

#endif
