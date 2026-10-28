// Diagramme: Tag laufend oder je Stunde, Woche, Monat, Jahr; wo die Linie steht.
#include <pebble.h>
#include "diagramm_daten.h"
#include "host/pruefen.h"

static Logbuch s_lb;
static Diagramm s_d;

static Habit prv_habit(HabitType typ, int line) {
  Habit h;
  habit_vorgabe(&h);
  h.id = 3;
  h.type = (uint8_t)typ;
  h.line = (uint16_t)line;
  h.created = (int32_t)ort(2026, 1, 5, 9, 0);
  return h;
}

static void prv_tag(void) {
  const time_t jetzt = ort(2026, 10, 28, 14, 20);
  Habit h = prv_habit(HABIT_GOOD, 8);
  logbuch_leeren(&s_lb, 3);
  logbuch_eintragen(&s_lb, ort(2026, 10, 28, 7, 10));
  logbuch_eintragen(&s_lb, ort(2026, 10, 28, 7, 50));
  logbuch_eintragen(&s_lb, ort(2026, 10, 28, 12, 0));
  diagramm_rechnen(&h, &s_lb, ANSICHT_TAG, 0, jetzt, 0, &s_d);
  GLEICH(s_d.n, 24);
  PRUEFE(s_d.laufend);
  GLEICH(s_d.wert[6], 0);
  GLEICH(s_d.wert[7], 2);
  GLEICH(s_d.wert[11], 2);
  GLEICH(s_d.wert[12], 3);
  GLEICH(s_d.wert[23], 3);
  GLEICH(s_d.summe, 3);
  GLEICH(s_d.linie, 8);
  PRUEFE(!(s_d.zukunft & (1u << 14)));
  PRUEFE(s_d.zukunft & (1u << 15));
  char t[24];
  diagramm_titel(&s_d, kalender_tag(jetzt), t, sizeof(t));
  PRUEFE(strcmp(t, "Today") == 0);
  // Gestern: nichts in der Zukunft.
  diagramm_rechnen(&h, &s_lb, ANSICHT_TAG, 1, jetzt, 0, &s_d);
  GLEICH(s_d.zukunft, 0);
  diagramm_titel(&s_d, kalender_tag(jetzt), t, sizeof(t));
  PRUEFE(strcmp(t, "Yesterday") == 0);
  diagramm_rechnen(&h, &s_lb, ANSICHT_TAG, 3, jetzt, 0, &s_d);
  diagramm_titel(&s_d, kalender_tag(jetzt), t, sizeof(t));
  PRUEFE(strcmp(t, "Sun 25 Oct") == 0);
  // Stunden-Bloecke: je Stunde, die Linie nur bei Bloecken zu einer Stunde.
  h.period_type = PERIOD_ONCE_PER;
  h.period_unit = UNIT_HOURS;
  h.period_count = 2;
  h.line = 1;
  diagramm_rechnen(&h, &s_lb, ANSICHT_TAG, 0, jetzt, 0, &s_d);
  PRUEFE(!s_d.laufend);
  GLEICH(s_d.wert[7], 2);
  GLEICH(s_d.wert[12], 1);
  GLEICH(s_d.linie, -1);
  h.period_count = 1;
  diagramm_rechnen(&h, &s_lb, ANSICHT_TAG, 0, jetzt, 0, &s_d);
  GLEICH(s_d.linie, 1);
  // Woechentlich: keine Tageslinie.
  h.period_unit = UNIT_WEEKS;
  diagramm_rechnen(&h, &s_lb, ANSICHT_TAG, 0, jetzt, 0, &s_d);
  PRUEFE(s_d.laufend);
  GLEICH(s_d.linie, -1);
  // Aelter als sieben Tage: nur die Summe.
  logbuch_eintragen(&s_lb, ort(2026, 11, 6, 9, 0));
  diagramm_rechnen(&h, &s_lb, ANSICHT_TAG, 9, ort(2026, 11, 6, 9, 0), 0, &s_d);
  PRUEFE(s_d.ohne_stunden);
  GLEICH(s_d.summe, 3);
  GLEICH(diagramm_zurueck_max(ANSICHT_TAG), 6);
}

static void prv_woche_monat(void) {
  const time_t jetzt = ort(2026, 10, 28, 14, 20); // Mittwoch
  Habit h = prv_habit(HABIT_BAD, 3);
  logbuch_leeren(&s_lb, 3);
  for (int i = 0; i < 4; i++) logbuch_eintragen(&s_lb, ort(2026, 10, 26, 9, i));
  logbuch_eintragen(&s_lb, ort(2026, 10, 28, 10, 0));
  logbuch_eintragen(&s_lb, ort(2026, 10, 1, 10, 0));
  diagramm_rechnen(&h, &s_lb, ANSICHT_WOCHE, 0, jetzt, 0, &s_d);
  GLEICH(s_d.n, 7);
  GLEICH(s_d.tag_von, kalender_tag_aus_datum(2026, 10, 26));
  GLEICH(s_d.wert[0], 4);
  GLEICH(s_d.wert[2], 1);
  GLEICH(s_d.summe, 5);
  GLEICH(s_d.tage, 3);
  GLEICH(s_d.zukunft, 0x78);
  GLEICH(s_d.linie, 3);
  char t[24];
  diagramm_titel(&s_d, kalender_tag(jetzt), t, sizeof(t));
  PRUEFE(strcmp(t, "This week") == 0);
  diagramm_marke(&s_d, 0, true, t, sizeof(t));
  PRUEFE(strcmp(t, "M") == 0);
  // Mit Sonntag zuerst beginnt die Woche am 25.
  diagramm_rechnen(&h, &s_lb, ANSICHT_WOCHE, 0, jetzt, 6, &s_d);
  GLEICH(s_d.tag_von, kalender_tag_aus_datum(2026, 10, 25));
  GLEICH(s_d.wert[1], 4);
  diagramm_marke(&s_d, 0, true, t, sizeof(t));
  PRUEFE(strcmp(t, "S") == 0);
  diagramm_rechnen(&h, &s_lb, ANSICHT_WOCHE, 4, jetzt, 0, &s_d);
  diagramm_titel(&s_d, kalender_tag(jetzt), t, sizeof(t));
  PRUEFE(strcmp(t, "28 Sep - 4 Oct") == 0);
  diagramm_rechnen(&h, &s_lb, ANSICHT_WOCHE, 3, jetzt, 0, &s_d);
  diagramm_titel(&s_d, kalender_tag(jetzt), t, sizeof(t));
  PRUEFE(strcmp(t, "5-11 Oct") == 0);

  diagramm_rechnen(&h, &s_lb, ANSICHT_MONAT, 0, jetzt, 0, &s_d);
  GLEICH(s_d.n, 31);
  GLEICH(s_d.wert[0], 1);
  GLEICH(s_d.wert[25], 4);
  GLEICH(s_d.summe, 6);
  GLEICH(s_d.tage, 28);
  PRUEFE(s_d.zukunft & (1u << 28));
  PRUEFE(!(s_d.zukunft & (1u << 27)));
  diagramm_titel(&s_d, kalender_tag(jetzt), t, sizeof(t));
  PRUEFE(strcmp(t, "October 2026") == 0);
  diagramm_rechnen(&h, &s_lb, ANSICHT_MONAT, 8, jetzt, 0, &s_d);
  GLEICH(s_d.n, 28); // Februar 2026
  GLEICH(s_d.zukunft, 0);
}

static void prv_jahr(void) {
  const time_t jetzt = ort(2026, 3, 10, 12, 0);
  Habit h = prv_habit(HABIT_GOOD, 2);
  logbuch_leeren(&s_lb, 3);
  // Januar 31 Eintraege (1 je Tag), Maerz bis heute 20 (2 je Tag).
  for (int d = 1; d <= 31; d++) logbuch_eintragen(&s_lb, ort(2026, 1, d, 9, 0));
  for (int d = 1; d <= 10; d++) {
    logbuch_eintragen(&s_lb, ort(2026, 3, d, 9, 0));
    logbuch_eintragen(&s_lb, ort(2026, 3, d, 10, 0));
  }
  diagramm_rechnen(&h, &s_lb, ANSICHT_JAHR, 0, jetzt, 0, &s_d);
  GLEICH(s_d.n, 12);
  PRUEFE(s_d.zehntel);
  GLEICH(s_d.wert[0], 10); // 1,0 je Tag
  GLEICH(s_d.wert[1], 0);
  GLEICH(s_d.wert[2], 20); // 2,0 je Tag in den zehn Tagen bis heute
  GLEICH(s_d.linie, 20);
  GLEICH(s_d.summe, 51);
  GLEICH(s_d.tage, 31 + 28 + 10);
  PRUEFE(s_d.zukunft & (1u << 3));
  PRUEFE(!(s_d.zukunft & (1u << 2)));
  char t[24];
  diagramm_titel(&s_d, kalender_tag(jetzt), t, sizeof(t));
  PRUEFE(strcmp(t, "2026") == 0);
  diagramm_marke(&s_d, 11, true, t, sizeof(t));
  PRUEFE(strcmp(t, "D") == 0);
  // Einmal im Monat: Summen, keine Linie.
  h.period_type = PERIOD_ONCE_PER;
  h.period_unit = UNIT_MONTHS;
  diagramm_rechnen(&h, &s_lb, ANSICHT_JAHR, 0, jetzt, 0, &s_d);
  PRUEFE(!s_d.zehntel);
  GLEICH(s_d.wert[0], 31);
  GLEICH(s_d.wert[2], 20);
  GLEICH(s_d.linie, -1);
  // Vorjahr: alles vergangen.
  diagramm_rechnen(&h, &s_lb, ANSICHT_JAHR, 1, jetzt, 0, &s_d);
  GLEICH(s_d.zukunft, 0);
  GLEICH(s_d.summe, 0);
  GLEICH(s_d.tage, 365);
}

int main(void) {
  prv_tag();
  prv_woche_monat();
  prv_jahr();
  PRUEF_ENDE("diagramm");
}
