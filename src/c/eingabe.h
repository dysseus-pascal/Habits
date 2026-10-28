#pragma once
#include <pebble.h>
#include "habit.h"

// Kleine Eingabefenster fuer das Formular und die Einstellungen.

// Bis zu drei Felder nebeneinander, Oben/Unten aendert das gewaehlte, Mitte
// geht zum naechsten (am letzten: fertig), Zurueck zum vorigen (am ersten:
// abbrechen). Gehalten laufen Oben/Unten schnell.
typedef struct {
  int wert, min, max, schritt;
} Feld;
typedef void (*FeldText)(void *daten, int feld, const int *werte, char *puffer, size_t groesse);
typedef void (*FeldFertig)(void *daten, const int *werte);
void feldwahl_zeigen(const char *titel, const Feld *felder, int anzahl, FeldText text,
                     FeldText vorschau, FeldFertig fertig, void *daten);

// Eine Zahl 0..max; 0 heisst "None", wenn `null_text` gesetzt ist.
void zahl_zeigen(const char *titel, int wert, int min, int max, const char *null_text,
                 FeldFertig fertig, void *daten);
// Eine Uhrzeit in Minuten nach Mitternacht, in Schritten von 5 Minuten.
void uhrzeit_zeigen(const char *titel, int minute, FeldFertig fertig, void *daten);

// Der Name: Vorschlaege, Diktat (falls die Uhr ein Mikrofon hat) oder
// Buchstabe fuer Buchstabe. Schreibt in h->name; ein Vorschlag aus der Liste
// der schlechten macht eine neue Gewohnheit schlecht.
void name_zeigen(Habit *h, bool neu);

// Wochentage und Zeiten fuer "Scheduled".
void tage_zeigen(Habit *h);
void zeiten_zeigen(Habit *h);

// "Mon, Wed" / "Weekdays" / "Every day" und "08:00, 20:00".
void tage_text(uint8_t tage, char *puffer, size_t groesse);
void zeiten_text(const Habit *h, char *puffer, size_t groesse);
