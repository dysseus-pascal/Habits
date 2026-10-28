// Zeitraeume nach dem Kalender, Zaehlen darin, Ziel und Stand.
#include <pebble.h>
#include "zeitraum.h"
#include "host/pruefen.h"

static Habit prv_einmal(int n, PeriodUnit einheit, time_t angelegt) {
  Habit h;
  habit_vorgabe(&h);
  h.id = 1;
  h.period_type = PERIOD_ONCE_PER;
  h.period_count = (uint16_t)n;
  h.period_unit = einheit;
  h.created = (int32_t)angelegt;
  return h;
}

static void prv_tag(void) {
  Habit h;
  habit_vorgabe(&h);
  const time_t t = ort(2026, 10, 28, 14, 0);
  Zeitraum z = zeitraum(&h, t, 0);
  GLEICH(z.von, ort(2026, 10, 28, 0, 0));
  GLEICH(z.bis, ort(2026, 10, 29, 0, 0));
  PRUEFE(zeitraum_tage(&z));
  h.type = HABIT_BAD;
  h.period_type = PERIOD_ONCE_PER; // bei schlechten ohne Bedeutung
  z = zeitraum(&h, t, 0);
  GLEICH(z.tag_bis - z.tag_von, 1);
  // Der Tag der Umstellung ist ein Tag mit 25 Stunden.
  habit_vorgabe(&h);
  z = zeitraum(&h, ort(2026, 10, 25, 12, 0), 0);
  GLEICH(z.bis - z.von, 25 * 3600);
}

static void prv_stunden(void) {
  const time_t angelegt = ort(2026, 10, 1, 9, 17);
  Habit h = prv_einmal(5, UNIT_HOURS, angelegt);
  Zeitraum z = zeitraum(&h, ort(2026, 10, 28, 14, 30), 0);
  GLEICH(z.stunde_von, 10);
  GLEICH(z.stunde_bis, 15);
  GLEICH(z.von, ort(2026, 10, 28, 10, 0));
  GLEICH(z.bis, ort(2026, 10, 28, 15, 0));
  PRUEFE(!zeitraum_tage(&z));
  // Der letzte Block endet um Mitternacht.
  z = zeitraum(&h, ort(2026, 10, 28, 23, 0), 0);
  GLEICH(z.stunde_von, 20);
  GLEICH(z.stunde_bis, 24);
  GLEICH(z.bis, ort(2026, 10, 29, 0, 0));
  // 24 Stunden sind der Tag.
  h.period_count = 24;
  z = zeitraum(&h, ort(2026, 10, 28, 23, 0), 0);
  PRUEFE(zeitraum_tage(&z));
  // Zaehlen aus den Stunden.
  h.period_count = 2;
  Logbuch lb;
  logbuch_leeren(&lb, 1);
  logbuch_eintragen(&lb, ort(2026, 10, 28, 9, 59));
  logbuch_eintragen(&lb, ort(2026, 10, 28, 10, 0));
  logbuch_eintragen(&lb, ort(2026, 10, 28, 11, 59));
  z = zeitraum(&h, ort(2026, 10, 28, 10, 30), 0);
  GLEICH(zeitraum_zahl(&lb, &z), 2);
  z = zeitraum(&h, ort(2026, 10, 28, 9, 30), 0);
  GLEICH(zeitraum_zahl(&lb, &z), 1);
}

static void prv_tage(void) {
  const time_t angelegt = ort(2026, 10, 20, 18, 0); // Dienstag
  Habit h = prv_einmal(3, UNIT_DAYS, angelegt);
  Zeitraum z = zeitraum(&h, ort(2026, 10, 28, 1, 0), 0);
  // Bloecke 20.-22., 23.-25., 26.-28., 29.-31.
  GLEICH(z.tag_von, kalender_tag_aus_datum(2026, 10, 26));
  GLEICH(z.tag_bis, kalender_tag_aus_datum(2026, 10, 29));
  // Vor dem Anlegen liegt der Block davor (abgerundet, nicht gegen 0).
  z = zeitraum(&h, ort(2026, 10, 19, 12, 0), 0);
  GLEICH(z.tag_von, kalender_tag_aus_datum(2026, 10, 17));
  Logbuch lb;
  logbuch_leeren(&lb, 1);
  logbuch_eintragen(&lb, ort(2026, 10, 25, 23, 0));
  logbuch_eintragen(&lb, ort(2026, 10, 26, 7, 0));
  logbuch_eintragen(&lb, ort(2026, 10, 28, 23, 59));
  z = zeitraum(&h, ort(2026, 10, 27, 1, 0), 0);
  GLEICH(zeitraum_zahl(&lb, &z), 2);
}

static void prv_wochen_monate(void) {
  const time_t angelegt = ort(2026, 10, 7, 12, 0); // Mittwoch
  Habit h = prv_einmal(2, UNIT_WEEKS, angelegt);
  Zeitraum z = zeitraum(&h, ort(2026, 10, 28, 12, 0), 0);
  // Ab Montag 5.10. in Bloecken zu zwei Wochen: 5.-18., 19.10.-1.11.
  GLEICH(z.tag_von, kalender_tag_aus_datum(2026, 10, 19));
  GLEICH(z.tag_bis, kalender_tag_aus_datum(2026, 11, 2));
  // Mit Sonntag als erstem Tag: ab 4.10.: 4.-17., 18.-31.10.
  z = zeitraum(&h, ort(2026, 10, 28, 12, 0), 6);
  GLEICH(z.tag_von, kalender_tag_aus_datum(2026, 10, 18));
  GLEICH(z.tag_bis, kalender_tag_aus_datum(2026, 11, 1));

  h = prv_einmal(1, UNIT_MONTHS, angelegt);
  z = zeitraum(&h, ort(2026, 10, 28, 12, 0), 0);
  GLEICH(z.tag_von, kalender_tag_aus_datum(2026, 10, 1));
  GLEICH(z.tag_bis, kalender_tag_aus_datum(2026, 11, 1));
  h.period_count = 3;
  z = zeitraum(&h, ort(2027, 2, 14, 12, 0), 0);
  // Okt-Dez, Jan-Mrz
  GLEICH(z.tag_von, kalender_tag_aus_datum(2027, 1, 1));
  GLEICH(z.tag_bis, kalender_tag_aus_datum(2027, 4, 1));
  z = zeitraum(&h, ort(2026, 9, 30, 12, 0), 0);
  GLEICH(z.tag_von, kalender_tag_aus_datum(2026, 7, 1));
}

static void prv_ziel_stand(void) {
  Habit h;
  habit_vorgabe(&h);
  const Tag mi = kalender_tag_aus_datum(2026, 10, 28);
  GLEICH(habit_ziel(&h, mi), 1); // ohne Ziel: einmal
  h.line = 8;
  GLEICH(habit_ziel(&h, mi), 8);
  GLEICH(habit_stand(&h, 7, 8), STAND_OFFEN);
  GLEICH(habit_stand(&h, 8, 8), STAND_ERFUELLT);
  GLEICH(habit_stand(&h, 9, 8), STAND_ERFUELLT);

  h.period_type = PERIOD_SCHEDULED;
  h.line = 0;
  h.days = 1 << 2; // nur mittwochs
  h.times_n = 2;
  GLEICH(habit_ziel(&h, mi), 2);
  GLEICH(habit_ziel(&h, mi + 1), 0);
  GLEICH(habit_stand(&h, 0, 0), STAND_FREI);
  h.line = 3;
  GLEICH(habit_ziel(&h, mi + 1), 3);

  h.type = HABIT_BAD;
  h.line = 5;
  GLEICH(habit_ziel(&h, mi), 5);
  GLEICH(habit_stand(&h, 3, 5), STAND_OFFEN);
  GLEICH(habit_stand(&h, 4, 5), STAND_NAHE);
  GLEICH(habit_stand(&h, 5, 5), STAND_ERREICHT);
  GLEICH(habit_stand(&h, 6, 5), STAND_UEBER);
  h.line = 0;
  GLEICH(habit_stand(&h, 30, 0), STAND_OFFEN); // ohne Grenze keine Warnung
}

int main(void) {
  prv_tag();
  prv_stunden();
  prv_tage();
  prv_wochen_monate();
  prv_ziel_stand();
  PRUEF_ENDE("zeitraum");
}
