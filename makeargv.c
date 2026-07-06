/* ---------------------------------------------------------------- */
/*                                                                  */
/* makeargv.c  tokenizza una stringa con blank come separatore      */
/*             l'ultimo parametro indica quanti token estrarre      */
/*             i primi n-1 singoli, l'ultimo con il resto della     */
/*             stringa                                              */
/*                                                                  */
/* ---------------------------------------------------------------- */
/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* smf80ext headers */
#include "smf80ext.h"

// tokenize a string and return token in array of char
int makeargv(char *string, char *argv[], int argvsize, int max_argc)
{
 char *p = string;
 int  i;
 int argc = 0;
 for(i = 0; i < argvsize; i++) {
  while(isspace(*p))  p++;         // skip leading whitespace

  if(*p != '\0') argv[argc++] = p; // token nr. argc found
  else {
   argv[argc] = 0;                 // no more args
   break;                          // exit from loop
  }

  /* if last token requested exit   */
  if (argc == max_argc) break;

  /* scan over arg, skip whitespace */
  while(*p != '\0' && !isspace(*p)) p++;

  /* terminate arg: */
  if(*p != '\0' && i < argvsize-1)
   *p++ = '\0';
 }
 return argc;
}
