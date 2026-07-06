/* -------------------------------------------------------------------------- */
/*                                                                            */
/* getlrow.c                                                                  */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Description: Ritorna una stringa allocata dinamicamente contenente       */
/*                una riga logica ottenuta concatenando le righe con "+"      */
/*                come ultimo carattere.                                      */
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

#include "smf80ext.h"

static int continua(char * str);
extern char pgm_name[9];

// routine di acquisizione di una riga logica, concatenando dove richiesto
extern char * getlrow( char * buff, FILE * parm ) {
  static int nrow = 0;                // numero della riga fisica corrente
  int llr;                            // bytes letti, incluso EOL
  const int ll_riga =  MAX_LL;        // max ll di array per riga letta
  char * int_buff   =  malloc(4096);  // get a chunk of memory

  llr=fread(int_buff, 1, ll_riga, parm);
  if ( ferror(parm) ) {
   printf("% 8s E: errore lettura file dei parametri\n", pgm_name);
   fprintf(stderr, "Error reading parameter file\n" );
   return buff;
  }
  if ( feof(parm) ) {
   printf(" \n% 8s -------------------- EOF parametri ----------------------------\n", " ");
   printf(" \n");
   buff[0]='\0';
   return buff;
  }
  nrow++;
  int_buff[strlen(int_buff)-1] = '\0'; // elimina LF finale per agevolare concatenazione
  printf("% 8s %3d -->%s\n", " ", nrow, int_buff);
//  se richiesta continuazione con ultimo carattere = '+'
//  legge e concatena finche richiesto
  while (continua(int_buff)) {
    char * new_str = alloca(ll_riga); // buffer temporaneo per concatenazione
    memset(new_str, 0, ll_riga);      // azzera
    llr=fread(new_str, 1, ll_riga, parm);
    if (feof(parm)) {                // ERRORE : chiesta continuazione, trovato EOF
      printf("% 8s E: richiesta continuazione riga ma incontrato EOF !\n", " ");
      exit(8);
    }
    nrow++;                                // conteggia la riga aggiuntiva
    new_str[strlen(new_str)-1] = '\0';     // elimina LF finale
    if ( new_str[0] == '*' ) continue;     // commento da ignorare
    printf("% 8s %3d    %s\n", " ", nrow, new_str);
    strcat(int_buff, new_str);             // la concatena alla precedente
  }

  buff = realloc(buff, strlen(int_buff) +1); // alloca spazio necessario
  strcpy(buff,int_buff);                   // copia la riga logica e la restituisce
  free(int_buff);
  return buff;
}
//verifica se la riga deve continuare
static int continua(char * str){
  for (int i=strlen(str)-1; ; i--){
    if ( str[i] == '+' ) {
      str[i] = '\0';
      return 1;
    }
    else if ( !isspace(str[i]) ){
      return 0;
    }
  }
  return 0;
}
