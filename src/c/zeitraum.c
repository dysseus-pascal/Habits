#include "zeitraum.h"

// Ganzzahlige Division, die nach unten rundet: ein Zeitpunkt vor dem Anlegen
// liegt im Block davor, nicht im ersten.
static int32_t prv_unten(int32_t a, int32_t b) {
  return a >= 0 ? a / b : -((-a + b - 1) / b);
}

static Zeitraum prv_tage(Tag von, Tag bis) {
  return (Zeitraum){
    .von = kalender_zeit(von, 0), .bis = kalender_zeit(bis, 0),
    .tag_von = von, .tag_bis = bis, .stunde_von = -1, .stunde_bis = -1,
  };
}

Zeitraum zeitraum(const Habit *h, time_t t, int erster_wochentag) {
  const Tag tag = kalender_tag(t);
  if (h->type == HABIT_BAD || h->period_type != PERIOD_ONCE_PER) return prv_tage(tag, tag + 1);
  const int32_t n = h->period_count > 0 ? h->period_count : 1;
  const Tag angelegt = kalender_tag(h->created);
  switch (h->period_unit) {
    case UNIT_HOURS: {
      if (n >= 24) return prv_tage(tag, tag + 1);
      const int von = kalender_stunde(t) / n * n;
      const int bis = von + n < 24 ? von + n : 24;
      return (Zeitraum){
        .von = kalender_zeit(tag, von * 60), .bis = kalender_zeit(tag, bis * 60),
        .tag_von = tag, .tag_bis = tag + 1, .stunde_von = (int8_t)von, .stunde_bis = (int8_t)bis,
      };
    }
    case UNIT_WEEKS: {
      const Tag anker = kalender_wochenbeginn(angelegt, erster_wochentag);
      const Tag woche = kalender_wochenbeginn(tag, erster_wochentag);
      const Tag von = anker + prv_unten(woche - anker, 7 * n) * 7 * n;
      return prv_tage(von, von + 7 * n);
    }
    case UNIT_MONTHS: {
      const int32_t anker = kalender_monat(angelegt);
      const int32_t von = anker + prv_unten(kalender_monat(tag) - anker, n) * n;
      return prv_tage(kalender_monatsbeginn(von), kalender_monatsbeginn(von + n));
    }
    case UNIT_DAYS:
    default: {
      const Tag von = angelegt + prv_unten(tag - angelegt, n) * n;
      return prv_tage(von, von + n);
    }
  }
}

int zeitraum_zahl(const Logbuch *lb, const Zeitraum *z) {
  if (z->stunde_von < 0) return logbuch_tage(lb, z->tag_von, z->tag_bis);
  uint8_t stunden[24];
  if (!logbuch_stunden(lb, z->tag_von, stunden)) return 0;
  int summe = 0;
  for (int s = z->stunde_von; s < z->stunde_bis; s++) summe += stunden[s];
  return summe;
}

bool zeitraum_tage(const Zeitraum *z) {
  return z->stunde_von < 0;
}

bool habit_geplant_am(const Habit *h, Tag tag) {
  return (h->days >> kalender_wochentag(tag)) & 1;
}

int habit_ziel(const Habit *h, Tag tag) {
  if (h->type == HABIT_BAD) return h->line;
  if (h->period_type == PERIOD_SCHEDULED && !h->line) {
    return habit_geplant_am(h, tag) ? h->times_n : 0;
  }
  return h->line > 0 ? h->line : 1;
}

Stand habit_stand(const Habit *h, int zahl, int ziel) {
  if (h->type == HABIT_BAD) {
    if (!ziel) return STAND_OFFEN;
    if (zahl > ziel) return STAND_UEBER;
    if (zahl == ziel) return STAND_ERREICHT;
    return zahl * 10 >= ziel * 8 ? STAND_NAHE : STAND_OFFEN;
  }
  if (!ziel) return STAND_FREI;
  return zahl >= ziel ? STAND_ERFUELLT : STAND_OFFEN;
}
