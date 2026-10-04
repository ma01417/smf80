# --------------------------------------------------------------- #
#                                                                 #
# Makefile per compilare tstdsn.c, programma di prova per         #
# get_DSName() (getdsn.c)                                         #
#                                                                 #
# A.Brezzi ottobre 2026                                           #
#                                                                 #
# Uso:                                                            #
#  make -f tstdsn.mk                             - solo modifiche #
#  make -u -f tstdsn.mk                          - unconditional  #
#  make -f tstdsn.mk clean                       - pulizia        #
#                                                                 #
#  Variabili: AGGR e DBG come in smf80ext.mk                      #
#                                                                 #
# --------------------------------------------------------------- #
# Elenco sorgenti C nativo
SRCS    = getdsn.c
# dipendenze per i sorgenti C
DEPS    = $(SRCS:.c=.u)
# oggetti da compilazione dei sorgenti C
OBJS    = $(SRCS:.c=.o)
# programma principale
TARGET  = tstdsn
# oggetto del programma principale (compilato a parte, poi linkato)
MAINO   = $(TARGET).o
# dipendenze del programma principale
MAINU   = $(TARGET).u
#
AGGR    =
DBG     =
OPT     = -O
MAK     = -qmakedep=gcc
DLST    = ./lst
#
CC      = xlc
CFLAGS  = -Wc,list -qsource $(DBG) $(OPT) $(MAK) $(AGGR)
OFLAGS  = -c
L       = .lst
# NOTA: sostituire <UserId> con la propria utenza z/OS
# NOTA: sostituire <MyPrefix> con il prefisso delle proprie librerie PDS/PDSE
MYOUT   = /u/<UserId>/bin/

# compila il programma
$(TARGET): $(MAINO) $(OBJS)
	$(CC) $(DBG) -o $(MYOUT)$@ $(MAINO) $(OBJS) >$(DLST)/$@.lnk 2>&1
	echo Linkato il programma $@ messaggi binder in $(DLST)/$@.lnk  modulo in $(MYOUT)$@
	cp $(MYOUT)$@ "//'<MyPrefix>.LLIB($@)'"
	echo copiato il modulo $@ nel PDSE //'<MyPrefix>.LLIB($@)'
#
$(OBJS) $(MAINO): $$*.c
	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$(DLST)/$*$L
	echo Compilata la funzione $*.c listing in $(DLST)/$*$L
#
# pulizia degli oggetti per la ricompilazione totale
.PHONY : clean
clean:
	$(RM) -f $(OBJS) $(MAINO)
	$(RM) -f $(DEPS) $(MAINU)
#
# dipendenze dagli header generate da -qmakedep (ignorate se assenti)
.INCLUDE .IGNORE : $(DEPS) $(MAINU)
