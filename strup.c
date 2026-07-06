/* ---------------------------------------------------------------- */
/*                                                                  */
/* strup.c   funzione per convertire in upper-case                  */
/*                                                                  */
/* ---------------------------------------------------------------- */
/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

#include <stdlib.h>
#include <string.h>

// translate to upper case a string
void strup(char *s, char *u) {
  int i=-1;
  do {
    i++;
    u[i] = toupper(s[i]);
  } while ( s[i]!='\0' );
  return;
}
