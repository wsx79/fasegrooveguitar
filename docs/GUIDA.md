# Fase Groove Guitar – Guida

*🇬🇧 [English guide](GUIDE.md)*

Questa guida descrive la [v0.5.0](https://github.com/wsx79/fasegrooveguitar/releases/tag/v0.5.0). Controlli e posizioni sono conservati; pulsanti, menu e colori comuni hanno uno stile uniforme.

**Preset Arranger:** nel menu degli stili trovi tredici nuove voci `Arranger: …`, coordinate con Fase Groove Arranger 0.4.7. Scegli lo stesso genere in Piano, Drums e Guitar e usa LINK per tonalità, tempo e trasporto. Il Jackin segue la direzione funky/disco indicata, con chops, basso articolato e chitarra percussiva. I preset precedenti restano disponibili e i progetti già salvati conservano le proprie scelte; i livelli e gli effetti della DAW partecipano sempre al mix finale.

## 1. Installazione

**Windows:** avvia `FaseGrooveGuitar-Windows-Setup.exe` dalla pagina
[Releases](https://github.com/wsx79/fasegrooveguitar/releases/latest) (oppure scompatta lo zip e copia
i file come sotto). **macOS / Linux:** scarica lo zip per il tuo sistema e scompattalo.

| Sistema | Copia | In |
|---|---|---|
| Windows | `Fase Groove Guitar.vst3` | `C:\Program Files\Common Files\VST3\` |
| macOS | `Fase Groove Guitar.vst3` / `Fase Groove Guitar.component` | `~/Library/Audio/Plug-Ins/VST3/` / `~/Library/Audio/Plug-Ins/Components/` |
| Linux | `Fase Groove Guitar.vst3` | `~/.vst3/` |

Poi rifai la scansione dei plugin nella DAW. Su macOS, se il plugin è bloccato, esegui una volta nel
Terminale: `xattr -cr ~/Library/Audio/Plug-Ins/VST3/"Fase Groove Guitar.vst3"` (e lo stesso per il
`.component`). L'app **Standalone** funziona senza DAW.

Caricalo come **strumento** su una traccia MIDI. Premi play nella DAW (o **PLAY** nel plugin) e il
groove suona con la chitarra guida.

## 2. La finestra

La stessa disposizione di Fase Groove Piano e Fase Groove Drums:

- **Intestazione:** PRESET, TUTTO (tutto casuale), annulla / ripeti, schermo intero, STILE, RESET,
  OPZIONI; TRASPORTO E TEMPO (⏮ ▶ ■, SYNC DAW, il tempo, LINK, Sequencer); il display (groove,
  genere, l'accordo che suona, la sezione, dove si trova la riproduzione).
- **GROOVE:** il groove, il genere dei dadi, la densità, la durata delle parti e le parti A-D.
- **PARTE DI CHITARRA:** la riga scelta nella griglia. **ACCORDI:** tonalità, giro di accordi, i quattro accordi.
- **FEEL E BRANO**, **SUONO GUIDA E MIDI OUT**.
- La grande **griglia** (o VELOCITY, MIXER, FILL) ed **EXPORT MIDI E AUDIO**.

## 3. Il groove

- **GROOVE:** 43 groove in 13 generi (gli stessi di Fase Groove Piano e Drums); clicca il nome per il
  browser (generi a sinistra, ogni groove con l'immagine del suo ritmo). **Groove casuale** è quello
  di NUOVO GROOVE, **User pattern** quello modificato nella griglia.
- **CASUALE / GROOVE (dadi):** un nuovo groove casuale nel genere scelto in **GENERE DEL GROOVE**.
  **DENSITÀ** va da pochi accordi a pennate in sedicesimi con scratch.
- **Parti A-D:** il groove in quattro parti (mezza battuta ciascuna, o una battuta con **Durata
  parti: 1 bar**); suonale in qualsiasi ordine (A A A B...), rigenerane una con il suo dado, bloccala
  con il lucchetto perché i dadi la lascino stare.
- **STILE** (in alto): groove, accordi, swing, feel, fill e suono di un genere in un clic. **RESET**
  rimette tutto come all'inizio.

## 4. Accordi

- **Tonalità** (il menu a sinistra): gli accordi si spostano con lei.
- **GIRO DI ACCORDI...:** il browser dei giri: 174 giri in dieci categorie, ognuno con i suoi accordi
  nella tonalità; un clic lo carica. A destra il **circolo delle quinte** (come in Fase Groove Piano):
  clicca una tonalità per cambiarla; i pallini sono i tuoi accordi: trascinane uno su un altro accordo
  per cambiarlo. Sotto: tonalità, **scala** (13, come nel piano), **colore** (triadi, settime, none) e
  quanti **accordi** suonano (da 1 a 8, uno per battuta), e **NUOVO GIRO NEL CIRCOLO DELLE QUINTE**:
  ogni accordo una quinta sotto il precedente, a volte con una dominante secondaria. Il **dado**
  accanto a GIRO DI ACCORDI... sceglie un giro del genere del groove (o un nuovo giro nel circolo).
- **I quattro slot:** un accordo per battuta, a ciclo (quattro battute). Clicca uno slot per
  cambiare fondamentale o tipo (maj7, m7, 7, m9, maj9, 9, m11, 13, 7sus4, 6/9, maggiore, minore),
  copiare l'accordo di un'altra battuta o metterlo su tutte. Lo slot che suona è illuminato.
- **MIDI IN** (attivo di default): quando sull'**ingresso MIDI** del plugin è tenuto un accordo, la
  chitarra suona quell'accordo (con il suo ritmo) invece degli slot, e lo segue quando cambia. Suona
  la tua tastiera sulla traccia della chitarra, o manda il **MIDI out di Fase Groove Piano** alla
  traccia della chitarra: la chitarra suona gli accordi del piano. La nota più bassa è la
  fondamentale. Una nota sola suona come power chord. Spento: le note dell'ingresso MIDI passano
  (e suonano la chitarra guida).

## 5. La griglia

12 righe, i modi in cui un chitarrista suona un accordo, × 32 step (due battute di sedicesimi, o 64
con parti di una battuta):

| Riga | Cosa suona |
|---|---|
| **DOWN** (pennata giù) | l'accordo intero su sei corde, dal basso all'acuto |
| **UP** (pennata su) | le quattro corde acute, dall'acuto al basso |
| **STAB** | tre corde acute, corto |
| **CHANK** | una corda acuta, cortissimo (il chop funk a nota singola) |
| **SCRATCH** (scratch stoppato) | le corde stoppate dalla mano: un click senza nota. Riempie i sedicesimi che nessun altro colpo suona |
| **SLIDE** | tre corde acute con slide da un semitono sotto |
| **SWELL** | l'accordo intero che entra in fade e resta (il trucco della manopola del volume) |
| **ARP** (arpeggio) | una corda alla volta, su e giù per l'accordo |
| **LICK** | una frase sulla pentatonica dell'accordo (minore su un accordo minore) |
| **OCTAVE** (ottave) | la fondamentale e la sua ottava (disco!) |
| **POWER** (power chord) | fondamentale, quinta e ottava sulle corde basse |
| **ROOT** (fondamentale) | la fondamentale sulle corde basse |

Un nuovo accordo ferma quello che sta ancora suonando: una chitarra, sei corde.

| Azione | Risultato |
|---|---|
| Clic | colpo (di nuovo clic: pausa) |
| Shift + clic | accento |
| Alt + clic | ghost (molto leggero) |
| Ctrl / Cmd + clic | **rake**: una pennata lenta, si sentono tutte le corde |
| Trascina | dipinge |
| Clic destro su uno step | la probabilità dello step (suona ogni tanto) e un roll (2-4 pennate veloci) |
| Clic sul nome di una riga | la ascolti sull'accordo della battuta e la scegli in PARTE DI CHITARRA |
| Doppio clic sul nome di una riga | le sue velocity |
| Clic destro sul nome di una riga | mute, nuova linea, svuota, ottava, livello |
| Il dado di una riga | una nuova linea solo per quella riga |

La prima modifica di un preset lo trasforma nello **User pattern** (il preset resta com'era).

## 6. Parte di chitarra

La riga scelta: **MUTE**, il suo dado, **OTTAVA** (su o giù di ottave, il voicing mantiene la
forma), **ASCOLTA**, **LIVELLO** (le sue velocity MIDI) e **VOLUME** (solo nel suono guida).
**SUONA** mostra le note che suona sull'accordo del momento.

## 7. Feel

- **SWING:** sposta un sedicesimo sì e uno no, come lo mostrano le drum machine: 50% dritto, 67%
  shuffle in terzina, 75% il più pesante. Ogni genere porta il suo swing (lo stesso della batteria).
- **UMANIZZA:** piccole variazioni di tempo e velocity, e le note leggere vanno e vengono in una frase
  di quattro battute. **NUOVO FEEL** ne crea un'altra (salvata con il progetto).
- **VELOCITY:** il livello generale. **ACCENTS:** pennate su, scratch, chank e arpeggi più forti in
  levare e più leggeri sugli altri sedicesimi, come suonano i chitarristi funk.
- **MIXER:** il **push / pull** di ogni riga (ms in anticipo o in ritardo sul beat) e la sua quota di
  swing; **TIMING...** ha feel pronti (pennate in ritardo, scratch in anticipo, jackin', neo soul pigro...).
- **TEMPLATE...:** il timing e gli accenti di un loop audio o di un file MIDI su qualsiasi groove
  (trascina un loop audio sulla finestra per usarlo come template).

## 8. Brano

- **Fill** ogni 1, 2, 4 o 8 battute, in uno **stile**: strum in crescendo, corsa di ottave, scratch in
  crescendo, chank in salita, slide in salita, stab a raffica, corsa di lick, corsa di arpeggio, bassi
  in movimento, swell, drop out, mix (uno diverso ogni tanto) o **personalizzato** (disegnato nella
  vista FILL).
- **BIG STRUM:** una grande pennata giù sulla battuta dopo ogni fill (e ogni 8 battute del drop).
- **Variazione:** ogni 4 o 8 battute l'ultima battuta cambia (uno slide, un pick-up, un lick, uno stop).
- **Sezione del brano:** Intro / Outro (chank e scratch), Break (un accordo tenuto), Build-up (uno
  scratch che cresce), Drop, Main.
- **BRANO INTERO...:** la finestra del brano: battute, variazione e fill di chiusura di ogni sezione;
  trascina o salva il brano intero (con i marker), una sezione, o ogni sezione come clip a sé.

## 9. Suono e MIDI

- **Suono guida:** la chitarra guida, corde pizzicate sintetizzate: **Clean Funk**, **Warm Jazz**,
  **Disco Chorus**, **Nylon**, con **TIMBRO**, **RIVERBERO** e **VOLUME**. Spegnila per suonare il
  MIDI con il tuo strumento di chitarra.
- **STRUM:** il tempo tra due corde di una pennata (0 ms: tutte le corde insieme). È anche nel MIDI,
  così il tuo strumento di chitarra suona la pennata allo stesso modo.
- **CANALE MIDI:** il canale delle note (1 di default).
- **Sequencer:** spento, il groove non suona con la DAW (per esempio mentre la clip trascinata suona
  sulla stessa traccia); ▶ lo fa comunque ascoltare.

## 10. Export e import

- **TRASCINA MIDI NELLA DAW:** trascina il riquadro arancione su una traccia di chitarra (o
  tastiera). **Note:** tutte le righe su una traccia, o una traccia per riga. **Lunghezza clip:** il
  loop completo (finché groove, accordi, fill e variazioni tornano all'inizio), 1–16 battute, o il
  brano intero.
- **SALVA .MID...**, **SALVA .WAV...** (il suono guida: un loop senza stacchi, il brano intero o gli stem).
- **IMPORTA...** (o trascina un `.mid` sulla finestra): un file MIDI (una parte di chitarra, accordi,
  un riff di piano): il ritmo nella griglia, ogni colpo nella riga a cui somiglia (una pennata, uno
  stab, ottave, una nota...), e gli accordi delle prime quattro battute negli slot.
- **I MIEI GROOVE:** salva la griglia come tuo groove, ricaricalo in qualsiasi progetto. **PRESET:** tutto.

## 11. LINK

**LINK** (in TRASPORTO E TEMPO) collega Fase Groove Guitar con **Fase Groove Piano** e **Fase Groove
Drums** (e altre istanze): stesso genere, swing, tempo, sync DAW e tasto play in tutti, sullo stesso
beat. Cambialo in uno qualsiasi. Perché la chitarra suoni gli accordi del piano, manda il MIDI out
del piano alla traccia della chitarra (MIDI IN, sezione 4).

## 12. Scorciatoie e opzioni

- **Spazio:** play / pausa. **Ctrl+Z / Ctrl+Shift+Z (Ctrl+Y):** annulla / ripeti.
- **OPZIONI** (o un clic sul logo): lingua (English, Italiano, Español, Français, Deutsch,
  Português), dimensione dell'interfaccia, suggerimenti, aggiornamenti automatici; **INFO**: crediti e link.

## 13. Suonare con il piano e novità della 0.4

- **TONALITÀ** (accanto a Sequencer) mostra tonalità e scala ("TONALITÀ  A Dorian"). Accesa nella
  chitarra e in Fase Groove Piano, con LINK attivo, mantiene la stessa tonalità e scala; gli accordi
  si spostano per la via più corta. Scegliere la relativa (da Do maggiore a La minore) lascia gli
  accordi dove sono.
- **SOLISTA** (nell'intestazione PARTE DI CHITARRA, con LINK attivo): la chitarra suona il riff
  principale e Fase Groove Piano collegato risponde solo ogni tanto, anche nella sua griglia, nel
  piano roll e nel MIDI esportato.
- **Forme di chitarra vere:** ogni accordo è una forma suonabile (una nota per corda, entro quattro
  tasti, terza e settima sempre presenti) e resta vicino all'accordo prima.
- **Pennate:** in su più veloci e sulle corde alte, le corde centrali della pennata in giù più forti,
  pennate ghost sulle corde alte, una piccola variazione per corda. Il chank suona le due corde alte.
- **ESECUZIONE...** (in SUONO GUIDA E MIDI OUT):
  - *Lascia suonare*: le pennate suonano fino all'accordo dopo.
  - *Spazio per basso e piano*: niente corde basse, stab più in alto.
  - *Anticipa i cambi d'accordo*: l'ultimo ottavo della battuta suona l'accordo dopo.
  - *Keyswitch* per chitarre campionate (sustain, palm mute, nota morta, slide; note da C-2 a D#1) e
    swell di volume con CC 11, anche nelle tracce per corsia esportate.
- Gli slide si sovrappongono alla nota d'arrivo (legato nelle chitarre campionate). Gli export si
  scrivono prima in un file temporaneo: un salvataggio fallito non perde mai la clip vecchia.
