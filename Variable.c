/*
 * Variable.c $Id: Variable.c,v 1.21 2004-06-16 12:04:26 jatoivol Exp $
 */

#include <string.h>
#include <stdlib.h>
#include "Variable.h"
#include "potential.h"
#include "errorhandler.h"

Variable new_variable(const char* symbol, const char* name, 
		      char** states, int cardinality){
  static long id = VAR_MIN_ID;
  int i;
  double *dpointer;
  Variable v = (Variable) malloc(sizeof(vtype));

  v->cardinality = cardinality;
  v->id = id++;
  v->probability = NULL; 
  /* FIXME: This NULL is probably NOT suitable, because the variables without parents 
   * will then have NULL as v->probability. The reason: HUGIN files don't have this 
   * "potential ( A ) { data = ( 1 1 1 ... 1 1 1 ) }" for independent variables. */
 
  strncpy(v->symbol, symbol, VAR_SYMBOL_LENGTH);
  v->symbol[VAR_SYMBOL_LENGTH] = '\0';

  if(variable_name(v, name) == ERROR_NULLPOINTER)
    /* DANGER! The name can be omitted and consequently be NULL */
    v->name[0] = '\0';

  // "the ownership" of the states (array of strings) changes
  variable_statenames(v, states); 

  v->likelihood = (double *) calloc(cardinality, sizeof(double));
  /* initialise likelihoods to 1 */
  for(dpointer=v->likelihood, i=0; i < cardinality; *dpointer++ = 1, i++);

  return v;
}


int variable_name(Variable v, const char *name){
  if(!name)
    return ERROR_NULLPOINTER;
  strncpy(v->name, name, VAR_NAME_LENGTH);
  v->name[VAR_NAME_LENGTH] = '\0';
  return NO_ERROR;
}


// "the ownership" of the states (array of strings) changes
int variable_statenames(Variable v, char **states){
  v->statenames = states;
  return 0;
}


Variable copy_variable(Variable v){
  int i;
  if(v == NULL)
    return NULL;
  Variable copy = (Variable) malloc(sizeof(vtype));
  copy->cardinality = v->cardinality;
  copy->id = v->id;

  strncpy(copy->name, v->name, VAR_NAME_LENGTH);
  copy->name[VAR_NAME_LENGTH] = '\0';

  copy->likelihood = (double *) calloc(copy->cardinality, sizeof(double));
  /* initialise likelihoods to 1 */
  for(i = 0; i < copy->cardinality; i++)
    copy->likelihood[i] = v->likelihood[i];

  return v;
}


void free_variable(Variable v){
  if(v == NULL)
    return;
  free(v->likelihood);
  if(v->probability != NULL)
    free(v->probability);
  free(v);
}


int equal_variables(Variable v1, Variable v2){
  return (v1->id == v2->id);
}


unsigned long get_id(Variable v){
  return v->id;
}


char *get_symbol(Variable v){
  return v->symbol;
}


int update_likelihood(Variable v, double likelihood[]){

  int i;
  if(v == NULL)
    return ERROR_NULLPOINTER;

  for(i = 0; i < v->cardinality; i++)
    (v->likelihood)[i] = likelihood[i];

  return NO_ERROR;
}


int number_of_values(Variable v){
  if(v == NULL)
    return ERROR_NULLPOINTER;
  else
    return v->cardinality;
}


/* the "ownership" of the potential changes */
int set_probability(Variable v, potential p){
  if(v == NULL)
    return ERROR_NULLPOINTER;

  if(v->probability != NULL)
    free(v->probability);
  v->probability = p;
  return NO_ERROR;
}
