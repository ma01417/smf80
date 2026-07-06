/*********************************************************************/
/* smf81fmt.h                                                        */
/*   Author: Alessandro Brezzi                                       */
/*   Created: 22/06/2026                                             */
/*********************************************************************/

#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

/*********************************************************************/
/*                                                                   */
/* strutture usate in smf81dec.c per descrivere i record SMF 81 RACF */
/*                                                                   */
/* Types                                                             */
/*********************************************************************/

#ifdef _AMB_
  typedef long time_t;
#endif

// #pragma pack(push)
#pragma pack(1)
/* map out SMF81HDR */
typedef struct {
//  uint16_t SMF81LEN; non accedo a RDW
//  uint16_t SMF81SEG;
  uint8_t  SMF81FLG;
  uint8_t  SMF81RTY;
  uint32_t SMF81TME;
  uint32_t SMF81DTE;
  char     SMF81SID[4];
  char     SMF81RDS[44];    // RACF DB name at IPL
  char     SMF81RVL[6];     // volume of DB RACF
  char     SMF81RUN[3];     // unit name
  char     SMF81UDS[44];    // UADS name at IPL
  char     SMF81UVL[6];     // UADS volume name
  uint8_t  SMF81OPT;        // options indicator
  uint8_t  SMF81OP2;        // options indicator 2
  uint8_t  SMF81OP3;        // options indicator 3
  uint8_t  SMF81AOP;        // audit options
  uint8_t  SMF81AO2;        // audit options 2
  uint8_t  SMF81TMO;        // terminal verification options indicator
  uint8_t  SMF81PIV;        // maximum pwd interval 0-254
  uint16_t SMF81REL;        // offset of the first relocate section
  uint16_t SMF81CNT;        // number of relocate sections
  uint8_t  SMF81VER;        // version indicator; as of 1.8.1 SMF81VRM is used instead
  char     SMF81QL[8];      // single level data set name
  uint8_t  SMF81OP4;        // options indicator 4
  uint8_t  SMF81OP5;        // options indicator 5
  uint16_t SMF81RPD;        // System retention period in effect
  uint8_t  SMF81SLV;        // Security level for ERASE-ON-SCRATCH in effect
  uint8_t  SMF81SLC;        // Security level for auditing in effect
  char     SMF81VRM[4];     // FMID for RACF
  uint8_t  SMF81BOP;        // SETROPTS options
  uint16_t SMF81SIN;        // Partner LU-verification session key interval
  char     SMF81JSY[8];     // JES NJE NAME user ID
  char     SMF81JUN[8];     // JES UNDEFINEDUSER user ID
  uint8_t  SMF81BOX;        // SETROPTS option extensions
  char     SMF81PRI[3];     // Default primary language for an installation
  char     SMF81SEC[3];     // Default secondary language for an installation
  uint8_t  SMF81KBL;        // Level of KERB segment processing in effect
  int8_t   SMF81PMN;        // Minimum days between password changes (signed)
  uint8_t  SMF81OP6;        // options indicator 6
  uint8_t  SMF81ML2;        // More SETROPTS options
  uint8_t  SMF81ALG;        // Password encryption algorithm in effect
  char     SMF81VXC[8];     // VMXEVENT control profile is in effect
  char     SMF81VXA[8];     // VMXEVENT audit profile is in effect
  uint16_t SMF81PHI;        // Password phrase interval
  char     SMF81reserved[55];
} SMF81HDR;

/* Relocate Section Variable Data         */
/* map out SMF80DTS data section          */
typedef struct {
  uint8_t  SMF81DTP;        //  data type
  uint8_t  SMF81DLN;        //  length of data that follow
  char     SMF81DTA[255];   //  data
} SMF81DTS;

/* map out SMF81DTS data section RACF DB  */
//   ..{UCBSZOSM2.SYS1.J000.RACF.BKUP
//   0....+....1....+....2....+....3.
//   11CECCEEDEDF0EEEF4DFFF4DCCC4CDED
//   EE043229624212821B1000B9136B2247
typedef struct {
  uint8_t  SMF81S30_TP;      //  data type 30
  uint8_t  SMF81S30_DL;      //  length of data that follow
  uint8_t  SMF81S30_FL;      //  flag bit meaning:
                             //    0  active if set
                             //    1  BKUP if set, PRIM otherwise
  char     SMF81S30_UN[3];   //  unit name
  char     SMF81S30_VL[6];   //  volume name
  uint8_t  SMF81S30_NM;      //  DS seq number
  char     SMF81S30_DS[44];  //  DS name: warning not all bytes allocated
                             //    effective length = SMF81S30_DL -11
} SMF81S30;

// map out SMF81DTS data section CLASSES
// smf81dec ---------- Rel 21('15') 2026-06-09 ----------
//     0 Head ..ÜDATASET .
//            0....+....1....+....2....+....3....+....4....+....5....+....6....+....7....+....
//          h 10FCCECECE40
//          l 5AC413125304
typedef struct {
  uint8_t  SMF81S21_TP;      //  data type 30
  uint8_t  SMF81S21_DL;      //  length of data that follow
  uint8_t  SMF81S21_FL;      //  flag bits meaning:
                             //    0 Stats enabled
                             //    1 Audit enabled
                             //    2 Active
                             //    3 Grouping ??
  char     SMF81S21_CL[8];   //  class name
  uint8_t  SMF81S21_F2;      //  flag bits meaning:
                             //    1 Always log
                             //    5 Profile log
} SMF81S21;

#pragma pack(pop)

/*********************************************************************/
/* Constants                                                         */
/*********************************************************************/
/* compute sizes of sections */
static const int SMF81HDR_SIZE = sizeof(SMF81HDR);
/*********************************************************************/
/* Forward declarations                                              */
/*********************************************************************/
/* mainline support */
