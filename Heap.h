#ifndef __HEAP_H__
#define __HEAP_H__

#include "Variable.h"
#include "Graph.h"

#define PARENT(i) (i/2)
#define LEFT(i) (2*i)
#define RIGHT(i) (2*i+1)
/* Teep� p = p -1; temppu. Ja sovita synnit. */

typedef struct {
    Variable V;
  /* MVK: Tarvitaan ilmeisesti t�llainen Vs. T�ytyi lis�t�. Mik� se on? */
    Variable* Vs;
    int primary_key;
    int secondary_key;
  /* MVK: Mik� n on? T�ytyi lis�t�, kun sellaista oli k�ytetty. */
    int n;
} Heap_item;

typedef struct {
    Heap_item* array;
    int heap_size;
  /* MVK: Mik� on orig_size ? */
    int orig_size;
} Heap;

Heap* build_heap(Graph* Gm);

void heapify(Heap*, int);

Variable* extract_min(Heap* H, Graph* G);

int get_heap_index(Heap* H, Variable v);

void clean_heap_item(Heap_item* hi, Variable V_removed, Graph* G);

#endif /* __HEAP_H__ */
