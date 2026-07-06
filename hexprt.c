/* -------------------------------------------------------------------------- */
/*                                                                            */
/* hexprt.c                                                                   */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Description: Stampa in esadecimale, formato DUMP una area di memoria     */
/*                passata come argomento, per la lunghezza richiesta          */
/*                                                                            */
/* -------------------------------------------------------------------------- */

/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// macros to give the maximum and minimum between two integer
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define MIN(x,y) (((x) < (y)) ? (x) : (y))

// max line length
#define LR 80

/* ========================================================= */

extern void hexprt( FILE *fdmp, char *head, char *src, int ll, char *pgm_name ) {
 char *pc = src;        // pointer to char
 char src_line[LR+1];
 char line_h1[LR+1], line_h2[LR+1], line_t0[LR+1];
 char hex_v[3];
 int  b2prt;
 unsigned int base = 0;
// inizia a spacchettare la riga
 fprintf(fdmp, "% 8s ---------- %s ---------- \n", pgm_name, head);
 while ( ll>0 ) {       // loop fino che non ha finito
   int  fl_end=0;       // segnala la fine della stringa in input
   b2prt = MIN(ll, LR);
   memcpy(src_line, pc, b2prt);
   pc += LR; ll-=LR;
   for (int i=0; i<b2prt; i++) {
     if (isprint(src_line[i]) && src_line[i]!='\0') line_t0[i] = src_line[i];
     else line_t0[i] = '.';
     snprintf(hex_v, 3, "%02X", src_line[i]);
     line_h1[i]=hex_v[0]; line_h2[i]=hex_v[1];
     line_t0[i+1]='\0'; line_h1[i+1]='\0'; line_h2[i+1]='\0';
   }
   fprintf(fdmp, "%5d Head %s\n", base, line_t0);
   fprintf(fdmp, "           0....+....1....+....2....+....3....+....4....+....5....+....6....+....7....+....\n", base);
   fprintf(fdmp, "         h %s\n", line_h1);
   fprintf(fdmp, "         l %s\n", line_h2);
   base += LR;
 }
return;
}
