// Logbuch: Stunden und Tage zaehlen, der Ring rueckt vor, Rueckgaengig,
// Speichern nur der geaenderten Faecher.
#include <pebble.h>
#include "logbuch.h"
#include "speicher.h"
#include "host/pruefen.h"

static Logbuch s_lb;

static void prv_eintragen(void) {
  logbuch_leeren(&s_lb, 7);
  const time_t t = ort(2026, 10, 28, 14, 35);
  const Tag heute = kalender_tag(t);
  PRUEFE(logbuch_eintragen(&s_lb, t));
  PRUEFE(logbuch_eintragen(&s_lb, t + 60));
  PRUEFE(logbuch_eintragen(&s_lb, ort(2026, 10, 28, 9, 0)));
  GLEICH(logbuch_tag(&s_lb, heute), 3);
  GLEICH(logbuch_tag(&s_lb, heute - 1), 0);
  GLEICH(logbuch_tag(&s_lb, heute + 1), 0);
  uint8_t st[24];
  PRUEFE(logbuch_stunden(&s_lb, heute, st));
  GLEICH(st[14], 2);
  GLEICH(st[9], 1);
  GLEICH(st[10], 0);
  GLEICH(s_lb.letzter_log, ort(2026, 10, 28, 9, 0));
  GLEICH(s_lb.vorletzter_log, t + 60);

  // Am naechsten Tag: die Zeile von gestern rutscht eins weiter.
  PRUEFE(logbuch_eintragen(&s_lb, ort(2026, 10, 29, 7, 15)));
  GLEICH(logbuch_tag(&s_lb, heute + 1), 1);
  GLEICH(logbuch_tag(&s_lb, heute), 3);
  PRUEFE(logbuch_stunden(&s_lb, heute, st));
  GLEICH(st[14], 2);
  PRUEFE(logbuch_stunden(&s_lb, heute + 1, st));
  GLEICH(st[7], 1);
  GLEICH(logbuch_tage(&s_lb, heute - 5, heute + 5), 4);

  // Nach dem letzten Eintrag ist nichts eingetragen.
  PRUEFE(logbuch_stunden(&s_lb, heute + 3, st));
  GLEICH(st[7], 0);

  // Sieben Tage spaeter sind die Stunden von heute weg, der Tag bleibt.
  PRUEFE(logbuch_eintragen(&s_lb, ort(2026, 11, 4, 12, 0)));
  PRUEFE(logbuch_stunden(&s_lb, heute + 1, st));
  PRUEFE(!logbuch_stunden(&s_lb, heute, st));
  GLEICH(logbuch_tag(&s_lb, heute), 3);
}

static void prv_ring(void) {
  logbuch_leeren(&s_lb, 7);
  const Tag a = kalender_tag_aus_datum(2026, 1, 10);
  PRUEFE(logbuch_eintragen(&s_lb, kalender_zeit(a, 600)));
  // 743 Tage spaeter liegt a noch im Ring, 744 Tage spaeter nicht mehr.
  PRUEFE(logbuch_eintragen(&s_lb, kalender_zeit(a + LOGBUCH_TAGE - 1, 600)));
  GLEICH(logbuch_tag(&s_lb, a), 1);
  GLEICH(logbuch_tage(&s_lb, a, a + LOGBUCH_TAGE), 2);
  PRUEFE(logbuch_eintragen(&s_lb, kalender_zeit(a + LOGBUCH_TAGE, 600)));
  GLEICH(logbuch_tag(&s_lb, a), 0);
  GLEICH(logbuch_tag(&s_lb, a + LOGBUCH_TAGE), 1);
  // Der Platz von a ist jetzt der von a + 744 - und nur der zaehlt.
  GLEICH(logbuch_tage(&s_lb, a - 10, a + LOGBUCH_TAGE + 1), 2);
  // Ein Eintrag vor dem Ring hat keinen Platz.
  PRUEFE(!logbuch_eintragen(&s_lb, kalender_zeit(a, 600)));
  // Eine Luecke ueber den ganzen Ring hinaus leert alles.
  PRUEFE(logbuch_eintragen(&s_lb, kalender_zeit(a + 3 * LOGBUCH_TAGE, 600)));
  GLEICH(logbuch_tage(&s_lb, a, a + 3 * LOGBUCH_TAGE + 1), 1);
  // Dazwischen beginnt jeder Tag bei 0, auch einer, der im Ring schon einmal
  // belegt war.
  logbuch_leeren(&s_lb, 7);
  for (int i = 0; i < 10; i++) PRUEFE(logbuch_eintragen(&s_lb, kalender_zeit(a + i, 600)));
  PRUEFE(logbuch_eintragen(&s_lb, kalender_zeit(a + LOGBUCH_TAGE + 5, 600)));
  GLEICH(logbuch_tag(&s_lb, a + LOGBUCH_TAGE + 4), 0);
  GLEICH(logbuch_tag(&s_lb, a + LOGBUCH_TAGE + 5), 1);
  GLEICH(logbuch_tage(&s_lb, a, a + 2 * LOGBUCH_TAGE), 1 + 4);
}

static void prv_grenzen(void) {
  logbuch_leeren(&s_lb, 7);
  // Eine Uhr, die nach dem Zuruecksetzen im Jahr 2000 steht, traegt nichts ein.
  PRUEFE(!logbuch_eintragen(&s_lb, ort(2000, 1, 1, 12, 0)));
  PRUEFE(!logbuch_eintragen(&s_lb, ort(2024, 12, 31, 23, 0)));
  GLEICH(s_lb.letzter_log, 0);
  const time_t t = ort(2026, 10, 28, 10, 0);
  for (int i = 0; i < 300; i++) logbuch_eintragen(&s_lb, t);
  GLEICH(logbuch_tag(&s_lb, kalender_tag(t)), 255);
  uint8_t st[24];
  logbuch_stunden(&s_lb, kalender_tag(t), st);
  GLEICH(st[10], 255);
}

static void prv_rueckgaengig(void) {
  logbuch_leeren(&s_lb, 7);
  PRUEFE(!logbuch_rueckgaengig(&s_lb));
  const time_t a = ort(2026, 10, 27, 22, 0), b = ort(2026, 10, 28, 8, 30);
  logbuch_eintragen(&s_lb, a);
  logbuch_eintragen(&s_lb, b);
  PRUEFE(logbuch_rueckgaengig(&s_lb));
  GLEICH(logbuch_tag(&s_lb, kalender_tag(b)), 0);
  GLEICH(logbuch_tag(&s_lb, kalender_tag(a)), 1);
  uint8_t st[24];
  logbuch_stunden(&s_lb, kalender_tag(b), st);
  GLEICH(st[8], 0);
  GLEICH(s_lb.letzter_log, a);
  // Nur ein Schritt zurueck: davor ist nichts mehr bekannt.
  GLEICH(s_lb.vorletzter_log, 0);
  PRUEFE(logbuch_rueckgaengig(&s_lb));
  GLEICH(logbuch_tag(&s_lb, kalender_tag(a)), 0);
  GLEICH(s_lb.letzter_log, 0);
  PRUEFE(!logbuch_rueckgaengig(&s_lb));
}

static void prv_speichern(void) {
  attrappe_persist_leeren();
  logbuch_leeren(&s_lb, 7);
  const time_t t = ort(2026, 10, 28, 14, 35);
  logbuch_eintragen(&s_lb, t);
  logbuch_pause(&s_lb, t + 3600);
  logbuch_speichern(&s_lb, 3);
  // Beim ersten Eintrag wird der ganze Ring geleert: alle vier Faecher.
  GLEICH(attrappe_schreibvorgaenge(), 4);
  Logbuch b;
  logbuch_laden(&b, 3, 7);
  GLEICH(logbuch_tag(&b, kalender_tag(t)), 1);
  GLEICH(b.pause_bis, t + 3600);
  GLEICH(b.letzter_log, t);
  GLEICH(b.geaendert, 0);
  // Danach schreibt ein Eintrag den Kopf und das Fach seines Tages.
  logbuch_eintragen(&b, t + 60);
  logbuch_speichern(&b, 3);
  GLEICH(attrappe_schreibvorgaenge(), 6);
  logbuch_speichern(&b, 3);
  GLEICH(attrappe_schreibvorgaenge(), 6);
  // Ein Logbuch, das einer anderen Gewohnheit gehoerte, gilt als leer.
  logbuch_laden(&b, 3, 8);
  GLEICH(logbuch_tag(&b, kalender_tag(t)), 0);
  GLEICH(b.habit_id, 8);
  logbuch_loeschen(3);
  PRUEFE(!persist_exists(FACH_LOG(3, 0)));
  PRUEFE(!persist_exists(FACH_LOG(3, 3)));
  logbuch_laden(&b, 3, 7);
  GLEICH(b.letzter_log, 0);
}

int main(void) {
  prv_eintragen();
  prv_ring();
  prv_grenzen();
  prv_rueckgaengig();
  prv_speichern();
  PRUEF_ENDE("logbuch");
}
