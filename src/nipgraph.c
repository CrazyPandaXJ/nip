/* nipgraph.c $Id: nipgraph.c,v 1.6 2010-12-02 18:15:21 jatoivol Exp $
 */


#include "nipgraph.h"

/* Internal helper functions */
static void nip_sort_graph_variables(nip_graph g);

static nip_clique* nip_cluster_list_to_clique_array(nip_int_array_list clusters, nip_variable* vars, int n);

static nip_heap nip_build_cluster_heap(nip_graph gm);

static nip_heap nip_build_sepset_heap(nip_clique* cliques, int num_of_cliques);


/*** GRAPH MANAGEMENT ***/

nip_graph nip_new_graph(unsigned n) {
    nip_graph newgraph = (nip_graph) malloc(sizeof(nip_graph_struct));
    if(!newgraph)
      return NULL;

    newgraph->size = n; newgraph->top = 0;

    newgraph->adj_matrix = (int*) calloc(n*n, sizeof(int));
    if(!(newgraph->adj_matrix)){
      free(newgraph);
      return NULL;
    }

    memset(newgraph->adj_matrix, 0, n*n*sizeof(int)); /* calloc does this? */

    newgraph->variables = (nip_variable*) calloc(n, sizeof(nip_variable));
    if(!(newgraph->variables)){
      free(newgraph->adj_matrix);
      free(newgraph);
      return NULL;
    }

    newgraph->var_ind = NULL;
    return newgraph;
}

nip_graph nip_copy_graph(nip_graph g) {
    int n, var_ind_size;
    nip_graph g_copy;

    n = g->size;
    g_copy = nip_new_graph(n);

    memcpy(g_copy->adj_matrix, g->adj_matrix, n*n*sizeof(int));
    memcpy(g_copy->variables, g->variables, n*sizeof(nip_variable));
    if (g->var_ind == NULL)
      g_copy->var_ind = NULL;
    else {
      var_ind_size = (g->max_id - g->min_id +1)*sizeof(long);
      
      g_copy->var_ind = (unsigned long*) malloc(var_ind_size);
      if(!(g_copy->var_ind)){
	nip_free_graph(g_copy);
	return NULL;
      }

      memcpy(g_copy->var_ind, g->var_ind, var_ind_size);
      g_copy->max_id = g->max_id;
      g_copy->min_id = g->min_id;
    }

    return g_copy;
}

void nip_free_graph(nip_graph g) { 
  if(g == NULL)
    return;
  free(g->adj_matrix);
  free(g->variables);
  free(g->var_ind); /* JJT: I added this here, but is it correct..? */
  free(g);
}

/*** GETTERS ***/

int nip_graph_size(nip_graph g) {
  if(g)
    return g->size;
  return -1;
}

nip_variable* nip_graph_variables(nip_graph g) {
  if(g)
    return g->variables; /* g still responsible for freeing it */
  return NULL;
}

int nip_graph_index(nip_graph g, nip_variable v) {
    int i;

    /* NOTE: Relies on variable ids being nearly consecutive
     * If the implementation changes, this becomes a lot less
     * memory efficient and needs to be modified. */

    /* AR: Bugger. adjmatrix does not change. What if children were 
       added only after all variables have been included? */
    if(g == NULL || v == NULL)
      return -1;

    if (g->var_ind != NULL) {
      i = g->var_ind[nip_variable_id(v) - g->min_id];
      return nip_equal_variables(g->variables[i], v)? i: -1;
    }
    else /* Backup linear search */
      for (i = 0; i < g->size; i++)
	if (nip_equal_variables(g->variables[i], v))
	  return i;

    return -1;
} 


int nip_graph_neighbours(nip_graph g, nip_variable v,
			 nip_variable* neighbours) {
    int i, j;
    int n = nip_graph_size(g);
    int vi = nip_graph_index(g, v);

    if (n < 0 || vi < 0)
      return -1; /* invalid input */

    j = 0;
    for (i = 0; i < n; i++)
      if (NIP_ADJM(g, vi, i))
	neighbours[j++] = g->variables[i]; /* neighbours array big enough? */

    return j; /* # of neighbours */
}


int nip_graph_is_child(nip_graph g, nip_variable parent, nip_variable child) {
    int i,j;
    i = nip_graph_index(g, parent);
    j = nip_graph_index(g, child);
    if(i<0 || j<0)
      return 0;
    return NIP_ADJM(g, i, j);
}

/*** SETTERS ***/

int nip_graph_add_variable(nip_graph g, nip_variable v){
    if (g->top == g->size)
      return NIP_ERROR_GENERAL; /* Cannot add more items. */

    g->variables[g->top] = v;
    g->top++;

    if (g->top == g->size)
      nip_sort_graph_variables(g);

    return NIP_NO_ERROR;
}

int nip_graph_add_child(nip_graph g, nip_variable parent, nip_variable child){
    int parent_i, child_i;

    parent_i = nip_graph_index(g, parent);
    child_i = nip_graph_index(g, child);

    if (parent_i < 0 || child_i < 0)
      return NIP_ERROR_INVALID_ARGUMENT;

    NIP_ADJM(g, parent_i, child_i) = 1;

    return NIP_NO_ERROR;
}

/*** OPERATIONS (methods) ***/

/*int varcomp(nip_variable v1, nip_variable v2) {
    return nip_variable_id(v1) - nip_variable_id(v2);
}*/

static void nip_sort_graph_variables(nip_graph g) {
    int i, id;

    if(!g)
      return;

    /* find min and max ids */
    g->min_id = nip_variable_id(g->variables[0]); 
    g->max_id = nip_variable_id(g->variables[0]);
    for (i = 1; i < g->size; i++) {
        id = nip_variable_id(g->variables[i]);
        g->min_id = (id < g->min_id)?id:g->min_id;
        g->max_id = (id > g->max_id)?id:g->max_id;
    }

    /* allocate new index array */
    if (g->var_ind)
      free(g->var_ind);
    g->var_ind = (unsigned long*) calloc(g->max_id - g->min_id + 1, 
					 sizeof(long));
    if(!(g->var_ind))
      return;
	
    for (i = 0; i < g->size; i++)
      g->var_ind[nip_variable_id(g->variables[i]) - g->min_id] = i;

    return;
}


nip_graph make_graph_undirected(nip_graph g) {
    nip_graph gu;   
    int i,j,n;
    
    if (g == NULL || g->variables == NULL)
        return NULL;

    n = g->size;
    gu = nip_copy_graph(g);

    /* Create the undirected graph */
    for (i = 0; i < n; i++)
      for (j = 0; j < n; j++)
	NIP_ADJM(gu, i, j) = NIP_ADJM(g, i, j) || NIP_ADJM(g, j, i);

    return gu;
}


nip_graph nip_moralise(nip_graph g) {
    int i,j,n,v;
    nip_graph gm;

    if (g == NULL || g->variables == NULL)
      return NULL;
    
    n = g->size;
    gm = nip_copy_graph(g);

    /* Moralisation */
    for (v = 0; v < n; v++)       /* Iterate variables */
      for (i = 0; i < n; i++) 
	if (NIP_ADJM(g, i, v))            /* If i parent of v, find those */
	  for (j = i+1; j < n; j++){ /* parents of v which are > i */
	    NIP_ADJM(gm, i, j) |= NIP_ADJM(g, j, v);
	    NIP_ADJM(gm, j, i) |= NIP_ADJM(g, j, v);
	  }
    
    return gm;
}

/* Additional interface edges. By Janne Toivola */
nip_graph nip_add_interface_edges(nip_graph g){
  int i,j,n;
  nip_variable v1, v2;
  nip_graph gi;
  
  if (g == NULL || g->variables == NULL)
    return NULL;
  
  n = g->size;
  gi = nip_copy_graph(g);
  
  /* compare variable by variable */
  for (i = 0; i < n; i++)       /* Iterate variables */
    for (j = i+1; j < n; j++) {
      v1 = g->variables[i];
      v2 = g->variables[j];
      /* join interface variables */
      if (((NIP_IF(v1) & NIP_INTERFACE_OLD_OUTGOING) && 
	   (NIP_IF(v2) & NIP_INTERFACE_OLD_OUTGOING)) ||
	  ((NIP_IF(v1) & NIP_INTERFACE_OUTGOING) && 
	   (NIP_IF(v2) & NIP_INTERFACE_OUTGOING))) {
	NIP_ADJM(gi, i, j) = 1;
	NIP_ADJM(gi, j, i) = 1;
      }
    }
  return gi;
}


static nip_clique* nip_cluster_list_to_clique_array(nip_int_array_list clusters, nip_variable* vars, int n) {
    int n_vars, i;
    int ncliques, clique_counter;
    nip_int_array_link lnk;
    nip_clique* cliques;
    nip_variable* clique_vars;

    ncliques = NIP_LIST_LENGTH(clusters);

    cliques = (nip_clique*) calloc(ncliques, sizeof(nip_clique));
    if(!cliques)
      return NULL;

    clique_vars = (nip_variable*) calloc(n, sizeof(nip_variable));
    if(!clique_vars){
      free(cliques);
      return NULL;
    }

    clique_counter = ncliques;
    for (lnk = NIP_LIST_ITERATOR(clusters); 
	 lnk != NULL; 
	 lnk = NIP_LIST_NEXT(lnk)) {

      /* Fill the array of clique variables */
      n_vars = 0;
      for (i = 0; i < n; i++)
	if (lnk->data[i])
	  clique_vars[n_vars++] = vars[i];

      /* Create a new clique */
      cliques[--clique_counter] = nip_new_clique(clique_vars, n_vars);
      /* This ^^^^^^^^^^^^^^^^ is sort of dangerous. */

      /* Clean up in case of errors */
      if(cliques[clique_counter] == NULL){
	for(i = clique_counter + 1; i < ncliques; i++)
	  nip_free_clique(cliques[i]);
	free(clique_vars);
	free(cliques);
	return NULL;
      }
    }
    
    free(clique_vars);
    return cliques;
}


int nip_triangulate_graph(nip_graph gm, nip_clique** clique_p) {
    int i, j, j_index, k, k_index, n;
    int clique_count = 0;
    int cluster_size;
    nip_variable* min_cluster;
    nip_heap h;
    nip_int_array_list clusters = NULL;
    int* variable_set; /* [i] true, if variable[i] is in the cluster */

    n = gm->size;
    h = nip_build_cluster_heap(gm); /* TODO: refactor this stuff */
    clusters = nip_new_int_array_list();

    for (i = 0; i < n; i++) {

      cluster_size = nip_extract_min_cluster(h, &min_cluster);
      
      /* New variable_set for this cluster */
      variable_set = (int*) calloc(n, sizeof(int));
      if(!variable_set) {
	/* FIXME: better error handling */
	nip_free_int_array_list(clusters);
	return -1;
      }
      /*memset(variable_set, 0, n*sizeof(int)); // calloc did this*/
      
      for (j = 0; j < cluster_size; j++) {
	/* Find out which variables belong to the cluster */
	j_index = nip_graph_index(gm, min_cluster[j]);
	variable_set[j_index] = 1;
    
	/* Add new edges to Gm. */
	for (k = j+1; k < cluster_size; k++) {
	  k_index = nip_graph_index(gm, min_cluster[k]);
	  
	  NIP_ADJM(gm, j_index, k_index) = 1;
	  NIP_ADJM(gm, k_index, j_index) = 1;
	}
      }

      /* Add the cluster to a list of cliques if valid */
      if (!nip_int_array_list_contains_subset(clusters, variable_set, n))
	nip_prepend_int_array(clusters, variable_set, n);
      else
	free(variable_set);

      /* MVK: memory leak fix */
      free(min_cluster);
    }

    /* Create a set of cliques from the found variable sets */
    *clique_p = nip_cluster_list_to_clique_array(clusters, gm->variables, n);
    clique_count = NIP_LIST_LENGTH(clusters);

    nip_free_heap(h);
    nip_free_int_array_list(clusters); /* JJT: Free the cluster list */
    
    return clique_count;
}


int nip_find_cliques(nip_graph g, nip_clique** cliques_p) {
    nip_graph gu, gm, gi;
    int n_cliques = 0;

    gm = nip_moralise(g);
    gi = nip_add_interface_edges(gm); /* added by JJT 6.3.2006 */
    nip_free_graph(gm);
    gu = nip_make_graph_undirected(gi);
    nip_free_graph(gi);

    /* triangulate and create a set of cliques */
    n_cliques = nip_triangulate_graph(gu, cliques_p);

    /* test if triangulate failed */
    if(n_cliques < 0){
      nip_free_graph(gu);
      return -1;
    }

    /* find a set of suitable sepsets to connect the cliques */
    if(nip_find_sepsets(*cliques_p, n_cliques) != NIP_NO_ERROR)
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);

    /* JJT: I added some free_graph stuff here, because I suspected
     * memory leaks... */
    nip_free_graph(gu);

    return n_cliques;
}

/*** Used to be in clique.c for some odd reason ***/
int nip_find_sepsets(nip_clique *cliques, int num_of_cliques){

  int inserted = 0;
  int i;
  nip_sepset s;
  nip_clique one, two;

#ifdef NIP_DEBUG_CLIQUE
  int j, k;
  int ok = 1;
#endif

  nip_heap h = nip_build_sepset_heap(cliques, num_of_cliques);
  /* TODO: get a list of all sepsets */
  if(!h){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
    return NIP_ERROR_GENERAL;
  }

  while(inserted < num_of_cliques - 1){
    /* Extract the "best" candidate sepset from the heap. */
    if(nip_extract_min_sepset(h, &s) != NIP_NO_ERROR){
      /* In case of error */
      nip_free_heap(h);
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
      return NIP_ERROR_GENERAL;
    }

    one = s->cliques[0];
    two = s->cliques[1];

    /* Unmark MUST be done before searching (cliques_connected). */
    for(i = 0; i < num_of_cliques; i++)
      nip_unmark_clique(cliques[i]);

    /* Prevent loops by checking if the cliques
     * are already in the same tree. */
    if(!nip_cliques_connected(one, two)){

      /* TODO: remove from the list of sepsets */
      nip_mark_useful_sepset(h, s); /* MVK */

#ifdef NIP_DEBUG_CLIQUE
      printf("In nipgraph.c: Trying to add ");
      print_sepset(s);

      printf(" to ");
      print_clique(one);

      printf(" and ");
      print_clique(two);
#endif

      if(nip_add_sepset(one, s) != NIP_NO_ERROR){
	nip_free_heap(h);
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
	return NIP_ERROR_GENERAL;
      }	
      if(nip_add_sepset(two, s) != NIP_NO_ERROR){
	nip_free_heap(h);
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
	return NIP_ERROR_GENERAL;
      }

      inserted++;
    }
  }

  /* TODO: free the list of remaining sepsets */
  nip_free_heap(h);

  return NIP_NO_ERROR;
}


/*** Refactored from Heap.c ***/
static nip_heap nip_build_cluster_heap(nip_graph gm) {
    int i,j, n;

    nip_heap_item hi;
    nip_variable* Vs_temp;

    nip_heap h = (nip_heap) malloc(sizeof(nip_heap_struct));
    if(!h)
      return NULL;

    n = nip_graph_size(gm);

    Vs_temp = (nip_variable*) calloc(n, sizeof(nip_variable));
    if(!Vs_temp){
      free(h);
      return NULL;
    }

    h->heap_items = (nip_heap_item*) calloc(n, sizeof(nip_heap_item));
    if(!(h->heap_items)){
      free(Vs_temp);
      free(h);
    }
    for (i=0; i < n; i++) {
      h->heap_items[i] = (nip_heap_item) malloc(sizeof(nip_heap_item_struct));
      if(!h->heap_items[i]){
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
	while (i >= 0)
	  free(h->heap_items[i--]);
	free(Vs_temp);
	free(h);
	return NULL;
      }
    }

    h->heap_size = n;
    h->orig_size = n;
    h->useless_sepsets = NULL;
    
    for (i = 0; i < n; i++) {
      hi = h->heap_items[i];
      hi->nvariables = nip_graph_neighbours(gm, gm->variables[i], Vs_temp)+1;
      /* graph_neighbours could be modified to use the array Vs directly;
	 the cost associated with it would be having all Vs take
	 get_size(G) units of memory. */

      hi->variables = (nip_variable *) calloc(hi->nvariables, 
					      sizeof(nip_variable));
      if(!(hi->variables)){
	/* Something went wrong, clean up */
	free(Vs_temp);
	for(j = 0; j < i; j++) {
	  free(h->heap_items[j]->variables);
	  free(h->heap_items[j]);
	}
	free(h->heap_items);
	free(h);
      }

      hi->variables[0] = gm->variables[i];
      
      for (j = 1; j < hi->nvariables; j++) /* Copy variable pointers */
	hi->variables[j] = Vs_temp[j-1]; /* Note the index-shifting */
      
      hi->primary_key = nip_graph_edges_added(hi->variables, 
					      hi->nvariables);
      hi->secondary_key = nip_cluster_weight(hi->variables, hi->nvariables);
      hi->s = NULL; /* not a sepset heap */
    }

    free(Vs_temp);

    for (i = n/2 -1; i >= 0; i--)
      nip_heapify(h, i);

    return h;
}

nip_heap nip_build_sepset_heap(nip_clique* cliques, int num_of_cliques) {
    int i,j, k = 0;
    int n = (num_of_cliques * (num_of_cliques - 1)) / 2;
    int hi_index = 0;
    int isect_size;
    int retval;
    nip_clique neighbours[2];
    nip_variable *isect;

    nip_heap_item hi;

    nip_heap h = (nip_heap) malloc(sizeof(nip_heap_struct));
    if(!h)
      return NULL;

    h->heap_items = (nip_heap_item*) calloc(n, sizeof(nip_heap_item));
    if(!h->heap_items){
      free(h);
      return NULL;
    }
    for(i=0; i<n; i++){
      h->heap_items[i] = (nip_heap_item) malloc(sizeof(nip_heap_item_struct));
      if(!h->heap_items[i]){
	nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
	while (i >= 0)
	  free(h->heap_items[i--]);
	free(h);
	return NULL;
      }
    }

    h->heap_size = n;
    h->orig_size = n;
    h->useless_sepsets = (nip_sepset *) calloc(n, sizeof(nip_sepset));
    if(!h->useless_sepsets){
      for(i=0; i<n; i++)
	free(h->heap_items[i]);
      free(h->heap_items);
      free(h);
      return NULL;
    }
    
    /* Go through each pair of cliques. Create candidate sepsets. */
    for(i = 0; i < num_of_cliques - 1; i++) {
      for(j = i + 1; j < num_of_cliques; j++) {

        hi = h->heap_items[hi_index++];
	neighbours[0] = cliques[i];
	neighbours[1] = cliques[j];

	/* Take the intersection of two cliques. */
	retval = nip_clique_intersection(cliques[i], cliques[j],
					 &isect, &isect_size);
	if(retval != NIP_NO_ERROR){
	  nip_report_error(__FILE__,__LINE__,retval,1);
	  nip_free_heap(h);
	  return NULL;
	}
	hi->s = nip_new_sepset(isect, isect_size, neighbours);
	free(isect);

	/* In case of failure, free all sepsets and the heap. */
	if(!(hi->s)){
	  nip_report_error(__FILE__, __LINE__, NIP_ERROR_GENERAL, 1);
	  for(k = 0; k < hi_index - 1; k++){
	    hi = h->heap_items[k];
	    nip_free_sepset(hi->s);
	    hi->s = NULL;
	  }
	  for(k = 0; k < n; k++)
	    h->useless_sepsets[k] = NULL; /* all of them freed? */
	  nip_free_heap(h);
	  return NULL;
	}

	/* Initially, all sepsets are marked as useless (to be freed later) */
	h->useless_sepsets[k++] = hi->s;

	/* We must use a negative value, because we have a min-heap. */
        hi->primary_key = -isect_size;

        hi->secondary_key =
	  nip_cluster_weight(cliques[i]->variables, 
			     cliques[i]->p->num_of_vars) +
	  nip_cluster_weight(cliques[j]->variables, 
			     cliques[j]->p->num_of_vars);
	hi->variables = NULL; /* this is a sepset heap, no need for variables */
      }
    }
    /* Check Cormen, Leiserson, Rivest */
    for (i = n/2 -1; i >= 0; i--)
      nip_heapify(h, i);

    return h;
}