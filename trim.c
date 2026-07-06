/* ---------------------------------------------------------------- */
/*                                                                  */
/* trim.c   funzione di trim con le funzioni rtrim (right trim)     */
/*          per eliminare spazi al termine della stringa e          */
/*          ltrim (left trim) per eliminare spazi all'inizio        */
/*          della stringa                                           */
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
#include <ctype.h>

extern char *rtrim(char *s);
extern char *ltrim(char *s);

// trim a string both sides
extern char *trim(char *s) {

    // elimina spazi finali
    s = rtrim(s);
    // elimina spazi iniziali
    s = ltrim(s);

    return s;
}
/* ---------------------------------------------------------------- */
/* rtrim    funzione di trim degli spazi finali della stringa       */
/*          data                                                    */
/* ---------------------------------------------------------------- */
// trim trailing spaces
extern char *rtrim(char *s) {
    char *ptr;
    int str_l;

    // se stringa nulla o vuota non fa nulla
    if (!s)
      return NULL;   // handle NULL string
    if (!*s)
      return s;      // handle empty string

    str_l = strlen(s) - 1;
    for ( (ptr = s + str_l); (ptr >= s) && (isspace((unsigned char)*ptr)); --ptr);

    ptr[1] = '\0';

    return s;
}
/* ---------------------------------------------------------------- */
/* ltrim    funzione di trim degli spazi iniziale della stringa     */
/*          fornita                                                 */
/* ---------------------------------------------------------------- */

// trim leading spaces
extern char *ltrim(char *s) {
    char *ptr;
    int str_l;

    // se stringa nulla o vuota non fa nulla
    if (!s)
      return NULL;   // handle NULL string
    if (!*s)
      return s;      // handle empty string

    str_l = strlen(s) - 1;

    for ( (ptr = s); (ptr <= s + str_l) && (isspace((unsigned char)*ptr)); ++ptr);
    s = ptr;
    return s;
}
