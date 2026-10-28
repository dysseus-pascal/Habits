// Gewohnheiten: bereinigen, anlegen, aendern, loeschen, Plaetze und ids.
#include <pebble.h>
#include "habit.h"
#include "logbuch.h"
#include "speicher.h"
#include "host/pruefen.h"

static Habit prv_neu(const char *name) {
  Habit h;
  habit_vorgabe(&h);
  strncpy(h.name, name, HABIT_NAME_LEN - 1);
  return h;
}

static void prv_bereinigen(void) {
  Habit h;
  memset(&h, 0xff, sizeof(h));
  habit_bereinigen(&h);
  GLEICH(h.name[HABIT_NAME_LEN - 1], 0);
  GLEICH(h.type, HABIT_GOOD);
  GLEICH(h.insistence, INSIST_NONE);
  GLEICH(h.period_type, PERIOD_FREE);
  GLEICH(h.period_unit, UNIT_DAYS);
  GLEICH(h.period_count, HABIT_ANZAHL_MAX);
  GLEICH(h.days, 0x7f);
  GLEICH(h.line, HABIT_LINE_MAX);
  PRUEFE(h.times_n >= 1 && h.times_n <= HABIT_ZEITEN_MAX);

  h = prv_neu("");
  habit_bereinigen(&h);
  PRUEFE(strcmp(h.name, "Habit") == 0);

  h = prv_neu("Pills");
  h.period_unit = UNIT_HOURS;
  h.period_count = 30;
  h.times_n = 4;
  h.times[0] = 20 * 60;
  h.times[1] = 8 * 60;
  h.times[2] = 20 * 60;
  h.times[3] = 1440 + 12 * 60; // ausserhalb des Tages: 12:00
  h.days = 0;
  habit_bereinigen(&h);
  GLEICH(h.period_count, 24);
  GLEICH(h.times_n, 3);
  GLEICH(h.times[0], 8 * 60);
  GLEICH(h.times[1], 12 * 60);
  GLEICH(h.times[2], 20 * 60);
  GLEICH(h.times[3], 0);
  GLEICH(h.days, 0x7f);

  h = prv_neu("x");
  h.times_n = 0;
  habit_bereinigen(&h);
  GLEICH(h.times_n, 1);
  h.period_count = 0;
  habit_bereinigen(&h);
  GLEICH(h.period_count, 1);
}

static void prv_sammlung(void) {
  attrappe_persist_leeren();
  habits_laden();
  GLEICH(habits_anzahl(), 0);
  PRUEFE(habits_nr(0) == NULL);
  const time_t jetzt = ort(2026, 10, 28, 10, 0);
  Habit w = prv_neu("Water");
  w.line = 8;
  const uint16_t a = habits_speichern(&w, jetzt);
  Habit c = prv_neu("Coffee");
  c.type = HABIT_BAD;
  const uint16_t b = habits_speichern(&c, jetzt + 5);
  GLEICH(a, 1);
  GLEICH(b, 2);
  GLEICH(habits_anzahl(), 2);
  PRUEFE(strcmp(habits_nr(0)->name, "Water") == 0);
  GLEICH(habits_nr(1)->created, jetzt + 5);
  GLEICH(habits_mit_id(a)->line, 8);

  // Aendern behaelt id, Platz und created.
  Habit w2 = *habits_mit_id(a);
  w2.line = 6;
  GLEICH(habits_speichern(&w2, jetzt + 100), a);
  GLEICH(habits_mit_id(a)->line, 6);
  GLEICH(habits_mit_id(a)->created, jetzt);
  GLEICH(habits_anzahl(), 2);

  // Nach dem Laden steht alles wieder da.
  habits_laden();
  GLEICH(habits_anzahl(), 2);
  GLEICH(habits_mit_id(a)->line, 6);
  GLEICH(habits_mit_id(b)->type, HABIT_BAD);

  // Loeschen nimmt das Logbuch mit; die neue auf dem freien Platz beginnt
  // leer und bekommt eine neue id.
  const int platz = habits_platz(a);
  Logbuch lb;
  logbuch_laden(&lb, platz, a);
  logbuch_eintragen(&lb, jetzt);
  logbuch_speichern(&lb, platz);
  habits_loeschen(a);
  GLEICH(habits_anzahl(), 1);
  PRUEFE(habits_mit_id(a) == NULL);
  PRUEFE(!persist_exists(FACH_LOG(platz, 0)));
  PRUEFE(!persist_exists(FACH_HABIT(platz)));
  const uint16_t d = habits_speichern(&w, jetzt);
  GLEICH(d, 3);
  GLEICH(habits_platz(d), platz);
  // In der Reihenfolge des Anlegens: Coffee vor der neuen Water.
  GLEICH(habits_nr(0)->id, b);
  GLEICH(habits_nr(1)->id, d);

  // Voll ist voll.
  for (int i = habits_anzahl(); i < HABITS_MAX; i++) PRUEFE(habits_speichern(&w, jetzt) != 0);
  GLEICH(habits_anzahl(), HABITS_MAX);
  GLEICH(habits_speichern(&w, jetzt), 0);

  // Ein kaputtes Fach (zu kurz, id 0) wird uebergangen.
  attrappe_persist_leeren();
  const uint16_t null = 0;
  persist_write_data(FACH_HABIT(4), &null, sizeof(null));
  habits_laden();
  GLEICH(habits_anzahl(), 0);
}

int main(void) {
  prv_bereinigen();
  prv_sammlung();
  PRUEF_ENDE("habit");
}
