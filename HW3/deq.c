#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "deq.h"
#include "error.h"

// indices and size of array of node pointers
typedef enum {Head,Tail,Ends} End;

typedef struct Node {
  struct Node *np[Ends];        // next/prev neighbors
  Data data;
} *Node;

typedef struct {
  Node ht[Ends];                // head/tail nodes
  int len;
} *Rep;

static Rep rep(Deq q) {
  if (!q) ERROR("zero pointer");
  return (Rep)q;
}

static void put(Rep r, End e, Data d) {
  Node newNode = malloc(sizeof(*newNode));
  if (!newNode) ERROR("malloc() failed");
  
  newNode->data = d;
  
  if(r->len == 0){
    newNode->np[Head] = NULL;
    newNode->np[Tail] = NULL;
    
    r->ht[Head] = newNode;
    r->ht[Tail] = newNode;
    r->len++;

    return;
  }

  if(e == Head){
    newNode->np[Tail] = r->ht[Head];
    newNode->np[Head] = NULL;

    r->ht[Head]->np[Head] = newNode;
    r->ht[Head] = newNode;
    r->len++;
  }

  else if(e == Tail){
    newNode->np[Head] = r->ht[Tail];
    newNode->np[Tail] = NULL;

    r->ht[Tail]->np[Tail] = newNode;
    r->ht[Tail] = newNode;
    r->len++;
  }
}

static Data ith(Rep r, End e, int i)  { 
  if(i < 0 || i >= r->len) ERROR("index out of range");

  if(e == Head){
    Node currentNode = r->ht[Head];
    for(int j = 0; j < i; j++){
      currentNode = currentNode->np[Tail];
    }
    return currentNode->data;
  }

  else if(e == Tail){
    Node currentNode = r->ht[Tail];
    for(int k = 0; k < i; k++){
      currentNode = currentNode->np[Head];
    }
    return currentNode->data;
  }

  ERROR("invalid end");
 }

static Data get(Rep r, End e){

    if (e == Head) {
        Node oldNode = r->ht[Head];
        Data d = oldNode->data;

        r->ht[Head] = oldNode->np[Tail];

        if (r->ht[Head])
            r->ht[Head]->np[Head] = NULL;
        else
            r->ht[Tail] = NULL;

        r->len--;
        free(oldNode);

        return d;
    }

    else if (e == Tail) {
        Node oldNode = r->ht[Tail];
        Data d = oldNode->data;

        r->ht[Tail] = oldNode->np[Head];

        if (r->ht[Tail])
            r->ht[Tail]->np[Tail] = NULL;
        else
            r->ht[Head] = NULL;

        r->len--;
        free(oldNode);

        return d;
    }

    ERROR("invalid end");
}

static Data rem(Rep r, End e, Data d){

    if (e == Head) {
        Node currentNode = r->ht[Head];

        for (int i = 0; i < r->len; i++) {

            if (currentNode->data == d) {
                Data result = currentNode->data;

                // Removing the head
                if (currentNode->np[Head] == NULL) {
                    r->ht[Head] = currentNode->np[Tail];

                    if (r->ht[Head])
                        r->ht[Head]->np[Head] = NULL;
                    else
                        r->ht[Tail] = NULL;
                }

                // Removing a node after the head
                else {
                    currentNode->np[Head]->np[Tail] =
                        currentNode->np[Tail];

                    if (currentNode->np[Tail])
                        currentNode->np[Tail]->np[Head] =
                            currentNode->np[Head];
                    else
                        r->ht[Tail] = currentNode->np[Head];
                }

                r->len--;
                free(currentNode);

                return result;
            }

            currentNode = currentNode->np[Tail];
        }

        return NULL;
    }

    else if (e == Tail) {
        Node currentNode = r->ht[Tail];

        for (int i = 0; i < r->len; i++) {

            if (currentNode->data == d) {
                Data result = currentNode->data;

                // Removing the tail
                if (currentNode->np[Tail] == NULL) {
                    r->ht[Tail] = currentNode->np[Head];

                    if (r->ht[Tail])
                        r->ht[Tail]->np[Tail] = NULL;
                    else
                        r->ht[Head] = NULL;
                }

                // Removing a node before the tail
                else {
                    currentNode->np[Tail]->np[Head] =
                        currentNode->np[Head];

                    if (currentNode->np[Head])
                        currentNode->np[Head]->np[Tail] =
                            currentNode->np[Tail];
                    else
                        r->ht[Head] = currentNode->np[Tail];
                }

                r->len--;
                free(currentNode);

                return result;
            }

            currentNode = currentNode->np[Head];
        }

        return NULL;
    }

    ERROR("invalid end");
}

extern Deq deq_new() {
  Rep r=(Rep)malloc(sizeof(*r));
  if (!r) ERROR("malloc() failed");
  r->ht[Head]=0;
  r->ht[Tail]=0;
  r->len=0;
  return r;
}

extern int deq_len(Deq q) { return rep(q)->len; }

extern void deq_head_put(Deq q, Data d) {        put(rep(q),Head,d); }
extern Data deq_head_get(Deq q)         { return get(rep(q),Head);   }
extern Data deq_head_ith(Deq q, int i)  { return ith(rep(q),Head,i); }
extern Data deq_head_rem(Deq q, Data d) { return rem(rep(q),Head,d); }

extern void deq_tail_put(Deq q, Data d) {        put(rep(q),Tail,d); }
extern Data deq_tail_get(Deq q)         { return get(rep(q),Tail);   }
extern Data deq_tail_ith(Deq q, int i)  { return ith(rep(q),Tail,i); }
extern Data deq_tail_rem(Deq q, Data d) { return rem(rep(q),Tail,d); }

extern void deq_map(Deq q, DeqMapF f) {
  for (Node n=rep(q)->ht[Head]; n; n=n->np[Tail])
    f(n->data);
}

extern void deq_del(Deq q, DeqMapF f) {
  if (f) deq_map(q,f);
  Node curr=rep(q)->ht[Head];
  while (curr) {
    Node next=curr->np[Tail];
    free(curr);
    curr=next;
  }
  free(q);
}

extern Str deq_str(Deq q, DeqStrF f) {
  char *s=strdup("");
  for (Node n=rep(q)->ht[Head]; n; n=n->np[Tail]) {
    char *d=f ? f(n->data) : n->data;
    char *t; asprintf(&t,"%s%s%s",s,(*s ? " " : ""),d);
    free(s); s=t;
    if (f) free(d);
  }
  return s;
}
