/*
 * fileio.c $Id: fileio.c,v 1.15 2004-06-24 10:55:14 mvkorpel Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileio.h"
#include "errorhandler.h"

/* #define DEBUG_IO */

int count_words(const char *s, int *chars){

  return count_tokens(s, chars, 0, NULL, 0, 0);

}

int count_tokens(const char *s, int *chars, int quoted_strings,
		 char *separators, int numof_separators, int sep_tokens){
  int tokens = 0, state = 0, i;
  int separator_found = 0;
  char ch;
  if(chars)
    *chars = 0;

  /*
   * States:
   *   0: waiting for start of token
   *   1: processing token (not quoted string)
   *   2: processing quoted string
   */
  while (*s != '\0'){

    if(separators)
      for(i = 0; i < numof_separators; i++)
	if(*s == separators[i]){
	  separator_found = 1;
	  break;
	}

    if(state != 2 && separator_found){
      if(sep_tokens)
	tokens++;
      state = 0;
      separator_found = 0;
    }
    else if(quoted_strings && state != 2 && (*s == '"')){

      /* Check if we have a matching '"' */
      i = 1;
      while((ch = s[i++]) != '\0')
	if(ch == '"'){

	  /* Quoted string starts from here. */
	  state = 2;
	  tokens++;
	  break;
	}
    }
    else if(state == 0){
      if((*s != ' ') && (*s != '\t') && (*s != '\n')){
	tokens++;	  
	state = 1;
      }
    }
    else if(state == 1 &&
	    ((*s == ' ') || (*s == '\t') || (*s == '\n')))
      state = 0;
    else if(quoted_strings && state == 2 && (*s == '"'))
      state = 0;

    s++;
    if(chars)
      (*chars)++;
  }
  return tokens;
}


int *tokenise(const char s[], int n, int quoted_strings,
	      char *separators, int numof_separators,
	      int sep_tokens){
  int *indices;
  int i = 0, j = 0, state = 0, arraysize = 2*n, k;
  int separator_found = 0;
  char ch, ch2;

  if(s == NULL){
    report_error(__FILE__, __LINE__, ERROR_NULLPOINTER, 0);
    return NULL;
  }
  if(n < 0){
    report_error(__FILE__, __LINE__, ERROR_INVALID_ARGUMENT, 1);
    return NULL;
  }

  if(n == 0)
    return NULL;

  indices = (int *) calloc(arraysize, sizeof(int));

  /* Couldn't allocate memory */
  if(!indices){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  while ((ch = s[i]) != '\0'){

    if(separators)
      for(k = 0; k < numof_separators; k++)
	if(ch == separators[k]){
	  separator_found = 1;
	  break;
	}

    if(state != 2 && separator_found){
      if(state == 1)
	indices[j++] = i;

      state = 0;

      if(sep_tokens){
	indices[j++] = i;
	indices[j++] = i + 1;
      }

      separator_found = 0;
    }
    else if(quoted_strings && state != 2 && (ch == '"')){

      /* Check if we have a matching '"' */
      k = i + 1;
      while((ch2 = s[k++]) != '\0')
	if(ch2 == '"'){

	  /* If we were not done processing the previous token,
	   * mark it as done.
	   */
	  if(state == 1)
	    indices[j++] = i;

	  /* Quoted string starts from here. */
	  state = 2;
	  indices[j++] = i;
	  break;
	}
    }
    else if(state == 0){
      if((ch != ' ') && (ch != '\t') && (ch != '\n')){
	indices[j++] = i;
	state = 1;
      }
    }
    else if(state == 1 &&
	    ((ch == ' ') || (ch == '\t') || (ch == '\n'))){
      indices[j++] = i;
      state = 0;
    }
    else if(quoted_strings && state == 2 && (ch == '"')){
      indices[j++] = i + 1;
      state = 0;
    }

    /* Have we found enough words? If so, break out. */
    if(j == arraysize)
      break;
    i++;
#ifdef DEBUG_IO
    printf("i = %d, state = %d\n", i, state);
#endif
  }

  /* Mark the end of a word that extends to the end of the string */
  if(j == arraysize - 1)
    indices[j] = i;

  /* Not enough words */
  else if(j < arraysize - 1){
#ifdef DEBUG_IO
    printf("tokenise failed to find the specified amount of tokens\n", i);
    printf("j = %d, arraysize = %d\n", j, arraysize);
    printf("indices =\n");
    for(i = 0; i < j; i++)
      printf("%d ", indices[i]);
    printf("\n");
#endif
    free(indices);
    return NULL;
  }
  
  return indices;
}

char **split(const char s[], int indices[], int n){
  int i, wordlength, begin, end;
  char **words = (char **) calloc(n, sizeof(char *));

  /* Couldn't allocate memory */
  if(!words){
    report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
    return NULL;
  }

  for(i = 0; i < n; i++){
    begin = indices[2*i];
    end = indices[2*i+1];
    wordlength = end - begin;
    words[i] = (char *) calloc(wordlength + 1, sizeof(char));

    /* Couldn't allocate memory */
    if(!words[i]){
      report_error(__FILE__, __LINE__, ERROR_OUTOFMEMORY, 1);
      return NULL;
    }
    strncpy(words[i], &s[begin], wordlength);
    words[i][wordlength] = '\0';
  }

  return words;
}
