#include "eingabe.h"
#include "eintragen.h"
#include "kalender.h"
#include "liste.h"

// Wochentage und Uhrzeiten fuer "Scheduled" (Abschnitt 4.2). Beide aendern
// die Arbeitskopie im Formular; gespeichert wird erst mit "Save habit".

static const char *const s_tage[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday",
                                     "Sunday"};
static const char *const s_kurz[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

void tage_text(uint8_t tage, char *puffer, size_t groesse) {
  tage &= 0x7f;
  if (tage == 0x7f) {
    strncpy(puffer, "Every day", groesse - 1);
  } else if (tage == 0x1f) {
    strncpy(puffer, "Weekdays", groesse - 1);
  } else if (tage == 0x60) {
    strncpy(puffer, "Weekends", groesse - 1);
  } else {
    puffer[0] = 0;
    const int erster = kalender_erster_wochentag();
    for (int k = 0; k < 7; k++) {
      const int t = (erster + k) % 7;
      if (!((tage >> t) & 1)) continue;
      const size_t n = strlen(puffer);
      snprintf(puffer + n, groesse - n, "%s%s", n ? ", " : "", s_kurz[t]);
    }
  }
  puffer[groesse - 1] = 0;
}

void zeiten_text(const Habit *h, char *puffer, size_t groesse) {
  puffer[0] = 0;
  for (int i = 0; i < h->times_n; i++) {
    char uhr[10];
    eintragen_uhrzeit(h->times[i], uhr, sizeof(uhr));
    const size_t n = strlen(puffer);
    snprintf(puffer + n, groesse - n, "%s%s", i ? ", " : "", uhr);
  }
}

// --- Wochentage ---

static uint16_t prv_tage_anzahl(Liste *l) {
  return 7;
}

static int prv_tag(uint16_t i) {
  return (kalender_erster_wochentag() + i) % 7;
}

static void prv_tage_zeile(Liste *l, uint16_t i, Zeile *z) {
  const Habit *h = l->daten;
  strncpy(z->titel, s_tage[prv_tag(i)], sizeof(z->titel) - 1);
  z->haken = (h->days >> prv_tag(i)) & 1;
}

static void prv_tage_waehlen(Liste *l, uint16_t i) {
  Habit *h = l->daten;
  const uint8_t neu = h->days ^ (uint8_t)(1 << prv_tag(i));
  // Mindestens ein Tag: ein Plan ohne Tag erinnerte nie.
  if (neu) h->days = neu;
  else vibes_short_pulse();
  liste_auffrischen(l);
}

static const ListeArt s_tage_art = {
  .anzahl = prv_tage_anzahl,
  .zeile = prv_tage_zeile,
  .waehlen = prv_tage_waehlen,
};

void tage_zeigen(Habit *h) {
  liste_zeigen(&s_tage_art, "DAYS", h);
}

// --- Uhrzeiten ---

typedef struct {
  Habit *h;
  int welche; //< die geaenderte Zeit, HABIT_ZEITEN_MAX fuer eine neue
} Zeiten;

static uint16_t prv_zeiten_anzahl(Liste *l) {
  const Zeiten *z = l->daten;
  return z->h->times_n + (z->h->times_n < HABIT_ZEITEN_MAX);
}

static void prv_zeiten_zeile(Liste *l, uint16_t i, Zeile *z) {
  const Zeiten *zt = l->daten;
  if (i >= zt->h->times_n) {
    strncpy(z->titel, "Add time", sizeof(z->titel) - 1);
    return;
  }
  eintragen_uhrzeit(zt->h->times[i], z->titel, sizeof(z->titel));
  if (zt->h->times_n > 1) strncpy(z->untertitel, "Hold to remove", sizeof(z->untertitel) - 1);
}

static void prv_zeit_gesetzt(void *daten, const int *werte) {
  Zeiten *z = daten;
  Habit *h = z->h;
  if (z->welche >= h->times_n) {
    if (h->times_n >= HABIT_ZEITEN_MAX) return;
    z->welche = h->times_n++;
  }
  h->times[z->welche] = (uint16_t)werte[0];
  habit_zeiten_ordnen(h);
}

static void prv_zeiten_waehlen(Liste *l, uint16_t i) {
  Zeiten *z = l->daten;
  Habit *h = z->h;
  z->welche = i;
  // Eine neue Zeit beginnt eine Stunde nach der letzten.
  const int start = i < h->times_n ? h->times[i] : (h->times[h->times_n - 1] + 60) % 1440;
  uhrzeit_zeigen(i < h->times_n ? "Change time" : "New time", start, prv_zeit_gesetzt, z);
}

static void prv_zeiten_lang(Liste *l, uint16_t i) {
  Zeiten *z = l->daten;
  Habit *h = z->h;
  if (i >= h->times_n || h->times_n <= 1) return;
  memmove(&h->times[i], &h->times[i + 1], (size_t)(h->times_n - i - 1) * sizeof(h->times[0]));
  h->times_n--;
  h->times[h->times_n] = 0;
  vibes_short_pulse();
  liste_auffrischen(l);
}

static void prv_zeiten_schluss(Liste *l) {
  free(l->daten);
}

static const ListeArt s_zeiten_art = {
  .anzahl = prv_zeiten_anzahl,
  .zeile = prv_zeiten_zeile,
  .waehlen = prv_zeiten_waehlen,
  .lang = prv_zeiten_lang,
  .schluss = prv_zeiten_schluss,
};

void zeiten_zeigen(Habit *h) {
  Zeiten *z = calloc(1, sizeof(Zeiten));
  if (!z) return;
  z->h = h;
  liste_zeigen(&s_zeiten_art, "TIMES", z);
}
