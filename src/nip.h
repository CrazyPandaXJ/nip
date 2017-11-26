/**
 * @file
 * @brief Top level abstractions of the NIP system: probably all you need to use
 * 
 * Include this header to use the NIP library. This includes:
 * - methods for creating, reading, and writing a DBN model,
 * - reading data from files,
 * - running inference and learning steps with the model & data,
 * - freeing the memory after use.
 *
 * @author Janne Toivola
 * @copyright &copy; 2007,2012 Janne Toivola <br>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. <br>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. <br>
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __NIP_H__
#define __NIP_H__

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

/* The exposed part of NIP */
#include "niperrorhandler.h" ///< runtime error reporting
#include "niplists.h" ///< structures for lists of basic data
//#include "nipheap.h" ///< priority heap for building graphs
#include "nipvariable.h" ///< random categorical variables
#include "nippotential.h" ///< multidimensional probability distributions
#include "nipjointree.h" ///< clique tree and probabilistic inference
//#include "nipgraph.h" ///< TODO: doxygen + error handling
#include "nipparsers.h"
//#include "huginnet.tab.h"

/* Hidden: nipgraph.h, nipheap.h, nipstring.h */

/* TODO: consider separating these from NIP core, let users have their own */
#define TIME_SERIES_LENGTH(ts) ( (ts)->length ) ///< gets time series length
#define UNCERTAIN_SERIES_LENGTH(ucs) ( (ucs)->length ) ///< gets ucs length

#define NIP_FIELD_SEPARATOR ' '
#define NIP_HAD_A_PREVIOUS_TIMESLICE 1

/* "How probable is the impossible" (0 < epsilon << 1) */
/*#define PARAMETER_EPSILON 0.00001*/

/**
 * Direction of inference: either forward or backward in time or sequence
 */
enum nip_direction_type {BACKWARD, FORWARD};
typedef enum nip_direction_type nip_direction; /// hide enum notation

/**
 * Data structure containing all necessary stuff for running 
 * probabilistic inference with a model for a single time step, 
 * except the input data itself
 */
typedef struct {
  int num_of_cliques; ///< number of cliques/potentials in the join tree
  nip_clique *cliques; ///< the actual cliques/potentials

  int num_of_vars; ///< number of random variables in the model
  nip_variable *variables; ///< the actual variables (names of values etc.)
  
  int num_of_nexts; ///< Number of variables in 'next' and 'previous'
  nip_variable *next; /**< The variables that will substitute 
			 another one in the next timeslice. */
  nip_variable *previous; /**< The variables that are substituted 
			     by some variables from the previous timeslice. */

  int outgoing_interface_size; ///< number of variables in I_{t}->
  nip_variable* outgoing_interface; ///< I_{t}->
  nip_variable* previous_outgoing_interface; ///< I_{t-1}->

  int incoming_interface_size; ///< number of variables in I_{t}<-
  nip_variable* incoming_interface; ///< I_{t}<-

  nip_clique in_clique; /**< The clique which receives the message
			   from the past timeslices */

  nip_clique out_clique; /**< The clique which handles the connection to the 
			    future timeslices */

  int num_of_children; ///< number of children < num_of_vars
  nip_variable *children; ///< all the variables that have parents
  nip_variable *independent; ///< ...and those who dont. (Redundant?)

  int node_size_x; ///< node width, for drawing the graph
  int node_size_y; ///< node height, for drawing the graph

  // TODO: Any extra data parsed from the model file?
} nip_struct;

typedef nip_struct* nip; /// hide pointer notation


/**
 * Structure for storing a batch of "crisp" observations
 */
typedef struct {
  nip model; ///< The model (variables and state names)

  int num_of_hidden; ///< Number of latent variables
  nip_variable *hidden; ///< The variables never observed (but not missing)

  int num_of_observed; ///< == model->num_of_vars - num_of_hidden
  nip_variable *observed; /**< Variables included in data
                           (even if missing in each time step) */

  int length; ///< Number of time steps
  int** data; ///< The time series data
  /* TODO: Should there be a cache for extremely large time series? */
} time_series_struct;

typedef time_series_struct* time_series;

/* TODO: Should there be something else for streams / online inference? */

/**
 * Structure for storing "soft" uncertain observations or inference results
 */
typedef struct {
  int num_of_vars; ///< number of variables
  nip_variable* variables; ///< variables of interest

  int length; ///< length of the time series or sequence
  double*** data; ///< probability distribution of every variable at every t
} uncertain_series_struct;

typedef uncertain_series_struct* uncertain_series;


/**
 * Makes the model forget all the given evidence.
 *
 * NOTE: also the priors specified in the NET file are cleared
 * (but remain intact in the variables) so you'll have to re-enter them
 * as soft evidence. (FIXME: priors should not be treated as evidence!)
 * @param model Your pointer to the whole probabilistic model
 * @see use_priors() */
void reset_model(nip model);


/**
 * Makes all the conditional probabilities uniform and forgets all evidence.
 * In other words, the model will be as if it was never initialised with 
 * any parameters at all. 
 * (All the variables and the join tree will be there, of course)
 * @param model Your pointer to the whole probabilistic model */
void total_reset(nip model);


/**
 * Enters the priors of independent variables into the model as evidence.
 * (FIXME: priors should not be treated as evidence!)
 * This has to be done after reset_model and parse_model if you have any
 * other kind of priors than uniform ones. If you need to suppress the 
 * initialisation of the variables representing another one from the 
 * previous timeslice, use 'has_history == 1'. 
 * (Usually has_history == 0 only for the first timeslice)
 * @param model Your pointer to the whole probabilistic model
 * @param has_history Non-zero (true) when considering incoming
 * evidence from a previous time slice instead of priors (independent
 * variables). */
void use_priors(nip model, int has_history);


/**
 * Creates a model according to a net file. 
 * Remember to free the model when done with it.
 * @param file the name of the net file as a string 
 * @return null in case of any errors, or a pointer to the whole
 * probabilistic model
 */
nip parse_model(char* file);


/**
 * Writes the parameters of \p model into Hugin NET file named \p filename
 * @param model the model to write
 * @param filename the name of output file
 * @return NIP_NO_ERROR if successfull
 */
int write_model(nip model, char* filename);


/**
 * Gets rid of \p model and frees some memory.
 * @param model Your pointer to the whole probabilistic model
 */
void free_model(nip model);


/**
 * Reads data from the data file and constructs a set of time series 
 * according to the given model. Remember to free results afterwards.
 * @param model The random variables and all
 * @param datafile Name of the input file as a string
 * @param results Pointer where the time_series is set, if N>0
 * @return Length T of the time series, or 0 in case of any issues.
 */
int read_timeseries(nip model, char* datafile, 
		    time_series **results);


/**
 * Writes a set of time series data into a file. Essentially CSV with
 * blank rows as separators between each time series, and value "null"
 * for missing data.
 * TODO: quotes and escape sequences for odd characters.
 * @param ts_set Array of time_series'
 * @param n_series Number of time_series'
 * @param filename Name of the output file
 * @return NIP_NO_ERROR if successful
 */
int write_timeseries(time_series *ts_set, int n_series, char* filename);


/**
 * Method for freeing the huge chunk of memory used by a time series.
 * Note that this does not free the model that was passed as a
 * parameter in read_timeseries().
 * @param ts The time series to be freed
 */
void free_timeseries(time_series ts);


/**
 * Tells the length of a time series. 
 * (Yes, we are dealing with batch processing.)
 * @param ts Time series
 * @return Length, or 0 for null pointer
 */
int timeseries_length(time_series ts);


/**
 * Writes the inferred probabilities of a given variable into a file. 
 * (Batch mode)
 * @param ucs_set Array of inference results (one series each)
 * @param n_series Number of time series in \p ucs_set
 * @param v Variable of interest
 * @param filename Name of the output file
 * @return NIP_NO_ERROR if successful
 */
int write_uncertainseries(uncertain_series *ucs_set, int n_series, 
			  nip_variable v, char* filename);


/**
 * The method for freeing the memory used by inference results.
 * @param ucs Inference result (one series) to be freed
 */
void free_uncertainseries(uncertain_series ucs);


/**
 * Tells the length of an uncertain time series.
 * @param The inference result
 * @return Length T, or 0 for null pointer
 */
int uncertainseries_length(uncertain_series ucs);


/**
 * Method for reading an observation from the time series. 
 * You'll have to specify the variable of interest \p v.
 *
 * NOTE: DO NOT alter the string returned by the function! 
 * The returned value may be NULL if the variable was not observed at
 * the specified moment in time, or \p ts is shorter than that.
 * @param ts Time series read from a file
 * @param v One of the model variables
 * @param time Time step in [0, T-1] */
char* get_observation(time_series ts, nip_variable v, int time);


/**
 * Method for modifying an observation in the time series. 
 * - Variable \p v must be one of the observed variables in the
 * time series.
 * - The observation must match one of the states of that
 * variable.
 * @param ts Time series
 * @param v One of the model variables
 * @param time Time step in [0, T-1]
 * @param observation One of the states of variable \p v
 * @return non-zero integer if anything went wrong
 */
int set_observation(time_series ts, nip_variable v, int time, 
		    char* observation);


/**
 * Tells the model about observations in current time step. 
 * @param model Your pointer to the whole probabilistic model
 * @param varname Name of the observed model variable
 * @param observation The observed state
 * @return In case of an error, a non-zero value is returned. 
 */
int insert_hard_evidence(nip model, char* varname, char* observation);


/**
 * If an observation has some uncertainty, the evidence can be inserted 
 * with this procedure. 
 * @param model Your pointer to the whole probabilistic model
 * @param varname Name of the variable
 * @param distribution Array of probabilities [0.0, 1.0] of each state
 * @return NIP_NO_ERROR if everything went well. 
 */
int insert_soft_evidence(nip model, char* varname, double* distribution);


/**
 * Method for inserting part of the evidence at a specified step \p t in a
 * time series \p ts into the (time slice) \p model. 
 *
 * NOTE: \p model may be different from the one used for reading the data.
 *
 * Only the variables marked with \p mark_mask will be considered: 
 * - mark_mask == MARK_BOTH : all suitable evidence of the time step is used
 * - mark_mask == MARK_OFF  : only unmarked variables are used
 * - mark_mask == MARK_ON   : only marked variables are used 
 * @param ts Input evidence
 * @param t Time step
 * @param model The inference engine
 * @param mark_mask Flags for including or excluding marked variables
 * @see nip_mark_variable()
 * @see nip_unmark_variable() */
int insert_ts_step(time_series ts, int t, nip model, char mark_mask);


/**
 * Method for inserting part of the evidence at a specified step \p t
 * in an uncertain time series \p ucs into \p model. 
 *
 * NOTE: \p model may be different from the one used for inferring \p ucs.
 *
 * Only the variables marked with \p mark_mask will be considered.
 * @param ucs Input evidence
 * @param t Time step
 * @param model The inference engine
 * @param mark_mask Flags for including or excluding marked variables
 * @see nip_mark_variable()
 * @see nip_unmark_variable() */
int insert_ucs_step(uncertain_series ucs, int t, nip model, char mark_mask);


/**
 * This algorithm computes the marginal posterior probability
 * distributions for given variables and for every time step according
 * to the timeseries. It uses only forward propagation, so the result
 * of time step t is not affected by the rest of the
 * timeseries. You'll have to specify the variables of interest in 
 * \p vars array and the number of the variables. 
 * Writes also the average log. likelihood if \p loglikelihood is not
 * a null pointer.
 *
 * NOTE: Only evidence for the marked variables is used. Unmarked are
 * ignored and you can thus easily omit evidence for a variable over
 * entire time series.
 *
 * @param ts The input data
 * @param vars Variables of interest
 * @param nvars Length of \p vars
 * @param loglikelihood A pointer where avg. log. likelihood is written
 * @return Marginal probability distributions of each variable of
 * interest for each time step given the past evidence
 * @see nip_mark_variable()
 * @see nip_unmark_variable()
 */
uncertain_series forward_inference(time_series ts, nip_variable vars[], 
				   int nvars, double* loglikelihood);


/**
 * This one computes the probability distributions for every variable
 * of interest and for every time step according to the timeseries.
 * It uses both forward and backward propagation, so the result of time 
 * step t is affected by the whole timeseries. 
 * You'll have to specify the variables of interest in the vars array 
 * and the number of the variables.
 * Returns also the average log. likelihood if \p loglikelihood is not
 * a null pointer.
 *
 * NOTE: Only evidence for the marked variables is used. Unmarked are
 * ignored and you can thus easily omit evidence for a variable over
 * entire time series.
 *
 * @param ts The input data
 * @param vars Variables of interest
 * @param nvars Length of \p vars
 * @param loglikelihood A pointer where avg. log. likelihood is written
 * @return Marginal probability distributions of each variable of
 * interest for each time step given all the evidence
 * @see nip_mark_variable()
 * @see nip_unmark_variable() */
uncertain_series forward_backward_inference(time_series ts, 
					    nip_variable vars[], int nvars,
					    double* loglikelihood);


/**
 * Fetches you the variable with a given symbol / name. 
 * @param model The model where to look from
 * @param symbol Exact name to look for
 * @return Reference to one of the model variables, or NULL.
 */
nip_variable model_variable(nip model, char* symbol);


/**
 * Makes the join tree consistent i.e. does the inference on a single
 * timeslice. Useful after inserting evidence, which does not include
 * this.
 * @param model The inference engine
 */
void make_consistent(nip model);


/**
 * Computes the most likely state sequence of the variables of
 * interest, given the time series observations. In other words, this
 * function implements the idea also known as the Viterbi algorithm,
 * Max-Sum inference, or dynamic programming.
 * (The model is included in the time_series.)  
 * TODO: This is not implemented yet! */
time_series mlss(nip_variable vars[], int nvars, time_series ts);


/**
 * Trains the given model according to the given time series with EM
 * algorithm. Stops when the average improvement of log. likelihood of
 * the data is small enough.
 * The \p learning_curve can be NULL: if it isn't, it will be
 * emptied and filled with average log. likelihood values for each
 * iteration.
 *
 * NOTE: Call random_seed() before this! 
 *
 * NOTE: The model is included in the (first) time_series.
 *
 * NOTE: Only evidence for the marked variables is used. Unmarked are
 * ignored and you can thus easily omit evidence for an entire variable.
 *
 * @param ts The input data for training: an array of time series'
 * @param n_ts Number of time series' in \p ts
 * @param threshold Minimum required improvement in log. likelihood / slice
 * @param learning_curve Possible pointer to a (stub) list of
 * log. likelihood numbers, or null if not required
 * @return An error code in case of any errors
 */
nip_error_code em_learn(time_series* ts, int n_ts, double threshold, 
			nip_double_list learning_curve);


/**
 * Tells the likelihood of observations (not normalised). 
 * You must normalise the result with the mass computed before 
 * the evidence was put in. 
 * @param model The inference engine
 * @return Relative sum over all cliques of the join tree
 */
double model_prob_mass(nip model);


/**
 * Calculates the marginal probability distribution of a variable.
 * The join tree MUST be consistent before calling this.
 * @param model NIP model that contains the variable
 * @param v Random variable of interest
 * @return an array of doubles (remember to free the result when not needed).
 * The returned array is of size v->cardinality.
 * In case of problems, NULL is returned.
 * @see make_consistent()
 */
double* get_probability(nip model, nip_variable v);


/**
 * Calculates the joint probability distribution of a set of variables.
 * The join tree MUST be consistent before calling this.
 * @param model The model that contains the variables
 * @param vars  The variables whose distribution we want
 * @param num_of_vars The number of variables (size of "vars")
 * @return the result as a potential (remember to free the result afterwards).
 * The variables of the potential are in the same order as they were given.
 * In case of problems, NULL is returned.
 * @see make_consistent()
 */
nip_potential get_joint_probability(nip model, nip_variable *vars, 
				    int num_of_vars);


/**
 * Samples time series data according to a model. 
 *
 * NOTE: Call random_seed() before this!
 * (and take care it is not done more often than once per second) 
 *
 * @param model The generative model
 * @param length Number of samples (length)
 * @return Sampled time series */
time_series generate_data(nip model, int length);


/**
 * Sets the seed number for rand & co. 
 * If \p seedpointer is NULL, this shuffles the value from 
 * time of day (seconds).
 *
 * NOTE: Do not call this more often than once per second! 
 * (unless you want the same random seed as last time) 
 *
 * @param seedpointer Pointer to seed value, or NULL
 * @return the used seed number
 */
long random_seed(long* seedpointer);

/**
 * Utility function for generating single observations. 
 * @param distribution Array of probabilities in [0, 1]
 * @param size Number of states (length of \p distribution)
 * @return Random integer in [0, size-1]
 * @see random_seed() for initialization of pseudorandom generator.
 */
int lottery(double* distribution, int size);


/**
 * Prints the cliques in the given nip model to stdout.
 * TODO: Make it more like "fprintf_cliques".
 * @param model The probabilistic model
 */
void print_cliques(nip model);

#endif /* __NIP_H__ */
