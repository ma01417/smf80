#ifdef __COMPILER_VER__
#pragma filetag("IBM-1140")
#define _AMB_ ZOS
#endif

#ifndef IRRCDT_H
#define IRRCDT_H

#include "irrcseq.h"   /* RacseqProfile/RacseqStatus/RacseqRC - forma di
                         * output richiesta, vedi sotto */

/* irrcdt.h - pseudo-classe "_CDT": RACROUTE REQUEST=STAT (Class Descriptor
 * Table, statica+dinamica) instradato dentro la STESSA forma di output
 * (RacseqProfile) usata da R_admin/IRRSEQ00, cosi' che il resto del
 * programma (es. testrseq.c) non debba distinguere i due casi: e' per
 * questo che questo modulo dipende da irrcseq.h invece di essere del
 * tutto autonomo come irrauth.h - non puo' esistere una versione
 * "ridotta" che non conosca RacseqProfile/RacseqSegment/RacseqField,
 * dato che e' proprio quello il formato che deve produrre.
 *
 * Isolato in un proprio file perche' la logica di parsing CNST/CNSX
 * (RACROUTE STAT) e' indipendente dal parsing ADMN_PROF_MAP di R_admin -
 * vedi irrcseq.c racseq_extract()/racseq_extract_next(), che instradano
 * qui in base a class_name=="_CDT" tramite irrcdt_is_pseudo_class().
 *
 * Documentazione d'uso completa (semantica dei campi del profilo "_CDT"
 * risultante, convenzioni di class_name/profile_name) nei commenti di
 * racseq_extract()/racseq_extract_next() in irrcseq.h - qui solo le
 * firme delle funzioni di instradamento interne, richiamate da
 * irrcseq.c. */

int irrcdt_is_pseudo_class(const char *class_name);

/* class_name_arg: nome della CLASSE RACF da cercare nella CDT (RACROUTE
 * CLASS=), non un profilo - vedi racseq_extract() in irrcseq.h. */
RacseqRC irrcdt_extract(const char *class_name_arg, RacseqProfile *out, RacseqStatus *status);

/* start_after_name/previous->profile_name: nome dell'ultima CLASSE
 * restituita (RACROUTE NEXT=), non un profilo - vedi racseq_extract_next()
 * in irrcseq.h. Consuma `previous` esattamente come racseq_extract_next()
 * stessa (la libera internamente, anche se previous coincide con out). */
RacseqRC irrcdt_extract_next(const char *start_after_name, RacseqProfile *previous,
                              RacseqProfile *out, RacseqStatus *status);

#endif /* IRRCDT_H */
