/* ---------------------------------------------------------------- */
/*                                                                  */
/* trim.c   funzione di trim con le funzioni rtrim (right trim)     */
/*          per eliminare spazi al termine della stringa e          */
/*          ltrim (left trim) per eliminare spazi all'inizio        */
/*          della stringa                                           */
/*                                                                  */
/* ---------------------------------------------------------------- */
/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern char *rtrim(char *s);
extern char *ltrim(char *s, int ll);

// trim a string both sides
extern char *trim(char *s, int ll) {

    // elimina spazi finali
    s = rtrim(s);
    // elimina spazi iniziali
    s = ltrim(s, ll);

    return s;
}
/* ---------------------------------------------------------------- */
/* rtrim    funzione di trim degli spazi finali della stringa       */
/*          data                                                    */
/* ---------------------------------------------------------------- */
// trim trailing spaces
extern char *rtrim(char *s) {
    char *ptr;
    int str_l;

    // se stringa nulla o vuota non fa nulla
    if (!s)
      return NULL;   // handle NULL string
    if (!*s)
      return s;      // handle empty string

    str_l = strlen(s) - 1;
    for ( (ptr = s + str_l); (ptr >= s) && (isspace((unsigned char)*ptr)); --ptr);

    ptr[1] = '\0';

    return s;
}
/* ---------------------------------------------------------------- */
/* ltrim    funzione di trim degli spazi iniziale della stringa     */
/*          fornita                                                 */
/* ---------------------------------------------------------------- */

// trim leading spaces
// extern char *ltrim(char *s) {
//     char *ptr;
//     int str_l;

//     // se stringa nulla o vuota non fa nulla
//     if (!s)
//       return NULL;   // handle NULL string
//     if (!*s)
//       return s;      // handle empty string

//     str_l = strlen(s) - 1;

//     for ( (ptr = s); (ptr <= s + str_l) && (isspace((unsigned char)*ptr)); ++ptr);
//     s = ptr;
//     return s;
// }
/*
 * analizza_stringa
 * -----------------
 * Rimuove gli spazi iniziali (solo quelli, non quelli interni o finali)
 * da una stringa, spostando il resto del contenuto all'inizio del buffer
 * (modifica il buffer "in place" e ne restituisce il puntatore).
 *
 * Parametri:
 *   s         - puntatore al buffer da elaborare. Può essere NULL.
 *   ll        - numero massimo di caratteri da considerare a partire
 *               dall'inizio del buffer. Serve a delimitare l'elaborazione
 *               anche quando il buffer non contiene un terminatore '\0'
 *               entro quel numero di caratteri (es. buffer non
 *               null-terminated, oppure per troncare volutamente una
 *               stringa più lunga).
 *
 * Valore restituito:
 *   Il puntatore s stesso (buffer modificato), oppure NULL se s
 *   era NULL.
 *
 * Nota sulla sicurezza del terminatore finale:
 *   Il ciclo di copia si ferma o perché ha incontrato un '\0' entro
 *   "ll" caratteri (caso normale), oppure perché ha raggiunto
 *   il limite "ll" senza trovare un '\0' (il buffer non è
 *   terminato, o lo è oltre il limite indicato).
 *   In quest'ultimo caso scrivere un '\0' in coda sarebbe potenzialmente
 *   fuori dai limiti del buffer (se non sono stati rimossi spazi
 *   iniziali non c'è nessuno spazio "liberato" dallo shift a sinistra).
 *   Per questo il terminatore viene scritto solo se:
 *     - il ciclo si è fermato per aver trovato un '\0' reale (sempre
 *       sicuro, la posizione di scrittura è <= alla posizione del '\0'
 *       originale), oppure
 *     - sono stati rimossi spazi iniziali (inizio > 0): lo shift a
 *       sinistra libera esattamente "inizio" byte in coda al contenuto
 *       utile, quindi scrivere il terminatore in quella posizione
 *       rimane sempre dentro i limiti originali del buffer.
 *   Se il ciclo si ferma per raggiunto limite di lunghezza E non sono
 *   stati rimossi spazi iniziali, il terminatore NON viene scritto per
 *   evitare una scrittura fuori dai limiti del buffer.
 */
extern char *ltrim(char *s, int ll) {
    if (s == NULL) {
        return NULL;
    }

    /* Fase 1: individua la posizione del primo carattere non-spazio,
       senza superare "ll". "inizio" sarà il numero di spazi
       iniziali da saltare. */
    int inizio = 0;
    while (inizio < ll && s[inizio] == ' ') {
        inizio++;
    }

    /* Fase 2: copia (shift a sinistra) tutto il contenuto a partire da
       "inizio" verso la posizione 0 del buffer, fermandosi al primo
       '\0' incontrato oppure al raggiungimento di "ll".
       "scrittura" conta quanti caratteri sono stati effettivamente
       copiati (= lunghezza della stringa risultante, terminatore
       escluso). "lettura" tiene traccia della posizione di lettura nel
       buffer originale, e il suo valore finale serve a capire perché
       il ciclo si è fermato (vedi Fase 3). */
    int scrittura = 0;
    int lettura = inizio;
    while (lettura < ll && s[lettura] != '\0') {
        s[scrittura] = s[lettura];
        scrittura++;
        lettura++;
    }

    /* Fase 3: decide se e dove scrivere il terminatore '\0' finale.
       fine_per_lunghezza è vero se il ciclo si è fermato perché ha
       raggiunto "ll" (non perché ha trovato un '\0' reale).
       spazi_iniziali_rimossi è vero se sono stati saltati spazi
       iniziali (inizio > 0), il che garantisce che ci sia spazio
       libero nel buffer per il terminatore senza sforare i limiti. */
    int fine_per_lunghezza = (lettura == ll);
    int spazi_iniziali_rimossi = (inizio > 0);

    if (!fine_per_lunghezza || spazi_iniziali_rimossi) {
        s[scrittura] = '\0';
    }

    return s;
}