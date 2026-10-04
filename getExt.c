/* ---------------------------------------------------------------- */
/*                                                                  */
/* getHex.c restituisce stringa con nome del file estratto dal      */
/*          path e l'extent del file                                */
/*                                                                  */
/* ---------------------------------------------------------------- */
/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

/* deve precedere ogni include di sistema per esporre basename() */
#define _XOPEN_SOURCE_EXTENDED 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

/*  return the filename and extension */
extern char *getExt(const char *path_name, char *ext)
{
  char *p, *s, *file_name, *path_copy;
  size_t len;

  // basename() puo' modificare la stringa ricevuta: lavora su una copia
  path_copy = strdup(path_name);
  if ( path_copy == NULL ) return NULL;  // no memory

  s = basename(path_copy);               // ottiene nome completo
  p = strrchr(s, '.'); // cerca separtore ext.

  if(p == NULL) {                        // char '.' not found
    free(path_copy);
    return NULL;
  }

  len = (size_t)(p - s);                 // lunghezza del nome senza ext.
  file_name = malloc(len + 1);
  if ( file_name == NULL ) {             // no memory
    free(path_copy);
    return NULL;
  }
  memcpy(file_name, s, len);             // copia il nome
  file_name[len] = '\0';
  strcpy(ext, p + 1);                    // copia il suffisso

  free(path_copy);
  return file_name;
}

