/* Yhten�inen virhetilanteiden hallintaj�rjestelm�! */
/* Pit�isi l�yt�� t�lle jotain k�ytt��... ja kehitt�� systeemi�. */
/*http://www.opengroup.org/onlinepubs/007904975/functions/xsh_chap02_03.html*/
/* #include <errno.h> ??? */
#include <stdio.h>
#include "errorhandler.h"

/* A global variable for counting errors */
int ERROR_COUNTER = 0;

/* Errorcode of the last error */
int ERRORCODE = NO_ERROR; 

void report_error(int errorcode, int verbose){
  ERRORCODE = errorcode;
  ERROR_COUNTER++;
}

void reset_errorhandler(){
  ERRORCODE = NO_ERROR;
  ERROR_COUNTER = 0;
}

int check_errortype(){
  return ERRORCODE;
}

int check_errorcounter(){
  return ERROR_COUNTER;
}
