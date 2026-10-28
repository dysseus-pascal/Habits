#pragma once
#include <pebble.h>
#include "habit.h"
#include "zeitraum.h"

// Der Stand einer Gewohnheit jetzt: Eintraege im laufenden Zeitraum, das
// Ziel oder die Grenze dazu, und was das bedeutet.
typedef struct {
  int zahl;
  int ziel;
  Stand stand;
  Zeitraum zeitraum;
  time_t letzter_log;
} Lage;

Lage eintragen_lage(const Habit *h, time_t jetzt);

// Traegt ein (Abschnitt 4.4), speichert und stellt die Wecker neu. false, wenn
// die Uhrzeit nicht gestellt ist.
bool eintragen(uint16_t id, time_t jetzt, Lage *nachher);
// Nimmt den letzten Eintrag zurueck.
bool eintragen_rueckgaengig(uint16_t id);
// "Skip": keine Erinnerung mehr bis zum Ende des laufenden Zeitraums, nach
// Plan bis zur naechsten geplanten Zeit.
void eintragen_auslassen(uint16_t id, time_t jetzt);
// Bis `bis` ist jede faellige Erinnerung gezeigt (planer_faellig).
void eintragen_erinnert(uint16_t id, time_t bis);
// Nach dem Aendern: ein "Skip" von vorher gilt nicht mehr.
void eintragen_pause_aufheben(uint16_t id);

// Eine Zeile wie "Today 3/8", "Until 12:00 0/1" oder "Today 2 · max 3".
void eintragen_text(const Habit *h, const Lage *l, time_t jetzt, char *puffer, size_t groesse);
// Uhrzeit nach der Einstellung der Uhr: "20:00" oder "8:00 PM".
void eintragen_uhrzeit(int minute, char *puffer, size_t groesse);
