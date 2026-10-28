// Planer: wann jede Stufe erinnert, Pause, Abstand, faellig, Morgenbilanz.
#include <pebble.h>
#include "planer.h"
#include "host/pruefen.h"

static const Rahmen R = {.aktiv_von = 8 * 60, .aktiv_bis = 21 * 60, .erster_wochentag = 0};
static Logbuch s_lb;
static Termin s_t[16];

static Habit prv_gut(Insistence stufe) {
  Habit h;
  habit_vorgabe(&h);
  h.id = 5;
  h.insistence = stufe;
  h.created = (int32_t)ort(2026, 10, 1, 12, 0);
  return h;
}

static int prv_plan(const Habit *h, time_t ab, int max) {
  return planer_termine(h, &s_lb, ab, &R, s_t, max);
}

static void prv_none(void) {
  Habit h = prv_gut(INSIST_NONE);
  logbuch_leeren(&s_lb, 5);
  GLEICH(prv_plan(&h, ort(2026, 10, 28, 10, 0), 8), 0);
  h.type = HABIT_BAD;
  h.line = 1;
  logbuch_eintragen(&s_lb, ort(2026, 10, 28, 9, 0));
  logbuch_eintragen(&s_lb, ort(2026, 10, 28, 9, 1));
  GLEICH(prv_plan(&h, ort(2026, 10, 28, 10, 0), 8), 0);
}

static void prv_light(void) {
  Habit h = prv_gut(INSIST_LIGHT);
  h.line = 2;
  logbuch_leeren(&s_lb, 5);
  const time_t jetzt = ort(2026, 10, 28, 10, 0);
  GLEICH(prv_plan(&h, jetzt, 2), 2);
  GLEICH(s_t[0].art, TERMIN_BILANZ);
  GLEICH(s_t[0].zeit, ort(2026, 10, 29, 8, 0));
  GLEICH(s_t[0].habit_id, 0);
  GLEICH(s_t[1].zeit, ort(2026, 10, 30, 8, 0));
  // Heute erfuellt: erst die Bilanz fuer morgen.
  logbuch_eintragen(&s_lb, jetzt - 60);
  logbuch_eintragen(&s_lb, jetzt - 30);
  GLEICH(prv_plan(&h, jetzt, 1), 1);
  GLEICH(s_t[0].zeit, ort(2026, 10, 30, 8, 0));

  // Stunden-Bloecke: Erinnerung zu Beginn des naechsten, in den aktiven Stunden.
  h = prv_gut(INSIST_LIGHT);
  h.period_type = PERIOD_ONCE_PER;
  h.period_unit = UNIT_HOURS;
  h.period_count = 2;
  logbuch_leeren(&s_lb, 5);
  GLEICH(prv_plan(&h, ort(2026, 10, 28, 10, 30), 8), 8);
  GLEICH(s_t[0].art, TERMIN_ERINNERUNG);
  GLEICH(s_t[0].habit_id, 5);
  GLEICH(s_t[0].zeit, ort(2026, 10, 28, 12, 0));
  GLEICH(s_t[4].zeit, ort(2026, 10, 28, 20, 0));
  // 22:00 liegt nach den aktiven Stunden, 0:00 auch; 6:00 davor; 8:00 zaehlt.
  GLEICH(s_t[5].zeit, ort(2026, 10, 29, 8, 0));
  // Im Block eingetragen: diese Erinnerung faellt weg.
  logbuch_eintragen(&s_lb, ort(2026, 10, 28, 10, 15));
  prv_plan(&h, ort(2026, 10, 28, 10, 30), 8);
  GLEICH(s_t[0].zeit, ort(2026, 10, 28, 14, 0));

  // Schlecht: nur ueber der Grenze, morgen frueh.
  h = prv_gut(INSIST_LIGHT);
  h.type = HABIT_BAD;
  h.line = 3;
  logbuch_leeren(&s_lb, 5);
  for (int i = 0; i < 3; i++) logbuch_eintragen(&s_lb, ort(2026, 10, 28, 9, i));
  GLEICH(prv_plan(&h, ort(2026, 10, 28, 10, 0), 8), 0);
  logbuch_eintragen(&s_lb, ort(2026, 10, 28, 9, 30));
  GLEICH(prv_plan(&h, ort(2026, 10, 28, 10, 0), 8), 1);
  GLEICH(s_t[0].art, TERMIN_BILANZ);
  GLEICH(s_t[0].zeit, ort(2026, 10, 29, 8, 0));
}

static void prv_heavy(void) {
  Habit h = prv_gut(INSIST_HEAVY);
  logbuch_leeren(&s_lb, 5);
  const time_t jetzt = ort(2026, 10, 28, 10, 0);
  const int n = prv_plan(&h, jetzt, 16);
  GLEICH(n, 16);
  bool richtig = true;
  for (int i = 0; i < n; i++) {
    const int m = kalender_minute(s_t[i].zeit);
    if (s_t[i].zeit <= jetzt || m < 8 * 60 || m >= 21 * 60) richtig = false;
    if (i > 0 && kalender_tag(s_t[i].zeit) == kalender_tag(s_t[i - 1].zeit)) {
      const time_t d = s_t[i].zeit - s_t[i - 1].zeit;
      if (d < 45 * 60 || d > 120 * 60) richtig = false;
    }
    if (!richtig) {
      printf("  heavy %d: %s\n", i, zeige(s_t[i].zeit));
      break;
    }
  }
  PRUEFE(richtig);
  // Morgen geht es nach 8:00 weiter.
  PRUEFE(kalender_tag(s_t[n - 1].zeit) == kalender_tag(jetzt) + 1);
  // Dieselbe Rechnung gibt dieselben Zeiten.
  Termin vorher[16];
  memcpy(vorher, s_t, sizeof(vorher));
  prv_plan(&h, jetzt + 60, 16);
  PRUEFE(memcmp(vorher, s_t, sizeof(vorher)) == 0);
  // Nach einem Eintrag (das Ziel ist 1): heute nichts mehr.
  logbuch_eintragen(&s_lb, jetzt);
  prv_plan(&h, jetzt, 4);
  GLEICH(kalender_tag(s_t[0].zeit), kalender_tag(jetzt) + 1);
  // Mit Ziel 3 und einem Eintrag: weiter, aber fruehestens 45 min danach.
  h.line = 3;
  prv_plan(&h, jetzt, 4);
  PRUEFE(s_t[0].zeit >= jetzt + 45 * 60);
  PRUEFE(kalender_tag(s_t[0].zeit) == kalender_tag(jetzt));

  // Stunden-Bloecke: Abstaende 15-40 min bei zwei Stunden, nur im Block.
  h = prv_gut(INSIST_HEAVY);
  h.period_type = PERIOD_ONCE_PER;
  h.period_unit = UNIT_HOURS;
  h.period_count = 2;
  logbuch_leeren(&s_lb, 5);
  logbuch_eintragen(&s_lb, ort(2026, 10, 28, 10, 5));
  prv_plan(&h, ort(2026, 10, 28, 10, 30), 4);
  // Der Block 10-12 ist erfuellt: erst ab 12:15.
  PRUEFE(s_t[0].zeit >= ort(2026, 10, 28, 12, 15));
  PRUEFE(s_t[0].zeit <= ort(2026, 10, 28, 12, 40));
}

static void prv_critical(void) {
  Habit h = prv_gut(INSIST_CRITICAL);
  logbuch_leeren(&s_lb, 5);
  GLEICH(prv_plan(&h, ort(2026, 10, 28, 10, 2), 3), 3);
  GLEICH(s_t[0].zeit, ort(2026, 10, 28, 10, 5));
  GLEICH(s_t[1].zeit, ort(2026, 10, 28, 10, 10));
  GLEICH(s_t[2].zeit, ort(2026, 10, 28, 10, 15));
  // Vor den aktiven Stunden: ab 8:00.
  prv_plan(&h, ort(2026, 10, 28, 6, 0), 1);
  GLEICH(s_t[0].zeit, ort(2026, 10, 28, 8, 0));
  // Nach den aktiven Stunden geht Critical weiter bis Mitternacht.
  GLEICH(prv_plan(&h, ort(2026, 10, 28, 23, 40), 8), 8);
  GLEICH(s_t[0].zeit, ort(2026, 10, 28, 23, 45));
  GLEICH(s_t[3].zeit, ort(2026, 10, 29, 8, 0));
  // Eingetragen: erst morgen.
  logbuch_eintragen(&s_lb, ort(2026, 10, 28, 10, 3));
  prv_plan(&h, ort(2026, 10, 28, 10, 4), 1);
  GLEICH(s_t[0].zeit, ort(2026, 10, 29, 8, 0));
  // Skip: Pause bis Mitternacht, auch ohne Eintrag.
  logbuch_leeren(&s_lb, 5);
  logbuch_pause(&s_lb, ort(2026, 10, 29, 0, 0));
  prv_plan(&h, ort(2026, 10, 28, 10, 4), 1);
  GLEICH(s_t[0].zeit, ort(2026, 10, 29, 8, 0));
  // Am Tag der Umstellung nach der Wanduhr.
  logbuch_leeren(&s_lb, 5);
  prv_plan(&h, ort(2026, 3, 29, 6, 0), 2);
  GLEICH(s_t[0].zeit, ort(2026, 3, 29, 8, 0));
  GLEICH(s_t[1].zeit, ort(2026, 3, 29, 8, 5));
}

static void prv_nach_plan(void) {
  Habit h = prv_gut(INSIST_LIGHT);
  h.period_type = PERIOD_SCHEDULED;
  h.times_n = 2;
  h.times[0] = 8 * 60;
  h.times[1] = 22 * 60; // ausserhalb der aktiven Stunden: der Plan gilt
  h.days = 0x1f;        // Mo-Fr
  logbuch_leeren(&s_lb, 5);
  GLEICH(prv_plan(&h, ort(2026, 10, 30, 10, 0), 3), 3); // Freitag
  GLEICH(s_t[0].zeit, ort(2026, 10, 30, 22, 0));
  GLEICH(s_t[1].zeit, ort(2026, 11, 2, 8, 0)); // Montag
  GLEICH(s_t[2].zeit, ort(2026, 11, 2, 22, 0));
  // Eine halbe Stunde vorher eingetragen: zaehlt fuer 22:00.
  logbuch_eintragen(&s_lb, ort(2026, 10, 30, 21, 35));
  prv_plan(&h, ort(2026, 10, 30, 21, 36), 1);
  GLEICH(s_t[0].zeit, ort(2026, 11, 2, 8, 0));

  // Critical: ab der Zeit alle 5 Minuten bis zur naechsten.
  h.insistence = INSIST_CRITICAL;
  logbuch_leeren(&s_lb, 5);
  GLEICH(prv_plan(&h, ort(2026, 10, 28, 8, 7), 3), 3);
  GLEICH(s_t[0].zeit, ort(2026, 10, 28, 8, 10));
  GLEICH(s_t[2].zeit, ort(2026, 10, 28, 8, 20));
  logbuch_eintragen(&s_lb, ort(2026, 10, 28, 8, 12));
  prv_plan(&h, ort(2026, 10, 28, 8, 13), 1);
  GLEICH(s_t[0].zeit, ort(2026, 10, 28, 22, 0));
  // Skip bis zur naechsten Zeit.
  logbuch_leeren(&s_lb, 5);
  logbuch_pause(&s_lb, ort(2026, 10, 28, 22, 0));
  prv_plan(&h, ort(2026, 10, 28, 8, 13), 1);
  GLEICH(s_t[0].zeit, ort(2026, 10, 28, 22, 0));

  // Heavy: zur Zeit, dann bis drei Stunden danach alle 20-45 Minuten.
  h.insistence = INSIST_HEAVY;
  logbuch_leeren(&s_lb, 5);
  const int n = prv_plan(&h, ort(2026, 10, 28, 7, 0), 12);
  GLEICH(s_t[0].zeit, ort(2026, 10, 28, 8, 0));
  bool richtig = true;
  int k = 1;
  for (; k < n && s_t[k].zeit < ort(2026, 10, 28, 12, 0); k++) {
    const time_t d = s_t[k].zeit - s_t[k - 1].zeit;
    if (d < 20 * 60 || d > 45 * 60 || s_t[k].zeit >= ort(2026, 10, 28, 11, 0)) richtig = false;
  }
  PRUEFE(richtig);
  PRUEFE(k >= 4);
  GLEICH(s_t[k].zeit, ort(2026, 10, 28, 22, 0));
}

static void prv_liste(void) {
  Termin liste[4];
  int n = 0;
  const Termin a[] = {{100, 1, TERMIN_ERINNERUNG}, {400, 1, TERMIN_ERINNERUNG}, {700, 1, TERMIN_ERINNERUNG}};
  const Termin b[] = {{50, 0, TERMIN_BILANZ}, {400, 0, TERMIN_BILANZ}, {400, 0, TERMIN_BILANZ}};
  const Termin c[] = {{50, 0, TERMIN_BILANZ}, {60, 2, TERMIN_ERINNERUNG}, {1000, 2, TERMIN_ERINNERUNG}};
  planer_einfuegen(liste, &n, 4, a, 3);
  GLEICH(n, 3);
  planer_einfuegen(liste, &n, 4, b, 3);
  GLEICH(n, 4);
  GLEICH(liste[0].zeit, 50);
  GLEICH(liste[1].zeit, 100);
  GLEICH(liste[2].zeit, 400);
  GLEICH(liste[3].zeit, 400);
  GLEICH(liste[3].art, TERMIN_BILANZ); // die zweite Bilanz um 400 fiel weg, 700 auch
  planer_einfuegen(liste, &n, 4, c, 3);
  GLEICH(n, 4);
  GLEICH(liste[0].art, TERMIN_BILANZ); // keine zweite Bilanz um 50
  GLEICH(liste[1].zeit, 60);
  GLEICH(liste[3].zeit, 400);
  planer_abstand(liste, n);
  GLEICH(liste[0].zeit, 50);
  GLEICH(liste[1].zeit, 50 + PLANER_ABSTAND_S);
  GLEICH(liste[2].zeit, 50 + 2 * PLANER_ABSTAND_S);
  GLEICH(liste[3].zeit, 50 + 3 * PLANER_ABSTAND_S);
}

static void prv_faellig(void) {
  Habit h = prv_gut(INSIST_CRITICAL);
  logbuch_leeren(&s_lb, 5);
  PRUEFE(planer_faellig(&h, &s_lb, ort(2026, 10, 28, 8, 5) + 20, &R));
  PRUEFE(planer_faellig(&h, &s_lb, ort(2026, 10, 28, 8, 4), &R));  // zwei Minuten frueher
  PRUEFE(!planer_faellig(&h, &s_lb, ort(2026, 10, 28, 7, 50), &R));
  // Gezeigt bis 8:07: um 8:06 nichts mehr, um 8:10 wieder.
  logbuch_erinnert(&s_lb, ort(2026, 10, 28, 8, 7));
  PRUEFE(!planer_faellig(&h, &s_lb, ort(2026, 10, 28, 8, 6), &R));
  PRUEFE(planer_faellig(&h, &s_lb, ort(2026, 10, 28, 8, 10), &R));
  // Eingetragen: nicht mehr faellig.
  logbuch_eintragen(&s_lb, ort(2026, 10, 28, 8, 9));
  PRUEFE(!planer_faellig(&h, &s_lb, ort(2026, 10, 28, 8, 10), &R));
  // Ein verschobener Wecker holt nach: Termin 8:00, Wecker erst um 8:06.
  logbuch_leeren(&s_lb, 5);
  h.period_type = PERIOD_SCHEDULED;
  h.insistence = INSIST_LIGHT;
  h.times_n = 1;
  h.times[0] = 8 * 60;
  PRUEFE(planer_faellig(&h, &s_lb, ort(2026, 10, 28, 8, 6), &R));
  PRUEFE(!planer_faellig(&h, &s_lb, ort(2026, 10, 28, 8, 11), &R));
  // Die Bilanz ist keine Erinnerung.
  h = prv_gut(INSIST_LIGHT);
  PRUEFE(!planer_faellig(&h, &s_lb, ort(2026, 10, 29, 8, 0), &R));
}

static void prv_bilanz(void) {
  Habit h = prv_gut(INSIST_LIGHT);
  h.line = 2;
  logbuch_leeren(&s_lb, 5);
  logbuch_eintragen(&s_lb, ort(2026, 10, 27, 20, 0));
  int zahl = -1, ziel = -1;
  const Tag mi = kalender_tag_aus_datum(2026, 10, 28);
  PRUEFE(planer_bilanz(&h, &s_lb, mi, &R, &zahl, &ziel));
  GLEICH(zahl, 1);
  GLEICH(ziel, 2);
  logbuch_eintragen(&s_lb, ort(2026, 10, 27, 23, 59));
  PRUEFE(!planer_bilanz(&h, &s_lb, mi, &R, &zahl, &ziel));
  // Woechentlich: nur am Morgen nach dem Ende der Woche.
  h.period_type = PERIOD_ONCE_PER;
  h.period_unit = UNIT_WEEKS;
  h.period_count = 1;
  h.line = 0;
  logbuch_leeren(&s_lb, 5);
  PRUEFE(!planer_bilanz(&h, &s_lb, mi, &R, &zahl, &ziel));
  PRUEFE(planer_bilanz(&h, &s_lb, kalender_tag_aus_datum(2026, 10, 26), &R, &zahl, &ziel));
  GLEICH(ziel, 1);
  // Stunden-Bloecke und nach Plan gehoeren nicht hinein.
  h.period_unit = UNIT_HOURS;
  PRUEFE(!planer_bilanz(&h, &s_lb, mi, &R, &zahl, &ziel));
  h.period_type = PERIOD_SCHEDULED;
  PRUEFE(!planer_bilanz(&h, &s_lb, mi, &R, &zahl, &ziel));
  // Schlecht: gestern ueber der Grenze.
  h = prv_gut(INSIST_LIGHT);
  h.type = HABIT_BAD;
  h.line = 1;
  logbuch_eintragen(&s_lb, ort(2026, 10, 27, 9, 0));
  PRUEFE(!planer_bilanz(&h, &s_lb, mi, &R, &zahl, &ziel));
  logbuch_eintragen(&s_lb, ort(2026, 10, 27, 10, 0));
  PRUEFE(planer_bilanz(&h, &s_lb, mi, &R, &zahl, &ziel));
  GLEICH(zahl, 2);
  GLEICH(ziel, 1);
  h.insistence = INSIST_HEAVY;
  PRUEFE(!planer_bilanz(&h, &s_lb, mi, &R, &zahl, &ziel));
}

int main(void) {
  prv_none();
  prv_light();
  prv_heavy();
  prv_critical();
  prv_nach_plan();
  prv_liste();
  prv_faellig();
  prv_bilanz();
  PRUEF_ENDE("planer");
}
