# --------------------------------------------------------------- #
#                                                                 #
# Makefile per compilare smf80ext.c                               #
#                                                                 #
# A.Brezzi giugno 2024                                            #
#                                                                 #
# --------------------------------------------------------------- #
HEADERS := smf80fmt.h \
           smf80sup.h \
           smf80ext.h
MYMEMB  := strup \
           trim \
           openf \
           createStr \
           getExt \
           hexprt \
           getEvt \
           getEvq \
           getCls \
           getSez \
           makeargv \
           crfilter \
           fltparm \
           getlrow \
           findevt \
           crparm \
           gettrow \
           chkparm \
           fmtDtTm \
           creDTA_2 \
           getParm
#
DBG     :=
OPT     := -O
MYFUNC  := $(MYMEMB:+".c")
MYOBJ   := $(MYMEMB:+"$O")
MYLIB   := myext
CC      := xlc
CFLAGS  := -Wc,list $(OPT) $(DBG)
AR      := ar
ARFLAGS := -rcs
OFLAGS  := -c
L       := .lst
# NOTA: sostituire <UserId> con la propria utenza z/OS
# NOTA: sostituire <MyPrefix> con il prefisso delle proprie librerie PDS/PDSE
MYOUT   := /u/<UserId>/bin/

# compila il programma
smf80dmp: $(HEADERS) $(MYOBJ) $(MYLIB:¬"lib":+"$A") $$*.c
	$(CC) $(CFLAGS) -L. -o $(MYOUT)$@ $@.c -l $(MYLIB) >$*$L
	echo Compilato il programma $@.c listing in $*$L, modulo in $(MYOUT)$@
	cp $(MYOUT)$@ "//'<MyPrefix>.LLIB($@)'"
	echo copiato il modulo $@ nel PDSE //'<MyPrefix>.LLIB($@)'
	cp $@$L "//'<MyPrefix>.C.DBG($@)'"
	echo copiato listing $@ in libreria per FA //'<MyPrefix>.C.DBG($@)'

# gestione della libreria
myext.a .LIBRARY : $(MYOBJ)
	$(AR) $(ARFLAGS) lib$(MYLIB:+"$A") $*$O
	$(AR) -tv lib$(MYLIB$A)

# compila le funzioni in base alle dipendenze
# chkparm.o : findevt.o getEvq.o getEvt.o \
#             $(HEADERS) $$*.c
# 	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$*$L
# 	$(AR) $(ARFLAGS) $(MYLIB:¬"lib":+"$A") $*$O   >>$*$L
# 	$(AR) -tv $(MYLIB:¬"lib":+"$A")       >>$*$L
# 	echo Compilata la funzione $*.c listing in $*$L
#
# createStr.o : $(HEADERS) $$*.c
# 	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$*$L
# 	$(AR) $(ARFLAGS) $(MYLIB:¬"lib":+"$A") $*$O   >>$*$L
# 	$(AR) -tv $(MYLIB:¬"lib":+"$A")       >>$*$L
# 	echo Compilata la funzione $*.c listing in $*$L
#
# crfilter.o : $(HEADERS) $$*.c
# 	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$*$L
# 	$(AR) $(ARFLAGS) $(MYLIB:¬"lib":+"$A") $*$O   >>$*$L
# 	$(AR) -tv $(MYLIB:¬"lib":+"$A")       >>$*$L
# 	echo Compilata la funzione $*.c listing in $*$L
#
# crparm.o : $(HEADERS) $$*.c
# 	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$*$L
# 	$(AR) $(ARFLAGS) $(MYLIB:¬"lib":+"$A") $*$O   >>$*$L
# 	$(AR) -tv $(MYLIB:¬"lib":+"$A")       >>$*$L
# 	echo Compilata la funzione $*.c listing in $*$L
#
# findevt.o : strevt.h $$*.c
# 	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$*$L
# 	$(AR) $(ARFLAGS) $(MYLIB:¬"lib":+"$A") $*$O   >>$*$L
# 	$(AR) -tv $(MYLIB:¬"lib":+"$A")       >>$*$L
# 	echo Compilata la funzione $*.c listing in $*$L
#
# getCls.o : makeargv.o openf.o $(HEADERS) $$*.c
# 	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$*$L
# 	$(AR) $(ARFLAGS) $(MYLIB:¬"lib":+"$A") $*$O   >>$*$L
# 	$(AR) -tv $(MYLIB:¬"lib":+"$A")       >>$*$L
# 	echo Compilata la funzione $*.c listing in $*$L
#
# getEvq.o : openf.o makeargv.o trim.o $(HEADERS) $$*.c
# 	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$*$L
# 	$(AR) $(ARFLAGS) $(MYLIB:¬"lib":+"$A") $*$O   >>$*$L
# 	$(AR) -tv $(MYLIB:¬"lib":+"$A")       >>$*$L
# 	echo Compilata la funzione $*.c listing in $*$L
#
# getEvt.o : openf.o makeargv.o trim.o $(HEADERS) $$*.c
# 	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$*$L
# 	$(AR) $(ARFLAGS) $(MYLIB:¬"lib":+"$A") $*$O   >>$*$L
# 	$(AR) -tv $(MYLIB:¬"lib":+"$A")       >>$*$L
# 	echo Compilata la funzione $*.c listing in $*$L
#
# getlrow.o : $(HEADERS) $$*.c
# 	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$*$L
# 	$(AR) $(ARFLAGS) $(MYLIB:¬"lib":+"$A") $*$O   >>$*$L
# 	$(AR) -tv $(MYLIB:¬"lib":+"$A")       >>$*$L
# 	echo Compilata la funzione $*.c listing in $*$L
#
# getParm.o : chkparm.o getCls.o getEvt.o getlrow.o gettrow.o openf.o \
#             trim.o $(HEADERS) $$*.c
# 	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$*$L
# 	$(AR) $(ARFLAGS) $(MYLIB:¬"lib":+"$A") $*$O   >>$*$L
# 	$(AR) -tv $(MYLIB:¬"lib":+"$A")       >>$*$L
# 	echo Compilata la funzione $*.c listing in $*$L
#
# gettrow.o : chkparm.o crfilter.o crparm.o makeargv.o strup.o \
#             $(HEADERS) $$*.c
# 	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$*$L
# 	$(AR) $(ARFLAGS) $(MYLIB:¬"lib":+"$A") $*$O   >>$*$L
# 	$(AR) -tv $(MYLIB:¬"lib":+"$A")       >>$*$L
# 	echo Compilata la funzione $*.c listing in $*$L
#
# makeargv.o : $(HEADERS) $$*.c
# 	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$*$L
# 	$(AR) $(ARFLAGS) $(MYLIB:¬"lib":+"$A") $*$O   >>$*$L
# 	$(AR) -tv $(MYLIB:¬"lib":+"$A")       >>$*$L
# 	echo Compilata la funzione $*.c listing in $*$L
# ricompila se modificata funzione utilizzata
$(MYOBJ):: $(MYFUNC)
	$(CC) $(CFLAGS) $(OFLAGS) $*.c        >$*$L
	$(AR) $(ARFLAGS) $(MYLIB:¬"lib":+"$A") $*$O   >>$*$L
	$(AR) -tv $(MYLIB:¬"lib":+"$A")       >>$*$L
	echo Compilata la funzione $*.c listing in $*$L

# pulizia degli oggetti per la ricompilazione totale
CLEANUP:
	$(RM) $(MYOBJ)

