# --------------------------------------------------------------- #
#                                                                 #
# Makefile per compilare smf80ext.c                               #
#                                                                 #
# A.Brezzi giugno 2024                                            #
#                                                                 #
# Uso:                                                            #
#  make -f smf80ext.mk                           - solo modifiche #
#  make -p -f smf80ext.mk [>makeout.txt]         - listing make   #
#  make -u -f smf80ext.mk                        - unconditional  #
#  make -f smf80ext.mk clean                     - pulizia        #
#                                                                 #
#  Nelle regole si possono aggiungere le variabili sotto          #
#  illustrate                                                     #
#  Variabili                                                      #
#   AGGR=-Wc,AGGREGATE                                            #
#    nel listing produce una tabella con gli offset dei campi     #
#    nelle struct e nelle union                                   #
#   DBG=-g                                                        #
#    crea file .dbg per il debugger; attenzione: se usata non     #
#    consente la ottimizzazione del codice                        #
#                                                                 #
#  NOTA: make non rileva il cambio di variabili/opzioni: quando   #
#  si attiva o disattiva DBG o AGGR usare -u, es.                 #
#    make -u -f smf80ext.mk DBG=-g                                #
#                                                                 #
# --------------------------------------------------------------- #
# Elenco sorgenti C nativo
SRCS    = strup.c \
          trim.c \
          openf.c \
          createStr.c \
          getExt.c \
          hexprt.c \
          fmtDtTm.c \
          getEvt.c \
          getEvq.c \
          get_CDT.c \
          getSez.c \
          makeargv.c \
          crfilter.c \
          fltparm.c \
          getlrow.c \
          findevt.c \
          crparm.c \
          gettrow.c \
          chkparm.c \
          getParm.c \
          creDTA_2.c \
          irrauth.c \
          irrcdt.c \
          smf81dec.c
# dipendenze per i sorgenti C
DEPS    = $(SRCS:.c=.u)
#
METAL   = racfree_metal.c \
          racmode_metal.c \
          racstat_metal.c \
          racauth_metal.c
# dipendenze per i sorgenti METAL normali
MDEP    = $(METAL:.c=.u)
# oggetti da compilazione dei sorgenti C
OBJS    = $(SRCS:.c=.o)
# programma principale
TARGET  = smf80ext
# oggetto del programma principale (compilato a parte, poi linkato)
MAINO   = $(TARGET).o
# dipendenze del programma principale
MAINU   = $(TARGET).u
#
AGGR    =
DBG     =
OPT     = -O
MAK     = -qmakedep=gcc
MOBJS   = $(METAL:.c=.o)
# sorgenti assembler generati dal compilatore METAL
MSRCS   = $(METAL:.c=.s)
DLST    = ./lst
#
CC      = xlc
CFLAGS  = -Wc,list -qsource $(DBG) $(OPT) $(MAK) $(AGGR)
MFLAGS  = -S -qmetal -qsource -qlanglvl=extc99 $(MAK)
MINCL   = -Wc,NOSEARCH -I /usr/include/metal/
MPROLOG = metalc_prolog.mac
OFLAGS  = -c
L       = .lst
# NOTA: sostituire <UserId> con la propria utenza z/OS
# NOTA: sostituire <MyPrefix> con il prefisso delle proprie librerie PDS/PDSE
MYOUT   = /u/<UserId>/bin/
SDB     = smf81dec

# compila il programma
# smf80ext: $(HEADERS) $(MYOBJ) $(MOBJS) $(MYLIB:¬"lib":+"$A") $(IRRSEQ:¬"lib":+"$A") $$*.c
$(TARGET): $(MAINO) $(OBJS) $(MOBJS)
#	$(CC) $(CFLAGS) -L. -o $(MYOUT)$@ $@.c -l $(MYLIB) -l $(IRRSEQ) >$*$L
	$(CC) $(DBG) -o $(MYOUT)$@ $(MAINO) $(OBJS) $(MOBJS) >$(DLST)/$@.lnk 2>&1
	echo Linkato il programma $@ messaggi binder in $(DLST)/$@.lnk  modulo in $(MYOUT)$@
	cp $(MYOUT)$@ "//'<MyPrefix>.LLIB($@)'"
	echo copiato il modulo $@ nel PDSE //'<MyPrefix>.LLIB($@)'
	cp $(DLST)/$@$L "//'<MyPrefix>.C.DBG($@)'"
	echo copiato listing $@ in libreria per FA //'<MyPrefix>.C.DBG($@)'
	cp $(DLST)/$(SDB)$L "//'<MyPrefix>.C.DBG($(SDB))'"
	echo copiato listing $(SDB) in libreria per FA //'<MyPrefix>.C.DBG($(SDB))'
#
$(MSRCS): $$*.c
	$(CC) $(MFLAGS) $(MINCL) -Wc,LONGNAME -o $*.s $*.c  >$(DLST)/$*$L
	echo Compilata la funzione METAL $*.c listing in $(DLST)/$*$L
#
$(MOBJS): $$*.s $(MPROLOG)
	cat $(MPROLOG) $*.s > $*.full.s
	as -mgoff -o $*.o $*.full.s
# 	$(AR) $(ARFLAGS) $(MYLIB:¬"lib":+"$A") $*$O  >>$(DLST)/$*$L
# 	$(AR) -tv $(MYLIB:¬"lib":+"$A")      >>$*$L
	rm $*.full.s
	echo Assemblata la funzione METAL $* oggetto in $*.o
#
$(OBJS) $(MAINO): $$*.c
	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$(DLST)/$*$L
# 	$(AR) $(ARFLAGS) $(MYLIB:¬"lib":+"$A") $*$O  >>$(DLST)/$*$L
# 	$(AR) -tv $(MYLIB:¬"lib":+"$A")      >>$*$L
	echo Compilata la funzione $*.c listing in $(DLST)/$*$L
#
# pulizia degli oggetti per la ricompilazione totale
.PHONY : clean
clean:
	$(RM) -f $(OBJS) $(MAINO)
	$(RM) -f $(MOBJS)
	$(RM) -f $(MSRCS)
	$(RM) -f $(DEPS) $(MDEP) $(MAINU)
#
# dipendenze dagli header generate da -qmakedep (ignorate se assenti)
.INCLUDE .IGNORE : $(DEPS) $(MDEP) $(MAINU)
