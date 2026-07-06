
/* -------------------------------------------------------------------------------- */
/*                                                                                  */
/*  Funzione per l'apertura di un file di cui sono passati nome,                    */
/*  parametri di accesso, descrizione del file da aprire                            */
/*                                                                                  */
/*  Ritorna il file descriptor                                                      */
/* -------------------------------------------------------------------------------- */
/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FILE *openf(const char *filename, const char *open_parm, const char *op_desc, const char *pgmname){
/* apertura file con i record SMF da trattare     */
 FILE *file_handler;
 char myname[] = "openf";
 printf("\n% 8s Apertura file %s  %s\n", myname,op_desc, filename);
 file_handler = fopen(filename, open_parm);
 if( file_handler == NULL )
   fprintf(stderr, "% 8s E: impossibile aprire il file %s\n", pgmname, filename);
 return file_handler;
}

