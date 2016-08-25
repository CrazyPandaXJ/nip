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
 * @copyright &copy; 2012  Janne Toivola <br>
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
#include "niplists.h"
#include "nipvariable.h"
#include "nippotential.h"
#include "nipjointree.h"
#include "niperrorhandler.h"
#include "nipparsers.h"
#include "huginnet.tab.h"

/* Hidden: nipgraph.h, nipheap.h, nipstring.h */

/* TODO: consider separating these from NIP core, let users have their own */
#define TIME_SERIES_LENGTH(ts) ( (ts)->length )
#define UNCERTAIN_SERIES_LENGTH(ucs) ( (ucs)->length )

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
 * The data structure containing all necessary stuff for running 
 * probabilistic inference with a model, except data itself
 */
typedef struct{
  int num_of_cliques;  ///< number of cliques/potentials in the join tree
  nip_clique *cliques; ///< the actual cliques/potentials
  int num_of_vars;         ///< number of random variables in the model
  nip_variable *variables; ///< the actual variables (names of values etc.)
  
  nip_variable *next;     /**< The variables that will substitute 
			     another one in the next timeslice. */
  nip_variable *previous; /**< The variables that are substituted 
			     by some variables from the previous timeslice. 
			     (Waste of memory?) */
  int num_of_nexts; ///< Number of variables in 'next' and 'previous'

  int outgoing_interface_size; ///< number of variables in outgoing interface
  nip_variable* outgoing_interface;          ///< I_{t}->
  nip_variable* previous_outgoing_interface; ///< I_{t-1}->
  int incoming_interface_size; ///< number of variables in incoming interface
  nip_variable* incoming_interface;          ///< I_{t}<-

  nip_clique in_clique;  /**< The clique which receives the message
			    from the past timeslices */

  nip_clique out_clique; /**< The clique which handles the connection to the 
			    future timeslices */

  nip_variable *children;    ///< all the variables that have parents
  nip_variable *independent; ///< ...and those who dont. (Redundant?)
  int num_of_children;       ///< number of children < num_of_vars

  int node_size_x; ///< node width, for drawing the graph
  int node_size_y; ///< node height, for drawing the graph
}nip_struct;

typedef nip_struct* nip; /// hide pointer notation


/**
 * Structure for storing a batch of "crisp" observations
 */
typedef struct{
  nip model; ///< The model (contains the variables and state names)
  nip_variable *hidden;   ///< The latent variables
  int num_of_hidden;      ///< Number of latent variables
  nip_variable *observed; ///< The observed variables
  int num_of_observed;    ///< == model->num_of_vars - num_of_hidden

  int length; ///< Number of time steps
  int **data; ///< The time series data
  /* TODO: Should there be a cache for extremely large time series? */
}time_series_struct;

typedef time_series_struct* time_series;

/* TODO: Should there be something else for streams / online inference? */

/**
 * Structure for storing "soft" uncertain observations or inference results
 */
typedef struct{
  nip_variable* variables; ///< variables of interest
  int num_of_vars;         ///< number of variables
  int length;              ///< length of the time series or sequence
  double*** data; ///< probability distribution of every variable at every t
}uncertain_series_struct;

typedef uncertain_series_struct* uncertain_series;


/**
 * Makes the model forget all the given evidence.
 * NOTE: also the priors specified in the NET file are cleared
 * (but remain intact in the variables) so you'll have to re-enter them
 * as soft evidence. (FIXME: priors should not be treated as evidence!) */
void reset_model(nip model);


/**
 * Makes all the conditional probabilities uniform and forgets all evidence.
 * In other words, the model will be as if it was never initialised with 
 * any parameters at all. 
 * (All the variables and the join tree will be there, of course) */
void total_reset(nip model);


/**
 * Enters the priors of independent variables into the model as evidence.
 * (FIXME: priors should not be treated as evidence!)
 * This has to be done after reset_model and parse_model if you have any
 * other kind of priors than uniform ones. If you need to suppress the 
 * initialisation of the variables representing another one from the 
 * previous timeslice, use 'has_history == 0'. 
 * (Usually has_history == 1 only for the first timeslice)
 */
void use_priors(nip model, int has_history);


/**
 * Creates a model according to a net file. 
 * Remember to free the model when done with it.
 * @param file the name of the net file as a string 
 */
nip parse_model(char* file);


/**
 * Writes the parameters of <model> into Hugin NET file named <filename> 
 * @param model the model to write
 * @param filename the name of output file
 */
int write_model(nip model, char* filename);


/**
 * Gets rid of a model and frees some memory.
 */
void free_model(nip model);


/**
 * Reads data from the data file and constructs a set of time series 
 * according to the given model. Remember to free results afterwards.
 * @param model The random variables and all
 * @param datafile Name of the input file as a string
 * @results pointer Where the time_series is set
 */
int read_timeseries(nip model, char* datafile, 
		    time_series **results);


/**
 * Writes a set of time series data into a file. 
 * @param ts_set Array of time_series'
 * @param n_series Number of time_series'
 * @param filename Name of the output file
 */
int write_timeseries(time_series *ts_set, int n_series, char* filename);


/**
 * A method for freeing the huge chunk of memory used by a time series. 
 * Note that this does not free the model. 
 */
void free_timeseries(time_series ts);


/* Tells the length of the time series. */
int timeseries_length(time_series ts);


/* Writes the inferred probabilities of given variable into a file. */
int write_uncertainseries(uncertain_series *ucs_set, int n_series, 
			  nip_variable v, char* filename);


/* A method for freeing the memory used by an uncertain time series. */
void free_uncertainseries(uncertain_series ucs);


/* Tells the length of the uncertain time series. */
int uncertainseries_length(uncertain_series ucs);


/* A method for reading an observation from the time series. 
 * You'll have to specify the variable. DO NOT alter the string returned 
 * by the function! The returned value may be NULL if the variable was not 
 * observed at the specified moment in time. The time span is [0, T-1] */
char* get_observation(time_series ts, nip_variable v, int time);


/* A method for setting an observation in the time series. */
int set_observation(time_series ts, nip_variable v, int time, 
		    char* observation);


/* This is a function for telling the model about observations. 
 * In case of an error, a non-zero value is returned. */
int insert_hard_evidence(nip model, char* varname, char* observation);


/* If an observation has some uncertainty, the evidence can be inserted 
 * with this procedure. The returned value is 0 if everything went well. */
int insert_soft_evidence(nip model, char* varname, double* distribution);


/* Method for inserting part of the evidence at a specified step <t> in a
 * time series <ts> into the (time slice) model <ts->model>. 
 * Only the variables marked with <mark_mask> will be considered. 
 * mark_mask == MARK_BOTH  => all suitable evidence of the time step is used
 * mark_mask == MARK_OFF   => only unmarked variables are used
 * mark_mask == MARK_ON    => only marked variables are used */
int insert_ts_step(time_series ts, int t, nip model, char mark_mask);


/* Method for inserting part of the evidence at a specified step <t> in an
 * uncertain time series <ucs> into the model <ucs->model>.  Only the
 * variables marked with <mark_mask> will be considered. */
int insert_ucs_step(uncertain_series ucs, int t, nip model, char mark_mask);


/* This algorithm computes the marginal posterior probability distributions
 * for every hidden variable and for every time step according to the
 * timeseries.  It uses only forward propagation, so the result of time
 * step t is not affected by the rest of the timeseries.  You'll have to
 * specify the variables of interest in the vars array and the number of
 * the variables.  Returns also the average log. likelihood if
 * <loglikelihood> is not NULL.
 *
 * NOTE: only evidence for the marked variables is used. Unmarked are
 * ignored and you can thus easily omit evidence for an entire variable.
 */
uncertain_series forward_inference(time_series ts, nip_variable vars[], 
				   int nvars, double* loglikelihood);


/* This one computes the probability distributions for every 
 * hidden variable and for every time step according to the timeseries.
 * It uses both forward and backward propagation, so the result of time 
 * step t is affected by the whole timeseries. 
 *  You'll have to specify the variables of interest in the vars array 
 * and the number of the variables. 
 * Returns also the average log. likelihood if <loglikelihood> is not NULL
 *
 * NOTE: only evidence for the marked variables is used. Unmarked are
 * ignored and you can thus easily omit evidence for an entire variable.
 */
uncertain_series forward_backward_inference(time_series ts, 
					    nip_variable vars[], int nvars,
					    double* loglikelihood);


/* Fetches you the variable with a given symbol. */
nip_variable model_variable(nip model, char* symbol);


/* Makes the join tree consistent i.e. does the inference on a single 
 * timeslice (useful after inserting evidence). */
void make_consistent(nip model);


/* Computes the most likely state sequence of the variables, given the time
 * series. In other words, this function implements the idea also known as
 * the Viterbi algorithm. (The model is included in the time_series.)  
 * NOTE: this is not implemented yet! */
time_series mlss(nip_variable vars[], int nvars, time_series ts);


/* Teaches the given model according to the given time series with
 * EM-algorithm. Returns an error code.  
 * NOTE: call random_seed() before this!  
 * NOTE2: the model is included in the time_series 
 * <learning_curve> can be NULL: if it isn't, it will be
 * emptied and filled with average log. likelihood values for each
 * iteration... 
 *
 * NOTE: only evidence for the marked variables is used. Unmarked are
 * ignored and you can thus easily omit evidence for an entire variable.
 */
nip_error_code em_learn(time_series* ts, int n_ts, double threshold, 
			nip_double_list learning_curve);


/* Tells the likelihood of observations (not normalised). 
 * You must normalise the result with the mass computed before 
 * the evidence was put in. */
double model_prob_mass(nip model);


/*
 * Calculates the probability distribution of a variable.
 * The join tree MUST be consistent before calling this.
 * Parameters:
 * - model: the model that contains the variable
 * - v: the variable whose distribution we want
 * Returns an array of doubles (remember to free the result when not needed).
 * The returned array is of size v->cardinality.
 * In case of problems, NULL is returned.
 */
double* get_probability(nip model, nip_variable v);


/*
 * Calculates the joint probability distribution of a set of variables.
 * The join tree MUST be consistent before calling this.
 * Parameters:
 * - model: the model that contains the variables
 * - vars: the variables whose distribution we want
 * - num_of_vars: the number of variables (size of "vars")
 * Returns the result as a potential 
 * (remember to free the result when not needed).
 * The variables of the potential are in the same order as they were given.
 * In case of problems, NULL is returned.
 */
nip_potential get_joint_probability(nip model, nip_variable *vars, 
				    int num_of_vars);


/* Generates time series data according to a model. 
 * NOTE: call random_seed() before this! 
 * (and take care it is not done more often than once per second) */
time_series generate_data(nip model, int length);


/* Sets the seed number for rand & co. 
 * If seedpointer is NULL, this shuffles the value from time of day.
 * Returns the value of seed number.
 *
 * NOTE: don't call this more often than once per second! 
 * (unless you want the same random seed as last time) */
long random_seed(long* seedpointer);

/* For generating single observations. */
int lottery(double* distribution, int size);


/*
 * Prints the cliques in the given nip model.
 */
void print_cliques(nip model);

#endif /* __NIP_H__ */
