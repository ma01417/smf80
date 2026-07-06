/* -------------------------------------------------------------------------- */
/*                                                                            */
/* CreateStr.c                                                                */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Description: constructor per struttura linked di stringhe a lunghezza    */
/*                variabile                                                   */
/*                                                                            */
/* -------------------------------------------------------------------------- */

/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

/* ---------------------------------------------------------------- */
/* struttura per costruire linked list di stringhe                  */
/* ---------------------------------------------------------------- */
struct string {
  size_t size;
  char *data;
};


// constructor per string
extern struct string *createStr(const char *initial) {
  assert (initial != NULL);
  struct string *new_string = malloc(sizeof(*new_string));
  if (new_string != NULL) {
    new_string->size = strlen(initial);
    new_string->data = strdup(initial);
    }
    return new_string;
}

