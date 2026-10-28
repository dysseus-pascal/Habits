#pragma once
#include <pebble.h>
#include "habit.h"

// Die Fenster der App. Jedes legt sich selbst auf den Stapel und raeumt beim
// Schliessen hinter sich auf.

void hauptmenue_zeigen(void);
void meine_zeigen(void);

typedef enum { AUSWAHL_DIAGRAMM, AUSWAHL_AENDERN, AUSWAHL_LOESCHEN, AUSWAHL_EINTRAGEN } AuswahlZweck;
void auswahl_zeigen(AuswahlZweck zweck);

// id 0: eine neue Gewohnheit.
void formular_zeigen(uint16_t id);
void diagramm_zeigen(uint16_t id);
void einstellungen_zeigen(void);

// Traegt ein und bestaetigt es (Abschnitt 6): kurz, bei schlechten
// Gewohnheiten mit Heavy und Critical mit Warnung. Mit Rueckgaengig.
void bestaetigung_zeigen(uint16_t id);
// Loeschen mit Rueckfrage (Abschnitt 4.3).
void loeschen_fragen(uint16_t id);

// Erinnerung durch einen Wecker; `still` in der Ruhezeit.
void erinnerung_zeigen(uint16_t id, bool still);
// Die Morgenbilanz; false, wenn es nichts zu berichten gibt.
bool bilanz_zeigen(bool still);

// Schliesst alle Fenster; ohne Fenster beendet sich die App. So endet sie
// auch nach Quick Launch und Wecker: dort liegt kein Hauptmenue darunter.
void app_beenden(void);
