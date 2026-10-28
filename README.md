# Habits

Gewohnheiten auf der Pebble: anlegen, eintragen, als Diagramm ansehen und, je
nach Nachdruck, daran erinnert werden. Gute Gewohnheiten haben ein Ziel,
schlechte eine Grenze. Alles läuft auf der Uhr, ohne Telefon.

Gebaut nach einer Benutzeranfrage (`habit_app.md`, „Habit Tracker Watch App —
Full Design Specification“). Für die Pebble Time 2 (emery), die Pebble 2 Duo
(flint) und die Pebble Round 2 (gabbro). Die Oberfläche ist englisch wie die
Anfrage.

## Bedienung

Hauptmenü: **My habits** (See charts, Add habit, Edit habit, Delete habit),
**Log a habit**, **Settings**, **Exit**.

- **Eintragen:** eine Gewohnheit wählen, fertig. Die Bestätigung zeigt den
  Stand („6 of 8“), vibriert und spielt, wenn eingestellt, einen Ton. Unten
  nimmt den Eintrag zurück, solange sie steht.
- **Quick Launch:** in den Einstellungen der Uhr (Quick Launch) Habits auf eine
  Taste legen. Dann öffnet die App gleich die Liste zum Eintragen und geht
  danach zurück zum Zifferblatt. Eine eigene Anmeldung dafür gibt es bei
  Pebble nicht: jede App lässt sich dort wählen.
- **Diagramme:** Oben und Unten wechseln Tag, Woche, Monat, Jahr (die Uhr hat
  kein Links und Rechts). Mitte geht einen Ausschnitt zurück (gestern, letzte
  Woche …), lang gedrückt wieder zu heute. Ziel oder Grenze stehen als
  gestrichelte Linie mit ihrem Wert.
- **Namen:** Vorschläge für häufige Gewohnheiten (ein schlechter Vorschlag
  macht eine neue Gewohnheit schlecht), Buchstabe für Buchstabe mit einem Rad
  (Oben/Unten drehen, Mitte nimmt, lang Mitte speichert) oder Diktat, wenn das
  Telefon verbunden ist.
- **Einstellungen:** Vibration und Ton je Typ (Ton nur auf Uhren mit
  Lautsprecher), helles oder dunkles Thema, die aktiven Stunden („Reminders
  from/until“, Vorgabe 08:00–21:00).

## Nachdruck

| Stufe | Gute Gewohnheit | Schlechte Gewohnheit | Ruhezeit |
|---|---|---|---|
| None | nur eintragen | nur eintragen | – |
| Light | eine Erinnerung, wenn ein Zeitraum unerfüllt endet: am nächsten Morgen in der Bilanz, bei Stunden-Blöcken zu Beginn des nächsten | am nächsten Morgen in der Bilanz, wenn die Grenze überschritten war | zeigt sich ohne Vibration |
| Heavy | Anstöße in zufälligen Abständen im Zeitraum, nur in den aktiven Stunden, bis er erfüllt ist | Warnung beim Eintragen ab 80 % der Grenze | bleibt still |
| Critical | alle 5 Minuten ab Beginn des Zeitraums (frühestens mit den aktiven Stunden) bis er erfüllt ist | Warnung beim Eintragen, bleibt stehen bis zu einem Tastendruck | vibriert trotzdem |

Bei **Scheduled** erinnert jede Stufe zur geplanten Zeit; Heavy danach bis drei
Stunden lang in zufälligen Abständen, Critical alle 5 Minuten bis zur nächsten
geplanten Zeit. Ein Eintrag bis 30 Minuten vorher zählt für die Zeit.

In der Erinnerung trägt Mitte (✓) ein, Unten (✗) lässt den Zeitraum aus
(„Skip“, bei Scheduled bis zur nächsten Zeit), Zurück schließt sie; dann
erinnert die App weiter.

## Wie die Anfrage gelesen wurde

Wo die Anfrage offen ist oder die Uhr anders kann, gilt:

- **Zeiträume nach dem Kalender:** Tag ab Mitternacht; „Once per n hours“ in
  Blöcken ab Mitternacht (0–2, 2–4 …, der letzte endet um Mitternacht);
  n Tage ab dem Tag des Anlegens; n Wochen ab der Woche des Anlegens; n Monate
  ab dem Monat des Anlegens. Schlechte Gewohnheiten und Scheduled gelten je Tag.
- **Die Woche** beginnt am Sonntag, wenn die Uhr auf US-Englisch steht, sonst
  am Montag. Pebble hat keine eigene Einstellung dafür.
- **„Start of next day“** ist der Beginn der aktiven Stunden: dann kommt die
  Morgenbilanz mit allem, was gestern (oder letzte Woche, letzten Monat)
  unerfüllt endete. Um Mitternacht zu vibrieren wäre keine Erinnerung.
- **Ziel 0:** eine gute Gewohnheit ohne Ziel ist mit einem Eintrag erfüllt
  (die Anfrage: „treats 0 logs as incomplete“), Scheduled ohne Ziel mit so
  vielen Einträgen, wie Zeiten für den Tag geplant sind. Eine Linie im
  Diagramm gibt es nur mit Ziel oder Grenze.
- **Die Linie** steht nur dort, wo ein Balken dasselbe misst wie das Ziel:
  „einmal alle zwei Wochen“ hat in einem Tagesbalken keinen Platz. Die
  Tagesansicht zeigt die laufende Summe (bei Stunden-Blöcken die Zahl je
  Stunde), die Jahresansicht bei Gewohnheiten je Tag den Schnitt je Tag.
- **Einträge** werden gezählt statt einzeln gespeichert: je Stunde für die
  letzten sieben Tage, je Tag für 744 Tage (zwei volle Kalenderjahre). Die
  Anfrage lässt das zu („day-aggregated counts“). Älter als sieben Tage zeigt
  die Tagesansicht nur noch die Summe.
- **Grenzen:** höchstens 20 Gewohnheiten, Namen bis 31 Byte, bis zu vier
  Zeiten am Tag, Ziel und Grenze bis 99.
- **Ruhezeit:** sie lässt sich nicht vorausberechnen und gilt, wenn der Wecker
  kommt. Den Ton kann die Uhr in der Ruhezeit selbst stumm schalten, auch bei
  Critical.
- **Wecker:** eine App hat höchstens acht. Habits stellt immer die nächsten
  acht Termine und plant nach jedem Start, Eintrag und Wecker neu; jeder
  Wecker zeigt alles, was um seine Zeit fällig ist.
- **Noch nicht:** der Abgleich mit dem Telefon (Abschnitt 9 der Anfrage).

## Bauen und prüfen

```sh
pebble build                     # build/Habits.pbw
sh tools/host_tests.sh           # Rechnung auf dem Rechner (TZ=Europe/Zurich)
sh tools/screenshots.sh emery    # Bilder im Emulator, mit Beispieldaten
python3 tools/make_icons.py resources/images
```

`tools/host_tests.sh` prüft Kalender (auch an den Tagen der Umstellung),
Logbuch, Gewohnheiten, Zeiträume, den Planer der Erinnerungen und die
Diagramme mit dem C-Compiler des Rechners; `pebble.h` kommt als Attrappe aus
`tools/host`. `tools/screenshots.sh` baut eine Kopie mit Beispieldaten
(`src/c/demo.c`, Schalter in `src/c/pruef.h`) und fährt die Oberfläche im
Emulator ab, die Wecker eingeschlossen.

## Aufbau

| Datei | Inhalt |
|---|---|
| `kalender.c` | Tagnummern, Wochen, Monate, Ortszeit ohne `mktime` |
| `habit.c` | die Gewohnheiten im Speicher der Uhr |
| `logbuch.c` | Zählungen je Stunde und Tag, Rückgängig, Skip |
| `zeitraum.c` | Zeitraum, Ziel und Stand |
| `planer.c`, `wecker.c` | wann erinnert wird, und die Wecker dazu |
| `diagramm_daten.c`, `diagramm.c` | Diagramme |
| `eintragen.c`, `meldung.c` | Eintragen, Bestätigung, Löschen |
| `erinnerung.c` | Erinnerung und Morgenbilanz |
| `liste.c`, `karte.c`, `feldwahl.c` | Bausteine der Oberfläche |
| `hauptmenue.c`, `auswahl.c`, `formular.c`, `name.c`, `plan.c`, `einstellungen_fenster.c` | die Fenster |
| `speicher.h` | die Fächer im Speicher der Uhr |

## Lizenz

Gemeinfrei, [CC0 1.0](LICENSE), wie Drinktervall. Kopieren, ändern, verkaufen,
einbauen — ohne Bedingung, ohne Namensnennung, ohne Rückfrage.

CC0 statt der Unlicense, weil das Schweizer Urheberrecht einen Verzicht gar
nicht kennt; CC0 trägt für genau diesen Fall eine Ersatzlizenz in sich, die
dasselbe erlaubt.

Alles im Repository ist eigene Arbeit. Die Symbole zeichnet
`tools/make_icons.py`; `tools/emu.py` stammt aus Drinktervall, das unter
derselben Widmung steht.
