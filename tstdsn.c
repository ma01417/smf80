/* ========================================================================== */
/*                                                                            */
/*   tstdsn.c                                                                 */
/*   (c) 2026 A.Brezzi                                                        */
/*                                                                            */
/*   Description: programma di prova per get_DSName() (getdsn.c)              */
/*                                                                            */
/*   Uso (JCL): PARM='/DD1 DD2 ...'                                           */
/*     per ogni DD indicata stampa la lista dei data set allocati;            */
/*     in coda stampa tutte le allocazioni del job step con i valori          */
/*     grezzi restituiti da SVC 99, per verificare le decodifiche.            */
/*     Senza parametri stampa solo l'elenco completo delle allocazioni.       */
/*                                                                            */
/* ========================================================================== */

/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

#include <stdio.h>
#include <stdlib.h>

#include "getdsn.h"

int main(int argc, char *argv[])
{
  st_dd_info *list;
  int i;

  for (i = 1; i < argc; i++) {
    printf("\n=== DD %s ===\n", argv[i]);
    list = get_DSName(argv[i]);
    if (list == NULL) {
      printf("DD non allocata\n");
      continue;
    }
    print_DSName(stdout, list);
    if (is_dd_dummy(list))
      printf("-> DD DUMMY: l'elaborazione collegata verrebbe saltata\n");
    free_DSName(list);
  }

  printf("\n=== Tutte le allocazioni del job step (valori grezzi) ===\n");
  dump_allocations(stdout);
  return 0;
}
