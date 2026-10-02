/* ========================================================================== */
/*                                                                            */
/*   get_CDT.h                                                                */
/*   (c) 2026 A.Brezzi                                                        */
/*                                                                            */
/*   Description: prototipi e strutture usate da get_CDT.c                    */
/*                                                                            */
/*                                                                            */
/* ========================================================================== */

/* Per utilizzare correttamente parentesi quadre graffe e tilde     */
#ifdef __COMPILER_VER__
 #pragma filetag ("IBM-1140")
 #define _AMB_ ZOS
#endif

/* text_field_bool() - vero se il campo TESTUALE (non booleano) `f` vale
 * esattamente `yes_value`. Serve per ACTIVE, che e' testo "YES"/"NO" e
 * non un RacseqField booleano vero e proprio (cdt_active_text() in
 * irrcseq.c lo deriva da SAFRC/RACFRC, non da un bit di CNST/CNSX). */
static int text_field_bool(const RacseqField *f, const char *yes_value);

/* dup_field_text() - copia un campo testuale in una stringa malloc'd e
 * NUL-terminata, posseduta dal CHIAMANTE. A differenza di
 * RacseqField.value (che punta dentro raw_buffer e diventa non valido
 * dopo racseq_free_profile()/dopo che racseq_extract_next() consuma
 * `previous`), i campi char* di st_sm80_cls (cls_member/cls_desc)
 * sopravvivono al profilo - vanno quindi COPIATI qui, mai referenziati
 * direttamente. Ritorna NULL se il campo non esiste o e' vuoto (XREF e'
 * spesso vuoto per le classi che non sono ne' grouping ne' member). */
static char *dup_field_text(const RacseqField *f);

/* populate_cls_elem() - riempie `node` (gia' allocato dal chiamante) con
 * i campi del profilo _CDT `p` (racseq_extract()/racseq_extract_next()
 * con class_name="_CDT", non ancora passato a racseq_free_profile()).
 * Mapping verso le colonne di Tabella 5.1 (irrcseq_specifiche.docx):
 *   cls_name   <- p->profile_name (nome della classe RACF - e' gia' sulla
 *                 RacseqProfile, non un campo del segmento CDT)
 *   cls_maxL   <- MAXLEN (numerico)
 *   cls_act    <- ACTIVE ("YES"/"NO", testo)
 *   cls_ope    <- OPERATTR (booleano, CNSTMFLG bit CNSTOPER 0x20)
 *   cls_mix    <- PRESCASE (booleano, CNSTFLG0 bit CNSTCASE: nomi profilo
 *                 con maiuscole/minuscole preservate = mixed case)
 *   cls_typ    <- RESGROUP (booleano, CNSTMFLG bit CNSTRGRP: la classe e'
 *                 essa stessa una resource group/grouping class)
 *   cls_member <- XREF (testo, nome della classe collegata - copiato con
 *                 dup_field_text())
 *   cls_RC     <- DEFRC (numerico)
 *   cls_desc   <- nessun campo descrittivo nella CDT stessa: lasciato a
 *                 NULL (e' facoltativo per definizione nella struttura) */
static void populate_cls_elem(const RacseqProfile *p, st_sm80_cls *node)

/* build_cdt_class_list() - scansiona l'intera CDT (statica+dinamica) e
 * costruisce la linked list st_sm80_cls, un nodo per classe, in testa
 * (ordine inverso di scansione). E' uno stand-in per il costruttore REALE
 * del programma ospite (non noto a questo esempio): mostra pero' l'intero
 * ciclo di vita end-to-end, incluso il punto critico gia' segnalato in
 * dup_field_text() - populate_cls_elem() copia cio' che serve PRIMA che
 * la prossima racseq_extract_next() consumi/liberi `p`. */
static st_sm80_cls *build_cdt_class_list(void)

