#include <regex.h>

#include "cache.h"
#include "a5protocol.h"

#include <stdint.h>
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Cache Implementation and testing file

// -------------------------------------------------------------------------------------------------
//                                      CACHE FUNCTIONS
// -------------------------------------------------------------------------------------------------

void setCachedTrue(LLnode_t* node, char* response, int res_l, regex_t head_reg, regex_t cl_reg,
regex_t num_reg, regex_t end_reg) {

    if (node->response == NULL || (node->hit == true)) {
        return;
    }

    int header_start = 0;
    int content_start = 0;
    int content_end = 0;
    int end_loc = 0;

    char* head = regex_init(head_reg, response, &header_start);

    char* cl = regex_init(cl_reg, response, &content_start);

    if (cl == NULL) {
        return;
    }

    char* num = regex_init(num_reg, cl+16, &content_end);

    int content_l = atoi(num);

    char* end = regex_init(end_reg, node->response, &end_loc);

    int offset = 0;
    char* cached = "Cached: True\r\n";
    int size = res_l + 16;
    char* new_res = (char*)calloc(size, sizeof(char));

    offset += header_start - 1;


    memcpy(new_res, response, offset);

    memcpy(new_res + offset, cached, 14);
    offset += 14;
    
    memcpy(new_res + offset, response + header_start - 1, end_loc-header_start);
    offset += (end_loc - header_start);

    memcpy(new_res + offset, response + end_loc-1, content_l);
    offset += content_l;

    new_res[offset] = '\0';


    free(node->response);
    node->response = new_res;
    node->res_l = size;
    node->hit = true;

    free(head);
    head = NULL;
    free(cl);
    cl = NULL;
    free(num);
    num = NULL;
    free(end);
    end = NULL;
}

//Set up a regex and check if matching
char* regex_init(regex_t regex, char* response, int* loc) {
    
    regmatch_t pm[1];
    char* result = NULL;

    int match = regexec(&regex, response, 1, pm, 0);
    int start = pm[0].rm_so;
    int end = pm[0].rm_eo;
    int size = end - start;

    if (match == REG_NOMATCH) {
        return result;
    }

    else {
        char* res = calloc(size+1, sizeof(char));
        for (int i = start; i < end; i++) {
            res[i-start] = response[i];
        }   
        res[size] = '\0';
        result = res;
    }

    *loc += (end+1);
    return result;
}

Cache_t* createCache(char* mode, int size) {

    if (size == 0) {
        return NULL;
    }

    Cache_t* c = (Cache_t*)malloc(sizeof(Cache_t));

    c->list = createList();
    c->mode = mode;
    c->size = size;
    
    return c;
}

void destroyCache(Cache_t** c) {

    if ((*c) != NULL) {
        destroyList(&((*c)->list));
        free((*c)->list);
        (*c)->list = NULL;
        free(*c);
        c = NULL;
    }
}

char* cacheHit(Cache_t* c, LLnode_t* node) {

    if ((c == NULL) || (node == NULL)) {
        return NULL;
    }
    if (node->response == NULL) {
        return NULL;
    }

    if (strcmp(c->mode, "FIFO") == 0) {

        return node->response;
    }

    moveToFront(c->list, node);

    return node->response;
}

void moveToFront(Linkedlist_t* L, LLnode_t* node) {
    if (L == NULL) {
        return;
    }

    if (node == NULL) {
        return;
    }

    // node to move is head
    if (node == L->head) {
        return;
    }
    // node to move is tail
    else if (node->next == NULL){
        node->prev->next = NULL;
        L->tail = node->prev;
        node->prev = NULL;
        node->next = L->head;
        L->head->prev = node;
        L->head = node;
    }
    else {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        node->prev = NULL;
        node->next = L->head;
        L->head->prev = node;
        L->head = node;
    }
}

void cacheMiss(Cache_t* c, char* host, char* uri, int32_t port, char* response, int res_l) {

    if (c != NULL) {

        if ((c->list->length >= c->size)) {
            deleteNode(c->list, c->list->tail);
        }
        insertNode(c->list, host, uri, port, response, res_l, false);

    }  
}

// -------------------------------------------------------------------------------------------------
//                                   LINKED LIST FUNCTIONS
// -------------------------------------------------------------------------------------------------

Linkedlist_t* createList(void) {

    Linkedlist_t* L = (Linkedlist_t*)malloc(sizeof(Linkedlist_t));
    L->head = NULL;
    L->tail = NULL;
    L->length = 0;

    return L;
}

void destroyList(Linkedlist_t** L) {

    LLnode_t* next;
    if (*L != NULL) {
        LLnode_t* n = (*L)->head;
        while (n != NULL) {
            next = n->next;
            free(n->host);
            n->host = NULL;
            free(n->uri);
            n->uri = NULL;
            free(n->response);
            n->response = NULL;
            free(n);
            n = next;
        }
        free(*L);
        (*L) = NULL;
    }

}

LLnode_t* findNode(Cache_t* c, char* host, char* uri, int32_t port) { 

    if (c == NULL) {
        return NULL;
    }

    Linkedlist_t* L = c->list;
    
    if (L == NULL) {
        return NULL;
    }

    if (L->head == NULL) {
        return NULL;
    }

    if (L->head == L->tail) {
        if (nodeEqual(L->head, host, uri, port)) {
            return L->head;
        }
    }
    else {

        LLnode_t* lcurr = L->head;
        LLnode_t* rcurr = L->tail;
        int l = 0;
        int r = L->length;

        while (l <= r) {

            if (nodeEqual(lcurr, host, uri, port)) {
                return lcurr;
            }
            else if (nodeEqual(rcurr, host, uri, port)) {
                return rcurr;
            }
            else {
                lcurr = lcurr->next;
                l++;
                rcurr = rcurr->prev;
                r--;
            }
        }
    }

    
    return NULL;

}

LLnode_t* newNode(int host_l, int uri_l,int32_t* port, int response_l, bool hit) {
    LLnode_t* node = (LLnode_t*)malloc(sizeof(LLnode_t));
    node->host = (char*)malloc(host_l + 1);
    node->uri = (char*)malloc(uri_l + 1);
    node->port = *port;
    node->res_l = response_l;
    node->response = (char*)malloc(response_l + 1);
    node->next = NULL;
    node->prev = NULL;
    node->hit = hit;

    return node;
}

void insertNode(Linkedlist_t* L, char* inhost, char* inuri, int32_t inport, char* inresponse, int res_l, bool hit) {

    if (L == NULL) {
        return;
    }

    int host_l = (int)strlen(inhost);
    int uri_l = (int)strlen(inuri);

    LLnode_t* node = newNode(host_l, uri_l, &inport, res_l, hit);

    memcpy(node->host, inhost, host_l);
    (node->host)[host_l] = '\0';
    memcpy(node->uri, inuri, uri_l);
    (node->uri)[uri_l] = '\0';
    memcpy(node->response, inresponse, res_l);
    (node->response)[res_l] = '\0';

    if (L->head != NULL) {
        node->next = L->head;
        L->head->prev = node;
    }
    else {
        L->tail = node;
    }

    L->head = node;

    L->length += 1;

}

void deleteNode(Linkedlist_t* L, LLnode_t* node) {

    if (L == NULL) {
        return;
    }

    if (node == NULL) {
        return;
    }

    // node to delete is head and tail
    if ((node == L->head) && (node == L->tail)) {
        free(node->host);
        free(node->uri);
        free(node->response);
        free(node);

        node = NULL;
        L->head = NULL;
        L->tail = NULL;
    }
    // node to delete is head
    else if (node->prev == NULL){
        node->next->prev = NULL;
        L->head = node->next;

        free(node->host);
        free(node->uri);
        free(node->response);
        free(node);
        node = NULL;
    }
    // node to delete is tail
    else if (node->next == NULL){
        node->prev->next = NULL;
        L->tail = node->prev;

        free(node->host);
        free(node->uri);
        free(node->response);
        free(node);
        node = NULL;
    }
    else {
        node->prev->next = node->next;
        node->next->prev = node->prev;

        free(node->host);
        free(node->uri);
        free(node->response);
        free(node);
        node = NULL;
    }

    L->length--;

}

void printList(Linkedlist_t* L) {

    if ((L == NULL) || (L->length == 0)) {
        printf("Empty List\n");
        return;
    }

    LLnode_t* curr = L->head;

    printf("---------------\nLength: %d\n---------------\n", L->length);

    int i = 0;
    while (curr != NULL) {
        printf("[Index %d] host: %s, uri: %s, port: %d\t", 
            i, curr->host, curr->uri, curr->port);

        if (curr->prev == NULL) {
            printf("prev: NULL, ");
        }
        else {
            printf("prev: %s, ", curr->prev->host);
        }
        if (curr->next == NULL) {
            printf("next: NULL\n");
        }
        else {
            printf("next: %s\n", curr->next->host);
        }

        i++;
        curr = curr->next;
    }

    printf("---------------\n");
    LLnode_t* h = L->head;
    LLnode_t* t = L->tail;
    printf("[Head] host: %s, uri %s, port %d\n", 
        h->host, h->uri, h->port);
    printf("[Tail] host: %s, uri %s, port %d\n", 
        t->host, t->uri, t->port);
    printf("---------------\n");

}

int nodeEqual(LLnode_t* node, char* host, char* uri, int32_t port) {

    return ((strcmp(node->host, host) == 0) 
    && (strcmp(node->uri, uri) == 0) 
    && (node->port == port));

}

void printNode(LLnode_t* node) {

    if (node == NULL) {
        printf("\nNot found\n");
    }
    else {
        printf("\n[Found] host: %s, uri: %s, port: %d\n", 
            node->host, node->uri, node->port);
    }

}
