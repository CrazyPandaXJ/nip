#include <stdlib.h>
#include "parser.h"
#include "Clique.h"
#include "Variable.h"
#include "potential.h"
#include "errorhandler.h"

/*
#define PRINT_POTENTIALS
*/

/*
#define DEBUG_BISONTEST
*/

#define EVIDENCE

/*
#define EVIDENCE1
*/

/*#define EVIDENCE2*/

#define EVIDENCE_SOTKU

/*
#define PRINT_JOINTREE
*/

int yyparse();

/*
 * Calculate the probability distribution of Variable "var".
 * The family of var must be among the Cliques in cliques[]
 * (the size of which is num_of_cliques).
 * This function allocates memory for the array "result". The size of
 * the array is returned in "size_of_result".
 */
void test_probability(double **result, int *size_of_result,
		      Variable var, Clique cliques[], int num_of_cliques){

  /* Find the Clique that contains the family of the interesting Variable. */
  Clique clique_of_interest = find_family(cliques, num_of_cliques, 
					  &var, 1);
  if(!clique_of_interest){
    printf("In bisontest.c : No clique found! Sorry.\n");
    *size_of_result = 0;
    return;
  }  

  *size_of_result = number_of_values(var);

  /* Allocate memory for the result */
  *result = (double *) calloc(*size_of_result, sizeof(double));
  if(!result){
    printf("In bisontest.c : Calloc failed.\n");
    *size_of_result = 0;
    return;
  }

  /* marginalisation */
  marginalise(clique_of_interest, var, *result);

  /* normalisation */
  normalise(*result, *size_of_result);

}

/*
 * Enter some evidence of Variable "observed".
 */
void test_evidence(Variable observed, double data[], Clique cliques[],
		  int num_of_cliques){

  int i;
  Clique clique_of_interest =
    find_family(cliques, num_of_cliques, &observed, 1);

#ifdef DEBUG_BISONTEST
  int evidence_retval;
#endif

#ifndef DEBUG_BISONTEST
  enter_evidence(clique_of_interest, observed, data);
#endif

#ifdef DEBUG_BISONTEST
  evidence_retval = enter_evidence(clique_of_interest, observed, data);
  printf("\n\nEntered evidence into ");
  print_Clique(clique_of_interest);
  printf("enter_evidence returned %d.\n", evidence_retval);
#endif /* DEBUG_BISONTEST */

  /* propagation: some action */

  /* Procedural guide: UNMARK all clusters. */
  for(i = 0; i < num_of_cliques; i++)
    unmark_Clique(cliques[i]);
  collect_evidence(NULL, NULL, clique_of_interest);

  /* Procedural guide: UNMARK all clusters. */
  for(i = 0; i < num_of_cliques; i++)
    unmark_Clique(cliques[i]);
  distribute_evidence(clique_of_interest);

}

int main(int argc, char *argv[]){

  int i, retval;
  int nip_num_of_cliques;
  int size_of_result;

  double* result;

  Clique *nip_cliques;

  Variable interesting;

#ifdef PRINT_POTENTIALS
  int j;
#endif

#ifdef DEBUG_BISONTEST
  int temp;
#endif

#ifdef EVIDENCE

#ifdef EVIDENCE1
  double probB[] = {0.25, 0.25, 0.40, 0.10};
  double probD[] = {0.2, 0.3, 0.5};
#endif /* EVIDENCE1 */

#ifdef EVIDENCE2
  double probB[] = {0.75, 0.21, 0.03, 0.01};
  double probD[] = {0.6, 0.1, 0.3};
#endif /* EVIDENCE1 */

#ifdef EVIDENCE_SOTKU
  double probC1[] = { 0.395, 0.605 };
  double probC4[] = { 0.018, 0.982 };
  double probC19[] = { 0.492, 0.508 };
#endif /* EVIDENCE_SOTKU */

  Variable observed[3];
  double *probs[3];
#endif /* EVIDENCE */

#ifdef EVIDENCE

#ifdef EVIDENCE_SOTKU
  probs[0] = probC1;
  probs[1] = probC4;
  probs[2] = probC19;
#endif

#ifndef EVIDENCE_SOTKU
  probs[0] = probB;
  probs[1] = probD;
#endif

#endif /* EVIDENCE */

  /* -- Start parsing the network definition file */
  if(argc < 2){
    if(open_infile("infile") != NO_ERROR)
      return -1;
  }
  else if(open_infile(argv[1]) != NO_ERROR)
    return -1;

  retval = yyparse();

  close_infile();

  if(retval != 0)
    return retval;
  /* The input file has been parsed. -- */

  nip_cliques = get_nip_cliques();
  nip_num_of_cliques = get_nip_num_of_cliques();

#ifdef PRINT_JOINTREE
  jtree_dfs(nip_cliques[0], print_Clique, print_Sepset);
#endif


  /* *********************************************************** */
#ifdef PRINT_POTENTIALS
  for(i = 0; i < nip_num_of_cliques; i++){
    printf("In bisontest.c : Clique of ");
    for(j = 0; j < clique_num_of_vars(nip_cliques[i]) - 1; j++)
      printf("%s ", get_symbol(clique_get_Variable(nip_cliques[i], j)));
    printf("and %s.\n", get_symbol(clique_get_Variable(nip_cliques[i], j)));
    
    print_potential(nip_cliques[i]->p);
  }
#endif /* PRINT_POTENTIALS */
  /* *********************************************************** */




  /* *********************************************************** */
#ifdef EVIDENCE
  /* add some evidence */
#ifndef EVIDENCE_SOTKU
  observed[0] = get_variable("B");
  observed[1] = get_variable("D");
#endif

#ifdef EVIDENCE_SOTKU
  observed[0] = get_variable("C1");
  observed[1] = get_variable("C4");
  observed[2] = get_variable("C19");
#endif



  test_evidence(observed[0], probs[0], nip_cliques, nip_num_of_cliques);

  test_evidence(observed[1], probs[1], nip_cliques, nip_num_of_cliques);
  
#ifdef EVIDENCE_SOTKU

  test_evidence(observed[2], probs[2], nip_cliques, nip_num_of_cliques);

#endif /* EVIDENCE_SOTKU */

#endif /* EVIDENCE */
  /* *********************************************************** */




  printf("\n\n");

#ifndef EVIDENCE
  /* another propagation */
  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  collect_evidence(NULL, NULL, nip_cliques[0]);

  for(i = 0; i < nip_num_of_cliques; i++)
    unmark_Clique(nip_cliques[i]);
  distribute_evidence(nip_cliques[0]);
#endif /* EVIDENCE */

  /* marginalisation */
  if(argc > 2)
    interesting = get_variable(argv[2]); /* THE variable */
  else
    interesting = get_variable("B"); /* THE variable */

  if(!interesting){
    printf("In bisontest.c : Variable of interest not found.\n");
    return 1;
  }

  test_probability(&result, &size_of_result, interesting, nip_cliques,
		   nip_num_of_cliques);

  printf("Normalised probability of %s:\n", get_symbol(interesting));
  for(i = 0; i < size_of_result; i++)
    printf("result[%d] = %f\n", i, result[i]);
  /* To be continued... */

  free(result);
  return 0;
}
