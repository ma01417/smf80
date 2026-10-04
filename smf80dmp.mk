# --------------------------------------------------------------- #
#                                                                 #
# Makefile per compilare smf80dmp.c                               #
#                                                                 #
# A.Brezzi giugno 2024                                            #
#                                                                 #
# Uso:                                                            #
#  make -f smf80dmp.mk                           - solo modifiche #
#  make -p -f smf80dmp.mk [>makeout.txt]         - listing make   #
#  make -u -f smf80dmp.mk                        - unconditional  #
#  make -f smf80dmp.mk clean                     - pulizia        #
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
#    make -u -f smf80dmp.mk DBG=-g                                #
#                                                                 #
#  Gli oggetti sono condivisi con smf80ext.mk (stessa directory   #
#  e stesse opzioni di compilazione).                             #
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
          irrcdt.c
# dipendenze per i sorgenti C
DEPS    = $(SRCS:.c=.u)
#
# routine METAL usate da get_CDT.c (FREEMAIN) e irrcdt.c (RACROUTE STAT)
METAL   = racfree_metal.c \
          racstat_metal.c
# dipendenze per i sorgenti METAL normali
MDEP    = $(METAL:.c=.u)
# oggetti da compilazione dei sorgenti C
OBJS    = $(SRCS:.c=.o)
# programma principale
TARGET  = smf80dmp
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

# compila il programma
$(TARGET): $(MAINO) $(OBJS) $(MOBJS)
	$(CC) $(DBG) -o $(MYOUT)$@ $(MAINO) $(OBJS) $(MOBJS) >$(DLST)/$@.lnk 2>&1
	echo Linkato il programma $@ messaggi binder in $(DLST)/$@.lnk  modulo in $(MYOUT)$@
	cp $(MYOUT)$@ "//'<MyPrefix>.LLIB($@)'"
	echo copiato il modulo $@ nel PDSE //'<MyPrefix>.LLIB($@)'
	cp $(DLST)/$@$L "//'<MyPrefix>.C.DBG($@)'"
	echo copiato listing $@ in libreria per FA //'<MyPrefix>.C.DBG($@)'
#
$(MSRCS): $$*.c
	$(CC) $(MFLAGS) $(MINCL) -Wc,LONGNAME -o $*.s $*.c  >$(DLST)/$*$L
	echo Compilata la funzione METAL $*.c listing in $(DLST)/$*$L
#
$(MOBJS): $$*.s $(MPROLOG)
	cat $(MPROLOG) $*.s > $*.full.s
	as -mgoff -o $*.o $*.full.s
	rm $*.full.s
	echo Assemblata la funzione METAL $* oggetto in $*.o
#
$(OBJS) $(MAINO): $$*.c
	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$(DLST)/$*$L
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
