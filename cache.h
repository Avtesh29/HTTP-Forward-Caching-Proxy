#include <stdint.h>
#include <regex.h>
#include <stdbool.h>
// cache.h
// Header file for cache.c

#define CONTENT_START "Content-Length: "
#define CONTENT_LENGTH "Content-Length: [ -~]{1,128}"

#define CONTENT_NUM "[ -~]{1,128}"

// -------------------------------------------------------------------------------------------------
//                                         STRUCT DEFINITION
// -------------------------------------------------------------------------------------------------

typedef struct LLnode {
    char* host;             // host name
    char* uri;              // uri dest
    int32_t port;           // port number
    char* response;         // GET response
    int res_l;              // response length
    struct LLnode* next;    // pointer to next node
    struct LLnode* prev;    // pointer to prev node
    bool hit;               // hit before?
} LLnode_t;

typedef struct Linkedlist {
    LLnode_t* head;         // pointer to head (first node)
    LLnode_t* tail;         // pointer to tail (last node)
    int length;             // length of linked list
} Linkedlist_t;


typedef struct Cache {
    Linkedlist_t* list;     // data structure to store requests/responses
    char* mode;             // FIFO or LRU
    int size;               // cache size passed from command line
} Cache_t;

// -------------------------------------------------------------------------------------------------
//                                      FUNCTION DECLARATIONS
// -------------------------------------------------------------------------------------------------

void moveToFront(Linkedlist_t* L, LLnode_t* node);

void setCachedTrue(LLnode_t* node, char* response, int res_l, regex_t head_reg, regex_t cl_reg,
    regex_t num_reg, regex_t end_reg);

char* regex_init(regex_t regex, char* request, int* loc);

LLnode_t* newNode(int host_l, int uri_l, int32_t* port, int response_l, bool hit);

void printNode(LLnode_t* node);
Linkedlist_t* createList(void);
void destroyList(Linkedlist_t** L);
void insertNode(Linkedlist_t* L, char* host, char* uri, int32_t port, char* response, int res_l, bool hit);
void deleteNode(Linkedlist_t* L, LLnode_t* node);
void printList(Linkedlist_t* L);
int nodeEqual(LLnode_t* node, char* host, char* uri, int32_t port);

LLnode_t* findNode(Cache_t* c, char* host, char* uri, int32_t port);


Cache_t* createCache(char* mode, int size);
void destroyCache(Cache_t** c);
char* cacheHit(Cache_t* c, LLnode_t* node);
void cacheMiss(Cache_t* c, char* host, char* uri, int32_t port, char* response, int res_l);


