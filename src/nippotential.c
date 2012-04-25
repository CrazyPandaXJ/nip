/*  NIP - Dynamic Bayesian Network library
    Copyright (C) 2012  Janne Toivola

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, see <http://www.gnu.org/licenses/>.
*/

/* nippotential.c 
 * Authors: Janne Toivola, Mikko Korpela
 * Version: $Id: nippotential.c,v 1.5 2011-01-23 23:01:47 jatoivol Exp $
 */

#include "nippotential.h"


/*#define DEBUG_POTENTIAL*/

static double* nip_get_potential_pointer(nip_potential p, int indices[]);

/* TODO: this could be a macro */
static void nip_choose_potential_indices(int source_indices[], 
					 int dest_indices[], 
					 int mapping[], int size_of_mapping);

/*
 * Returns a pointer to the potential array element corresponding to 
 * the given variable values (indices).
 */
static double* nip_get_potential_pointer(nip_potential p, int indices[]){
  int i;
  int index = 0;
  int card_temp = 1;
  /* THE mapping (JJ: I made this clearer on 22.5.2004)*/
  for(i = 0; i < p->dimensionality; i++){
    index += indices[i] * card_temp;
    card_temp *= p->cardinality[i];
  }
  return &(p->data[index]);
}


/* TODO: this could be a macro...
 * Drops the indices that are marginalised or multiplied. 
 * dest_indices[] must have the same size as mapping[] and smaller than 
 * source_indices[].
 */
static void nip_choose_potential_indices(int source_indices[], 
					 int dest_indices[], 
					 int mapping[], 
					 int size_of_mapping){
  int i;
  /* Warning: Write Only Code (TM) */
  for(i = 0; i < size_of_mapping; i++)
    dest_indices[i] = source_indices[mapping[i]];
  return;
}


nip_potential nip_new_potential(int cardinality[], int dimensionality, 
				double data[]){

  /* JJ NOTE: what if dimensionality = 0 i.e. dsize = 1 ???
   * Fixed 23.1.2011 */
  int i;
  int dsize;
  double* dpointer = NULL;
  nip_potential p;

  if(dimensionality < 0){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  p = (nip_potential) malloc(sizeof(nip_potential_struct));
  if(!p){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NULL;
  }  

  if(dimensionality > 0){
    p->cardinality = (int *) calloc(dimensionality, sizeof(int));
    p->temp_index = (int *) calloc(dimensionality, sizeof(int));
  }
  else {
    p->cardinality = (int *) calloc(1, sizeof(int));
    p->temp_index = (int *) calloc(1, sizeof(int));
  }
  if(!p->cardinality || !p->temp_index){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    free(p->cardinality);
    free(p);
    return NULL;
  }

  p->dimensionality = dimensionality;
  dsize = 1;
  for(i = 0; i < dimensionality; i++){
    dsize *= cardinality[i];
    p->cardinality[i] = cardinality[i];
  }
  if(dimensionality == 0) {
    p->cardinality[0] = 1;
  }

  p->size_of_data = dsize;
  p->data = (double *) calloc(dsize, sizeof(double));  
  if(!(p->data)){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    nip_free_potential(p);
    return NULL;
  }

  if(data == NULL){
    /* The array has to be initialised. Let's do it right away. */
    for(dpointer=p->data, i=0; i < dsize; *dpointer++ = 1, i++);
  }
  else{
    /* Just copy the contents of the array */
    for(i=0; i < dsize; i++)
      p->data[i] = data[i];
  }

  p->application_specific_properties = nip_new_string_pair_list();
  return p;
}


nip_error_code nip_set_potential_property(nip_potential p, 
					  char* key, char* value){
  nip_error_code err;
  if (!p || !key || !value){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NIP_ERROR_NULLPOINTER;
  }
  err = nip_append_string_pair(p->application_specific_properties, 
			       key, value);
  return err;
}

char* nip_get_potential_property(nip_potential p, char* key){
  char* value;
  if (!p || !key){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NULL;
  }
  value = nip_string_pair_list_search(p->application_specific_properties,
				      key);
  return value;
}

nip_potential nip_copy_potential(nip_potential p){
  nip_potential copy;
  if (p == NULL)
    return NULL;
  copy = nip_new_potential(p->cardinality, p->dimensionality, p->data);
  /* FIXME: copy p->application_specific_properties ? */
  return copy;
}

nip_error_code nip_retract_potential(nip_potential p, nip_potential ref){
  int i;
  if(!p || !ref){ /* catch null pointers */
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_NULLPOINTER, 1);
    return NIP_ERROR_NULLPOINTER;
  }
  if(NIP_DIMENSIONALITY(p) != NIP_DIMENSIONALITY(ref)){ /* check dimensions */
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }
  for(i=0; i < NIP_DIMENSIONALITY(p); i++)
    if(p->cardinality[i] != ref->cardinality[i]){
      nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
      return NIP_ERROR_INVALID_ARGUMENT;
    }

  for(i = 0; i < p->size_of_data; i++) /* actual work */
    p->data[i] = ref->data[i];

  return NIP_NO_ERROR;
}

void nip_free_potential(nip_potential p){
  if(p){
    nip_free_string_pair_list(p->application_specific_properties);
    free(p->cardinality);
    free(p->temp_index);
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
  /* NOTE: never do this when p->dimensionality == 0 */
  int i;
  int x = p->size_of_data;
  for(i = p->dimensionality - 1; i >= 0; i--){
    x /= p->cardinality[i];
    indices[i] = flat_index / x;    /* integer division */
    flat_index -= indices[i] * x;
  }
  return;
}


nip_error_code nip_general_marginalise(nip_potential source, 
				       nip_potential destination, 
				       int mapping[]){
  int i;
  double *potvalue;

  if(destination->dimensionality > source->dimensionality){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  /* index arrays  (eg. [5][4][3] <-> { 5, 4, 3 }) */
  if(destination->dimensionality == 0){
    /* the rare event of potential being scalar */
    destination->data[0] = 0;
    for(i = 0; i < source->size_of_data; i++)
      destination->data[0] += source->data[i];
    return NIP_NO_ERROR;
  }

  /* Remove old garbage */
  nip_uniform_potential(destination, 0.0);

  /* Linear traverse through array for easy access */
  for(i = 0; i < source->size_of_data; i++){
    /* flat index i  ->  index array  */
    nip_inverse_mapping(source, i, source->temp_index);

    /* remove extra indices, eg. if mapping = { 0, 2, 4 }, then
       source_indices { 2, 6, 7, 5, 3 } becomes dest_indices { 2, 7, 3 }*/
    nip_choose_potential_indices(source->temp_index, 
				 destination->temp_index, 
				 mapping,
				 destination->dimensionality);

    /* get pointer to the destination potential element where the current
     data should be added */
    potvalue = nip_get_potential_pointer(destination, destination->temp_index);
    *potvalue += source->data[i];
  }

  /* JJ NOTE: What if each potential had a small array called temp_index ??? 
   * The space is needed anyway in general_marginalise() and 
   * update_potential()... 
   * This was implemented 23.1.2011 */

  return NIP_NO_ERROR;
}


nip_error_code nip_total_marginalise(nip_potential source, 
				     double destination[], 
				     int variable){
  int i, j, x, index = 0, flat_index;

  if(variable >= source->dimensionality){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  /* index arrays  (eg. [5][4][3] <-> { 5, 4, 3 }) 
                         |  |  |
     variable index:     0  1  2... (or 'significance') */
  if(source->dimensionality == 0){
    destination[0] = source->data[0];
    return NIP_NO_ERROR;
  }
 
  /* initialization */
  for(i = 0; i < source->cardinality[variable]; i++)
    destination[i] = 0.0;

  for(i = 0; i < source->size_of_data; i++){
    /* partial inverse mapping to find out the destination index */
    flat_index = i;
    x = source->size_of_data;
    for(j = source->dimensionality - 1; j >= variable; j--){
      x /= source->cardinality[j];
      index = flat_index / x;    /* integer division */
      flat_index -= index * x;
    }
    /* THE sum */
    destination[index] += source->data[i]; 
  }

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
nip_error_code nip_normalise_potential(nip_potential p){
  if(!p){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }
  nip_normalise_array(p->data, p->size_of_data);
  return NIP_NO_ERROR;
}


/* Makes the potential a valid conditional probability distribution
 * assuming that the first variable is the (only) child */
nip_error_code nip_normalise_cpd(nip_potential p){
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
nip_error_code nip_normalise_dimension(nip_potential p, int dimension){
  int i, n;
  int* map = NULL; /* cardinality / mapping array */
  nip_potential denom = NULL;

  if(!p || dimension < 0 || dimension >= p->dimensionality){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  map = (int*) calloc(p->dimensionality - 1, sizeof(int));
  if(!map){
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_OUTOFMEMORY, 1);
    return NIP_ERROR_OUTOFMEMORY;
  }

  n = 0;
  for(i = 0; i < p->dimensionality; i++)
    if(i != dimension)
      map[n++] = p->cardinality[i];

  denom = nip_new_potential(map, p->dimensionality - 1, NULL);

  n = 0;
  for(i = 0; i < p->dimensionality; i++)
    if(i != dimension)
      map[n++] = i;

  /* the hard way */
  nip_general_marginalise(p, denom, map);    /* marginalise */
  nip_update_potential(NULL, denom, p, map); /* divide      */
  
  nip_free_potential(denom);
  free(map);
  return NIP_NO_ERROR;
}


nip_error_code nip_sum_potential(nip_potential sum, nip_potential increment){
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


nip_error_code nip_update_potential(nip_potential numerator, 
				    nip_potential denominator, 
				    nip_potential target, 
				    int mapping[]){
  int i;
  int nvars = 0;
  double* potvalue;
  int* source_index;

  if((numerator && denominator && 
      (numerator->dimensionality != denominator->dimensionality)) || 
     (numerator == NULL && denominator == NULL)){
    /* I hope the logic behind &&-evaluation is "fail fast" */
    nip_report_error(__FILE__, __LINE__, NIP_ERROR_INVALID_ARGUMENT, 1);
    return NIP_ERROR_INVALID_ARGUMENT;
  }

  if(numerator){
    nvars = numerator->dimensionality;
    source_index = numerator->temp_index;
  }
  else{
    nvars = denominator->dimensionality;
    source_index = denominator->temp_index;
  }

  if(nvars == 0){ /* when numerator & denominator are scalar */
    for(i = 0; i < target->size_of_data; i++){
      if(numerator)
	target->data[i] *= numerator->data[0];

      if(denominator){
	if(denominator->data[0])
	  target->data[i] /= denominator->data[0];
	else
	  target->data[i] = 0; /* see Procedural Guide p. 20 */
      }
    }
    return NIP_NO_ERROR;
  }

  /* The general idea is the same as in marginalise */
  for(i = 0; i < target->size_of_data; i++){
    nip_inverse_mapping(target, i, target->temp_index);
    nip_choose_potential_indices(target->temp_index, 
				 source_index, 
				 mapping, nvars);

    if(numerator){ /* THE multiplication */
      potvalue = nip_get_potential_pointer(numerator, source_index);
      target->data[i] *= *potvalue;
    }

    if(denominator){ /* THE division */
      potvalue = nip_get_potential_pointer(denominator, source_index);
      if(*potvalue != 0)
	target->data[i] /= *potvalue;
      else
	target->data[i] = 0;  /* see Procedural Guide p. 20 */
    }
  }

  return NIP_NO_ERROR;
}


nip_error_code nip_update_evidence(double numerator[], 
				   double denominator[], 
				   nip_potential target, 
				   int var){
  int i, source_index;

  /* target->dimensionality > 0  always */

  /* The general idea is the same as in marginalise */
  for(i = 0; i < target->size_of_data; i++){
    nip_inverse_mapping(target, i, target->temp_index); 
    /* some of the work above is useless (only one index is used) */
    source_index = target->temp_index[var];

    target->data[i] *= numerator[source_index];  /* THE multiplication */

    if(denominator != NULL)
      if(denominator[source_index] != 0)
	target->data[i] /= denominator[source_index];  /* THE division */
    /* ----------------------------------------------------------- */
    /* It is assumed that: denominator[i]==0 => numerator[i]==0 !!!*/
    /* ----------------------------------------------------------- */
  }

  return NIP_NO_ERROR;
}


nip_error_code nip_init_potential(nip_potential probs, 
				  nip_potential target, 
				  int mapping[]){

  /* probs is assumed to be normalised */

  int i;
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

  if(probs->dimensionality == 0)
    return NIP_NO_ERROR; /* probs is a scalar & normalised => probs == 1 */

  /* The general idea is the same as in marginalise */
  /** JJ NOTE: the fact that two potentials have the same 
   ** number of variables DOES NOT imply that the elements are 
   ** in the same order! (Had funny effects with the EM-algorithm :) 
   **/
  for(i = 0; i < target->size_of_data; i++){
    nip_inverse_mapping(target, i, target->temp_index);
    nip_choose_potential_indices(target->temp_index, 
				 probs->temp_index, 
				 mapping, 
				 probs->dimensionality);    

    potvalue = nip_get_potential_pointer(probs, probs->temp_index);
    target->data[i] *= *potvalue;  /* THE multiplication */
  }
  
  return NIP_NO_ERROR;
}


/* TODO: some better representation? */
void nip_fprintf_potential(FILE* stream, nip_potential p){
  int big_index, i;

  if(p->dimensionality == 0){
    fprintf(stream, "P(0) = %f\n", p->data[0]);
    return;
  }

  for(big_index = 0; big_index < p->size_of_data; big_index++){
    nip_inverse_mapping(p, big_index, p->temp_index);
    fprintf(stream, "P(");
    for(i = 0; i < p->dimensionality; i++){
      fprintf(stream, "%d", p->temp_index[i]);
      if(i < p->dimensionality - 1)
	fprintf(stream, ", ");
    }
    fprintf(stream, ") = %f\n", p->data[big_index]);
    
  }
}
