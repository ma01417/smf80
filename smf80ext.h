/* ****************************************************************** */
/* include di supporto per programma di filtro record SMF 80          */
/* contenente tutte le tipologie di dati e strutture usate            */
/*                                                                    */
/* A.Brezzi giugno 2024                                               */
/* ****************************************************************** */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

#include <time.h>       /* for clock_t, clock(), gmtime() ... */

#define PARNUM 11    //  numero di filtri ammessi

/*********************************************************************/
/* Defines                                                           */
/*********************************************************************/
/* number of seconds to Jan 1 1970, gmtime use 1 jan 1970 as base date */
#define  _sec_70  2208988800;       /* number of seconds to Jan 1 1970 */

#define MAX_TK_LL 45      // lunghezza+1 delle stringhe filtro massima
#define MAX_TK_ELE 100    // numero massimo di elementi da filtrare
#define MAX_LL 82         // lunghezza + 2 (EOL e EOS) righe da PDS

// per test su flag booleani
#define TRUE  1
#define FALSE 0

// macro to determine the number of elements of an array
#define NUMELE(x) (sizeof x / sizeof *x)
// macros to give the maximum and minimum between two integer
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define MIN(x,y) (((x) < (y)) ? (x) : (y))

// points to and returns next character in string
#define ADVANCE_CHAR(s) (*(++(*(s))))
// macro per fornire indice base vettore puntatori a struttura per DT2
#define IDT2(x) (x - 256)

/*********************************************************************
 * define some useful unsigned int of varyng size                    *
 *********************************************************************/
typedef unsigned long long *ull;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

// definiziome che rimpiazza la standard timespec in errore
typedef struct timespc
    {
    time_t tv_sec;
    long   tv_nsec;
    };

/* ----------------------------------------------------------- */
/* struttura per costruire linked list di stringhe             */
/* a lunghezza variabile                                       */
/* ----------------------------------------------------------- */
struct string {
  size_t size;
  char *data;
};

/* ------------------------------------------------------------ */
/* struttura per contenere elenco sezioni rilocabili            */
/* dei rec 80 SMF                                               */
/* ------------------------------------------------------------ */
typedef struct lk_sez_elem {
  int     sez_num;
  struct string * sez_desc;
  struct lk_sez_elem * next;
} st_sm80_sez;

/* ------------------------------------------------------------ */
/* struttura per contenere descrizione e dati Eventi SMF RACF   */
/* ------------------------------------------------------------ */
typedef struct lk_evt_elem {
  uint8_t evt_value;
  char    evt_name[9];
  long    evt_numf;
  long    evt_nume;
  struct string * evt_desc;
  struct lk_evt_elem * next;
} st_sm80_evt;

/* ------------------------------------------------------------ */
/* struttura per contenere Event Qualifier relativi ad eventi   */
/* ------------------------------------------------------------ */
typedef struct lk_evq_elem {
  uint8_t evq_value;
  char    evq_name[9];
  struct string * evq_desc;
  struct lk_evq_elem * next;
} st_sm80_evq;

/* ------------------------------------------------------------ */
/* definizione struttura per descrittori classi RACF            */
/* ------------------------------------------------------------ */
typedef struct lk_cls_elem {
  char    cls_name[9];        // nome della classe
  int     cls_maxL;           // massima lunghezza profili
  int     cls_act;            // flag 1/0 classe attiva o no
  int     cls_ope;            // flag 1/0 classe onora OPERATIONS
  int     cls_mix;            // flag 1/0 profili in mixed case
  int     cls_typ;            // flag 1/0 classi grouping
  struct string * cls_member; // Member class
  struct string * cls_RC;     // default RC
  struct string * cls_desc;   // stringa descrittiva della classe
  struct lk_cls_elem * next;
} st_sm80_cls;

/* ----------------------------------------------------------- */
/* definizione della lista concatenate per i filtri            */
/*    -- modificare con UNION per i tre tipi                   */
/*       e quindi una sola routine di carico e di ricerca      */
/* ----------------------------------------------------------- */
typedef struct lk_list_filter {
  char * filter_name;          // token ottenuto da file parametri
  uint32_t filter_namnum;       // quando necessario contiene valore
  struct string * filter_desc; // eventuale descrizione
  struct lk_list_filter * next;
} list_filter;

/* ----------------------------------------------------------- */
/* definizione della struttura per i parametri, pointer in     */
/* vettore runparm                                             */
/* contiene pointer al primo filtro nella linked list          */
/* ----------------------------------------------------------- */
typedef struct lk_list_param {
  char    param_name[9];       // nome del parametro da file
  uint8_t param_namnum;        // codice interno parametro
  char    param_oper[4];       // operatore da file
  uint8_t param_openum;        // codice interno operatore
  int     param_filt_type;     // o per stringa, 1 per numero, 2 per numero in AND
  list_filter  * param_filter; // linked list dei filtri richiesti
  int     param_num_f;         // numero di filtri presenti
  struct string * param_desc;  // descrizione del parametro
  struct lk_list_param * next;
} list_parameter;

/* ensemble of flags to represent section to be found */
typedef struct fl_section {
  /* sections */
  uint8_t header;
  uint8_t res;
  uint8_t class;
  uint8_t profile;
  uint8_t usrjes;
} SMF80SEC_FL;

// definizione della struttura per parametri elaborazione
typedef struct RUNPARM
 {
  // strutture caratteristiche da filtrare ; puntatore primo nodo liste
  list_parameter * root_param[PARNUM];  // array con puntatori ai nodi dei parametri
  struct fl_section smf80sec_fl;        // flag per le sezioni da elaborare
 } runparm;

/* ----------------------------------------------------------- */
/*  funzioni di supporto inserite in libreria libmyext.a       */
/* ----------------------------------------------------------- */
extern int              chkparm(runparm *p_param, st_sm80_cls *root_cls, st_sm80_evt *root_evt, char *fileCNTL); // check logici su insieme dei parametri
extern struct string  * createStr(const char *initial);                                           // constructor for var.length string
extern list_filter    * crfilter(uint32_t value, char *name, char *desc);                         // constructor per nodo filtri
extern list_parameter * crparm(char *name, int num, char *oper, uint8_t opnum, char *desc);       // constructor per nodo parametri
extern st_sm80_evt    * findevt(const char *name, st_sm80_evt *root_evt);                         // find event in the linkedlist
extern list_parameter * findparm(char *name, uint8_t num, list_parameter *root_parameter);        // restituisce putatore nodo par. per nome o valore
extern char           * getlrow( char * buff, FILE * parm );                                      // lettura di una riga logica di parametri
extern int              gettrow(char *buff, runparm *p_param);                                    // tokenizza la riga logica
extern st_sm80_cls    * getCls(char *filename, char *member);                                     // get linkedlist of RACF Classes
extern st_sm80_evq    * getEvq(char *filename, char *member, char *d_evt);                        // get linkedlist of event qual for an event
extern st_sm80_evt    * getEvt(char *filename, char *member);                                     // get linkedlist of events
extern char           * getExt(const char *path_name, char *ext);                                 // get file name and extention
extern runparm        * getParm(char *filePARM, char *fileCNTL, st_sm80_evt *root_evt) ;          // scan del file parametri con decodifica filtri
extern void             hexprt(FILE *fdmp, char *head, char *src, int ll, char *pgm_name);        // stampa in esadecimale
extern char           * ltrim(char *s, int ll);                                                   // trim leading blanks (ll = lunghezza massima da considerare)
extern int              makeargv(char *string, char *argv[], int argvsize, int max_argc);         // tokenize a string
extern FILE           * openf(const char *filename, const char *open_parm, const char *op_desc, const char * pgmname); // open a file
extern char           * rtrim(char *s);                                                           // trim trailing blanks
extern void             strup(char *s, char *u);                                                  // translate to upper case a string
extern char           * trim(char *s, int ll);                                                    // trim trailing & leading blanks (ll = lunghezza massima da considerare)
extern st_sm80_sez    * getSez(char *filename, char *member);                                     // get linkedlist of relocale sections
extern int              fltparm(list_parameter *p_par, char *name, uint32_t value);               // check di valori con filtri
extern char           * format_smftime(char * buffer, uint32_t smftime);                          // format ora in 1/100s rec SMF
extern void             stck_tm(char *pData, struct timespc *pTimespc);                           // Conv STCK in tempo da 1/1/1970 (UNIX time)
extern char           * format_smfdate(char * buffer, uint32_t smfdate);                          // format data SMF 0x0cyydddF
