/*********************************************************************/
/* smf80fmt.h                                                        */
/*   Author: Alessandro Brezzi                                       */
/*   Created: 30/01/2024                                             */
/*********************************************************************/

#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

/*********************************************************************/
/*                                                                   */
/* strutture usate in smf90ext.c per descrivere i record SMF 80 RACF */
/*                                                                   */
/* Types                                                             */
/*********************************************************************/

#ifdef _AMB_
  typedef long time_t;
#endif

// #pragma pack(push)
#pragma pack(1)
/* map out SMF80HDR */
typedef struct {
//  uint16_t SMF80LEN; non accedo a RDW
//  uint16_t SMF80SEG;
  uint8_t  SMF80FLG;
  uint8_t  SMF80RTY;
  uint32_t SMF80TME;
  uint32_t SMF80DTE;
  char     SMF80SID[4];
  uint16_t SMF80DES;        // descriptor flag
  uint8_t  SMF80EVT;        // event code
  uint8_t  SMF80EVQ;        // event qualifier
  char     SMF80USR[8];
  char     SMF80GRP[8];
  uint16_t SMF80REL;        // offset of first relocate sect (-4 -> no RDW)
  uint16_t SMF80CNT;        // number or relocate section
  uint8_t  SMF80ATH;        // authority used for the event
  uint8_t  SMF80REA;        // reason for logging
  uint8_t  SMF80TLV;        // terminal level of foreground user
  uint8_t  SMF80ERR;        // command processing error
  char     SMF80TRM[8];     // terminal id
  char     SMF80JBN[8];     // Job name
  uint32_t SMF80RST;        // Time, hundreds of second, that reader recog. the job
  uint32_t SMF80RSD;        // date that reader recognized the job
  char     SMF80UID[8];     // user identification field
  uint8_t  SMF80VER;        // version indicator
  uint8_t  SMF80RE2;        // additional reason for logging
  uint32_t SMF80VRM;        // Version / release / modification indicator of RACF
  char     SMF80SEC[8];     // security label of the user
  uint16_t SMF80RL2;        // Offset to extended-length rel.sections from SMF80FLG.
  uint16_t SMF80CT2;        // Count of extended-length relocate sections
  uint8_t  SMF80AU2;        // Authority used continued
  uint8_t  SMF80RSV;        // reserved
} SMF80HDR;

/* Relocate Section Variable Data         */
/* map out SMF80DTS data section          */
typedef struct {
  uint8_t  SMF80DTP;        //  data type
  uint8_t  SMF80DLN;        //  length of data that follow
  char     SMF80DTA[255];   //  data
} SMF80DTS;

/* per mappare i dati variabili           */
typedef union smf80_dta {
    char      s001_resource[255];
    char      s002_newname[255];
    uint8_t   s003_accreq;
    uint8_t   s004_accgra;
    // uint8_t   s005_dslev;
    // char      s006_rcmddta[255];
    // char      s007_data[255];
    // char      s008_name[20];
    // char      s009_resname[255];
    // char      s010_volser[7];
    // char      s011_volser2[7];
    // struct    S012_DATA[27];
    // char      s013_resfrom[255];
    // struct {
    //   char    s014_volser[6];
    //   char    s014_fvolume[6];
    // } S014_VOL;
    char      s017_class[8];
    struct s033_prof {
      struct {
        int s033_generic : 1;
        int s033_oldnew_nam : 1;
        int s033_reserved : 6;
      };
      char  s033_res_prof[254];
    };
  } smf80_dta; // mixed case

/* map out SMF80DT2 extended data section */
typedef struct {
  uint16_t SMF80TP2;        //  data type
  uint16_t SMF80DL2;        //  length of data that follow
  char  *  SMF80DA2[];      //  mixed data - variable
} SMF80DT2;

#pragma pack(pop)

// struttura per memorizzare i DTA
typedef struct s_dta {
  int dtp;                    // tipo dati
  int dln;                    // lunghezza dati
  int rec_num;                // record di riferimento (per dbg)
  SMF80DTS  * rel;            // offset dei dati
  struct s_dta * next;        // puntatore a prossima sezione ripetuta
} rel_sect;

// struttura per memorizzare i DA2
typedef struct s_dt2{
  int dt2;                    // tipo dati
  int dl2;                    // lunghezza dati
  int rec_num;                // record di riferimento (per dbg)
  SMF80DT2     * rel2;        // offset dei dati
  struct s_dt2 * next;        // puntatore a prossima sezione ripetuta
} rel_sec2;

/*********************************************************************/
/* Constants                                                         */
/*********************************************************************/
/* compute sizes of sections */
static const int SMF80HDR_SIZE = sizeof(SMF80HDR);
/*********************************************************************/
/* Forward declarations                                              */
/*********************************************************************/
/* mainline support */
