// Kalender: Tagnummern, Wochen, Monate und der Weg zur Sekunde, auch an den
// Tagen der Umstellung (TZ=Europe/Zurich).
#include <pebble.h>
#include "kalender.h"
#include "host/pruefen.h"

static void prv_tage(void) {
  GLEICH(kalender_tag_aus_datum(2000, 1, 1), 0);
  GLEICH(kalender_tag_aus_datum(2025, 1, 1), KALENDER_TAG_GUELTIG);
  GLEICH(kalender_tag_aus_datum(2000, 3, 1), 60); // 2000 ist ein Schaltjahr
  GLEICH(kalender_tag_aus_datum(2100, 3, 1) - kalender_tag_aus_datum(2100, 2, 28), 1); // 2100 nicht
  // Hin und zurueck fuer jeden Tag ueber 120 Jahre.
  bool alle = true;
  for (Tag t = -1000; t < 43000; t++) {
    int j, m, d;
    kalender_datum(t, &j, &m, &d);
    if (kalender_tag_aus_datum(j, m, d) != t) alle = false;
  }
  PRUEFE(alle);
  int j, m, d;
  kalender_datum(kalender_tag_aus_datum(2026, 10, 28), &j, &m, &d);
  GLEICH(j, 2026);
  GLEICH(m, 10);
  GLEICH(d, 28);
}

static void prv_wochen(void) {
  GLEICH(kalender_wochentag(0), 5);                                  // Sa 1.1.2000
  GLEICH(kalender_wochentag(kalender_tag_aus_datum(2026, 10, 28)), 2); // Mittwoch
  GLEICH(kalender_wochentag(kalender_tag_aus_datum(2026, 11, 1)), 6);  // Sonntag
  GLEICH(kalender_wochentag(-1), 4);                                   // Fr 31.12.1999
  const Tag mi = kalender_tag_aus_datum(2026, 10, 28);
  GLEICH(kalender_wochenbeginn(mi, 0), kalender_tag_aus_datum(2026, 10, 26)); // Montag
  GLEICH(kalender_wochenbeginn(mi, 6), kalender_tag_aus_datum(2026, 10, 25)); // Sonntag
  const Tag so = kalender_tag_aus_datum(2026, 11, 1);
  GLEICH(kalender_wochenbeginn(so, 0), mi - 2);
  GLEICH(kalender_wochenbeginn(so, 6), so);
  attrappe_sprache = "en_US";
  GLEICH(kalender_erster_wochentag(), 6);
  attrappe_sprache = "de_CH";
  GLEICH(kalender_erster_wochentag(), 0);
  attrappe_sprache = "en_GB";
  GLEICH(kalender_erster_wochentag(), 0);
}

static void prv_monate(void) {
  const int32_t okt = kalender_monat(kalender_tag_aus_datum(2026, 10, 28));
  GLEICH(okt, 2026 * 12 + 9);
  GLEICH(kalender_monatsbeginn(okt), kalender_tag_aus_datum(2026, 10, 1));
  GLEICH(kalender_monatsbeginn(okt + 3), kalender_tag_aus_datum(2027, 1, 1));
  GLEICH(kalender_monatstage(okt), 31);
  GLEICH(kalender_monatstage(2024 * 12 + 1), 29);
  GLEICH(kalender_monatstage(2026 * 12 + 1), 28);
  GLEICH(kalender_monatstage(2026 * 12 + 10), 30);
}

static void prv_zeit(void) {
  const Tag tag = kalender_tag_aus_datum(2026, 10, 28);
  GLEICH(kalender_zeit(tag, 8 * 60), ort(2026, 10, 28, 8, 0));
  GLEICH(kalender_zeit(tag, 0), ort(2026, 10, 28, 0, 0));
  GLEICH(kalender_zeit(tag, 1440), ort(2026, 10, 29, 0, 0));
  GLEICH(kalender_tag(ort(2026, 10, 28, 23, 59) + 59), tag);
  GLEICH(kalender_tag(ort(2026, 10, 29, 0, 0)), tag + 1);
  GLEICH(kalender_minute(ort(2026, 10, 28, 14, 35)), 14 * 60 + 35);
  GLEICH(kalender_stunde(ort(2026, 10, 28, 14, 35)), 14);

  // Fruehling: am 29.3.2026 springt die Uhr um 2:00 auf 3:00.
  const Tag fr = kalender_tag_aus_datum(2026, 3, 29);
  GLEICH(kalender_zeit(fr, 0), ort(2026, 3, 29, 0, 0));
  GLEICH(kalender_zeit(fr, 90), ort(2026, 3, 29, 1, 30));
  GLEICH(kalender_zeit(fr, 8 * 60), ort(2026, 3, 29, 8, 0));
  GLEICH(kalender_zeit(fr, 1440) - kalender_zeit(fr, 0), 23 * 3600);
  // 2:30 gibt es nicht: eine Stunde spaeter, 3:30.
  GLEICH(kalender_minute(kalender_zeit(fr, 150)), 3 * 60 + 30);
  GLEICH(kalender_tag(kalender_zeit(fr, 150)), fr);

  // Herbst: am 25.10.2026 geht die Uhr um 3:00 auf 2:00 zurueck.
  const Tag he = kalender_tag_aus_datum(2026, 10, 25);
  GLEICH(kalender_zeit(he, 8 * 60), ort(2026, 10, 25, 8, 0));
  GLEICH(kalender_zeit(he, 1440) - kalender_zeit(he, 0), 25 * 3600);
  GLEICH(kalender_minute(kalender_zeit(he, 150)), 150); // 2:30 gibt es zweimal, eine davon

  // Jede Minute eines Jahres: die Wanduhr zeigt, was verlangt war, ausser in
  // der ausgefallenen Stunde.
  bool alle = true;
  for (Tag t = kalender_tag_aus_datum(2026, 1, 1); t < kalender_tag_aus_datum(2027, 1, 1); t++) {
    for (int m = 0; m < 1440; m += 7) {
      const time_t z = kalender_zeit(t, m);
      const bool luecke = t == fr && m >= 120 && m < 180;
      if (kalender_tag(z) != t || (!luecke && kalender_minute(z) != m)) {
        if (alle) printf("  kalender_zeit(%d, %d) = %s\n", (int)t, m, zeige(z));
        alle = false;
      }
    }
  }
  PRUEFE(alle);
}

int main(void) {
  prv_tage();
  prv_wochen();
  prv_monate();
  prv_zeit();
  PRUEF_ENDE("kalender");
}
