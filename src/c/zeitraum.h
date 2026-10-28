#pragma once
#include <pebble.h>
#include "habit.h"
#include "logbuch.h"

// Der Zeitraum, in dem Ziel oder Grenze einer Gewohnheit gilt - immer nach
// dem Kalender (habit_app.md, Abschnitt 3):
//   schlecht, frei, nach Plan   der Tag
//   einmal alle n Stunden       Bloecke ab Mitternacht (0-2, 2-4 ...); der
//                               letzte endet um Mitternacht
//   einmal alle n Tage          Bloecke ab dem Tag des Anlegens
//   einmal alle n Wochen        Kalenderwochen, ab der Woche des Anlegens
//   einmal alle n Monate        Kalendermonate, ab dem Monat des Anlegens

typedef struct {
  time_t von, bis;      //< [von, bis)
  Tag tag_von, tag_bis; //< die Tage [tag_von, tag_bis)
  int8_t stunde_von;    //< bei Stunden-Bloecken: [stunde_von, stunde_bis) am Tag tag_von;
  int8_t stunde_bis;    //< sonst -1
} Zeitraum;

Zeitraum zeitraum(const Habit *h, time_t t, int erster_wochentag);
// Eintraege im Zeitraum. Bei Stunden-Bloecken aus den Stunden: aelter als
// sieben Tage sind sie nicht mehr da, dann 0.
int zeitraum_zahl(const Logbuch *lb, const Zeitraum *z);
// Dauert der Zeitraum einen Tag oder laenger?
bool zeitraum_tage(const Zeitraum *z);

// Was im Zeitraum fehlt, damit er erfuellt ist: das Ziel `line`, ohne Ziel
// ein Eintrag. Nach Plan das Ziel des Tages, ohne Ziel so viele Eintraege,
// wie Zeiten fuer den Tag geplant sind (0 an einem freien Tag).
int habit_ziel(const Habit *h, Tag tag);

typedef enum {
  STAND_OFFEN,     //< gut: Ziel nicht erreicht; schlecht: unter 80 % der Grenze
  STAND_ERFUELLT,  //< gut: Ziel erreicht
  STAND_FREI,      //< gut, nach Plan: heute nichts geplant
  STAND_NAHE,      //< schlecht: ab 80 % der Grenze
  STAND_ERREICHT,  //< schlecht: genau an der Grenze
  STAND_UEBER,     //< schlecht: darueber
} Stand;

Stand habit_stand(const Habit *h, int zahl, int ziel);

// Ist die Gewohnheit `tag` nach Plan dran?
bool habit_geplant_am(const Habit *h, Tag tag);
