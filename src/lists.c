/*
 * Functions for using list structures
 * (a C++ implementation would use STL)
 *
 * $Id: lists.c,v 1.6 2007-01-05 16:58:42 jatoivol Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lists.h"
#include "errorhandler.h"


/* 
 * make_<T>list operations 
 */

doublelist make_doublelist(){
  doublelist dl = (doublelist) malloc(sizeof(doubleliststruct));
  /* Q: What if NULL was returned? */
  dl->length = 0;
  dl->first  = NULL;
  dl->last   = NULL;
  return dl;
}

stringlist make_stringlist(){
  stringlist sl = (stringlist) malloc(sizeof(stringliststruct));
  /* Q: What if NULL was returned? */
  sl->length = 0;
  sl->first  = NULL;
  sl->last   = NULL;
  return sl;
}

variablelist make_variablelist(){
  variablelist vl = (variablelist) malloc(sizeof(variableliststruct));
  /* Q: What if NULL was returned? */
  vl->length = 0;
  vl->first  = NULL;
  vl->last   = NULL;
  return vl;
}

potentialList make_potentialList(){
  potentialList pl = (potentialList) malloc(sizeof(potentialListStruct));
  /* Q: What if NULL was returned? */
  pl->length = 0;
  pl->first  = NULL;
  pl->last   = NULL;
  return pl;
}

interfaceList make_interfaceList(){
  interfaceList l = (interfaceList) malloc(sizeof(interfaceListStruct));
  /* Q: What if NULL was returned? */
  l->length = 0;
  l->first  = NULL;
  l->last   = NULL;
  return l;
}


/*
 * append_<T> operations
 */

int append_double(doublelist l, double d){
  doublelink new = (doublelink) malloc(sizeof(doublelinkstruct));

  if(!l){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = d;
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;

  l->last = new;
  l->length++;
  return NO_ERROR;
}

int append_string(stringlist l, char* s){
  stringlink new = (stringlink) malloc(sizeof(stringlinkstruct));

  if(!l || !s){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = s;
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;

  l->last = new;
  l->length++;
  return NO_ERROR;
}

int append_variable(variablelist l, variable v){
  variablelink new = (variablelink) malloc(sizeof(variablelinkstruct));

  if(!l || !v){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = v;
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;

  l->last = new;
  l->length++;
  return NO_ERROR;
}

int append_potential(potentialList l, potential p, 
		     variable child, variable* parents){
  potentialLink new = (potentialLink) malloc(sizeof(potentialLinkStruct));

  if(!l || !p){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = p;
  new->child = child;
  /* The "ownership" of the parents[] array changes! */
  new->parents = parents;
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;

  l->last = new;
  l->length++;
  return NO_ERROR;
}

int append_interface(interfaceList l, variable var, char* next){
  interfaceLink new = (interfaceLink) malloc(sizeof(interfaceLinkStruct));

  if(!l || !var){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->var = var;
  new->next = next;
  /* The ownership of the "next" string changes! */
  new->fwd = NULL;
  new->bwd = l->last;
  if(l->first == NULL)
    l->first = new;
  else
    l->last->fwd = new;

  l->last = new;
  l->length++;
  return NO_ERROR;
}


/*
 * prepend_<T> operations
 */

int prepend_double(doublelist l, double d){
  doublelink new = (doublelink) malloc(sizeof(doublelinkstruct));

  if(!l){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = d;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;

  l->first = new;
  l->length++;
  return NO_ERROR;
}

int prepend_string(stringlist l, char* s){
  stringlink new = (stringlink) malloc(sizeof(stringlinkstruct));

  if(!l || !s){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = s;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;

  l->first = new;
  l->length++;
  return NO_ERROR;
}

int prepend_variable(variablelist l, variable v){
  variablelink new = (variablelink) malloc(sizeof(variablelinkstruct));

  if(!l || !v){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = v;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;

  l->first = new;
  l->length++;
  return NO_ERROR;
}

int prepend_potential(potentialList l, potential p, 
		      variable child, variable* parents){
  potentialLink new = (potentialLink) malloc(sizeof(potentialLinkStruct));

  if(!l || !p){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->data = p;
  new->child = child;
  new->parents = parents;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;

  l->first = new;
  l->length++;
  return NO_ERROR;
}

int prepend_interface(interfaceList l, variable var, char* next){
  interfaceLink new = (interfaceLink) malloc(sizeof(interfaceLinkStruct));

  if(!l || !var){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return ERROR_INVALID_ARGUMENT;
  }

  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return ERROR_OUTOFMEMORY;
  }

  new->var = var;
  new->next = next;
  new->bwd = NULL;
  new->fwd = l->first;
  if(l->last == NULL)
    l->last = new;
  else
    l->first->bwd = new;

  l->first = new;
  l->length++;
  return NO_ERROR;
}


/*
 * list_to_<T>_array conversions
 */

double* list_to_double_array(doublelist l){
  int i;
  doublelink ln;
  double* new;
  
  if(!l){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  if(l->length == 0)
    return NULL;

  new = (double*) calloc(l->length, sizeof(double));
  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  ln = l->first;
  for(i = 0; i < l->length; i++){
    new[i] = ln->data; /* the data is copied here */
    ln = ln->fwd;
  }
  return new;
}

char** list_to_string_array(stringlist l){
  int i;
  stringlink ln;
  char** new;
  
  if(!l){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  if(l->length == 0)
    return NULL;

  new = (char**) calloc(l->length, sizeof(char*));
  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  ln = l->first;
  for(i = 0; i < l->length; i++){
    new[i] = ln->data; /* the pointer is copied here */
    ln = ln->fwd;
  }
  return new;
}

variable* list_to_variable_array(variablelist l){
  int i;
  variablelink ln;
  variable* new;
  
  if(!l){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  if(l->length == 0)
    return NULL;

  new = (variable*) calloc(l->length, sizeof(variable));
  if(!new){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  ln = l->first;
  for(i = 0; i < l->length; i++){
    new[i] = ln->data; /* the pointer is copied here */
    ln = ln->fwd;
  }
  return new;
}


/*
 * empty_<T>list operations... don't free the dynamically allocated contents
 */

void empty_doublelist(doublelist l){
  doublelink ln;

  if(!l)
    return;
  
  ln = l->last;

  l->last = NULL;
  while(ln != NULL){
    free(ln->fwd); /* free(NULL) is O.K. at the beginning */
    ln = ln->bwd;
  }
  free(l->first);
  l->first = NULL;
  l->length = 0;
  return;
}

void empty_stringlist(stringlist l){
  stringlink ln;

  if(!l)
    return;
  
  ln = l->last;

  l->last = NULL;
  while(ln != NULL){
    free(ln->fwd); /* free(NULL) is O.K. at the beginning */
    ln = ln->bwd;
  }
  free(l->first);
  l->first = NULL;
  l->length = 0;
  return;
}

void empty_variablelist(variablelist l){
  variablelink ln;

  if(!l)
    return;
  
  ln = l->last;

  l->last = NULL;
  while(ln != NULL){
    free(ln->fwd); /* free(NULL) is O.K. at the beginning */
    ln = ln->bwd;
  }
  free(l->first);
  l->first = NULL;
  l->length = 0;
  return;
}


void free_stringlist(stringlist l){
  stringlink ln;

  if(!l)
    return;
  
  ln = l->last;

  l->last = NULL;
  while(ln != NULL){
    if(ln->fwd != NULL){
      free(ln->fwd->data);
      free(ln->fwd);
    }
    ln = ln->bwd;
  }
  if(l->first != NULL){
    free(l->first->data);
    free(l->first);
    l->first = NULL;
  }
  l->length = 0;

  free(l);
  l = NULL;
  return;
}


void free_potentialList(potentialList l){
  potentialLink ln;

  if(!l)
    return;
  
  ln = l->last;

  l->last = NULL;
  while(ln != NULL){
    if(ln->fwd != NULL){
      free_potential(ln->fwd->data);
      free(ln->fwd->parents);
      free(ln->fwd);
    }
    ln = ln->bwd;
  }
  if(l->first != NULL){
    free_potential(l->first->data);
    free(l->first->parents);
    free(l->first);
    l->first = NULL;
  }
  l->length = 0;

  free(l);
  l = NULL;
  return;
}


void free_interfaceList(interfaceList l){
  interfaceLink ln;

  if(!l)
    return;
  
  ln = l->last;

  l->last = NULL;
  while(ln != NULL){
    if(ln->fwd != NULL){
      free(ln->fwd->next);
      free(ln->fwd);
    }
    ln = ln->bwd;
  }
  if(l->first != NULL){
    free(l->first->next);
    free(l->first);
    l->first = NULL;
  }
  l->length = 0;

  free(l);
  l = NULL;
  return;
}


/* Checks if the given string is in the list */
int stringlist_contains(stringlist l, char* string){
  stringlink s = NULL;

  if(!l || !string)
    return 0;

  s = l->first;
  while(s != NULL){
    if(strcmp(string, s->data) == 0){
      return 1;
    }
    s = s->fwd;
  }
  return 0;
}


/*
 * Iterator
 */
variable next_variable(variable_iterator* it){
  variable v;
  if(*it){
    v = (*it)->data;
    *it = (*it)->fwd;
  }
  else
    v = NULL;
  return v;
}


/*
 * Search method
 */
variable get_parser_variable(variablelist l, char *symbol){
  variable v; 
  variable_iterator it = l->first; /* a private copy */
  v = next_variable(&it);

  if(v == NULL)
    return NULL; /* didn't find the variable (possibly normal) */
  
  /* search for the variable reference */
  while(strcmp(symbol, get_symbol(v)) != 0){
    v = next_variable(&it);
    if(v == NULL){
      return NULL; /* didn't find the variable (a normal situation) */
    }
  }
  return v;
}

