/* ****************************************************************** */
/* include di supporto per programma di filtro record SMF 80          */
/* e programma di parsing dei parametri passati                       */
/*                                                                    */
/* A.Brezzi giugno 2024                                               */
/* ****************************************************************** */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

// enumerazione degli operatori ammessi
enum {EQ,NE,ABR} operatore;
// lista degli operatori logici ammissibili e stringa descrittiva
const char l_oper[][4] = {"=","<>","ABR"};
const char d_oper[][14] = {"uguale a","diverso da", "abbreviato da"};

// enumerazione dei filtri ammissibili
enum {EVENT,DESCR,USER,RESULT,JOBNAM,REASN,AUTH,RES,USRJES,CLASS,PROF} filtro;
// lista campi ammessi per i filtri
const char l_parm[][9] = {"EVENT","DESCR","USER","RESULT","JOBNAM","REASN","AUTH","RES","USRJES","CLASS","PROF"};
// flag per indicare se campo con filtro su stringa (0), numerico (1), numerico in AND (2)
const int l_parm_num[] = {1, 2, 0, 1, 0, 2, 2, 0, 0, 0, 0};
// lunghezze massime dei valori attesi nei parametri
const uint8_t l_max_ll[]={8,9,8,8,8,10,10,44,8,8,44};
// sezione corrispondente per reperire il valore corrente
const uint8_t l_sez[] = {0,0,0,0,0,0,0,1,1,17,33};

// lista valori description ricercabili e corrispondenti valori esadecimali
const char l_descr[][10] = {"VIOLATION", "USRNOTDEF", "WARNING"};
const uint16_t val_descr [] = {0x8000, 0x4000, 0x0100};
const char lddescr[][30] = {"The event is a violation",
                            "User is not defined to RACF",
                            "The event is a warning"};
// lista ragioni per il loggin (REASN) e corrispondenti valori esadecimali
const char l_reasn[][11] = {"CLAUDIT", "UAUDIT", "SPECIAL", "OPERATIONS", "PRAUDIT",
                            "VERFAIL", "ALWAYS", "CMDVIOL", "GLOBAUDIT"};
const uint8_t val_reasn [] = {0x80, 0x40, 0x20, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
const char l_dreasn[][58]={"Changes to this class of profile are being audited",
                           "User being audited",
                           "SPECIAL (or OPERATIONS) user being audited",
                           "OPERATIONS (or SPECIAL) user being audited",
                           "Resource audited due to the AUDIT option",
                           "RACROUTE REQUEST=VERIFY or initACEE failure",
                           "This command is always audited",
                           "Violation detected in command and CMDVIOL is in effect",
                           "Access to entity being audited due to GLOBALAUDIT option"};

// lista attributi autorizativi (AUTH) e corrispondenti valori esadecimali
const char l_auth[][11]= {"NORMAL", "SPECIAL", "ROAUDIT", "OPERATIONS", "AUDITOR",
                          "INSTEXIT", "FAILSOFT", "UBYPASS", "TRUSTED"};
const uint8_t val_auth [] = {0x80, 0x40, 0x20, 0x020, 0x10, 0x08, 0x04, 0x02, 0x01};
const char l_dauth[][35]={"Nessun privilegio","Amm.re RACF - SPECIAL","Accesso ai profili, no modifiche",
                         "Accesso ai dati su disco/nastro","Auditor RACF","Autorizzato vi EXIT RACF",
                         "RACF non attivo/disponibile","User *BYPASS* in RACROUTE",
                         "User di STC con attributo TRUSTED"};

