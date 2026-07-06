/* ================================================================= */
/*                                                                   */
/*   Name:                                                           */
/*      smf80dmp.c                                                   */
/*                                                                   */
/*   Autore:                                                         */
/*      A.Brezzi   alessandro.-brezzi@gmail.com                      */
/*                                                                   */
/*   Data:                                                           */
/*      10 ottobre 2024                                              */
/*                                                                   */
/*   Descrizione:                                                    */
/*      Filtra record SMF 80 (RACF) in base ad un file di criteri    */
/*      e ne crea una stampa esadecimale formattata con l'header     */
/*      e le varie sezioni DTA e DT2                                 */
/*                                                                   */
/*   Input:                                                          */
/*      DD:UTIPARM                                                   */
/*       file sequenziale contenente i criteri per l'estrazione.     */
/*       Ogni criterio e' composto da una caratteristica, operatore  */
/*       seguito da una lista di valori.                             */
/*       Tutti i criteri sono messi in AND, cioe' tutti debbono      */
/*       essere soddisfatti per il record in esame.                  */
/*        <nome car> <op> <lista valori> [ + ]                       */
/*          [ continuazione lista valori ]                           */
/*       dove:                                                       */
/*        <nome car> uno dei campi rec 80 riconosciuto tra           */
/*           EVENT  Evento SMF80EVT                                  */
/*           USER   User associato SMF80USR                          */
/*           RESULT Event Qualifier SMF80EVQ                         */
/*           JOBNAM AS name SMF80JBN                                 */
/*           REASN  Reson for logging SMF80REA                       */
/*           AUTH   Autorities used SMF80ATH                         */
/*           RES    Resource name (rel sect 01)                      */
/*           USRJES Res Owner in JESSPOOL resource (rel sect 01)     */
/*           CLASS  Class name (rel sect 17)                         */
/*           PROF   Profile used (rel sect 33)                       */
/*        <op> puo' essere uno tra                                   */
/*           '='    uguale uno nella lista presente ( OR )           */
/*           '<>'   diverso nessuno nella lista presente ( AND )     */
/*           'ABR'  abbreviato uno nella lista e' abbreviazione      */
/*        <lista valori> elenco dei valori da ricercare; e'          */
/*           possibile continuare la lista su piu' righe terminando  */
/*           la riga con '+'                                         */
/*           La massima lunghezza di ogni valore e' 44               */
/*      DD: UTI001                                                   */
/*       file sequanziale VBS o concatenazione di files contenente   */
/*       i record SMF da filtrare                                    */
/*      DD: UTICNTL                                                  */
/*       PDS FB 80 contenente eventi e eventi qualificatori per      */
/*       verifica e decodifica                                       */
/*                                                                   */
/*   Output:                                                         */
/*      DD: UTI002                                                   */
/*       file sequanziale VBA filtrato in base ai criteri dati       */
/*       contenente la stampa esadecimale                            */
/*                                                                   */
/*  Autorizzazioni richieste :                                       */
/*          READ  access al DS SMF                                   */
/*          ALTER / UPDATE access al DS UTI002                       */
/*                                                                   */
/*  Uso:    Tramite JCL con DD UTI001 UTI002 UTIPARM e UTICNTL       */
/*          preallocate                                              */
/*                                                                   */
/*                                                                   */
/* ================================================================= */

/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <stdint.h>
#define _XOPEN_SOURCE_EXTENDED 1
#include <libgen.h>

/* smf80ext headers */
#include "smf80ext.h"     // definizioni di supporto: strutture usate
#include "smf80sup.h"     // definizioni di supporto: costanti e variabili
#include "smf80fmt.h"     // definizione aree dei record SMF80

// costanti da usare in header messaggi printf()
char *pgm_name = NULL;
char vid_name[20] = "\0";
char ext_name[5]  = "\0";

/*********************************************************************/
/* Global variables                                                  */
/*********************************************************************/

/* structure to accomodate STCK time to be converted  */
struct timespc ts;
// per formattare l'ora corrente
char    l_t[9];                     // hh:mm:ss

runparm        * p_runparm;
list_filter    * p_fil;
list_parameter * p_par;
st_sm80_sez    * p_sez;
st_sm80_sez    * root_sez;
st_sm80_evt    * root_evt = NULL;
st_sm80_evt    * p_evt;

char smf_buf[32768];

SMF80HDR    *smf80hdr;
SMF80DTS    *smf80dts;
SMF80DT2    *smf80dt2;        // puntatore alle sezioni DT2 - extended
SMF80SEC_FL  smf80sec_fl;
SMF80SEC_FL  tobefind;

int          smf_dta_dtp;     // Data type delle sezioni rilocabili

rel_sect  *a_dta[100];
rel_sec2  *a_dt2[200];

rel_sect  * p_dta_s, * root_dta;
rel_sec2  * p_dt2_s, * root_dt2;

// buffer per stampa
char riga_dmp[280];

int num_dta = 0;
int num_dt2 = 0;

// minima e massima lunghezza dei record letti
uint32_t min_llrec = sizeof smf_buf;
uint32_t max_llrec = 0;

int  t_smfl = 0;          // total records read
int  t_smfe = 0;          // total records SMF 80
int  t_smfp = 0;          // total record filtered
int  num = 0;             // bytes trasferiti

int str_split(char *sp, char *token, char sep); // divide una stringa <t1>.<t2>... nei componenti base

// funzioni decodifica orari
void stck_tm(char *pData, struct timespc *pTimespc); // funzione di conversione di orario in formato stck
// contructors per sezioni relocabili
extern rel_sect *createDta(const SMF80DTS *p_dts, rel_sect * p_root, int nrec);    // constructor struttura dati sez rilocabili
extern rel_sec2 *createDt2(const SMF80DT2 *p_dt2, rel_sec2 * p_root2, int nrec);   // constructor struttura dati sez rilocabili extended
// free memory per i nodi delle sezioni
extern void free_dta( rel_sect * p_node);
extern void free_dt2( rel_sec2 * p_node);

// routine che restituisce la descrizione di una sezione
char     *findsez(int n_sez, st_sm80_sez * p_sez);   // restituisce pointer descrizione sezione
// routine che restituisce il puntatore all'evento
extern st_sm80_evt *findevtn(uint8_t num_evt, st_sm80_evt *root_evt);

// routine per testare se record corrisponde ad un filtro
int check_parm(char *name, uint8_t value, list_filter *p, int op, int fil_type);
// routine che restituisce max e min per orari forniti in array con date e time;
void check_clock(uint32_t smfdt, uint32_t smftm, uint32_t mind[2], uint32_t maxd[2]);

/* files handler  */
FILE *fsmf;    // FILE con SMF input
FILE *fdmp;    // FILE di DMP dei record estratti


/*********************************************************************/
/* smf80dmp mainline                                                 */
/*********************************************************************/

int main( int argc, char * argv[] ) {
 char fil_parm[] = "DD:UTIPARM";                  // DD per file dei parametri
 char fil_cntl[] = "DD:UTICNTL";                  // DD PDS per decodifiche
 char fil_smf[]  = "DD:UTI001";                   // file SMF VBS input
 char fil_dmp[]  = "DD:UTI002";                   // file con il DUMP dei rec 80

// minima e massima date ed ora incontrate nei rec SMF
 uint32_t  min_clk[] = {UINT_MAX, UINT_MAX};
 uint32_t  max_clk[] = {0, 0};

 uint32_t  smf80_date;
 uint32_t  smf80_time;
 uint8_t   smf80_type;

 char buf_dt[21];
 char buf_tm[21];

 char t_str[257];
 unsigned long num_prt;
 int  ll_cmp;

/************************************************************************
 * Mainline di lettura e formattazione record SMF
 * filtra solo record type 80
 ************************************************************************/
 /* estrae il nome del programma da __FILE__       */
 pgm_name = getExt(__FILE__, ext_name);
 if ( pgm_name == NULL ) {
   printf("E: impossibile determinare il nome dell'eseguibile\n");
 }
 memset(vid_name, ' ', strlen(pgm_name));

 /* imposta locale su Italia */
   setlocale(LC_ALL, "It_IT.IBM-1144");

 /* apertura file parametri elaborazione           */
 get_cl_time(l_t);
 printf("%s %s Inizio elaborazione (pgm '%s' del %s-%s (%s))\n", pgm_name, l_t, ext_name, __DATE__,__TIME__,__FILE__);
 printf("\n");
 if (argc == 1) {                                 //  imposta valore di default
   num_prt = 0;
   printf("% 8s non fornito limite stampa, stampo tutti\n", pgm_name);
 }
 else {
   char * stp_str;
   num_prt = strtoul( argv[1], &stp_str, 10);
   printf("% 8s fornito limite stampa, stampo %d record\n", pgm_name, num_prt);
 }
// caricamento eventi
 root_evt =  getEvt(fil_cntl, "SM80EVNT") ;
// carica le descrizioni delle sezioni
 root_sez =  getSez(fil_cntl, "SM80REL");
 p_sez = root_sez;

 p_runparm = getParm(fil_parm, fil_cntl, root_evt);
 if ( ! p_runparm ) {
   fprintf(stderr,"%s ERRORE nella lettura dei parametri\n", vid_name);
   return 8;
 }

 tobefind = p_runparm->smf80sec_fl;

 get_cl_time(l_t);
 printf(" \n");
 printf("%s %s Apertura DS elaborazione\n", pgm_name, l_t);
 /* apertura file con i record SMF da trattare     */
 fsmf=openf(fil_smf, "rb,recfm=VBS,lrecl=X,type=record", "SMF input", pgm_name);
 if ( !fsmf ) exit(8);

 /* apertura file per i dump dei record estratti   */
 fdmp=openf(fil_dmp, "w,recfm=VBA,lrecl=170", "DUMP sezioni SMF", pgm_name);
 if ( !fdmp ) exit(8);

 /* azzera strutture per info sezioni relocabili e rel extended */
 for ( int i=0; i<NUMELE(a_dta); i++ )
     a_dta[i] = NULL;
 for ( int i=0; i<NUMELE(a_dt2); i++ )
     a_dt2[i] = NULL;

 get_cl_time(l_t);
 printf("\n%s %s Lettura record SMF\n", pgm_name,l_t);

 printf("%s ricerca sezioni ", pgm_name);
 if ( tobefind.header  ) printf("%3d HEADER ",l_sez[EVENT]);
 if ( tobefind.res     ) printf("%3d RES ",l_sez[RES]);
 if ( tobefind.class   ) printf("%3d CLASS ",l_sez[CLASS]);
 if ( tobefind.profile ) printf("%3d PROFILE ",l_sez[PROF]);
 if ( tobefind.usrjes  ) printf("%3d USRJES",l_sez[RES]);
 printf("\n");

/*********************************************************************/
/* smf80dmp ciclo di lettura e filtro dei record correnti            */
/*********************************************************************/
// intesta progressione della lettura
 printf(" \n");
 printf("  ---ora-- ----Mrec.--- --SMF dt-- --SMF tme--  evt -name-   ------Description--------------------------------- ----estr.---\n");

 while ( 1 ) {  // ciclo infinito interrotto con break
   int num_op, fil_type;
   int fl_ok, op;                      // per filtro in AND nel ciclo
   num = fread( smf_buf, 1, sizeof smf_buf, fsmf );
   if ( feof(fsmf) ) break;            // finito input
   ++t_smfl;

   smf80hdr = (SMF80HDR *) smf_buf;    // punta ai dati
   fl_ok = 1;                          // inizializzato a no flag

   max_llrec = MAX(num, max_llrec);
   min_llrec = MIN(num, min_llrec);

// determina massima e minima data ed ora dei rec SMF
   smf80_date = smf80hdr->SMF80DTE;
   smf80_time = smf80hdr->SMF80TME;
   smf80_type = smf80hdr->SMF80RTY;

// richiamo a routine
   check_clock(smf80_date, smf80_time, min_clk, max_clk);

// stampa progressivo avanzamento
   if ( !(t_smfl%10000000)) {
     int m_rec = t_smfl / 1000000;
     get_cl_time(l_t);
     if ( smf80_type != 80 )
       printf("  %s %'5d %s %s % 4s % 8s % 50s %'12d\n",
              l_t, m_rec, format_smfdate(buf_dt, smf80_date), format_smftime(buf_tm, smf80_time),
              " ", " ", " ", t_smfp);
     else
       printf("  %s %'5d %s %s (%02X) %-8s %-50s %'12d\n",
              l_t, m_rec, format_smfdate(buf_dt, smf80_date), format_smftime(buf_tm, smf80_time),
              p_evt->evt_value, p_evt->evt_name, p_evt->evt_desc->data, t_smfp);
   }
// filtra solo record 80
   if ( smf80_type != 80 )
      continue;

// conteggia evento
   ++t_smfe;                                        // total records SMF 80
   p_evt = findevtn(smf80hdr->SMF80EVT, root_evt);  // puntatore all'elemento con evento
   if ( p_evt )
     ++p_evt->evt_numf;

/*********************************************************************/
/* smf80dmp filtri su Header del record SMF 80                       */
/*********************************************************************/

// -- EVENT
   p_par = p_runparm->root_param[EVENT];
   if ( p_par ) {
     if ( !fltparm(p_par, " ", smf80hdr->SMF80EVT) ) {
       continue;             // esce se filtro non soddisfatto
     }
   }
// -- USER
   p_par = p_runparm->root_param[USER];
   if ( p_par ) {
     memset(t_str, 0, sizeof t_str);
     memcpy(t_str, smf80hdr->SMF80USR,8);
     rtrim(t_str);
     if ( !fltparm(p_par, t_str, 0) ) {
       continue;             // esce se filtro non soddisfatto
     }
   }
// -- DESCR
   p_par = p_runparm->root_param[DESCR];
   if ( p_par ) {
     if ( !fltparm(p_par, " ", smf80hdr->SMF80DES) ) {
       continue;             // esce se filtro non soddisfatto
     }
   }
// -- RESULT
   p_par = p_runparm->root_param[RESULT];
   if ( p_par ) {
     if ( !fltparm(p_par, " ", smf80hdr->SMF80EVQ) ) {
       continue;             // esce se filtro non soddisfatto
     }
   }
// -- JOBNAM
   p_par = p_runparm->root_param[JOBNAM];
   if ( p_par ) {
     memset(t_str, 0, sizeof t_str);
     memcpy(t_str, smf80hdr->SMF80JBN,8);
     if ( t_str[0] == '\0' )
       continue;
     rtrim(t_str);
     if ( !fltparm(p_par, t_str, 0) ) {
       continue;             // esce se filtro non soddisfatto
     }
   }
// -- REASN
   p_par = p_runparm->root_param[REASN];
   if ( p_par ) {
     if ( !fltparm(p_par, " ", smf80hdr->SMF80REA) ) {
       continue;             // esce se filtro non soddisfatto
     }
   }
// -- AUTH
   p_par = p_runparm->root_param[AUTH];
   if ( p_par ) {
     if ( !fltparm(p_par, " ", smf80hdr->SMF80ATH) ) {
       continue;             // esce se filtro non soddisfatto
     }
   }

/*********************************************************************/
/* smf80dmp carica dati sezioni REL e REL EXT                        */
/*********************************************************************/
   for ( int i=0; i<NUMELE(a_dta); i++ ) { // se gia' presenti, libera memoria
     root_dta = (rel_sect *) a_dta[i];
     if ( root_dta ) {
       free_dta(root_dta);
       a_dta[i] = NULL;
     }
   }

   smf80dts = (SMF80DTS *)((uintptr_t) smf80hdr + (uintptr_t) smf80hdr->SMF80REL); // sezione dati corrente
   num_dta  = smf80hdr->SMF80CNT;
   for (int i=0; i<num_dta; ++i) {      // ricerca tutte le sezioni
     smf_dta_dtp = smf80dts->SMF80DTP;
     if ( smf_dta_dtp > NUMELE(a_dta) )
       fprintf(stderr,"% 8s Sezione %d '%x' oltre i limiti consentiti sezioni ammissibile\n", pgm_name, smf_dta_dtp, smf_dta_dtp);
     root_dta = a_dta[smf_dta_dtp];
     root_dta = createDta(smf80dts, root_dta, t_smfl);    // crea struttura
     a_dta[smf_dta_dtp] = root_dta;
     smf80dts =(SMF80DTS *)((uintptr_t) smf80dts + (uintptr_t) smf80dts->SMF80DLN +2); // prossima sezione dati
   }    // fine loop sezioni DTA sulle quali facciamo i filtri

/*********************************************************************/
/* smf80dmp filtri sulle sezioni relocabili o relocabili extended    */
/*********************************************************************/
// verifica presenza sezioni richieste, altrimenti scarta record
   if ( (tobefind.res) || (tobefind.usrjes) )
     if ( !a_dta[l_sez[RES]] )
       continue;                // manca sezione : scarto
   if ( tobefind.class )
     if ( a_dta[l_sez[CLASS]] == NULL ) {
       continue;
     }
   if ( tobefind.profile)
     if ( !a_dta[l_sez[PROF]] )
       continue;                // manca sezione : scarto

// -- CLASS
   p_par = p_runparm->root_param[CLASS];
   if ( p_par ) {
     int curr_sec = l_sez[CLASS];
     list_filter *pf = p_par->param_filter;  // primo valore richiesto
     smf80dts = a_dta[curr_sec]->rel;
     ll_cmp = smf80dts->SMF80DLN;
     memset(t_str, 0, sizeof t_str);
     memcpy(t_str, smf80dts->SMF80DTA, ll_cmp);
     if ( !fltparm(p_par, t_str, 0) )
       continue;                       // esce per filtri non soddisfatti
   }
// -- RES
   p_par = p_runparm->root_param[RES];
   if ( p_par ) {
     smf80dts = a_dta[l_sez[RES]]->rel;
     ll_cmp = MIN(smf80dts->SMF80DLN, sizeof t_str);
     memset(t_str, 0, sizeof t_str);
     memcpy(t_str, smf80dts->SMF80DTA, ll_cmp);
     rtrim(t_str);
     if ( !fltparm(p_par, t_str, 0) )
       continue;                       // esce per filtri non soddisfatti
   }
// -- USRJES
   p_par = p_runparm->root_param[USRJES];
   if ( p_par ) {
     int rc, t_num = 0;
     char res_buff[20];
     char *p_s = t_str;
     smf80dts = a_dta[l_sez[RES]]->rel;
     ll_cmp = MIN(smf80dts->SMF80DLN, sizeof t_str);
     memset(t_str, 0, sizeof t_str);
     memcpy(t_str, smf80dts->SMF80DTA, ll_cmp);
   // estrae componente USER da risorsa JES
     while ( t_num++ < 2  ) {          // estrae il secondo token USER della risorsa
       memset(res_buff, 0, sizeof res_buff);
       rc = str_split(p_s, res_buff, '.');
       p_s = NULL;
     }
     if ( !fltparm(p_par, res_buff, 0) )
       continue;                       // esce per filtri non soddisfatti
   }
// -- PROF
   p_par = p_runparm->root_param[PROF];
   if ( p_par ) {
     smf80_dta *p_dt33;
     smf80dts = a_dta[l_sez[PROF]]->rel;
     p_dt33   = smf80dts->SMF80DTA;
     ll_cmp = MIN(smf80dts->SMF80DLN - 1, sizeof t_str);  // elinina byte flag tipo prof
     memset(t_str, 0, sizeof t_str);
     memcpy(t_str, p_dt33->s033_res_prof, ll_cmp);
     rtrim(t_str);
     if ( !fltparm(p_par, t_str, 0) )
       continue;                       // esce per filtri non soddisfatti
   }

// se non ha passato i filtri allora lo scarta e tratta il prossimo
   if ( !fl_ok )  continue;              // scarta

// altrimenti formatta le info e scrive elenco sezioni e DUMP delle stesse
    ++t_smfp;
    if ( p_evt )
      ++p_evt->evt_nume;
    if ( (num_prt > 0) && (t_smfp > num_prt) ) break; // num totale rec estratti -> max richiesto (100 default

// carica le sezioni DA2 estese per stampa completa
   for ( int i=0; i<NUMELE(a_dt2); i++ ) {
     root_dt2 = (rel_sec2 *) a_dt2[i];
     if ( root_dt2 ) {
       free_dt2(root_dt2);
       a_dt2[i] = NULL;
     }
   }

   num_dt2 = smf80hdr->SMF80CT2;                      // numero di sezioni relocabili extended
   smf80dt2 = (SMF80DT2 *)((uintptr_t) smf80hdr + (uintptr_t) smf80hdr->SMF80RL2); // prima sezione dati
   for (int i=0; i<num_dt2; i++) {      // ricerca tutte le sezioni rilocabili extended
     smf_dta_dtp = smf80dt2->SMF80TP2;
     if ( IDT2(smf_dta_dtp) > NUMELE(a_dt2) )
       fprintf(stderr,"% 8s rec %d Sezione %d '%x' oltre i limiti consentiti sezioni extended ammissibile\n", pgm_name, t_smfl, smf_dta_dtp, smf_dta_dtp);
     root_dt2 = a_dt2[IDT2(smf_dta_dtp)];
     root_dt2 = createDt2(smf80dt2, root_dt2, t_smfl);
     a_dt2[IDT2(smf_dta_dtp)] = root_dt2;
     smf80dt2 =(SMF80DT2 *)((uintptr_t) smf80dt2 + (uintptr_t) smf80dt2->SMF80DL2 +4); // prossima sezione dati
   }    // fine loop sezioni DA2 senza filtri

/* ---------------------------------------------------------------------- */
/*                                                                        */
/*  parte per la stampa in formato DUMP del record                        */
/*                                                                        */
/* ---------------------------------------------------------------------- */
 /* intestazione file di controllo */
    memset(riga_dmp, '\0', sizeof riga_dmp);
    num = 0;
    for (int i=0; i<NUMELE(a_dta); ++i) { // stampa la sequenza delle sezioni REL
      p_dta_s = a_dta[i];                 // pointer to struct.
      if ( p_dta_s )
        {
        strcpy(t_str, " ");
        if ( p_dta_s->next )
          strcpy(t_str, "R");
        if ( p_dta_s ) num += sprintf(riga_dmp,"%s %d%s", riga_dmp, p_dta_s->dtp, t_str);
        }
    }
    for (int i=0; i<NUMELE(a_dt2); ++i) {  // stampa la sequenza delle sezioni REL EXT
      p_dt2_s = a_dt2[i];                 // pointer to struct.
      if ( p_dt2_s )
        {
        strcpy(t_str, " ");
        if ( p_dt2_s->next )
          strcpy(t_str, "R");
        if ( p_dt2_s ) num += sprintf(riga_dmp,"%s %d%s", riga_dmp, p_dt2_s->dt2, t_str);
        }
    }
    p_evt = findevtn(smf80hdr->SMF80EVT, root_evt);  // puntatore all'elemento con evento
    fprintf(fdmp," Rec.nr.  Evt (hx) Evento   Sezioni\n");
    fprintf(fdmp," -------- --- ---- -------- <-------------------------------------------->\n");
    fprintf(fdmp,"%9d %3d   %02x % 8s  %s\n", t_smfl, smf80hdr->SMF80EVT, smf80hdr->SMF80EVT,
            p_evt->evt_name, riga_dmp);
    hexprt(fdmp, "SMF 80 header", (char *) smf80hdr, SMF80HDR_SIZE, pgm_name);
    for (int i=0; i<NUMELE(a_dta); ++i) {       // stampa in hex le sezioni DTA
      char head[120];
      char * s_desc;
      p_dta_s = (rel_sect *) a_dta[i];                 // pointer to struct.
      s_desc = findsez(p_dta_s->dtp, root_sez);
      while ( p_dta_s ) {
        num=sprintf(head,"Rel %d('%x') %s", p_dta_s->dtp, p_dta_s->dtp, s_desc);
        hexprt(fdmp, head, (char *) p_dta_s->rel, p_dta_s->dln, pgm_name);
        p_dta_s = (rel_sect *) p_dta_s->next;
      }
    }
    for (int i=0; i<NUMELE(a_dt2); i++) {     // stampa in hex le sezioni DT2
      char head[120];
      char * s_desc;
      p_dt2_s = (rel_sec2 *) a_dt2[i];
      s_desc = findsez(p_dt2_s->dt2, root_sez);
      while ( p_dt2_s ) {
        num=sprintf(head,"Ext %d('%x') %s", p_dt2_s->dt2, p_dt2_s->dt2,  s_desc);
        hexprt(fdmp, head, (char *) p_dt2_s->rel2, p_dt2_s->dl2, pgm_name);
        p_dt2_s = (rel_sec2 *) p_dt2_s->next;
      }
    }

 }
   get_cl_time(l_t);
   printf(" \n");
   printf(" \n");
   printf("% 8s %s rec SMF 80 dal %s %s\n", pgm_name, l_t, format_smfdate(buf_dt, min_clk[0]), format_smftime(buf_tm,min_clk[1]));
   printf("% 8s %s rec SMF 80  al %s %s\n", pgm_name, l_t, format_smfdate(buf_dt, max_clk[0]), format_smftime(buf_tm,max_clk[1]));
   printf(" \n");
   printf("% 8s %s max lunghezza rec   %'14d\n", pgm_name, l_t, max_llrec);
   printf("% 8s %s min lunghezza rec   %'14d\n", vid_name, l_t, min_llrec);

   printf(" \n% 8s %s numero record letti %'14d\n", pgm_name, l_t, t_smfl);
   printf("% 8s %s di cui rec 80       %'14d\n", vid_name, l_t, t_smfe);
   printf("% 8s %s estratti ok         %'14d\n", vid_name, l_t, t_smfp);
   printf(" \n");
   p_evt = root_evt;
   printf("Evt (hx) Evento   Descrizione                                                Numero Rec.       Estratti\n");
   printf("--- ---- -------- ------------------------------------------------------- -------------- --------------\n");
   while (p_evt) {
     if ( p_evt->evt_numf )
       printf("%3d   %02x %-8s %-55s %'14d %'14d\n", p_evt->evt_value, p_evt->evt_value,
               p_evt->evt_name, p_evt->evt_desc->data, p_evt->evt_numf, p_evt->evt_nume);
     p_evt = p_evt->next;
   }
   printf("\n ");
   fclose(fsmf);
   return 0;
 }

/* ------------------------------------------------------ */
/* restituisce un token alla volta dalla stringa *sp con  */
/* il carattere sep come separatore (una sola istanza)    */
/* conserva i puntatori, reinizializzati se *sp non       */
/* e' nullo alle successive chiamate                      */
/* ------------------------------------------------------ */
int str_split(char *sp, char *token, char sep) {
 static char * p;         // mantiene memoria
 static char * n;         // inizio della stringa

 if ( sp ) {
  p=sp; // non nullo, nuova ricerca
 }

 n=p;                    // inizio token corrente

 p=strchr(n, sep);              // trova il prossimo divisore
 if ( !p ) {                    // non trovato o fine stringa
  // printf("finita la stringa, ultimo token %s\n",n);
  memcpy(token, n, strlen(n));  // tutto il resto e' ultimo token
  return 0;                     // segnala fine stringa
 }
 memcpy(token, n, (char *)p - (char *)n); // lo copia
 p++;                    // bypassa il punto trovato
 return 1;               // trovata istanza, ce ne sono altre
 }

/* -------------------------------------------------------------------------- */
/*   Description: ricerca e ritorna puntatore alla descrizione della          */
/*                sezione richiesta                                           */
/* -------------------------------------------------------------------------- */
char *findsez(int n_sez, st_sm80_sez * p_sez) {
  st_sm80_sez * p = p_sez;
  while ( p ) {
    if ( n_sez == p->sez_num) {
      return p->sez_desc->data;
    }
    else
      if ( n_sez < p->sez_num ) {    // lista ordinata, non serve arrivare in fondo
        break;
      }
    p = p->next;
  }
  return "== ND ==";
}

/**********************************************************************/
/* check_clock : testa minima e massima data/ora per rec SMF          */
/**********************************************************************/
void check_clock(uint32_t smfdt, uint32_t smftm, uint32_t mind[2], uint32_t maxd[2]){
  if ( smfdt > maxd[0] ) {
     maxd[0] = smfdt;
     maxd[1] = smftm;
   }
   else
     if (smfdt == maxd[0] && smftm > maxd[1]) {
       maxd[1] = smftm;
     }

   if (smfdt < mind[0] ) {
     mind[0] = smfdt;
     mind[1] = smftm;
   }
   else
     if (smfdt == mind[0] && smftm < mind[1]) {
       mind[1] = smftm;
     }
  return;
}
