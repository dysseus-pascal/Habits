#pragma once
#include <pebble.h>
#include "habit.h"
#include "logbuch.h"
#include "zeitraum.h"

// Wann die Uhr an eine Gewohnheit erinnert (habit_app.md, Abschnitt 5). Reine
// Rechnung ohne Wecker und Speicher, damit sie auf dem Rechner geprueft wird
// (tools/test_planer.c); die Wecker stellt wecker.c.
//
// GUTE GEWOHNHEITEN, solange der Zeitraum nicht erfuellt ist:
//   Light     eine Erinnerung, wenn ein Zeitraum unerfuellt endet: zu Beginn
//             des naechsten Stunden-Blocks, sonst am naechsten Morgen in der
//             Bilanz (Beginn der aktiven Stunden)
//   Heavy     in zufaelligen Abstaenden innerhalb des Zeitraums, nur in den
//             aktiven Stunden
//   Critical  alle 5 Minuten ab Beginn des Zeitraums (fruehestens mit den
//             aktiven Stunden) bis zu seinem Ende
//   Nach Plan erinnert jede Stufe zur geplanten Zeit; Heavy danach bis zu drei
//             Stunden zufaellig, Critical alle 5 Minuten bis zur naechsten Zeit.
//             Ein Eintrag bis 30 Minuten vorher zaehlt fuer die Zeit.
// SCHLECHTE GEWOHNHEITEN erinnern nur mit Light: am naechsten Morgen in der
// Bilanz, wenn die Grenze ueberschritten war. Heavy und Critical warnen beim
// Eintragen (eintragen.c), dafuer braucht es keinen Wecker.
//
// Die Ruhezeit der Uhr laesst sich nicht vorausberechnen; sie gilt erst, wenn
// der Wecker kommt (wecker.c).

#define PLANER_WECKER_MAX 8
#define PLANER_VORLAUF_S 30          //< so weit muss ein Wecker in der Zukunft liegen
#define PLANER_ABSTAND_S 240         //< zwischen zwei eigenen Weckern
#define PLANER_RUECKBLICK_S 600      //< so spaet darf ein Wecker kommen
#define PLANER_VORGRIFF_S 120        //< so frueh
#define PLANER_KRITISCH_S 300
#define PLANER_FRUEHER_S 1800
#define PLANER_NACHPLAN_S (3 * 3600) //< Heavy nach einer geplanten Zeit
// So weit plant die Uhr voraus. Reicht fuer die Bilanz nach einem Monat und
// jeden Wochenplan; gesammelt wird ohnehin nur bis zum achten Termin.
#define PLANER_HORIZONT_S (400 * 86400)

typedef enum { TERMIN_ERINNERUNG = 1, TERMIN_BILANZ = 2 } TerminArt;

typedef struct {
  time_t zeit;
  uint16_t habit_id; //< 0 bei der Bilanz
  uint8_t art;       //< TerminArt
} Termin;

typedef struct {
  int aktiv_von, aktiv_bis; //< Minuten des Tages, aktiv_von < aktiv_bis <= 1440
  int erster_wochentag;
} Rahmen;

// Die Termine einer Gewohnheit nach `ab`, aufsteigend, hoechstens `max`.
int planer_termine(const Habit *h, const Logbuch *lb, time_t ab, const Rahmen *r, Termin *aus,
                   int max);

// Fuegt Termine in die aufsteigende Liste ein und behaelt die fruehesten `max`;
// eine zweite Bilanz zur selben Zeit faellt weg.
void planer_einfuegen(Termin *liste, int *n, int max, const Termin *neu, int anzahl);

// Rueckt die Termine auseinander, bis zwischen zweien PLANER_ABSTAND_S liegen:
// die Uhr nimmt keinen Wecker naeher als eine Minute am anderen an.
void planer_abstand(Termin *liste, int n);

// Ist jetzt eine Erinnerung faellig? Ja, wenn ein Termin in
// (erinnert, jetzt + PLANER_VORGRIFF_S] liegt und nicht aelter als
// PLANER_RUECKBLICK_S ist. So holt jeder Wecker alles nach, was um seine Zeit
// faellig ist - auch das, was der Abstand verschoben hat.
bool planer_faellig(const Habit *h, const Logbuch *lb, time_t jetzt, const Rahmen *r);

// Gehoert die Gewohnheit in die Morgenbilanz des Tages `heute`? Dann stehen in
// zahl und ziel die Werte des Zeitraums, der um Mitternacht endete.
bool planer_bilanz(const Habit *h, const Logbuch *lb, Tag heute, const Rahmen *r, int *zahl,
                   int *ziel);
