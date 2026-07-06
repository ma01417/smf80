/* -------------------------------------------------------------------------- */
/*                                                                            */
/* fmtDtTm.c                                                                  */
/*   (c) 2024 A.Brezzi                                                        */
/*                                                                            */
/*   Description:                                                             */
/*     funzioni di decodifica delle date / orari presenti nel record          */
/*     SMF                                                                    */
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

/*********************************************************************
 * define some useful unsigned int of varyng size                    *
 *********************************************************************/
typedef unsigned long long *ull;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

/* Converte time da centesimi di secondi passati dalla mezzanotte */
/* in stringa leggibile della forma hh:mm:ss                      */
extern char *format_smftime(char * buffer, uint32_t smftime) {
  unsigned int seconds;
  unsigned int minutes;
  unsigned int hours;
  /* convert hundreds of seconds into seconds */
  seconds = smftime / 100;
  /* compute number of hours since midnight */
  hours = seconds / 3600;
  seconds -= 3600 * hours;
  /* compute number of minutes since start of hour */
  minutes = seconds / 60;
  /* compute number of seconds since start of minute */
  seconds -= 60 * minutes;
  /* format time string into given buffer */
  snprintf(buffer, 16, "%02.2d:%02.2d:%02.2d.%02.2d",
    hours, minutes, seconds, smftime % 100);
  return buffer;
}

/* funzione per convertire STCK nel tempo trascorso dal primo     */
/* gennaio 1970 per sfruttare le funzioni UNIX time per la        */
/* conversione                                                    */
// correggere definzione di timespec che non funziona
extern void stck_tm(char *pData, struct timespc *pTimespc)
  {
    unsigned long long stck;
    long  m_sec;
    stck  = *(ull) pData;
    stck  = stck/4096;      /* 4096 for stck to get microseconds in bottom bit */
    m_sec = stck%1000000;   /* save microseconds                               */
    stck  = stck/1000000;   /* save seconds from microseconds                  */
    stck  = stck - _sec_70; /* subtract seconds from 1 Jan '900 to 1 Jan '970  */
    pTimespc->tv_sec  = stck;  /* put it in the structure                      */
    pTimespc->tv_nsec = m_sec;
    return;
  }

/* funzione per convertire una data formato SMF 0x0CYYDDDF in      */
/* formato leggibile YYYY/MM/DD                                    */
/* 0C  ::= secolo, 0 1900, 1 2000 ....                             */
/* YY  ::= anno su due cifre                                       */
/* DDD ::= giorno dall'inizio dell'anno (julian)                   */
extern char *format_smfdate(char * buffer, uint32_t smfdate) {

  unsigned int year;
  unsigned int month;
  unsigned int day;

  int month_days[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

  /* convert hex year into decimal */
  year = 1900 + 100 * ((smfdate >> 24) & 0x0F) +
                 10 * ((smfdate >> 20) & 0x0F) +
                  1 * ((smfdate >> 16) & 0x0F);

  /* convert hex day into decimal */
  day = 100 * ((smfdate >> 12) & 0x0F) +
         10 * ((smfdate >>  8) & 0x0F) +
          1 * ((smfdate >>  4) & 0x0F);

  /* convert julian date into sane date                       */
  /* if it is multiple of 4 but not of 100 or multiple of 400 */
  /*        -> leap year                                      */
  if (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0))
    month_days[1] += 1;

  for (month = 0; (day > month_days[month]) && (month < 12); month++)
    day -= month_days[month];

  /* format date string into given buffer */
  snprintf(buffer, 16, "%02.2d/%02.2d/%02.2d", year, 1 + month, day);

  return buffer;
}

/* C implementation of digital clock  */
extern void get_cl_time(char * t[])
{
  time_t s;
  struct tm* current_time;

  /* time in seconds      */
  s = time(NULL);

  /* to get current time  */
  current_time = localtime(&s);

  /* print time in minutes,  */
  /* hours and seconds       */
  sprintf(t, "%02d:%02d:%02d",
    current_time->tm_hour,
    current_time->tm_min,
    current_time->tm_sec);
  return;
}
