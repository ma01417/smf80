#ifdef __COMPILER_VER__
#pragma filetag("IBM-1140")
#define _AMB_ ZOS
#endif

#ifndef RACSTAT_H
#define RACSTAT_H

#include "racauth.h"   /* RacauthStatus - stesso shape SAF_RC/RACF_RC/RACF_RS gia' usato per RACROUTE AUTH */

/* racstat.h - wrapper Metal C per RACROUTE REQUEST=STAT (SAFPSTAT, request
 * number 14), usato per leggere le voci della Class Descriptor Table (CDT),
 * sia statica che dinamica - a differenza di --class=CDT via R_admin/
 * IRRSEQ00, che restituisce solo le classi della Dynamic CDT. Layout del
 * parameter list trascritto campo per campo da due listing assemblati
 * reali (stessa disciplina di racauth_metal.c - "non indovinare un layout
 * di byte, prendilo da un listato reale"):
 *
 *   - statcdt.lst: RACROUTE REQUEST=STAT,CLASS='OPERCMDS',COPY=(R3),
 *     COPYLEN=(R2),RELEASE=77B0,MF=S - lookup puntuale per nome classe.
 *     ATTENZIONE: questo listato non inizializza mai R2/R3 prima della
 *     macro (nessuna LA/L visibile nel sorgente, confermato dalla Register
 *     Cross Reference: R2/R3 compaiono solo come "M" nelle ST della macro
 *     stessa) - e' un esempio "solo sintassi", usato QUI SOLO come fonte
 *     del layout del parameter list per la forma CLASS= (identico a
 *     statncdt.lst per la parte comune), non come prova che gira cosi'
 *     com'e' - stesso caveat gia' visto per il padding di PROFILE nei
 *     listing FASTAUTH/AUTH.
 *   - statncdt.lst: RACROUTE REQUEST=STAT,COPY=COPYAREA,COPYLEN=COPYLEN,
 *     NEXT=CLASSNM,WORKA=RRWA,RELEASE=77B0,MF=S - scansione completa
 *     (CLASSNM inizializzato a blank = dall'inizio della CDT). Questo e'
 *     un ciclo funzionante e completo (COPYAREA/COPYLEN/CLASSNM caricati
 *     esplicitamente prima del loop), usato come riferimento per:
 *       1. la logica di fine scansione (CNSTLGT, i primi 2 byte
 *          dell'area copiata, a zero = nessuna voce - controllato COSI',
 *          non tramite il SAF RC, nel loop di esempio);
 *       2. la conferma che COPYAREA e COPYCNSX sono UN UNICO buffer
 *          contiguo di 180 byte (CNSTCBLN=28 + CNSXCBLN=152, COPYLEN
 *          EQU *-COPYAREA = 0xB4 = 180) che RACF riempie interamente in
 *          un colpo solo - NON un puntatore CNSTCNSX verso storage
 *          RACF-owned separato (anche se quel puntatore, dentro alla
 *          voce CNST restituita, viene comunque valorizzato da RACF: il
 *          secondo esempio semplicemente lo ignora e indirizza CNSX
 *          direttamente nel proprio buffer subito dopo CNST via LA/USING
 *          statici, provando che il layout contiguo e' affidabile).
 *
 * CLASS= e NEXT= sono mutuamente esclusivi e condividono LO STESSO campo
 * fisico nel parameter list (offset +0x14 della parte RACF-specific,
 * dopo i 0x68 byte del SAF router ICHSAFP): "NEXT parameter set to zero"
 * nel listato CLASS= (statcdt.lst stmt 86), "Address of NEXT parameter"
 * = A(CLASSNM) nel listato NEXT= (statncdt.lst stmt 102) -
 * r_admin_stat_class() sceglie l'uno o l'altro in base a next_name. */

#define RACSTAT_ENTRY_LEN 180   /* CNSTCBLN(28) + CNSXCBLN(152), da statncdt.lst stmt 140/216 */

/* class_name: nome classe (8 byte, blank-padded a destra) per il lookup
 *   puntuale (RACROUTE CLASS=) - ignorato se next_name non e' NULL.
 * next_name: buffer di 8 byte blank-padded, IN/OUT - se non NULL, usa la
 *   modalita' di scansione (RACROUTE NEXT=): in ingresso il nome della
 *   classe da cui proseguire (blank = dall'inizio della CDT); in uscita
 *   RACF lo sovrascrive con il nome della classe restituita (nel loop di
 *   statncdt.lst il chiamante non lo aggiorna mai esplicitamente tra le
 *   iterazioni, quindi deve farlo RACROUTE stesso).
 * copy_area: buffer del chiamante di ESATTAMENTE RACSTAT_ENTRY_LEN byte,
 *   riceve la voce CNST+CNSX unita. I primi 2 byte (CNSTLGT, mappabile
 *   come unsigned short big-endian) sono 0 quando non c'e' nessuna voce
 *   (fine scansione per next_name, non trovata per class_name) - da
 *   controllare SEMPRE, anche quando il SAF RC e' 0.
 * status: SAF_RC/RACF_RC/RACF_RS, stesso shape di RacauthStatus - riempito
 *   sempre per diagnostica, ma NON affidabile per decidere se la chiamata
 *   e' andata a buon fine: confermato dal vivo il 2026-08-04, una
 *   scansione NEXT= restituiva SAFRC=4/RACFRC=0/RACFRS=0 su voci CDT
 *   perfettamente valide (in mezzo ad altre chiamate con SAFRC=0). Il solo
 *   segnale affidabile e' CNSTLGT (i primi 2 byte di copy_area) - vedi
 *   sopra - stessa disciplina del ciclo funzionante in statncdt.lst, che
 *   non controlla mai il SAF RC.
 * Ritorna il SAF RC grezzo, principalmente informativo (vedi sopra) - NON
 * usarlo per decidere se e' stata trovata una voce, controllare sempre
 * CNSTLGT in copy_area. */
#pragma linkage(r_admin_stat_class, OS)
#pragma map(r_admin_stat_class, "RSQFSTAT")
int r_admin_stat_class(const unsigned char class_name[8], unsigned char next_name[8],
                        unsigned char copy_area[RACSTAT_ENTRY_LEN], RacauthStatus *status);

#endif /* RACSTAT_H */
