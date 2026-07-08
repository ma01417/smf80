/* -------------------------------------------------------------------------- */
/*                                                                            */
/* smf81dec.c                                                                 */
/*   (c) 2026 A.Brezzi                                                        */
/*                                                                            */
/*   Description:                                                             */
/*                                                                            */
/*   Subroutine richiamata da smf80ext per decodificare i rec SMF 81          */
/*   inizializzazione del RACF.                                               */
/*   Chiamato con:                                                            */
/*     *file   file handler su cui scrivere la decodifica del record          */
/*     *smf81  puntatore al record smf81 da decodificare                      */
/*      ll     lunghezza del record passato                                   */
/*     *pgm    nome del programma chiamante                                   */
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
#include <stdint.h>
#include <time.h>

#include "smf81fmt.h"     // definizione aree dei record SMF81

// lunghezza riga separatori
#define LSEP 87

// risposte per per test su bit
#define SI    "  "
#define NO    "No"
#define TNOT  "Universal access for undefined terminals is NONE"
#define TREA  "Universal access for undefined terminals is READ"
#define RSWI  "Installation-defined RVARY SWITCH password in effect"
#define RSWP  "Default RVARY SWITCH password in effect"
#define RSSI  "Installation-defined RVARY STATUS password in effect"
#define RSSP  "Default RVARY STATUS password in effect"

// macro to determine the number of elements of an array
#define NUMELE(x) (sizeof x / sizeof *x)

// macro per testare un bit
#define CHECK_BIT(var, pos) (((var) >> 7-(pos)) & 1)

// struttura per memorizzare i DTA
typedef struct s_dta {
  int dtp;                    // tipo dati
  int dln;                    // lunghezza dati
  SMF81DTS  * rel;            // offset dei dati
  struct s_dta * next;        // puntatore a prossima sezione ripetuta
} rel_sect;

// funzione per stampa esadecimale dati
extern void hexprt( FILE *fdmp, char *head, char *src, int ll, char *pgm_name );

// funzione per estrazione pseudo stringhe da rec SMF
static char *strim(char *s, size_t ll);

// funzione per formattare byte come sequenza di bit
char *fmt_bit(char *buf, const uint8_t myByte);

// funzioni per decodifica della data nei rec SMF
extern char *format_smftime(char * buffer, uint32_t smftime);
extern char *format_smfdate(char * buffer, uint32_t smfdate);
// funzione per creazione stringhe di filler caratteri ripetuti ll
static char *copies(char *s, char filler, size_t ll);

// contructors per sezioni relocabili
rel_sect *createDt1(const SMF81DTS *p_dts, rel_sect * p_root, char *pgm_name ); // constructor struttura dati sez rilocabili
// free memory per i nodi delle sezioni
void free_dt1( rel_sect * p_node);

// sviluppo programma
extern int smf81dec(FILE *fdmp, char *smf_buf, size_t reclen)
{

//  puntatori alle informazioni SMF81
  SMF81HDR    *smf81hdr;
  SMF81DTS    *smf81dts;
  SMF81S30    *p30;                 // section 30 DB RACF datas
  SMF81S21    *p21;                 // section 21 CLASSES datas
  SMF81S32    *p32;                 // section 32 RACF Password & User rules

  char         fmt_buf[2][16];
  char         buf[101];
  char         ris[101];
  int          num_dta = 0;
  int          smf_dta_dtp;     // Data type delle sezioni rilocabili
// per formattare l'ora corrente
  char         l_t[9];          // hh:mm:ss

// nome del programma source e eseguibile
  char *pgm_name    = NULL;
  char ext_name[5]  = "\0";

//  puntatori alle sezioni DTA
  rel_sect  *a_dta[500];

  rel_sect  * p_dta_s, * root_dta;

//  azzera struttura per info sezioni rilocabili
  for ( int i=0; i<NUMELE(a_dta); i++ )
     a_dta[i] = NULL;

  smf81hdr = (SMF81HDR *) smf_buf;    // punta ai dati
// controlli di congruita' dei dati passati
  if (smf81hdr->SMF81RTY != 81)
      return 8;
  if (reclen < SMF81HDR_SIZE)
      return 8;

// decodifica del record, valori iniziali
  fprintf(fdmp,"\n");
// estrae il nome del programma da __FILE__
  pgm_name = getExt(__FILE__, ext_name);
  if ( pgm_name == NULL ) {
    printf("E: impossibile determinare il nome dell'eseguibile\n");
    }

  get_cl_time(l_t);
  fprintf(fdmp,"%s %s Inizio elaborazione (pgm '%s' del %s-%s (%s))\n", pgm_name, l_t, ext_name, __DATE__,__TIME__,__FILE__);
  fprintf(fdmp,"\n");
  fprintf(fdmp,"% 8s %.4s partenza RACF del %10s alle %8s FMID %.4s\n",pgm_name,
          strim(smf81hdr->SMF81SID,4),
          format_smfdate(fmt_buf[0],smf81hdr->SMF81DTE),
          format_smftime(fmt_buf[1],smf81hdr->SMF81TME),
          strim(smf81hdr->SMF81VRM,4));
  fprintf(fdmp,"%s\n",copies(buf,'=',LSEP));
  fprintf(fdmp,"\n");
  fprintf(fdmp," RACF inizializzato con DB %.44s su %.6s\n",
          strim(smf81hdr->SMF81RDS,44),
          strim(smf81hdr->SMF81RVL,6));

  fprintf(fdmp," User Attribute DS (UADS)  %.44s su %.6s\n",
          strim(smf81hdr->SMF81UDS,44),
          strim(smf81hdr->SMF81UVL,6));
  fprintf(fdmp,"\n");

// SMF81OPT
  fprintf(fdmp,"%s\n",copies(buf,'-',LSEP));
  fprintf(fdmp," RACF opzioni alla partenza :\n");
  fprintf(fdmp," SMF81OPT\n");
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OPT, 0)?NO:SI));
  fprintf(fdmp,"   %s RACROUTE REQUEST=VERIFY statistics\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OPT, 1)?NO:SI));
  fprintf(fdmp,"   %s DATASET statistics recorded\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OPT, 2)?SI:NO));
  fprintf(fdmp,"   %s ICHRIX01 active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OPT, 3)?SI:NO));
  fprintf(fdmp,"   %s ICHRCX01 active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OPT, 4)?SI:NO));
  fprintf(fdmp,"   %s ICHRDX01 active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OPT, 5)?SI:NO));
  fprintf(fdmp,"   %s ICHRIX02 active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OPT, 6)?SI:NO));
  fprintf(fdmp,"   %s ICHRCX02 active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OPT, 7)?SI:NO));
  fprintf(fdmp,"   %s ICHRDX02 new PWD exit active\n",ris);
  fprintf(fdmp,"\n");

// SMF81OP2
  fprintf(fdmp," SMF81OP2 Statistics:\n");
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP2, 0)?NO:SI));
  fprintf(fdmp,"   %s Tape volume statistics recorded\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP2, 1)?NO:SI));
  fprintf(fdmp,"   %s DASD volume statistics recorded\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP2, 2)?NO:SI));
  fprintf(fdmp,"   %s Terminal statistics recorded\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP2, 3)?SI:NO));
  fprintf(fdmp,"   %s command routine ICHCNX00 active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP2, 4)?SI:NO));
  fprintf(fdmp,"   %s command routine ICHCCX00 active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP2, 5)?NO:SI));
  fprintf(fdmp,"   %s Automatic DS Protection active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP2, 6)?SI:NO));
  fprintf(fdmp,"   %s Encryption exit routine, ICHDEX01 active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP2, 7)?SI:NO));
  fprintf(fdmp,"   %s ICHNCV00 naming conv table used\n",ris);
  fprintf(fdmp,"\n");

// SMF81OP3
  fprintf(fdmp," SMF81OP3 Protections:\n");
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP3, 0)?SI:NO));
  fprintf(fdmp,"   %s Tape volume protection in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP3, 1)?NO:SI));
  fprintf(fdmp,"   %s Duplicate DS name are to be defined\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP3, 2)?SI:NO));
  fprintf(fdmp,"   %s DASD volume protection in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP3, 3)?SI:NO));
  fprintf(fdmp,"   %s SMF contain version indicator\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP3, 4)?SI:NO));
  fprintf(fdmp,"   %s RACROUTE REQUEST=FASTAUTH preprocessing exit routine, ICHRFX01 active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP3, 5)?SI:NO));
  fprintf(fdmp,"   %s RACROUTE REQUEST=LIST pre/postprocessing exit routine, ICHRLX01 active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP3, 6)?SI:NO));
  fprintf(fdmp,"   %s RACROUTE REQUEST=LIST selection exit routine, ICHRLX02 active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP3, 7)?SI:NO));
  fprintf(fdmp,"   %s RACROUTE REQUEST=DEFINE postprocessing exit routine, ICHRDX02 active\n",ris);
  fprintf(fdmp,"\n");

// SMF81OP4
  fprintf(fdmp," SMF81OP4 more options:\n");
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP4, 0)?SI:NO));
  fprintf(fdmp,"   %s Tape DATASET in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP4, 1)?SI:NO));
  fprintf(fdmp,"   %s Protectall in effect\n",ris);
  if (CHECK_BIT(smf81hdr->SMF81OP4, 2))
    {
    fprintf(fdmp,"      /� \n");
    fprintf(fdmp,"     /  �\n");
    fprintf(fdmp,"    / || �   Protectall in warning mode\n");
    fprintf(fdmp,"   /  ||  �  ==========================\n");
    fprintf(fdmp,"   --------\n");
    }
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP4, 3)?SI:NO));
  fprintf(fdmp,"   %s Erase on scratch in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP4, 4)?SI:NO));
  fprintf(fdmp,"   %s Erase on scratch by SECLev in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP4, 5)?SI:NO));
  fprintf(fdmp,"   %s Erase on scratch for all DS in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP4, 6)?SI:NO));
  fprintf(fdmp,"   %s Enhanced generic naming is in effect\n",ris);
  fprintf(fdmp,"\n");

// SMF81OP5
  fprintf(fdmp," SMF81OP5 more options:\n");
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP5, 0)?SI:NO));
  fprintf(fdmp,"   %s Access control by program is in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP5, 1)?SI:NO));
  fprintf(fdmp,"   %s ACEE compression/expansion exit IRRACX01 compr/exp active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP5, 2)?SI:NO));
  fprintf(fdmp,"   %s RACROUTE REQUEST=FASTAUTH postprocessing exit ICHRFX04 active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP5, 3)?SI:NO));
  fprintf(fdmp,"   %s RACROUTE REQUEST=FASTAUTH preprocessing exit ICHRFX03 exit active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP5, 4)?SI:NO));
  fprintf(fdmp,"   %s SETROPTS NOADDCREATOR active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP5, 5)?SI:NO));
  fprintf(fdmp,"   %s IRREVX01 active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP5, 6)?SI:NO));
  fprintf(fdmp,"   %s ACEE exit IRRACX02 compr/exp\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP5, 7)?SI:NO));
  fprintf(fdmp,"   %s ICHDEX11 password exit active\n",ris);
  fprintf(fdmp,"\n");

// SMF81OP6
  fprintf(fdmp," SMF81OP6 Password\n");
  fprintf(fdmp,"     Maximum password interval day %d\n",smf81hdr->SMF81PIV);
  fprintf(fdmp,"     Minimum days between password changes %d\n",smf81hdr->SMF81PMN);
  fprintf(fdmp,"     Maximum password phrase interval day %d\n",smf81hdr->SMF81PHI);
  if(CHECK_BIT(smf81hdr->SMF81ALG, 0))
    strcpy(ris,"LEGACY");
  if(CHECK_BIT(smf81hdr->SMF81ALG, 1))
    strcpy(ris,"KDFAES");
  fprintf(fdmp,"   %s Password encryption algorithm\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP6, 0)?SI:NO));
  fprintf(fdmp,"   %s Mixed case passwords\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP6, 1)?SI:NO));
  fprintf(fdmp,"   %s New password phrase installation exit is active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP6, 2)?SI:NO));
  fprintf(fdmp,"   %s Field validation exit point (IRRVAF01) for custom fields active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81OP6, 3)?SI:NO));
  fprintf(fdmp,"   %s Special characters allowed in passwords\n",ris);
  fprintf(fdmp,"\n");
  fprintf(fdmp,"%s\n",copies(buf,'-',LSEP));

// SMF81AOP
  fprintf(fdmp," SMF81AOP Auditing\n");
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81AOP, 0)?SI:NO));
  fprintf(fdmp,"   %s User profile changes logged\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81AOP, 1)?SI:NO));
  fprintf(fdmp,"   %s Group profile changes logged\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81AOP, 2)?SI:NO));
  fprintf(fdmp,"   %s DataSet profile changes logged\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81AOP, 3)?SI:NO));
  fprintf(fdmp,"   %s TapeVol profile changes logged\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81AOP, 4)?SI:NO));
  fprintf(fdmp,"   %s DASD profile changes logged\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81AOP, 5)?SI:NO));
  fprintf(fdmp,"   %s Terminal profile changes logged\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81AOP, 6)?SI:NO));
  fprintf(fdmp,"   %s RACF command violation logged\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81AOP, 7)?SI:NO));
  fprintf(fdmp,"   %s SPECIAL user activity logged\n",ris);
  fprintf(fdmp,"\n");

// SMF81AO2
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81AO2, 0)?SI:NO));
  fprintf(fdmp,"   %s Operation user activity logged\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81AO2, 1)?SI:NO));
  fprintf(fdmp,"   %s Audit by security level is in effect\n",ris);
  fprintf(fdmp,"\n");
  fprintf(fdmp,"%s\n",copies(buf,'-',LSEP));

// SMF81TMO
  fprintf(fdmp," SMF81TMO Terminal Verification options\n");
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81TMO, 0)?SI:NO));
  fprintf(fdmp,"   %s User profile changes logged\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81TMO, 1)?TNOT:TREA));
  fprintf(fdmp,"   %s\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81TMO, 2)?SI:NO));
  fprintf(fdmp,"   %s REALDSN is in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81TMO, 3)?SI:NO));
  fprintf(fdmp,"   %s JES-XBMALLRACF is in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81TMO, 4)?SI:NO));
  fprintf(fdmp,"   %s JES-EARLYVERIFY is in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81TMO, 5)?SI:NO));
  fprintf(fdmp,"   %s JES-BATCHALLRACF is in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81TMO, 6)?SI:NO));
  fprintf(fdmp,"   %s RACROUTE REQUEST=FASTAUTH postprocessing exit routine,ICHRFX02, activ\n",ris);
  fprintf(fdmp,"\n");
  fprintf(fdmp,"%s\n",copies(buf,'-',LSEP));

// SMF81BOP
  fprintf(fdmp," SMF81BOP SETROPTS option\n");
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOP, 0)?SI:NO));
  fprintf(fdmp,"   %s SECLABELCONTROL in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOP, 1)?SI:NO));
  fprintf(fdmp,"   %s CATDSNS in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOP, 2)?SI:NO));
  fprintf(fdmp,"   %s MLQUIET in effectct\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOP, 3)?SI:NO));
  fprintf(fdmp,"   %s MLSTABLE in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOP, 4)?SI:NO));
  fprintf(fdmp,"   %s MLS in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOP, 5)?SI:NO));
  fprintf(fdmp,"   %s MLACTIVE in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOP, 6)?SI:NO));
  fprintf(fdmp,"   %s GENERICOWNER in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOP, 7)?SI:NO));
  fprintf(fdmp,"   %s SECLABELAUDIT in effect\n",ris);
  fprintf(fdmp,"\n");

// SMF81BOX
  fprintf(fdmp," SMF81BOX STEROPTS option extensions\n");
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOX, 0)?SI:NO));
  fprintf(fdmp,"   %s COMPATMODE in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOX, 1)?SI:NO));
  fprintf(fdmp,"   %s CATDSNS failures in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOX, 2)?SI:NO));
  fprintf(fdmp,"   %s MLS failures in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOX, 3)?SI:NO));
  fprintf(fdmp,"   %s MLACTIVE failures in effect\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOX, 4)?SI:NO));
  fprintf(fdmp,"   %s APPLAUDIT in effects\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOX, 5)?RSWI:RSWP));
  fprintf(fdmp,"   %s\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOX, 6)?RSSI:RSSP));
  fprintf(fdmp,"   %s\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81BOX, 7)?SI:NO));
  fprintf(fdmp,"   %s ENHANCEDGENERICOWNER in effect\n",ris);
  fprintf(fdmp,"\n");
  fprintf(fdmp,"   %.3s Default primary language\n",smf81hdr->SMF81PRI);
  fprintf(fdmp,"   %.3s Default secondary language\n",smf81hdr->SMF81SEC);
  fprintf(fdmp,"\n");

// SMF81ML2
  fprintf(fdmp," SMF81ML2 More SETROPTS options\n");
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81ML2, 0)?SI:NO));
  fprintf(fdmp,"   %s MLFSOBJ active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81ML2, 1)?SI:NO));
  fprintf(fdmp,"   %s MLIPCOBJ active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81ML2, 2)?SI:NO));
  fprintf(fdmp,"   %s MLNAMES active\n",ris);
  strcpy(ris,(CHECK_BIT(smf81hdr->SMF81ML2, 3)?SI:NO));
  fprintf(fdmp,"   %s SECLBYSYSTEM active\n",ris);
  fprintf(fdmp,"\n");
  fprintf(fdmp,"%s\n",copies(buf,'-',LSEP));

// SMF81JSY e SMF81JUN
  fprintf(fdmp," SMF81JSY SMF81JUN JES options\n");
  fprintf(fdmp," JES undefined user\n");
  fprintf(fdmp,"     JES NJE name userId  %.8s\n",smf81hdr->SMF81JSY);
  fprintf(fdmp,"     JES undefined userId %.8s\n",smf81hdr->SMF81JUN);
  fprintf(fdmp,"\n");

// SMF81QL e SMF81JUN
  fprintf(fdmp," SMF81QL SMF81RPD other\n");
  fprintf(fdmp,"   '%.8s' Single-level data set name\n",strim(smf81hdr->SMF81QL,8));
  fprintf(fdmp,"   %d System retention period in effect\n",smf81hdr->SMF81RPD);
  fprintf(fdmp,"\n");
/*********************************************************************/
/* smf80dmp carica dati sezioni REL e REL EXT                        */
/*********************************************************************/
   for ( int i=0; i<NUMELE(a_dta); i++ ) { // se gia' presenti, libera memoria
     root_dta = (rel_sect *) a_dta[i];
     if ( root_dta ) {
       free_dt1(root_dta);
       a_dta[i] = NULL;
     }
   }

   smf81dts = (SMF81DTS *)((long) smf81hdr + (long) smf81hdr->SMF81REL); // sezione dati corrente
   num_dta  = smf81hdr->SMF81CNT;
   for (int i=0; i<num_dta; ++i) {      // carica triplette per tutte le sezioni
     smf_dta_dtp = smf81dts->SMF81DTP;
     if ( smf_dta_dtp > NUMELE(a_dta) )
       fprintf(stderr,"% 8s Sezione %d '%x' oltre i limiti consentiti sezioni ammissibile\n", pgm_name, smf_dta_dtp, smf_dta_dtp);
     root_dta = a_dta[smf_dta_dtp];
     root_dta = createDt1(smf81dts, root_dta, pgm_name);    // crea struttura
     a_dta[smf_dta_dtp] = root_dta;
     smf81dts =(SMF81DTS *)((long) smf81dts + (long) smf81dts->SMF81DLN +2); // prossima sezione dati
   }

// decodifica delle sezioni rilocabili DTS
  for (int i=0; i<NUMELE(a_dta); ++i) {       // stampa in hex le sezioni DTA
      char head[120];
      int num;
      int sec_ind = 0;    // per testare prima sezione rilocabile da stampare
      int rdb_ind = 0;    // per testare inizio elenco DS DB RACF
      p_dta_s = (rel_sect *) a_dta[i];        // pointer to struct.

// estrae dati da sezioni rilocabili
      while ( p_dta_s ) {
        switch (p_dta_s->dtp) {
          case 30:     // section 30 DB RACF datas
          {
// SMF81S30 RACF DB datas
            char  ds_nam[45];
            p30 = (SMF81S30 *) p_dta_s->rel; // to decode section 30 values
            if(!rdb_ind++)                   // print header only for first section 30
              {
              fprintf(fdmp,"\n");
              fprintf(fdmp," SMF81S30 RACF DB informations\n");
              fprintf(fdmp,"   Active Use  Num Volume Dataset\n");
              fprintf(fdmp,"   ------ ---  --- ------ -------\n");
              }
            memcpy(ds_nam, p30->SMF81S30_DS, p30->SMF81S30_DL - 11);
            memset(ds_nam + p30->SMF81S30_DL - 11,0,1);
            fprintf(fdmp,"   % 6s %4s % 3u %.6s %s\n",
                  CHECK_BIT(p30->SMF81S30_FL, 0)?" Yes  ":" No   ",
                  CHECK_BIT(p30->SMF81S30_FL, 1)?"Back":"Prim",
                  p30->SMF81S30_NM,
                  p30->SMF81S30_VL,
                  ds_nam);
            break;
          }
          case 21:
          {
// SMF81S21 CLASSES datas
          char  lbuf[20];
          p21 = (SMF81S21 *) p_dta_s->rel; // to decode section 21 values
          if(!sec_ind++)                   // print header only for first section 21
            {
            fprintf(fdmp,"\n");
            fprintf(fdmp," SMF81S21 CLASS informations\n");
            fprintf(fdmp,"                               Generic Generic  Global\n");
            fprintf(fdmp,"   Name     Active Audit Stats Profile Commands Checking RACLIST GENLIST LogOpt\n");
            fprintf(fdmp,"   -------- ------ ----- ----- ------- -------- -------- ------- ------- -------\n");
            }
//          decodifica dei flag di log options
            strcpy(lbuf,"Never");
            if ( CHECK_BIT(p21->SMF81S21_F2, 1) )
              strcpy(lbuf,"Always");
            if ( CHECK_BIT(p21->SMF81S21_F2, 2) )
              strcpy(lbuf,"Never");
            if ( CHECK_BIT(p21->SMF81S21_F2, 3) )
              strcpy(lbuf,"Successes");
            if ( CHECK_BIT(p21->SMF81S21_F2, 4) )
              strcpy(lbuf,"Failures");
            if ( CHECK_BIT(p21->SMF81S21_F2, 5) )
              strcpy(lbuf,"Default");
            fprintf(fdmp,"   %.8s %6s %5s %5s %7s %8s %8s %7s %7s %8s\n",
                  p21->SMF81S21_CL,
                  CHECK_BIT(p21->SMF81S21_FL, 2)?"  Yes ":"  No  ",     // active
                  CHECK_BIT(p21->SMF81S21_FL, 1)?" Yes ":" No  ",       // audit
                  CHECK_BIT(p21->SMF81S21_FL, 0)?" Yes ":" No  ",       // stats
                  CHECK_BIT(p21->SMF81S21_FL, 3)?"  Yes  ":"  No   ",   // generic profile
                  CHECK_BIT(p21->SMF81S21_FL, 4)?"  Yes   ":"  No    ", // generic commands
                  CHECK_BIT(p21->SMF81S21_FL, 5)?"  Yes   ":"  No    ", // global checking
                  CHECK_BIT(p21->SMF81S21_FL, 6)?"  Yes  ":"  No   ",   // raclist
                  CHECK_BIT(p21->SMF81S21_FL, 7)?"  Yes  ":"  No   ",   // genlist
                  lbuf);
            break;
          }
          case 32:
          {
// SMF81S32 RACF Password & User rules
            char  lbuf[40];
            PWDRULE *p_syr;
            p32   = (SMF81S32 *) p_dta_s->rel; // to decode section 32 values
            p_syr = (PWDRULE *) p32->SMF81S32_SYR;
            fprintf(fdmp,"\n");
            fprintf(fdmp," %s\n","SMF81S32 RACF Password & User rules");
            fprintf(fdmp,"    Pass.   Pass.  Revoke   Pass.    Pass.   Pass.  Inact.   Model  Model  Model\n");
            fprintf(fdmp,"    Inter. History Attempt Warning  Syntax  Length Interval  (GDG) (User) (Group) GRPLIST\n");
            fprintf(fdmp,"   ------- ------- ------- ------- -------- ------ -------- ------ ------ ------- -------\n");
            fprintf(fdmp,"   %7d %7d %7d %7d %.8s %6d %8d ",p32->SMF81S32_PWI,
                                                            p32->SMF81S32_PWH,
                                                            p32->SMF81S32_URE,
                                                            p32->SMF81S32_PWL,
                                                            p_syr->PWD_RULE,
                                                            p_syr->PWD_MINL,
                                                            p32->SMF81S32_UIN);
            fprintf(fdmp," % 3s    % 3s    % 3s     %-3s\n",CHECK_BIT(p32->SMF81S32_IN1, 0)?"Yes":"No",
                                                            CHECK_BIT(p32->SMF81S32_IN1, 1)?"Yes":"No",
                                                            CHECK_BIT(p32->SMF81S32_IN1, 2)?"Yes":"No",
                                                            CHECK_BIT(p32->SMF81S32_IN1, 3)?"Yes":"No");
            p_syr += 1;
            for ( int j=0; j<NUMELE(p32->SMF81S32_SYR)-1; ++j )   // max 9 regole rimanenti
            {
              if ( p_syr->PWD_MINL == 0 )  break;  // fino alla prima struttura vuota
              fprintf(fdmp,"   %s %.8s\n",
                      copies(lbuf, ' ',32),
                      p_syr->PWD_RULE);
              p_syr += 1;
            }
            break;
         }
          default:
            fprintf(fdmp,"\n");
            fprintf(fdmp," %s\n","SMF81DTS unknown section");
            num=sprintf(head,"Rel %d('%x')", p_dta_s->dtp, p_dta_s->dtp);
            hexprt(fdmp, head, (char *) p_dta_s->rel, p_dta_s->dln, pgm_name);
            fprintf(fdmp,"\n");
            break;
      }
      p_dta_s = (rel_sect *) p_dta_s->next;
    }
    }
  return 0;
}

/* -------------------------------------------------------------------------- */
/*   copies: crea una stringa di ll caratteri filler                          */
/* -------------------------------------------------------------------------- */

static char *copies(char *s, char filler, size_t ll) {
  char *p;
  memset(s, filler, ll);
  p = s + ll;
  memset(p, '\0', 1);
return s;
}

/* ---------------------------------------------------------------- */
/* strim    funzione di trim degli spazi finali di un campo del     */
/*          record SMF non in formato stringa, ritorna              */
/*          puntatore a stringa formata correttamente               */
/* ---------------------------------------------------------------- */
// trim trailing spaces in non well formatted string i.e. CHAR in SMF
static char *strim(char *s, size_t ll) {
  /*
  Remove trailing whitespace from the given pseudo string.
  */
  while(ll && isspace(s[--ll]))
    s[ll] = '\0';
  return s;
}

/* -------------------------------------------------------------------------- */
/*   Description: ritorna una stringa con la decodifica formato bit           */
/*                di un unsigned char                                         */
/* -------------------------------------------------------------------------- */
char *fmt_bit(char *buf, const uint8_t myByte) {
  int i, n = 0;
  for ( i = 0; i <= 7; i++)
  {
   buf[i]=CHECK_BIT(myByte,i)?'1':'0';
  }
  buf[8]='\0';
  return buf;
}

/* -------------------------------------------------------------------------- */
/*   Description: constructor per struttura contenente dati sezioni           */
/*                relocabili rec SMF 80, sia normali che extended             */
/* -------------------------------------------------------------------------- */
rel_sect *createDt1(const SMF81DTS *p_dts, rel_sect * p_root, char *pgm_name )
{
  rel_sect *new_dta = (rel_sect *) malloc( sizeof(rel_sect));   // creato nuovo nodo
  rel_sect *p_dta = p_root;
  int smf_dta_dtp = p_dts->SMF81DTP;
// controllo
  if ( !new_dta ) {
    fprintf(stderr, "% 8s E: impossibile allocare nodo sezione rilocabile\n", pgm_name);
    exit(8);
  }
  new_dta->dtp  = smf_dta_dtp;
  new_dta->dln  = p_dts->SMF81DLN + 2;
  new_dta->rel  = (SMF81DTS *) p_dts;
  new_dta->next = NULL;

  if ( !p_root )
    return new_dta;

  while (p_dta->next != NULL)
    p_dta = (rel_sect *) p_dta->next;
  p_dta->next = (rel_sect *) new_dta;
  return p_root;
}

/**********************************************************************/
/* free_dt1 : libera la memoria allocata a nodi di sezioni relocabili */
/**********************************************************************/
void free_dt1( rel_sect * p_node) {
  rel_sect * p_next;
  while ( p_node ) {
//    printf("% 8s rec %d libero nodo %p, dta %d, prossimo %p\n", "Free", p_node->rec_num, p_node, p_node->dtp, p_node->next);
    p_next = (rel_sect *) p_node->next;
    free(p_node);
    p_node = (rel_sect *) p_next;
  }
  return;
}
