/*
 * potential.h $Id: potential.h,v 1.26 2004-08-25 11:14:41 mvkorpel Exp $
 */

#ifndef __POTENTIAL_H__
#define __POTENTIAL_H__

typedef struct pot_array_t {
  int size_of_data;
  int *cardinality;
  int num_of_vars;
  double *data;
} ptype;

/* typedef struct pot_array ptype; */
typedef ptype* potential;

/* Make a num_of_vars -dimension potential array. 
 * The potential array data[] can be null, if it is not known. */
potential make_potential(int cardinality[], int num_of_vars, double data[]);

/* Free the memory used by potential p. */
void free_potential(potential p);

/* Make a copy of a potential. 
 * Source and destination must be potentials of same cardinality! 
 */
void copy_potential(potential source, potential destination);

/* Gets a value from the potential p. Syntactic sugar. */
double get_pvalue(potential p, int indices[]);

/* Sets a value in the potential p and returns an error code.
 * Syntactic sugar.
 */
void set_pvalue(potential p, int indices[], double value);

/* Mapping from flat index to n-dimensional index, where n is the number of
 * variables in potential p.
 * USUALLY NOT NEEDED outside of potential.c */
void inverse_mapping(potential p, int flat_index, int indices[]);

/* Method for marginalising over certain variables. Useful in message passing
 * from clique to sepset. It is best that sepsets have two static potentials 
 * which take turns as the old and the new potential.
 * TAKE CARE OF THE ORDER OF VARIABLES! 
 * -source: the potential to be marginalised
 * -destination: the potential to put the answer into, variables will be 
 *               in the same order
 * -source_vars: indices of the marginalised variables in source potential
 *               (ascending order and between 0...num_of_vars-1 inclusive!) 
 * EXAMPLE: If sepset variables are the second (i.e. 1) and third (i.e. 2) 
 *          variable in a five variable clique, the call is 
 *          marginalise(cliquePotential, newSepsetPotential, {0, 3, 4}) 
 * -Returns an error code.
 */
int general_marginalise(potential source, potential destination, 
			int source_vars[]);

/* Method for finding out the probability distribution of a single variable 
 * according to a clique potential. This one is marginalisation too, but 
 * this one is not generic. The outcome is not normalized. 
 * -source: the potential to be marginalised
 * -destination: the double array for the answer
 *               SIZE OF THE ARRAY MUST BE CORRECT (check it from the variable)
 * -variable: the index of the variable of interest 
 */
int total_marginalise(potential source, double destination[], int variable);

/* Method for updating target potential by multiplying with numerator 
 * potential and dividing with denominator potential. Useful in message 
 * passing from sepset to clique. 
 * -target: the potential whose values are updated
 * -numerator: multiplier, usually the newer sepset potential (source)
 * -denominator: divider, usually the older sepset potential. This MUST have 
 *  similar geometry to numerator.
 * -extra_vars: an integer array which holds the target variable indices 
 *  (0...num_of_vars - 1 inclusive) that are NOT in source potentials and in 
 *  ascending order. Length of the array must be at least the number of 
 *  variables in source potentials. 
 * EXAMPLE: If two sepset variables are the third and fifth variables in 
 *  a five variable clique, the call is 
 *  update(newSepsetPotential, oldSepsetPotential, cliquePotential, {0, 1, 3}) 
 * -Returns an error code.
 * 
 * JJT: If denominator is NULL, only the multiplication is done.
 *      If numerator is NULL, and error is reported.
 */
int update_potential(potential numerator, potential denominator, 
		      potential target, int extra_vars[]);

/* Method for updating potential according to new evidence.
 * MUST BE: evidence[i] > 0 => v->likelihood[i] > 0
 * Otherwise a global retraction must be done before calling this !!!!!!
 * -numerator[]: new evidence about the variable
 * -denominator[]: old likelihood of the variable
 * -target: the potential to be updated
 * -var: the index (in the potential) of the variable that gets new evidence
 */
int update_evidence(double numerator[], double denominator[], 
		    potential target, int var);

/* This one implements the initialisation with observations. 
 * See the Procedural Guide page 25, step 2. 
 * Similar to update_potential() so look at the info about 
 * arguments there.
 */
int init_potential(potential probs, potential target, int extra_vars[]);

void print_potential(potential p);

#endif
