//TSTDSN   JOB (ACCT),'TEST GET_DSNAME',CLASS=A,MSGCLASS=X,
//             NOTIFY=&SYSUID
//*--------------------------------------------------------------------
//* Prova di get_DSName(): sostituire <MyPrefix>, <UserId> e i nomi
//* dei data set con valori esistenti sul proprio sistema.
//* Casi coperti: concatenazione di 3 data set, DUMMY, membro di PDS,
//* SYSOUT, PATH, DD non allocata (NONALLOC).
//*--------------------------------------------------------------------
//ST010    EXEC PGM=TSTDSN,
//             PARM='/UTI001 UTIDEC CNTLMEM OUTSYS MYPATH NONALLOC'
//STEPLIB  DD DISP=SHR,DSN=<MyPrefix>.LLIB
//UTI001   DD DISP=SHR,DSN=<MyPrefix>.SMF.DATA1
//         DD DISP=SHR,DSN=<MyPrefix>.SMF.DATA2
//         DD DISP=SHR,DSN=<MyPrefix>.SMF.DATA3
//UTIDEC   DD DUMMY
//CNTLMEM  DD DISP=SHR,DSN=<MyPrefix>.CNTL(SM80EVNQ)
//OUTSYS   DD SYSOUT=*
//MYPATH   DD PATH='/u/<UserId>/result.log',PATHOPTS=(ORDONLY)
//SYSPRINT DD SYSOUT=*
//SYSOUT   DD SYSOUT=*
//CEEDUMP  DD SYSOUT=*
