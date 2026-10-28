#include "eintragen.h"
#include "einstellungen.h"
#include "logbuch.h"
#include "wecker.h"

static Lage prv_lage(const Habit *h, const Logbuch *lb, time_t jetzt) {
  Lage l;
  l.zeitraum = zeitraum(h, jetzt, kalender_erster_wochentag());
  l.zahl = zeitraum_zahl(lb, &l.zeitraum);
  l.ziel = habit_ziel(h, l.zeitraum.tag_von);
  l.stand = habit_stand(h, l.zahl, l.ziel);
  l.letzter_log = lb->letzter_log;
  return l;
}

Lage eintragen_lage(const Habit *h, time_t jetzt) {
  Logbuch *lb = malloc(sizeof(Logbuch));
  Lage l = {0};
  if (!lb) return l;
  logbuch_laden(lb, habits_platz(h->id), h->id);
  l = prv_lage(h, lb, jetzt);
  free(lb);
  return l;
}

// Laedt das Logbuch, laesst `tun` daran arbeiten und speichert, wenn es
// etwas geaendert hat.
static bool prv_mit_logbuch(uint16_t id, bool (*tun)(Logbuch *, const Habit *, void *), void *daten) {
  const Habit *h = habits_mit_id(id);
  if (!h) return false;
  Logbuch *lb = malloc(sizeof(Logbuch));
  if (!lb) return false;
  const int platz = habits_platz(id);
  logbuch_laden(lb, platz, id);
  const bool ok = tun(lb, h, daten);
  if (lb->geaendert) logbuch_speichern(lb, platz);
  free(lb);
  return ok;
}

typedef struct {
  time_t jetzt;
  Lage *lage;
} Eintrag;

static bool prv_eintragen(Logbuch *lb, const Habit *h, void *daten) {
  Eintrag *e = daten;
  if (!logbuch_eintragen(lb, e->jetzt)) return false;
  if (e->lage) *e->lage = prv_lage(h, lb, e->jetzt);
  return true;
}

bool eintragen(uint16_t id, time_t jetzt, Lage *nachher) {
  Eintrag e = {.jetzt = jetzt, .lage = nachher};
  const bool ok = prv_mit_logbuch(id, prv_eintragen, &e);
  if (ok) wecker_planen();
  return ok;
}

static bool prv_rueckgaengig(Logbuch *lb, const Habit *h, void *daten) {
  (void)h;
  (void)daten;
  return logbuch_rueckgaengig(lb);
}

bool eintragen_rueckgaengig(uint16_t id) {
  const bool ok = prv_mit_logbuch(id, prv_rueckgaengig, NULL);
  if (ok) wecker_planen();
  return ok;
}

static bool prv_auslassen(Logbuch *lb, const Habit *h, void *daten) {
  const time_t jetzt = *(time_t *)daten;
  time_t bis = zeitraum(h, jetzt, kalender_erster_wochentag()).bis;
  if (h->type == HABIT_GOOD && h->period_type == PERIOD_SCHEDULED) {
    // Bis zur naechsten geplanten Zeit nach jetzt.
    const Tag heute = kalender_tag(jetzt);
    bis = jetzt + 8 * 86400;
    for (Tag d = heute; d <= heute + 7; d++) {
      if (!habit_geplant_am(h, d)) continue;
      for (int i = 0; i < h->times_n; i++) {
        const time_t t = kalender_zeit(d, h->times[i]);
        if (t > jetzt && t < bis) bis = t;
      }
    }
  }
  logbuch_pause(lb, bis);
  return true;
}

void eintragen_auslassen(uint16_t id, time_t jetzt) {
  if (prv_mit_logbuch(id, prv_auslassen, &jetzt)) wecker_planen();
}

static bool prv_erinnert(Logbuch *lb, const Habit *h, void *daten) {
  (void)h;
  logbuch_erinnert(lb, *(time_t *)daten);
  return true;
}

void eintragen_erinnert(uint16_t id, time_t bis) {
  prv_mit_logbuch(id, prv_erinnert, &bis);
}

static bool prv_aufheben(Logbuch *lb, const Habit *h, void *daten) {
  (void)h;
  (void)daten;
  if (lb->pause_bis) logbuch_pause(lb, 0);
  return true;
}

void eintragen_pause_aufheben(uint16_t id) {
  prv_mit_logbuch(id, prv_aufheben, NULL);
}

void eintragen_uhrzeit(int minute, char *puffer, size_t groesse) {
  const int h = (minute / 60) % 24, m = minute % 60;
  if (clock_is_24h_style()) {
    snprintf(puffer, groesse, "%02d:%02d", h, m);
  } else {
    snprintf(puffer, groesse, "%d:%02d %s", (h + 11) % 12 + 1, m, h < 12 ? "AM" : "PM");
  }
}

void eintragen_text(const Habit *h, const Lage *l, time_t jetzt, char *puffer, size_t groesse) {
  static const char *const tage[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
  static const char *const monate[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  char wann[16];
  const Zeitraum *z = &l->zeitraum;
  if (!zeitraum_tage(z)) {
    char uhr[10];
    eintragen_uhrzeit(z->stunde_bis * 60, uhr, sizeof(uhr));
    snprintf(wann, sizeof(wann), "Until %s", uhr);
  } else if (z->tag_bis - z->tag_von <= 1) {
    strncpy(wann, "Today", sizeof(wann));
  } else if (z->tag_bis - 1 - kalender_tag(jetzt) < 7) {
    snprintf(wann, sizeof(wann), "Until %s", tage[kalender_wochentag(z->tag_bis - 1)]);
  } else {
    int j, m, d;
    kalender_datum(z->tag_bis - 1, &j, &m, &d);
    snprintf(wann, sizeof(wann), "Until %s %d", monate[m - 1], d);
  }
  if (h->type == HABIT_BAD) {
    if (l->ziel) snprintf(puffer, groesse, "%s %d \xc2\xb7 max %d", wann, l->zahl, l->ziel);
    else snprintf(puffer, groesse, "%s %d", wann, l->zahl);
  } else if (h->line || (h->period_type == PERIOD_SCHEDULED && l->ziel)) {
    snprintf(puffer, groesse, "%s %d/%d", wann, l->zahl, l->ziel);
  } else {
    snprintf(puffer, groesse, "%s %d", wann, l->zahl);
  }
}
