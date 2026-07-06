# smf80

Estrazione, filtro e dump dei record SMF 80/81 (RACF) per z/OS.

Autore: A.Brezzi <alessandro.brezzi@gmail.com>

## Descrizione

Il progetto raccoglie un insieme di programmi e funzioni C, scritti per
essere compilati sotto z/OS (compilatore `xlc`, code page IBM-1140/EBCDIC),
per elaborare i record SMF tipo 80 (eventi RACF) e tipo 81
(inizializzazione RACF).

I due programmi principali sono:

- **`smf80dmp.c`** — Filtra i record SMF 80 in base a un file di criteri e
  ne produce una stampa esadecimale formattata (dump), comprensiva di
  header e delle varie sezioni relocabili DTA e DT2.
- **`smf80ext.c`** — Filtra i record SMF 80 in base a un file di criteri e
  ne crea un estratto (in formato originale) rispondente ai filtri
  forniti, da utilizzare per ulteriori elaborazioni a valle.

Entrambi i programmi condividono la stessa logica di parsing dei criteri
di filtro e si appoggiano alle funzioni di supporto elencate più sotto.

## Criteri di filtro (file DD:UTIPARM)

Il file di criteri è un file sequenziale in cui ogni riga ha la forma:

```
<nome car> <op> <lista valori> [ + ]
  [ continuazione lista valori ]
```

`<nome car>` è uno dei campi del record SMF 80 riconosciuti:

| Nome   | Descrizione                            | Campo      |
|--------|-----------------------------------------|------------|
| EVENT  | Evento                                  | SMF80EVT   |
| USER   | User associato                           | SMF80USR   |
| RESULT | Event Qualifier                          | SMF80EVQ   |
| JOBNAM | AS name                                  | SMF80JBN   |
| REASN  | Reason for logging                       | SMF80REA   |
| AUTH   | Autorità usate                           | SMF80ATH   |
| RES    | Resource name                            | rel. sect. 01 |
| USRJES | Resource Owner in JESSPOOL               | rel. sect. 01 |
| CLASS  | Class name                               | rel. sect. 17 |
| PROF   | Profile usato                            | rel. sect. 33 |

`<op>` può essere:

| Operatore | Significato |
|-----------|-------------|
| `=`  | uguale a uno dei valori nella lista (OR) |
| `<>` | diverso da tutti i valori nella lista (AND) |
| `ABR` | abbreviato, uno dei valori nella lista è abbreviazione del valore nel record |

`<lista valori>`: elenco dei valori da ricercare; la lista può continuare
su più righe terminando la riga con `+`. La lunghezza massima di ogni
valore è 44 caratteri.

Tutti i criteri presenti nel file sono posti in **AND** tra loro: un
record è selezionato solo se soddisfa tutti i criteri.

## DD utilizzate

| DD          | I/O    | Descrizione |
|-------------|--------|--------------|
| `UTIPARM`   | input  | file sequenziale con i criteri di estrazione |
| `UTI001`    | input  | file sequenziale VBS (o concatenazione) con i record SMF da filtrare |
| `UTICNTL`   | input  | PDS FB 80 con eventi ed event qualifier per verifica e decodifica |
| `UTI002`    | output | file sequenziale VBA con l'esito del filtro (dump esadecimale per `smf80dmp`, estratto per `smf80ext`) |

Autorizzazioni richieste: `READ` sui dataset SMF di input, `ALTER`/`UPDATE`
sul dataset di output (`UTI002`).

Uso: tramite JCL, con le DD sopra elencate preallocate.

## Struttura del progetto

**Programmi principali**

- `smf80dmp.c` — dump esadecimale dei record SMF 80 filtrati
- `smf80ext.c` — estrazione dei record SMF 80 filtrati
- `smf81dec.c` — decodifica dei record SMF 81 (inizializzazione RACF), richiamato da `smf80ext`

**Header di supporto**

- `smf80ext.h` — definizioni delle strutture usate
- `smf80sup.h` — costanti e variabili di supporto
- `smf80fmt.h` — layout delle aree del record SMF 80
- `smf81fmt.h` — layout delle aree del record SMF 81

**Funzioni di libreria** (raccolte nella libreria `myext`)

| File | Descrizione |
|------|--------------|
| `strup.c`     | conversione di una stringa in maiuscolo |
| `trim.c`      | `rtrim`/`ltrim`: eliminazione spazi iniziali/finali |
| `openf.c`     | apertura di un file con gestione parametri/descrizione |
| `createStr.c` | costruttore di liste di stringhe a lunghezza variabile |
| `createDta.c` | costruttore delle strutture per le sezioni relocabili DTA e DT2 |
| `creDTA_2.c`  | costruttore per la struttura dati sezioni relocabili (normali ed extended) |
| `getExt.c`    | estrae nome file ed estensione da un path |
| `hexprt.c`    | stampa in formato dump esadecimale di un'area di memoria |
| `getEvt.c`    | estrazione eventi da PDS con eventi e descrizioni |
| `getEvq.c`    | estrazione degli Event Qualifier relativi a un evento |
| `getCls.c`    | lettura delle classi RACF (estratte da REXX XCDTPROF) |
| `getSez.c`    | lettura delle sezioni dei record SMF 80 |
| `makeargv.c`  | tokenizzazione di una stringa (separatore blank) |
| `crfilter.c`  | costruttore della lista collegata dei filtri |
| `fltparm.c`   | verifica se un valore del record soddisfa i filtri |
| `getlrow.c`   | ricostruzione riga logica da righe concatenate con `+` |
| `findevt.c`   | ricerca di un evento nella lista collegata |
| `crparm.c`    | costruttore della lista collegata dei parametri di input |
| `gettrow.c`   | parsing riga logica: caratteristica, operatore, filtri |
| `chkparm.c`   | validazione dell'insieme di parametri e filtri |
| `fmtDtTm.c`   | decodifica di data/ora presenti nel record SMF |
| `getParm.c`   | lettura e validazione del file di parametri |
| `box.c`       | utility di supporto |

**Altri file**

- `smf80dmp.mk`, `smf80ext.mk` — makefile per la compilazione con `xlc` su z/OS
- `myIncl.txt` — note/appunti sugli include del progetto
- `smf80_riepilogo.pdf` — documento di riepilogo del progetto
- `LICENSE` — licenza del progetto (GNU GPLv3)

## Compilazione

La compilazione è pensata per l'ambiente z/OS UNIX System Services, con il
compilatore `xlc` e il tool `make` (nmake-like). I makefile di riferimento
sono `smf80dmp.mk` e `smf80ext.mk`: compilano le funzioni di supporto in
una libreria statica (`myext.a`) e poi il programma principale, producendo
i moduli eseguibili e i relativi listati di compilazione (`.lst`).

```sh
make -f smf80dmp.mk
make -f smf80ext.mk
```

## Licenza

Questo progetto è distribuito secondo i termini della licenza indicata nel
file [`LICENSE`](LICENSE) (GNU General Public License v3).
