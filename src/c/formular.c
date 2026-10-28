#include "eingabe.h"
#include "eintragen.h"
#include "fenster.h"
#include "liste.h"
#include "wecker.h"

// Anlegen und Aendern (Abschnitt 4.2): dieselbe Liste, beim Aendern mit den
// gespeicherten Werten. Die Zeilen haengen vom Typ und vom Zeitraum ab; was
// gerade nicht gilt, bleibt gespeichert, falls es zurueckgestellt wird.
// Zurueck verwirft, erst "Save habit" speichert.

typedef enum { Z_NAME, Z_TYP, Z_NACHDRUCK, Z_ZEITRAUM, Z_EINMAL, Z_TAGE, Z_ZEITEN, Z_LINIE, Z_SPEICHERN } Art;

typedef struct {
  Habit h;
  bool neu;
  uint8_t n;
  uint8_t zeilen[9];
} Formular;

static const char *const s_nachdruck[] = {"None", "Light", "Heavy", "Critical"};
static const char *const s_nachdruck_gut[] = {"log only", "if missed", "nudges", "every 5 min"};
static const char *const s_nachdruck_schlecht[] = {"log only", "next morning", "warnings", "loud warnings"};
static const char *const s_zeitraum[] = {"Free", "Once per", "Scheduled"};
static const char *const s_einheit[] = {"hour", "day", "week", "month"};

static void prv_zeilen(Formular *f) {
  f->n = 0;
  f->zeilen[f->n++] = Z_NAME;
  f->zeilen[f->n++] = Z_TYP;
  f->zeilen[f->n++] = Z_NACHDRUCK;
  if (f->h.type == HABIT_GOOD) {
    f->zeilen[f->n++] = Z_ZEITRAUM;
    if (f->h.period_type == PERIOD_ONCE_PER) {
      f->zeilen[f->n++] = Z_EINMAL;
    } else if (f->h.period_type == PERIOD_SCHEDULED) {
      f->zeilen[f->n++] = Z_TAGE;
      f->zeilen[f->n++] = Z_ZEITEN;
    }
  }
  f->zeilen[f->n++] = Z_LINIE;
  f->zeilen[f->n++] = Z_SPEICHERN;
}

static void prv_einmal_text(const Habit *h, int anzahl, int einheit, char *puffer, size_t groesse) {
  snprintf(puffer, groesse, "%d %s%s", anzahl, s_einheit[einheit], anzahl == 1 ? "" : "s");
  (void)h;
}

static uint16_t prv_anzahl(Liste *l) {
  Formular *f = l->daten;
  prv_zeilen(f);
  return f->n;
}

static void prv_zeile(Liste *l, uint16_t i, Zeile *z) {
  Formular *f = l->daten;
  const Habit *h = &f->h;
  const bool gut = h->type == HABIT_GOOD;
  char *u = z->untertitel;
  const size_t un = sizeof(z->untertitel);
  switch (f->zeilen[i]) {
    case Z_NAME:
      strncpy(z->titel, "Name", sizeof(z->titel) - 1);
      strncpy(u, h->name[0] ? h->name : "Choose a name", un - 1);
      break;
    case Z_TYP:
      strncpy(z->titel, "Type", sizeof(z->titel) - 1);
      strncpy(u, gut ? "Good habit" : "Bad habit", un - 1);
      z->marke = gut ? 1 : 2;
      break;
    case Z_NACHDRUCK:
      strncpy(z->titel, "Insistence", sizeof(z->titel) - 1);
      // Auf den breiten Uhren passt dazu, was die Stufe tut.
      if (PBL_DISPLAY_WIDTH >= 180) {
        snprintf(u, un, "%s: %s", s_nachdruck[h->insistence],
                 (gut ? s_nachdruck_gut : s_nachdruck_schlecht)[h->insistence]);
      } else {
        strncpy(u, s_nachdruck[h->insistence], un - 1);
      }
      break;
    case Z_ZEITRAUM:
      strncpy(z->titel, "Period", sizeof(z->titel) - 1);
      strncpy(u, s_zeitraum[h->period_type], un - 1);
      break;
    case Z_EINMAL:
      strncpy(z->titel, "Once per", sizeof(z->titel) - 1);
      prv_einmal_text(h, h->period_count, h->period_unit, u, un);
      break;
    case Z_TAGE:
      strncpy(z->titel, "Days", sizeof(z->titel) - 1);
      tage_text(h->days, u, un);
      break;
    case Z_ZEITEN:
      strncpy(z->titel, "Times", sizeof(z->titel) - 1);
      zeiten_text(h, u, un);
      break;
    case Z_LINIE:
      // Ein Feld, zwei Namen: Ziel bei guten, Grenze bei schlechten (Abschnitt 3).
      strncpy(z->titel, gut ? "Goal" : "Limit", sizeof(z->titel) - 1);
      if (!h->line) strncpy(u, "None", un - 1);
      else snprintf(u, un, "%d per %s", h->line, gut && h->period_type == PERIOD_ONCE_PER ? "period" : "day");
      break;
    case Z_SPEICHERN:
      strncpy(z->titel, "Save habit", sizeof(z->titel) - 1);
      break;
  }
}

static void prv_einmal_feld(void *daten, int feld, const int *werte, char *puffer, size_t groesse) {
  if (feld == 0) snprintf(puffer, groesse, "%d", werte[0]);
  else snprintf(puffer, groesse, "%s%s", s_einheit[werte[1]], werte[0] == 1 ? "" : "s");
}

static void prv_einmal_fertig(void *daten, const int *werte) {
  Habit *h = daten;
  h->period_count = (uint16_t)werte[0];
  h->period_unit = (uint8_t)werte[1];
  // Bloecke in Stunden liegen im Tag.
  if (h->period_unit == UNIT_HOURS && h->period_count > 24) h->period_count = 24;
}

static void prv_linie_fertig(void *daten, const int *werte) {
  ((Habit *)daten)->line = (uint16_t)werte[0];
}

static void prv_waehlen(Liste *l, uint16_t i) {
  Formular *f = l->daten;
  Habit *h = &f->h;
  switch (f->zeilen[i]) {
    case Z_NAME: name_zeigen(h, f->neu); return;
    case Z_TYP: h->type = h->type == HABIT_GOOD ? HABIT_BAD : HABIT_GOOD; break;
    case Z_NACHDRUCK: h->insistence = (h->insistence + 1) % INSIST_COUNT; break;
    case Z_ZEITRAUM: h->period_type = (h->period_type + 1) % PERIOD_COUNT; break;
    case Z_EINMAL: {
      const Feld felder[] = {
        {.wert = h->period_count, .min = 1, .max = HABIT_ANZAHL_MAX, .schritt = 1},
        {.wert = h->period_unit, .min = 0, .max = UNIT_COUNT - 1, .schritt = 1},
      };
      feldwahl_zeigen("Once per", felder, 2, prv_einmal_feld, NULL, prv_einmal_fertig, h);
      return;
    }
    case Z_TAGE: tage_zeigen(h); return;
    case Z_ZEITEN: zeiten_zeigen(h); return;
    case Z_LINIE:
      zahl_zeigen(h->type == HABIT_GOOD ? "Goal" : "Limit", h->line, 0, HABIT_LINE_MAX, "None",
                  prv_linie_fertig, h);
      return;
    case Z_SPEICHERN:
      if (!h->name[0]) {
        name_zeigen(h, f->neu);
        return;
      }
      if (habits_speichern(h, time(NULL))) {
        if (!f->neu) eintragen_pause_aufheben(h->id);
        wecker_planen();
        liste_schliessen(l);
      } else {
        vibes_double_pulse();
      }
      return;
  }
  liste_auffrischen(l);
}

static void prv_schluss(Liste *l) {
  free(l->daten);
}

static const ListeArt s_art = {
  .anzahl = prv_anzahl,
  .zeile = prv_zeile,
  .waehlen = prv_waehlen,
  .schluss = prv_schluss,
};

void formular_zeigen(uint16_t id) {
  Formular *f = calloc(1, sizeof(Formular));
  if (!f) return;
  const Habit *alt = habits_mit_id(id);
  if (alt) {
    f->h = *alt;
  } else {
    habit_vorgabe(&f->h);
    f->neu = true;
  }
  liste_zeigen(&s_art, f->neu ? "ADD HABIT" : "EDIT HABIT", f);
}
