/*
 * Variable.h $Id: Variable.h,v 1.25 2004-06-22 11:10:34 mvkorpel Exp $
 */

#ifndef __VARIABLE_H__
#define __VARIABLE_H__

#include "potential.h"
#define VAR_SYMBOL_LENGTH 20
#define VAR_NAME_LENGTH 40
#define VAR_STATENAME_LENGTH 20
#define VAR_MIN_ID 1

typedef struct {
  char symbol[VAR_SYMBOL_LENGTH + 1]; /* short symbol for the node */
  char name[VAR_NAME_LENGTH + 1]; /* label in the Net language*/
  char **statenames; /* a string array with <cardinality> strings */
  int cardinality;
  unsigned long id; /* unique id for every variable */
  double *likelihood; /* likelihood of each value */
  potential probability; /* probability of the variable, given parents */
  /* JJT: potential is mainly for making global retraction easier! */
} vtype;

typedef vtype *Variable;


struct varlist {
  Variable data;
  struct varlist *fwd;
  struct varlist *bwd;
};

typedef struct varlist varelement;
typedef varelement *varlink;


/* Creates a new Variable:
 * - symbol is a short name e.g. A (= array [A, \0])
 * - name is a more verbose name e.g. "rain" or NULL 
 * - states is an array of strings containing the state names or NULL
 * - cardinality is the number of states/values the variable has */
Variable new_variable(const char* symbol, const char* name, 
		      char** states, int cardinality);


/* Function for copying a Variable (if needed). Handle with care.
 * v: the Variable to be copied
 * Returns the copy.
 */
Variable copy_variable(Variable v);


/* Frees the memory used by the Variable v. */
void free_variable(Variable v);


/* Method for testing variable equality. 
 * This may be needed to keep potentials in order. INEQUALITIES ???
 */
int equal_variables(Variable v1, Variable v2);


/* An alternative interface for keeping variables 
 * and thus the potentials in order.
 */
unsigned long get_id(Variable v);


/* Returns the symbol of the Variable. It is a string. 
 * (or NULL if nullpointer given) */
char *get_symbol(Variable v);


/* Tells the length of the list of Variables. */
int total_num_of_vars();


/* Call this before searching through the list of variables. */
void reset_Variable_list();


/* Gives the next Variable in the list of Variables. Returns NULL when 
 * the end of list is reached. */
Variable next_Variable();


/* Gives v a new likelihood array. The size of the array
 * must match v->cardinality
 */
int update_likelihood(Variable v, double likelihood[]);


/* Returns the number of possible values in the Variable v. (-1 if v == NULL)
 */
int number_of_values(Variable v);


/* Sets the conditional probability of variable v.
 * If probability is already set, the previous values are freed from memory.
 * This means that the "ownership" of the potential will change.
 * DO NOT USE THE SAME POTENTIAL FOR ANY OTHER VARIABLE!
 */
int set_probability(Variable v, potential p);

#endif /* __VARIABLE_H__ */
