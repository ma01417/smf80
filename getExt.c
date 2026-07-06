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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _XOPEN_SOURCE_EXTENDED 1
#include <libgen.h>

/*  return the filename and extension */
extern char *getExt(const char *path_name, char *ext)
{
  char *p, *s, *file_name;
  file_name = malloc(100);
  if ( file_name==NULL) return NULL;     // no memory

  s = basename(path_name);               // ottiene nome completo
  p = strrchr(s, '.'); // cerca separtore ext.

  if(p == NULL) return NULL;             // char '.' not found

  strncpy(file_name, s,(int)((int) p - (int) s ));    // copia il nome
  strcpy(ext, ++p);                   // copia il suffisso

  file_name = realloc(file_name, strlen(file_name) + 1);

  return file_name;
}

