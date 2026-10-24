# Changelog

*🇬🇧 [English version](CHANGELOG.md)*

Tutte le modifiche importanti di **Fase Groove Guitar**.

## [0.10.4] - 2026-10-24

- Interno più leggero: parametri letti da un'unica tabella condivisa, timer con TimedCallback di JUCE (modulo comune FaseGroove). Suono invariato.

## [0.10.3] - 2026-10-24

- I preset di genere e il Random totale non abbassano più il volume della chitarra a circa -27 dB: la manopola Volume mantiene il tuo livello e il plugin si sente anche da solo.

## [0.10.2] - 2026-10-24

- Ogni groove casuale ha una parte di accordi udibile (alcuni erano quasi muti).
- Le luci della griglia seguono di nuovo il suono (erano in ritardo dalla 0.10.1).
- Il codice comune FaseGroove ora viene dal modulo comune; i progetti salvati non cambiano.

## [0.10.1] - 2026-10-24

- Il groove si prepara fuori dal thread audio e poi viene sostituito: cambiare un'impostazione non alloca più memoria nel thread audio.
- Editor più leggero: ridisegni e ricostruzioni solo quando cambia qualcosa, preset indicizzati per genere, codice inutilizzato rimosso. Il suono non cambia.

## [0.10.0] - 2026-10-23

- Groove casuali più vari: semi vicini non danno più copie quasi uguali, il dado su una corsia varia la seconda battuta, Jackin' Piano aggiunge spinte in sedicesimi, '90s ottave sui battiti, Funky & Disco sedicesimi stile Nile e ottave disco, Gospel lick in ottavi, Dance ottave; gli scratch fitti lasciano dei buchi.
- Corretto: parametri letti senza allocare nel thread audio (prima oltre 100 allocazioni per blocco); l'editor non ricostruisce più il groove 30 volte al secondo.

## [0.9.0] - 2026-10-22

- I groove casuali restano nel genere scelto: funk e jackin' su chank e scratch in sedicesimi, house e anni '90 su stab in levare, garage su pause da 2-step, deep e soulful su accordi estesi radi, afro su clave pennato, neo-soul e chill su voicing rilassati con lick, dance su stab corti, gospel su pennate in ottavi.
- Un seed nuovo cambia il groove, e la seconda battuta è una battuta sua, non la copia della prima.

## [0.8.0] - 2026-10-22

- Corretto: alcuni groove generati non si sentivano. Un pattern di soli sedicesimi deboli su pennata in su, chank o scratch scendeva verso MIDI 60, e la chitarra guida (velocity al quadrato) lo azzerava. Ogni battuta generata tiene ora un accento udibile. I ghost restano più piani del beat, e il controtempo resta il sedicesimo più forte.
- Un'intro funk, jackin' o jackin' piano che era solo uno scratch ghost ha ora un chank sul "&".

## [0.7.0] - 2026-10-22

- I groove casuali seguono il genere: funk in sedicesimi alternati, gospel in ottavi, afro su tumbao e partido alto, neo-soul e pad radi, power dance sui tempi forti. La seconda battuta risponde alla prima.
- Variazioni e turnaround restano sulla stessa griglia e non aggiungono lick o scratch a un genere che non li usa.
- I dadi degli accordi funky restano sui giri funk. I dance stabs hanno i propri vamp minori da power chord.
- Un groove casuale è diteggiato nello stile (shell e drop-2 per gospel e neo-soul, triadi per house e dance, prese piccole per il funk). La frase del lick segue lo stile. I groove preset tengono la diteggiatura di prima.
- Intro, build-up e fill mix seguono il genere: il funk tiene lo scratch, il gospel una pennata morbida, il dance i power chord, i pad lo swell.

## [0.6.0] - 2026-10-22

- Corretto: "Sempre" nel menu Chance della griglia veniva letto come un roll: la probabilità restava e il roll si azzerava.
- Corretto: "Lo stesso accordo di..." non faceva nulla sulle battute 5–8.
- Corretto: una modifica su un pattern da 2 battute cambiava solo la prima metà; passando a 4 battute suonava una seconda metà vecchia (simbolo, velocity, chance e roll).
- Corretto: uno slide sul primo tempo metteva l'abbellimento prima dell'inizio, quindi non suonava.
- Corretto: l'umanizzazione riscriveva ogni stab, slide e chank sull'ultima battuta della frase. Ora lo fa solo ogni tanto, e più spesso se l'umanizzazione è alta.

## [0.5.0] - 2026-10-20

- Tredici preset aggiuntivi `Arranger:`: palette, articolazioni e bilanciamenti coordinati con Fase Groove Arranger 0.4.7, incluso il nuovo Jackin nella direzione funky/disco.
- Indici e valori dei preset precedenti conservati; nessuna riscrittura automatica dei progetti salvati e nessuna modifica al tema o alla disposizione dei controlli.

## [0.4.0] - 2026-10-19

- Diteggiature vere: ogni tipo di accordo su ogni fondamentale è una forma suonabile (una nota per corda, entro quattro tasti, un barré e tre dita), terza e settima sempre presenti. Le forme seguono la progressione: ogni accordo resta vicino al precedente.
- Pennate: in levare più rapide e sulle corde alte, accenti più rapidi, nella pennata in battere le corde centrali più forti, ghost solo sulle corde alte, una piccola umanizzazione per corda; una pennata normale non dura mai più di gran parte di un sedicesimo.
- Chank sulle due corde alte dello stab (stile funk); lo scratch stoppato e le pennate ghost sono note stoppate per le chitarre campionate.
- Nuovo menu SUONO...: Lascia vibrare (le pennate suonano fino all'accordo successivo), Spazio a basso e piano (niente corde basse, stab più alti), Anticipa i cambi d'accordo (l'ultimo ottavo suona l'accordo successivo), Keyswitch per chitarre campionate (sustain, palm mute, nota stoppata, slide; note C-2..D#1) con crescendo di volume CC 11, anche nelle tracce per corsia esportate.
- Gli slide si sovrappongono alla nota d'arrivo (legato sulle chitarre campionate); un nuovo accordo non taglia più l'abbellimento.
- Corretto: note dell'accordo perse in alcune diteggiature (C# maj9/9/13/6/9 perdevano la terza); il power chord prende la quinta dell'accordo su diminuiti e aumentati; la scala del lick segue l'accordo (dominante, semidiminuito, diminuito, aumentato, sus).
- Corretto: un accordo tenuto in rivolto (C/E) veniva letto con la fondamentale sbagliata.
- Corretto: due colpi d'accordo sullo stesso step (o una big strum sopra uno scratch) tagliavano le corde a zero: ora sono una pennata sola, e una corda suona sempre almeno 30 ms.
- Corretto: scegliere la tonalità relativa (Do maggiore - La minore) sul circolo o col link KEY spostava gli accordi; una tonalità cambiata dall'host ora li sposta.
- Corretto: nessuna memoria allocata nel thread audio per accordi e note; le ottave restano nell'estensione della chitarra; la battuta di turnaround segue un nuovo seed; "Lydian Jack" chiamato bVII+.

## [0.3.4] - 2026-10-19

- Corretti i colpi anticipati da template e push ai tempi elevati; test di indipendenza dalla dimensione dei blocchi. README e privacy descrivono il controllo aggiornamenti facoltativo già presente.
- Controlli comuni a Piano, Drums e Guitar: stessi pulsanti, menu, font Inter, focus, stati attivi e colori dei comandi, mantenendo la disposizione attuale.
- I rilasci MIDI mantengono il canale della nota iniziale, anche cambiando canale durante la riproduzione.
- I preset vecchi ripristinano i valori predefiniti dei parametri mancanti, senza ereditare il preset precedente.
- MIDI Start e Song Position Pointer aggiornano la posizione del clock nel blocco corrente.
- L’ordinamento degli eventi simultanei evita il buffer temporaneo di stable_sort.
- Gestione dei blocchi audio vuoti senza posizioni di campione non valide.
- I rake lenti mantengono tutte le corde anche con blocchi piccoli e tempi elevati. L’ascolto manuale conserva lo strum tra blocchi; All Notes Off ferma anche il suono guida.

## [0.3.3] - 2026-10-19

- TONALITÀ mostra la tonalità ("TONALITÀ  A Dorian"); il riquadro del groove è un po' più stretto per fare spazio. TONALITÀ si può accendere anche prima del LINK: funziona appena collegato.

## [0.3.2] - 2026-10-19

- Export più sicuri: ogni .mid (e .wav) si scrive prima in un file temporaneo; quello esistente viene sostituito solo quando il nuovo è completo (disco pieno o permessi mancanti lasciano il file vecchio).
- Spazio riservato per blocchi audio molto grandi (render offline): niente cresce nel thread audio.
- Accordi da MIDI in ingresso: la stessa nota suonata due volte resta tenuta finché non vengono rilasciate entrambe; All Notes Off le libera tutte.

## [0.3.1] - 2026-10-19

- **TONALITÀ** accanto a Sequencer (con LINK attivo): stessa tonalità e scala di Fase Groove Piano; gli accordi si spostano per la via più corta. Il titolo del riquadro ora è solo TEMPO.
- Trascinando una clip la finestra si riduce subito alla piccola striscia di Fase Groove Piano (prima aspettava il rilascio, quindi spesso non succedeva) e torna grande da sola.

## [0.3.0] - 2026-10-19

- **SOLISTA** (nell'intestazione della parte di chitarra, con LINK attivo): la chitarra suona il riff principale e Fase Groove Piano collegato aggiunge solo qualche accordo ogni tanto. Spento, o senza LINK, non cambia nulla.
- Export MIDI rifatto: ogni nota abbinata alla sua fine, una corda ribattuta chiude la nota prima, due corsie sulla stessa nota insieme danno una nota sola, niente note appese o di durata zero.
- Accordi da MIDI in ingresso: nessuna allocazione di memoria nel thread audio.
- Nuovi test su ogni groove: note pulite, traccia unica e tracce per corsia coerenti, stesse impostazioni stessa clip.

## [0.2.2] - 2026-10-19

- Interfaccia sempre nella lingua scelta (all'apertura alcuni testi restavano in inglese).
- Finestra del brano identica a quella di Fase Groove Piano: tessere arancioni, totale su due righe, fill larghi quanto la riga, "Groove principale" sulla linea del tempo, RESET accanto al titolo; battute in italiano.
- Manopole PUSH e SWING del mixer più grandi.

## [0.2.1] - 2026-10-19

- Interfaccia molto più leggera: la cronologia annulla si salva solo dopo una modifica (non più copie dell'intero stato due volte al secondo) e i pannelli si aggiornano 10 volte al secondo, le luci 30.
- Mentre trascini una clip la finestra si riduce a una piccola striscia, come in Fase Groove Piano, così vedi le tracce della DAW.
- La finestra del brano intero disposta come quella di Fase Groove Piano: le tre tessere da trascinare, SALVA e CHIUDI su una riga.
- Export MIDI ripulito: note-off prima dei note-on sullo stesso tick, una nota dell'accordo ribattuta viene chiusa prima, niente note di durata zero o appese. Gli accordi suonano in ogni strumento come nel plug-in.
- Thread audio: niente più ordinamenti con allocazioni a ogni blocco.

## [0.2.0] - 2026-10-19

- **174 giri di accordi in un browser**, come quello dei groove: dieci categorie, ogni giro con i suoi
  accordi nella tonalità. I giri di Fase Groove Piano (costruiti su 13 scale come li costruisce il
  piano: triadi, settime o none, accordi fuori scala) e altri per la chitarra (disco & '90s, garage
  & UK, afro & latin, neo soul & jazz, gospel, circolo delle quinte).
- **Il circolo delle quinte** di Fase Groove Piano nel browser: cambia tonalità, trascina un accordo
  su un altro, vedi quanto bene si seguono gli accordi; **NUOVO GIRO NEL CIRCOLO DELLE QUINTE**.
- **Da 1 a 8 accordi**, uno per battuta (due righe di slot da cinque in su); accordi m7b5, dim e aug.
- **Finestra più piccola ed editor grande.** La finestra principale mostra il groove a colpo d'occhio
  (clic per modificarlo) e, in una seconda scheda, i fader dei volumi; restano lì i dadi, I MIEI
  GROOVE, IMPORTA, TEMPLATE e SVUOTA. **EDITOR...** apre l'editor sopra tutta la finestra, come il
  piano roll di Fase Groove Piano: trasporto, pennello (accento, colpo, ghost, rake), griglia,
  velocity, mixer e fill. Il menu col tasto destro di uno step lo cancella o lo sposta di un sedicesimo.
- **LINK e PLAY: niente più scatti** (il tempo scorre continuo e segue con dolcezza l'orologio condiviso).

## [0.1.0] - 2026-10-19

La prima versione: la chitarra della famiglia Fase Groove, con la stessa interfaccia di
Fase Groove Piano e Fase Groove Drums.

- **43 groove di chitarra in 13 generi** (gli stessi del piano e della batteria): accordi in levare,
  pennate house, il chicken scratch alla Nile Rodgers, ottave disco, arpeggi highlife, montuno,
  chop two-step, pennate gospel, lick neo soul, swell chill, power chord dance...
- **12 modi di suonare un accordo**, uno per riga della griglia: pennata giù, pennata su, stab,
  chank, scratch stoppato, slide, swell, arpeggio, lick, ottave, power chord, fondamentale. Ognuno
  con il voicing di un chitarrista in accordatura standard; un nuovo accordo ferma quello che sta
  ancora suonando (una chitarra, sei corde); lo scratch stoppato riempie da solo i sedicesimi tra
  gli altri colpi.
- **Accordi:** la tonalità, 18 giri di accordi per genere, quattro slot (uno per battuta), un dado
  per nuovi accordi. **MIDI IN**: la chitarra suona l'accordo tenuto sull'ingresso MIDI (la tua
  tastiera, o il MIDI out di Fase Groove Piano).
- **La pennata (Strum):** il tempo tra due corde (0–40 ms), pennate giù dal basso all'acuto, pennate
  su dall'acuto al basso, il rake (`f`) più lento; nel suono guida e nel MIDI.
- **Chitarra guida:** corde pizzicate sintetizzate in quattro suoni (Clean Funk, Warm Jazz, Disco
  Chorus, Nylon), con timbro, riverbero e volume.
- **NUOVO GROOVE** (groove casuali per genere, da pochi accordi a sedicesimi fitti), dadi per riga
  e per parte A-D con lucchetti, **TUTTO** casuale, **STILE** per genere, **RESET**.
- **Fill** (strum in crescendo, corsa di ottave, scratch in crescendo, chank in salita, slide in
  salita, stab a raffica, corsa di lick, corsa di arpeggio, bassi in movimento, swell, drop out,
  mix, personalizzato), un **big strum** dopo ogni fill, **variazioni**, **sezioni del brano** e la
  **finestra del brano** con il brano intero come un'unica clip.
- Viste **VELOCITY**, **MIXER** (con push / pull e swing per riga, feel **TIMING**) e **FILL**,
  **template** di groove, annulla / ripeti, preset, **I MIEI GROOVE**.
- **Import MIDI**: il ritmo di un file MIDI nella griglia e i suoi accordi negli slot.
- **Export**: trascina la clip MIDI nella DAW (una traccia o una per riga), salva il `.mid`, export
  WAV (loop, brano, stem).
- **LINK** con Fase Groove Piano e Drums: stesso genere, swing, tempo e tasto play, sullo stesso beat.
- Interfaccia in 6 lingue, aggiornamenti automatici, installer per Windows.
