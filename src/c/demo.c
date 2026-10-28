#include "pruef.h"
#ifdef HABITS_DEMO
#include "habit.h"
#include "logbuch.h"

// Beispieldaten fuer die Bilder (tools/screenshots.sh): sechs Gewohnheiten mit
// gut einem Jahr Verlauf, damit Woche, Monat und Jahr etwas zeigen. Die
// Zufallszahlen sind fest (ein einfacher Generator mit festem Start), so
// sehen die Bilder bei jedem Lauf gleich aus.

static uint32_t s_zufall = 12345;

static int prv_zufall(int lo, int hi) {
  s_zufall = s_zufall * 1103515245u + 12345u;
  return lo + (int)((s_zufall >> 16) % (uint32_t)(hi - lo + 1));
}

typedef struct {
  const char *name;
  uint8_t typ, nachdruck, zeitraum, einheit;
  uint16_t anzahl, line;
  uint8_t lo, hi;    //< Eintraege je Tag
  uint8_t heute;     //< davon heute (bis HABITS_DEMO)
} Vorlage;

static const Vorlage s_vorlagen[] = {
  {"Water", HABIT_GOOD, INSIST_LIGHT, PERIOD_FREE, UNIT_DAYS, 1, 8, 3, 10, 5},
  {"Walk", HABIT_GOOD, INSIST_HEAVY, PERIOD_FREE, UNIT_DAYS, 1, 0, 0, 2, 1},
  {"Coffee", HABIT_BAD, INSIST_HEAVY, PERIOD_FREE, UNIT_DAYS, 1, 3, 1, 5, 2},
  {"Sweets", HABIT_BAD, INSIST_LIGHT, PERIOD_FREE, UNIT_DAYS, 1, 1, 0, 3, 2},
  {"Pills", HABIT_GOOD, INSIST_CRITICAL, PERIOD_SCHEDULED, UNIT_DAYS, 1, 0, 1, 2, 1},
  {"Call mom", HABIT_GOOD, INSIST_LIGHT, PERIOD_ONCE_PER, UNIT_WEEKS, 1, 0, 0, 1, 0},
};

void demo_anlegen(void) {
  if (habits_anzahl()) return;
  const time_t jetzt = HABITS_DEMO;
  const Tag heute = kalender_tag(jetzt);
  Logbuch *lb = malloc(sizeof(Logbuch));
  if (!lb) return;
  for (unsigned i = 0; i < sizeof(s_vorlagen) / sizeof(s_vorlagen[0]); i++) {
    const Vorlage *v = &s_vorlagen[i];
    Habit h;
    habit_vorgabe(&h);
    strncpy(h.name, v->name, HABIT_NAME_LEN - 1);
    h.type = v->typ;
    h.insistence = v->nachdruck;
    h.period_type = v->zeitraum;
    h.period_unit = v->einheit;
    h.period_count = v->anzahl;
    h.line = v->line;
    if (v->zeitraum == PERIOD_SCHEDULED) {
      h.times_n = 2;
      h.times[0] = 8 * 60;
      h.times[1] = 20 * 60;
    }
    const uint16_t id = habits_speichern(&h, jetzt - 400 * 86400);
    const int platz = habits_platz(id);
    logbuch_leeren(lb, id);
    for (Tag d = heute - 400; d <= heute; d++) {
      // Die Woche hat ihren Rhythmus: am Wochenende weniger Wasser, mehr Kaffee.
      const bool wochenende = kalender_wochentag(d) >= 5;
      int n = d == heute ? v->heute : prv_zufall(v->lo, v->hi);
      if (d != heute && wochenende) n += v->typ == HABIT_BAD ? 1 : -1;
      if (v->zeitraum == PERIOD_ONCE_PER && d != heute) n = kalender_wochentag(d) == 6 && prv_zufall(0, 4);
      int minute = 7 * 60 + prv_zufall(0, 60);
      for (int k = 0; k < n; k++) {
        // Nach Plan kurz nach der geplanten Zeit, sonst ueber den Tag verteilt.
        if (v->zeitraum == PERIOD_SCHEDULED) minute = h.times[k % h.times_n] + prv_zufall(0, 20);
        const time_t t = kalender_zeit(d, minute);
        if (t > jetzt) break;
        logbuch_eintragen(lb, t);
        minute += prv_zufall(30, 180);
        if (minute >= 1440) break;
      }
    }
    logbuch_speichern(lb, platz);
  }
  free(lb);
}
#endif
