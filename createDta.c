/* -------------------------------------------------------------------------- */
/*                                                                            */
/*   Description: constructor per struttura contenente dati sezioni           */
/*                relocabili rec SMF 80, sia normali che extended             */
/*                                                                            */
/* -------------------------------------------------------------------------- */

// constructor struttura DTA
rel_sect *createDta(const SMF80DTS *p_dts) {
  rel_sect *new_dta = malloc(sizeof(*new_dta));
  if ( new_dta ) {
    new_dta->dtp = p_dts->SMF80DTP;
    new_dta->dtl = p_dts->SMF80DLN + 2;
    new_dta->rel = p_dts;
    }
    return new_dta;
}

// constructor struttura DT2
rel_sect2 *createDt2(const SMF80DT2 *p_dt2) {
  rel_sect2 *new_dt2 = malloc(sizeof(*new_dt2));
  if ( new_dt2 ) {
    new_dt2->dt2  = p_dt2->SMF80TP2;
    new_dt2->dl2  = p_dt2->SMF80DL2 + 2;
    new_dt2->rel2 = p_dt2;
    }
    return new_dt2;
}

