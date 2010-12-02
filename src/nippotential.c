/* nippotential.c 
 * Authors: Janne Toivola, Mikko Korpela
 * Version: $Id: nippotential.c,v 1.2 2010-11-26 17:06:02 jatoivol Exp $
 */

#include "nippotential.h"


/*#define DEBUG_POTENTIAL*/

static double* nip_get_potential_pointer(nip_potential p, int indices[]);

static void nip_choose_potential_indices(int source_indices[], 
					 int dest_indices[], 
					 int mapping[], int size_of_mapping);

/*
 * Returns a pointer to the potential array element corresponding to 
 * the given variable values (indices).
 */
static double* nip_get_potential_pointer(nip_potential p, int indices[]){

  int index = 0;
  int i;
  int card_temp = 1;

  /* THE mapping (JJ: I made this clearer on 22.5.2004)*/
  for(i = 0; i < p->num_of_vars; i++){
    index += indices[i] * card_temp;
    card_temp *= p->cardinality[i];
  }
  return &(p->data[index]);
}


/*
 * Drops the indices that are marginalised or multiplied. 
 * dest_indices[] must have the same size as mapping[] and smaller than 
 * source_indices[].
 * Returns an error code.
 */
static void nip_choose_potential_indices(int source_indices[], 
					 int dest_indices[], 
					 int mapping[], int size_of_mapping){
  int i;
  /* Warning: Write Only Code (TM) */
  for(i = 0; i < size_of_mapping; i++)
    dest_indices[i] = source_indices[mapping[i]];
  return;
}


nip_potential nip_new_potential(int cardinality[], int num_of_vars, 
			    double data[]){

  /* JJ NOTE: what if num_of_vars = 0 i.e. size_of_data = 1 ???
     (this can happen with sepsets...) */
  int i;
  int size_of_data = 1;
  int *cardinal;
  double *dpointer = NULL;
  nip_potential p = (nip_potential) malloc(sizeof(nip_potential_struct));

  if(!p){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }  

  if(num_of_vars){
    cardinal = (int *) calloc(num_of_vars, sizeof(int));
    if(!cardinal){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      free(p);
      return NULL;
    }  
  }
  else
    cardinal = NULL;

  for(i = 0; i < num_of_vars; i++){
    size_of_data *= cardinality[i];
    cardinal[i] = cardinality[i];
  }

  p->cardinality = cardinal;
  p->size_of_data = size_of_data;
  p->num_of_vars = num_of_vars;
  p->data = (double *) calloc(size_of_data, sizeof(double));
  
  if(!(p->data)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    nip_free_potential(p);
    return NULL;
  }

  if(data == NULL){
    /* The array has to be initialised. Let's do it right away. */
    for(dpointer=p->data, i=0; i < size_of_data; *dpointer++ = 1, i++);
  }
  else{
    /* Just copy the contents of the array */
    for(i=0; i < size_of_data; i++)
      p->data[i] = data[i];
  }
  return p;
}


nip_potential nip_copy_potential(nip_potential p){
  if (p == NULL)
    return NULL;
  return nip_new_potential(p->cardinality, p->num_of_vars, p->data);
}


void nip_free_potential(nip_potential p){
  if(p){
    free(p->cardinality);
    free(p->data);
    free(p);
  }
  return;
}


void nip_uniform_potential(nip_potential p, double value){
  int i;
  if(p){
    for(i = 0; i < p->size_of_data; i++)
      p->data[i] = value;
  }
  return;
}


void nip_random_potential(nip_potential p){
  int i;
  if(p){
    for(i = 0; i < p->size_of_data; i++)
      p->data[i] = rand()/(double)RAND_MAX;
    /*p->data[i] = drand48(); NON-ANSI! */
  }
  return;
}


double nip_get_potential_value(nip_potential p, int indices[]){
  double *ppointer = nip_get_potential_pointer(p, indices);
  if(ppointer != NULL)
    return *ppointer;
  else{
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return -1;
  }
}


void nip_set_potential_value(nip_potential p, int indices[], double value){
  double *ppointer = nip_get_potential_pointer(p, indices);
  if(ppointer != NULL)
    *ppointer = value;
  return;
}


void nip_inverse_mapping(nip_potential p, int flat_index, int indices[]){
  /* NOTE: the first variable (a.k.a the 0th variable) is 
     'least significant' in the sense that the value of it 
     has the smallest effect on the memory address */
  int x = p->size_of_data;
  int i;
  for(i = p->num_of_vars - 1; i >= 0; i--){
    x /= p->cardinality[i];
    indices[i] = flat_index / x;    /* integer division */
    flat_index -= indices[i] * x;
  }
  return;
}


int nip_general_marginalise(nip_potential source, nip_potential destination, 
			    int mapping[]){
  int i;
  int *source_indices = NULL; 
  int *dest_indices = NULL;
  double *potvalue;

  /* index arrays  (eg. [5][4][3] <-> { 5, 4, 3 }) */

  if(destination->num_of_vars){
    dest_indices = (int *) calloc(destination->num_of_vars, sizeof(int));
    
    if(!dest_indices){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      return NIP_ERROR_OUTOFMEMORY;
    }
  }
  else{ /* the rare event of potential being scalar */
    destination->data[0] = 0;

    for(i = 0; i < source->size_of_data; i++)
      destination->data[0] += source->data[i];
    return NIP_NO_ERROR;
  }

  /* source->num_of_vars > 0 always */
  source_indices = (int *) calloc(source->num_of_vars, sizeof(int));  
  if(!source_indices){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(dest_indices);
    return NIP_ERROR_OUTOFMEMORY;
  }

  /* Remove old garbage */
  for(i = 0; i < destination->size_of_data; i++)
    destination->data[i] = 0.0;

  /* Linear traverse through array for easy access */
  for(i = 0; i < source->size_of_data; i++){
    /* flat index i  ->  index array  */
    nip_inverse_mapping(source, i, source_indices);

    /* remove extra indices, eg. if mapping = { 0, 2, 4 }, then
       source_indices { 2, 6, 7, 5, 3 } becomes dest_indices { 2, 7, 3 }*/
    nip_choose_potential_indices(source_indices, dest_indices, mapping,
				 destination->num_of_vars);

    /* get pointer to the destination potential element where the current
     data should be added */
    potvalue = nip_get_potential_pointer(destination, dest_indices);
    *potvalue += source->data[i];
  }

  free(source_indices);
  free(dest_indices); /* is allocating and freeing slow??? */

  /* JJ NOTE: What if each potential had a small array called temp_indices ??? 
     The space is needed anyway in general_marginalise() and 
     update_potential()... */

  return NIP_NO_ERROR;
}


int nip_total_marginalise(nip_potential source, double destination[], 
			  int variable){
  int i, j, x, index = 0, flat_index;
  int *source_indices;

  /* index arrays  (eg. [5][4][3] <-> { 5, 4, 3 }) 
                         |  |  |
     variable index:     0  1  2... (or 'significance') */
  if(source->num_of_vars){
    source_indices = (int *) calloc(source->num_of_vars, sizeof(int));
    
    if(!source_indices){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);    
      return NIP_ERROR_OUTOFMEMORY;
    }
  }
  else{
    destination[0] = source->data[0];
    return NIP_NO_ERROR;
  }

  /* initialization */
  for(i = 0; i < source->cardinality[variable]; i++)
    destination[i] = 0;

  for(i = 0; i < source->size_of_data; i++){
    /* partial inverse mapping to find out the destination index */
    flat_index = i;
    x = source->size_of_data;
    for(j = source->num_of_vars - 1; j >= variable; j--){
      x /= source->cardinality[j];
      index = flat_index / x;    /* integer division */
      flat_index -= index * x;
    }
    /* THE sum */
    destination[index] += source->data[i]; 
  }
  free(source_indices);
  return NIP_NO_ERROR;
}


void nip_normalise_array(double result[], int array_size){
  int i;
  double sum = 0;
  for(i = 0; i < array_size; i++)
    sum += result[i];
  if(sum == 0)
    return;
  for(i = 0; i < array_size; i++)
    result[i] /= sum;
  return;
}


/* wrapper */
int nip_normalise_potential(nip_potential p){
  if(!p){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }
  nip_normalise_array(p->data, p->size_of_data);
  return NIP_NO_ERROR;
}


/* Makes the potential a valid conditional probability distribution
 * assuming that the first variable is the (only) child */
int nip_normalise_cpd(nip_potential p){
  int i, n;
  if(!p){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }
  n = p->cardinality[0]; /* first dimension */
  for(i = 0; i < p->size_of_data; i += n)
    nip_normalise_array(&(p->data[i]), n);
  return NIP_NO_ERROR;
}


/* NOT TESTED... */
int nip_normalise_dimension(nip_potential p, int dimension){
  int i, n;
  int* map = NULL; /* cardinality / mapping array */
  nip_potential denom = NULL;

  if(!p || dimension < 0 || dimension >= p->num_of_vars){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  map = (int*) calloc(p->num_of_vars - 1, sizeof(int));
  if(!map){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NIP_ERROR_OUTOFMEMORY;
  }

  n = 0;
  for(i = 0; i < p->num_of_vars; i++)
    if(i != dimension)
      map[n++] = p->cardinality[i];

  denom = nip_new_potential(map, p->num_of_vars - 1, NULL);

  n = 0;
  for(i = 0; i < p->num_of_vars; i++)
    if(i != dimension)
      map[n++] = i;

  /* the hard way */
  nip_general_marginalise(p, denom, map);    /* marginalise */
  nip_update_potential(NULL, denom, p, map); /* divide      */
  
  nip_free_potential(denom);
  free(map);
  return NIP_NO_ERROR;
}


int nip_sum_potential(nip_potential sum, nip_potential increment){
  int i;
  
  if(!sum || !increment || sum->size_of_data != increment->size_of_data){
    /* check for different geometry? */
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }
  
  for(i = 0; i < sum->size_of_data; i++)
    sum->data[i] += increment->data[i];

  return NIP_NO_ERROR;
}


int nip_update_potential(nip_potential numerator, nip_potential denominator, 
			 nip_potential target, int mapping[]){
  int i;
  int *source_indices = NULL; 
  int *target_indices = NULL;
  int nvars = 0;
  double *potvalue;

  if((numerator && denominator && 
      (numerator->num_of_vars != denominator->num_of_vars)) || 
     (numerator == NULL && denominator == NULL)){
    /* I hope the logic behind &&-evaluation is "fail fast" */
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  if(numerator)
    nvars = numerator->num_of_vars;
  else
    nvars = denominator->num_of_vars;

  if(nvars){
    source_indices = (int *) calloc(nvars, sizeof(int));
    
    if(!source_indices){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      return NIP_ERROR_OUTOFMEMORY;
    }
  }
  else{ /* when numerator & denominator are scalar */
    for(i = 0; i < target->size_of_data; i++){
      if(numerator)
	target->data[i] *= numerator->data[0];

      if(denominator){
	if(denominator->data[0])
	  target->data[i] /= denominator->data[0];
	else
	  target->data[i] = 0;
      }
    }
    return NIP_NO_ERROR;
  }

  target_indices = (int *) calloc(target->num_of_vars, sizeof(int));

  if(!target_indices){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(source_indices);
    return NIP_ERROR_OUTOFMEMORY;
  }

  /* The general idea is the same as in marginalise */
  for(i = 0; i < target->size_of_data; i++){
    nip_inverse_mapping(target, i, target_indices);
    nip_choose_potential_indices(target_indices, source_indices, 
				 mapping, nvars);

    if(numerator){ /* THE multiplication */
      potvalue = nip_get_potential_pointer(numerator, source_indices);
      target->data[i] *= *potvalue;
    }

    if(denominator){ /* THE division */
      potvalue = nip_get_potential_pointer(denominator, source_indices);
      if(*potvalue != 0)
	target->data[i] /= *potvalue;
      else
	target->data[i] = 0;  /* see Procedural Guide p. 20 */
    }
  }

  free(source_indices); /* JJ NOTE: GET RID OF THESE */
  free(target_indices);

  return NIP_NO_ERROR;
}


int nip_update_evidence(double numerator[], double denominator[], 
			nip_potential target, int var){

  int i, source_index;
  int *target_indices;

  /* target->num_of_vars > 0  always */
  target_indices = (int *) calloc(target->num_of_vars, sizeof(int));
  if(!target_indices){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NIP_ERROR_OUTOFMEMORY;
  }

  /* The general idea is the same as in marginalise */
  for(i = 0; i < target->size_of_data; i++){
    nip_inverse_mapping(target, i, target_indices); 
    /* some of the work above is useless (only one index is used) */
    source_index = target_indices[var];

    target->data[i] *= numerator[source_index];  /* THE multiplication */

    if(denominator != NULL)
      if(denominator[source_index] != 0)
	target->data[i] /= denominator[source_index];  /* THE division */
    /* ----------------------------------------------------------- */
    /* It is assumed that: denominator[i]==0 => numerator[i]==0 !!!*/
    /* ----------------------------------------------------------- */
  }

  free(target_indices);   /* JJ NOTE: GET RID OF THESE */
  return NIP_NO_ERROR;
}


int nip_init_potential(nip_potential probs, nip_potential target, 
		       int mapping[]){

  /* probs is assumed to be normalised */

  int i;
  int *probs_indices = NULL; 
  int *target_indices = NULL;
  double *potvalue;

  if(!mapping){
    if(probs->size_of_data != target->size_of_data){
      /* certainly different geometry but no mapping !?!? */
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
      return NIP_ERROR_NULLPOINTER;
    }
    else{ /* the potentials (may) have the same geometry */
      for(i = 0; i < target->size_of_data; i++)
	target->data[i] *= probs->data[i];
      return NIP_NO_ERROR;
    }
  }

  if(probs->num_of_vars){
    probs_indices = (int *) calloc(probs->num_of_vars, sizeof(int));
    if(!probs_indices){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      return NIP_ERROR_OUTOFMEMORY;
    }
  }
  else /* probs is a scalar & normalised => probs == 1 */
    return NIP_NO_ERROR;

  target_indices = (int *) calloc(target->num_of_vars, sizeof(int));

  if(!target_indices){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(probs_indices);
    return NIP_ERROR_OUTOFMEMORY;
  }

  /* The general idea is the same as in marginalise */
  /** JJ NOTE: the fact that two potentials have the same 
   ** number of variables DOES NOT imply that the elements are 
   ** in the same order! (Had funny effects with the EM-algorithm :) 
   **/
  for(i = 0; i < target->size_of_data; i++){
    nip_inverse_mapping(target, i, target_indices);
    nip_choose_potential_indices(target_indices, probs_indices, 
				 mapping, probs->num_of_vars);    

    potvalue = nip_get_potential_pointer(probs, probs_indices);

    target->data[i] *= *potvalue;  /* THE multiplication */
  }
  
  free(probs_indices); /* JJ NOTE: GET RID OF THESE? */
  free(target_indices);

  return NIP_NO_ERROR;
}


/* TODO: fprintf_potential... */
void nip_fprintf_potential(FILE* stream, nip_potential p){

  int big_index, i;
  int *indices = NULL;

  if(p->num_of_vars){
    indices = (int *) calloc(p->num_of_vars, sizeof(int));
    if(!indices){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
      return;
    }
  }
  else{
    fprintf(stream, "P(0) = %f\n", p->data[0]);
    return;
  }

  if(!indices)
    return;

  for(big_index = 0; big_index < p->size_of_data; big_index++){
    nip_inverse_mapping(p, big_index, indices);
    fprintf(stream, "P(");
    for(i = 0; i < p->num_of_vars; i++){
      fprintf(stream, "%d", indices[i]);
      if(i != p->num_of_vars - 1)
	fprintf(stream, ", ");
    }
    fprintf(stream, ") = %f\n", p->data[big_index]);
    
  }
  free(indices);
}