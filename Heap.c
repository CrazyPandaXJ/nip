/*
 * Heap.c $Id: Heap.c,v 1.10 2005-03-21 12:45:59 jatoivol Exp $
 */

#include <stdlib.h>
#include "Variable.h"
#include "Graph.h"
#include "Heap.h"
#include "errorhandler.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void free_useless_Sepsets(Heap *H);
static int lessthan(Heap_item h1, Heap_item h2);
static void heapify(Heap* H, int i);
static int get_heap_index(Heap* H, Variable v);
static void clean_heap_item(Heap_item* hi, Heap_item* min_cluster, Graph* G);
static int edges_added(Graph* G, Variable* vs, int n);
static int cluster_weight(Variable* vs, int n);

static int edges_added(Graph* G, Variable* vs, int n)
{
    /* vs is the array of variables in the cluster induced by vs[0] */

    int i,j, sum = 0;

    for (i = 0; i < n; i++)
        for (j = i+1; j < n; j++)
            sum += !is_child(G, vs[i], vs[j]);

      /* AR: Joo, po. looginen not. Tarkoitus on laskea, kuinka monta
         kaarta on lis�tt�v� graafiin, eli kaikki ne tapaukset, 
         kun A ei ole B:n lapsi. T�ss� k�yd��n taulukko vain yhteen suuntaan,
         eroaa alkuper�isest� ideasta kertoimella 2.*/
   
    return sum; /* Number of links to add */
}

static int cluster_weight(Variable* vs, int n)
{
    /* vs is the array of variables in the cluster induced by vs[0] */
    int i, prod = 1;
    
    for (i = 0; i < n; i++)
	prod *= number_of_values(vs[i]);

    return prod;
}

/* Heap management */

Heap* build_heap(Graph* Gm)
{
    int i,j, n;

    Heap_item* hi;
    Variable* Vs_temp;

    Heap* H = (Heap*) malloc(sizeof(Heap));
    if(!H)
      return NULL;

    n = get_size(Gm);

    Vs_temp = (Variable*) calloc(n, sizeof(Variable));
    if(!Vs_temp){
      free(H);
      return NULL;
    }

    H->heap_items = (Heap_item*) calloc(n, sizeof(Heap_item));
    if(!(H->heap_items)){
      free(Vs_temp);
      free(H);
    }

    H->heap_size = n;
    H->orig_size = n;
    H->useless_sepsets = NULL;
    
    for (i = 0; i < n; i++)
    {
        hi = &(H->heap_items[i]);
        hi->n = get_neighbours(Gm, Vs_temp, Gm->variables[i]) +1;
        /* get_neighbours could be modified to use the array Vs directly;
           the cost associated with it would be having all Vs take
           get_size(G) units of memory.
        */

	hi->Vs = (Variable *) calloc(hi->n, sizeof(Variable));
	if(!(hi->Vs)){
	  free(Vs_temp);
	  for(j = 0; j < i; j++)
	    free(H->heap_items[j].Vs);
	  free(H->heap_items);
	  free(H);
	}

	hi->Vs[0] = Gm->variables[i];
        
        for (j = 1; j < hi->n; j++)   /* Copy variable-pointers from Vs_temp */
            hi->Vs[j] = Vs_temp[j-1]; /* Note the index-shifting */

        hi->primary_key = edges_added(Gm, hi->Vs, hi->n);
        hi->secondary_key = cluster_weight(hi->Vs, hi->n);
	hi->s = NULL; /* not a sepset heap */
    }

    free(Vs_temp);

    for (i = n/2 -1; i >= 0; i--)
        heapify(H, i);

    return H;
}

Heap* build_sepset_heap(Clique* cliques, int num_of_cliques)
{
    int i,j, k = 0;
    int n = (num_of_cliques * (num_of_cliques - 1)) / 2;
    int hi_index = 0;
    int isect_size;
    int retval;
    Clique neighbours[2];
    Variable *isect;

    Heap_item* hi;

    Heap* H = (Heap*) malloc(sizeof(Heap));
    if(!H)
      return NULL;

    H->heap_items = (Heap_item*) calloc(n, sizeof(Heap_item));
    if(!H->heap_items){
      free(H);
      return NULL;
    }

    H->heap_size = n;
    H->orig_size = n;

    H->useless_sepsets = (Sepset *) calloc(n, sizeof(Sepset));
    if(!H->useless_sepsets){
      free(H->heap_items);
      free(H);
      return NULL;
    }
    
    /* Go through each pair of Cliques. Create candidate Sepsets. */
    for(i = 0; i < num_of_cliques - 1; i++)

      for(j = i + 1; j < num_of_cliques; j++){

        hi = &(H->heap_items[hi_index++]);
	neighbours[0] = cliques[i];
	neighbours[1] = cliques[j];

	/* Take the intersection of two Cliques. */
	retval = clique_intersection(cliques[i], cliques[j],
				     &isect, &isect_size);

	if(retval != 0)
	  return NULL;

	hi->s = make_Sepset(isect, isect_size, neighbours);

	/* In case of failure, free all Sepsets and the Heap. */
	if(!(hi->s)){
	  report_error(__FILE__, __LINE__, ERROR_GENERAL, 1);
	  for(k = 0; k < hi_index - 1; k++){
	    hi = &(H->heap_items[k]);
	    free_Sepset(hi->s);
	    hi->s = NULL;
	  }
	  for(k = 0; k < n; k++)
	    H->useless_sepsets[k] = NULL;

	  free_heap(H);
	  return NULL;
	}

	/* Initially, all Sepsets are marked as useless (to be freed later) */
	H->useless_sepsets[k++] = hi->s;

	free(isect);

	/* We must use a negative value, because we have a min-heap. */
        hi->primary_key = -isect_size;

        hi->secondary_key =
	  cluster_weight(cliques[i]->variables, cliques[i]->p->num_of_vars) +
	  cluster_weight(cliques[j]->variables, cliques[j]->p->num_of_vars);
	hi->Vs = NULL; /* this is a sepset heap, no need for hi->Vs */
      }

    /* Check Cormen, Leiserson, Rivest */
    for (i = n/2 -1; i >= 0; i--)
        heapify(H, i);

    return H;
}

static int lessthan(Heap_item h1, Heap_item h2)
{
    return (h1.primary_key < h2.primary_key) || 
	   (h1.primary_key == h2.primary_key && 
	    h1.secondary_key < h2.secondary_key);
}  

static void heapify(Heap* H, int i)
{
    int l,r;
    int min, flag;
    Heap_item temp;
    
    do {
        flag = 0;   
        l = LEFT(i); r = RIGHT(i);
    
        /* Note the difference between l (ell) and i (eye) */
    
        min = (l < H->heap_size && lessthan(H->heap_items[l], H->heap_items[i]))? l:i;
            
        if (r < H->heap_size && lessthan(H->heap_items[r], H->heap_items[min]))
            min = r;
            
        if (min != i)
        {
            /* Exchange array[min] and array[i] */
            temp = H->heap_items[min];
            H->heap_items[min] = H->heap_items[i];
            H->heap_items[i] = temp;
            
            i = min; flag = 1;
        }
    } while (flag);
}

int extract_min(Heap* H, Graph* G, Variable** cluster_vars)
{
    Heap_item min;	/* Cluster with smallest weight */
    int i, heap_i;

    if (H->heap_size < 1)
        return 0;
    
    min = H->heap_items[0];

#ifdef DEBUG
	printf("Eliminated node: %s (%i)\n", min.Vs[0]->symbol, min.n);
#endif    
    
    H->heap_items[0] = H->heap_items[H->heap_size -1];

    /* Mark this as NULL, so that free() won't be called twice */
    H->heap_items[H->heap_size -1].Vs = NULL;

    H->heap_size--;
    
    /* Iterate over neighbours of minimum element *
     * and update keys. The loop could be heavy.  */
	for (i = 1; i < min.n; i++)         
    {
		heap_i = get_heap_index(H, min.Vs[i]);
		clean_heap_item(&H->heap_items[heap_i], &min, G);
    }

    /* Rebuild the heap. */
    for (i = 1; i < min.n; i++)
		heapify(H, get_heap_index(H, min.Vs[i]));
    heapify(H, 0);
    
    *cluster_vars = min.Vs;
    return min.n; /* Cluster size*/
}

int extract_min_sepset(Heap* H, Sepset* sepset)
{
    Heap_item min;	/* Cluster with smallest weight */

    /* Empty heap, nothing to extract. */
    if (H->heap_size < 1)
      return ERROR_GENERAL;
    
    min = H->heap_items[0];

    /* Not a sepset heap */
    if(min.s == NULL)
      return ERROR_GENERAL;

    H->heap_items[0] = H->heap_items[H->heap_size -1];

    H->heap_size--;
    
    /* Rebuild the heap. Is this enough? */
    heapify(H, 0);
    
    *sepset = min.s;

    return 0;
}

/* Called by find_sepsets when a Sepset is accepted to the join tree. */
void mark_useful_Sepset(Heap* H, Sepset s){

  int i, n;

  if(!H || !s)
    return;

  n = H->orig_size;

  for(i = 0; i < n; i++)
    if(H->useless_sepsets[i] == s){
      H->useless_sepsets[i] = NULL;
      break;
    }

  return;
}

static void free_useless_Sepsets(Heap *H){

  int i, n;
  Sepset s;

  if(!H || !(H->useless_sepsets))
    return;

  n = H->orig_size;

  for(i = 0; i < n; i++){
    s = H->useless_sepsets[i];
    if(s != NULL)
      free_Sepset(s);
  }
}

static int get_heap_index(Heap* H, Variable v)
{
    /* Finds the heap element that contains the variable v */
    /* Linear search, but at the moment the only option */
    /* Hopefully this will not be too slow */

    int i;

    for (i = 0; i < H->heap_size; i++)
		if (equal_variables(H->heap_items[i].Vs[0], v))
			return i;

    return -1;
}

static void clean_heap_item(Heap_item* hi, Heap_item* min_cluster, Graph* G)
{
    int i, j, n_vars = 0, n_total;
    Variable v_i, V_removed = min_cluster->Vs[0];
    Variable* cluster_vars;

    /* Copy all variables in hi and min_cluster together.
       Copy hi first, because Vs[0] must be the generating node */
    n_total = hi->n + min_cluster->n;
    cluster_vars = (Variable*) calloc(n_total, sizeof(Variable));
    if(!cluster_vars)
      return;

    /*for (i = 0; i < hi->n; i++)
      cluster_vars[i] = hi->Vs[i];
      for (i = 0; i < min_cluster->n; i++)
      cluster_vars[hi->n +i] = min_cluster[i];*/

    memcpy(cluster_vars, hi->Vs, hi->n*sizeof(Variable));
    memcpy(cluster_vars+hi->n, min_cluster->Vs, 
	   min_cluster->n*sizeof(Variable));
		
    /* Remove duplicates and min_vs[0] */
    for (i = 0; i < n_total; i++)
      {
	v_i = cluster_vars[i];
	if (v_i == NULL) continue;
	for (j = i+1; j < n_total; j++)
	  {
	    if (cluster_vars[j] == NULL) continue;
	    if (equal_variables(v_i, cluster_vars[j]) ||
		equal_variables(V_removed, cluster_vars[j]))
	      cluster_vars[j] = NULL;
	  }
	cluster_vars[n_vars++] = v_i; /* Note: overwrites itself */
      }
		
    hi->n = n_vars;

    free(hi->Vs);

    hi->Vs = (Variable*) calloc(n_vars, sizeof(Variable));
    if(!(hi->Vs)){
      free(cluster_vars);
      return;
    }

    memcpy(hi->Vs, cluster_vars, n_vars*sizeof(Variable));

    free(cluster_vars);

    hi->primary_key = edges_added(G, hi->Vs, hi->n);
    hi->secondary_key = cluster_weight(hi->Vs, hi->n);

    return;
}

void free_heap(Heap* H){

  int i;
  Heap_item *hi;

  if(!H)
    return;

  /* Go through every heap item*/
  for(i = 0; i < H->orig_size; i++){
    hi = &(H->heap_items[i]);

    /* Free variable array in the heap item */
    free(hi->Vs);
    hi->Vs = NULL;
  }

  free_useless_Sepsets(H);
  
  free(H->heap_items);
  free(H->useless_sepsets);

  free(H);
  return;
}
