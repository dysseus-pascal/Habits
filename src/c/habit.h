#pragma once
#include <pebble.h>

// Eine Gewohnheit, wie sie die Anfrage beschreibt (habit_app.md, Abschnitt 3).
// Die Feldnamen sind die der Anfrage, damit ein spaeterer Abgleich mit dem
// Telefon dieselben Namen spricht.
//
// `line` IST EIN FELD FUER ZWEI DINGE: bei einer guten Gewohnheit das Ziel,
// bei einer schlechten die Grenze, 0 heisst keins. Ein Wechsel des Typs
// aendert nur, wie die Zahl gelesen wird.

#define HABITS_MAX 20
#define HABIT_NAME_LEN 32
#define HABIT_ZEITEN_MAX 4
#define HABIT_LINE_MAX 99
#define HABIT_ANZAHL_MAX 99

typedef enum { HABIT_GOOD = 0, HABIT_BAD, HABIT_TYPE_COUNT } HabitType;
typedef enum { INSIST_NONE = 0, INSIST_LIGHT, INSIST_HEAVY, INSIST_CRITICAL, INSIST_COUNT } Insistence;
typedef enum { PERIOD_FREE = 0, PERIOD_ONCE_PER, PERIOD_SCHEDULED, PERIOD_COUNT } PeriodType;
typedef enum { UNIT_HOURS = 0, UNIT_DAYS, UNIT_WEEKS, UNIT_MONTHS, UNIT_COUNT } PeriodUnit;

// So liegt sie im Speicher der Uhr (ein persist-Fach je Gewohnheit). Neue
// Felder nur hinten anfuegen und HABIT_FASSUNG erhoehen.
typedef struct {
  uint16_t id;                         //< 0 = Platz frei
  char name[HABIT_NAME_LEN];
  uint8_t type;                        //< HabitType
  uint8_t insistence;                  //< Insistence
  uint8_t period_type;                 //< PeriodType, nur bei guten
  uint8_t period_unit;                 //< PeriodUnit, fuer PERIOD_ONCE_PER
  uint16_t period_count;               //< 1..99, fuer PERIOD_ONCE_PER
  uint8_t days;                        //< PERIOD_SCHEDULED: Bit 0 = Montag ... Bit 6 = Sonntag
  uint8_t times_n;                     //< PERIOD_SCHEDULED: belegte Zeiten
  uint16_t times[HABIT_ZEITEN_MAX];    //< Minuten nach Mitternacht, aufsteigend
  uint16_t line;                       //< Ziel (GOOD) oder Grenze (BAD), 0 = keins
  int32_t created;
} Habit;

// Eine neue Gewohnheit mit den Vorgaben der Anfrage: gut, ohne Nachdruck,
// frei, line 0.
void habit_vorgabe(Habit *h);

// Bringt jedes Feld in seinen Bereich, sortiert die Zeiten und wirft doppelte
// weg. Nach dem Laden und vor dem Speichern: so kommt nichts Ungueltiges in
// die Planung, auch nicht aus einem alten oder kaputten Fach.
void habit_bereinigen(Habit *h);
// Nur die Zeiten: aufsteigend, ohne doppelte, mindestens eine.
void habit_zeiten_ordnen(Habit *h);

// --- Die Sammlung im Speicher der Uhr ---

void habits_laden(void);
int habits_anzahl(void);
// In der Reihenfolge des Anlegens (nach id).
const Habit *habits_nr(int i);
const Habit *habits_mit_id(uint16_t id);
// Platz im Speicher (0..HABITS_MAX-1) der Gewohnheit mit dieser id, sonst -1.
// Das Logbuch einer Gewohnheit liegt in den Faechern ihres Platzes.
int habits_platz(uint16_t id);

// Neu (id 0): bekommt id, Platz und `created`. Sonst ersetzt sie die
// gespeicherte mit derselben id. Rueckgabe: die id, 0 wenn kein Platz frei ist.
uint16_t habits_speichern(const Habit *h, time_t jetzt);

// Loescht die Gewohnheit und ihr ganzes Logbuch.
void habits_loeschen(uint16_t id);
