#include "einstellungen.h"
#include "eintragen.h"
#include "fenster.h"
#include "karte.h"
#include "liste.h"
#include "logbuch.h"
#include "planer.h"
#include "rueckmeldung.h"
#include "speicher.h"
#include "thema.h"

// Was ein Wecker zeigt (Abschnitt 5): die Erinnerung an eine Gewohnheit und
// die Morgenbilanz. Nach einem Wecker liegt kein Hauptmenue darunter - mit
// dem letzten Fenster endet die App wieder.

static void prv_erinnert(Karte *k, ButtonId taste) {
  if (taste == BUTTON_ID_SELECT) {
    // Jetzt eintragen: die Bestaetigung kommt an die Stelle der Erinnerung.
    bestaetigung_zeigen(k->id);
  } else if (taste == BUTTON_ID_DOWN) {
    eintragen_auslassen(k->id, time(NULL));
  } else if (taste == BUTTON_ID_UP) {
    return;
  }
  karte_schliessen(k);
}

void erinnerung_zeigen(uint16_t id, bool still) {
  const Habit *h = habits_mit_id(id);
  if (!h) return;
  const uint32_t symbole[3] = {0, RESOURCE_ID_ICON_CHECK, RESOURCE_ID_ICON_CROSS};
  Karte *k = karte_zeigen(symbole, prv_erinnert);
  if (!k) return;
  const time_t jetzt = time(NULL);
  const Lage l = eintragen_lage(h, jetzt);
  k->id = id;
  k->zeichen = ZEICHEN_GLOCKE;
  k->farbe = farben()->gut;
  strncpy(k->titel, h->name, sizeof(k->titel) - 1);
  if (h->period_type == PERIOD_SCHEDULED) {
    strncpy(k->oben, "Time for", sizeof(k->oben) - 1);
  } else if (h->insistence == INSIST_LIGHT && !zeitraum_tage(&l.zeitraum)) {
    strncpy(k->oben, "Missed", sizeof(k->oben) - 1);
  } else {
    strncpy(k->oben, "Reminder", sizeof(k->oben) - 1);
  }
  eintragen_text(h, &l, jetzt, k->text, sizeof(k->text));
  if (!still) rueckmeldung((HabitType)h->type, h->insistence == INSIST_CRITICAL);
}

// --- Morgenbilanz ---

typedef struct {
  uint8_t n;
  uint16_t id[HABITS_MAX];
  uint8_t zahl[HABITS_MAX], ziel[HABITS_MAX];
} Bilanz;

static uint16_t prv_anzahl(Liste *l) {
  return ((Bilanz *)l->daten)->n;
}

static void prv_zeile(Liste *l, uint16_t i, Zeile *z) {
  const Bilanz *b = l->daten;
  const Habit *h = habits_mit_id(b->id[i]);
  if (!h) return;
  strncpy(z->titel, h->name, sizeof(z->titel) - 1);
  z->marke = h->type == HABIT_BAD ? 2 : 1;
  if (h->type == HABIT_BAD) snprintf(z->untertitel, sizeof(z->untertitel), "Over: %d of max %d", b->zahl[i], b->ziel[i]);
  else snprintf(z->untertitel, sizeof(z->untertitel), "Missed: %d of %d", b->zahl[i], b->ziel[i]);
}

static void prv_waehlen(Liste *l, uint16_t i) {
  diagramm_zeigen(((Bilanz *)l->daten)->id[i]);
}

static void prv_schluss(Liste *l) {
  free(l->daten);
}

static const ListeArt s_bilanz = {
  .anzahl = prv_anzahl,
  .zeile = prv_zeile,
  .waehlen = prv_waehlen,
  .schluss = prv_schluss,
};

bool bilanz_zeigen(bool still) {
  const time_t jetzt = time(NULL);
  const Tag heute = kalender_tag(jetzt);
  const Rahmen r = einstellungen_rahmen();
  // Einmal am Tag, mit dem Beginn der aktiven Stunden. Kommt ein Wecker erst
  // spaeter am Tag (der der Bilanz war schon vergeben), dann bis zwei Stunden
  // danach - am Abend waere "gestern" kein Morgengruss mehr.
  const time_t morgen = kalender_zeit(heute, r.aktiv_von);
  if (persist_read_int(FACH_BILANZ) >= heute || jetzt + PLANER_VORGRIFF_S < morgen ||
      jetzt > morgen + 2 * 3600) {
    return false;
  }
  Bilanz *b = calloc(1, sizeof(Bilanz));
  Logbuch *lb = malloc(sizeof(Logbuch));
  if (!b || !lb) {
    free(b);
    free(lb);
    return false;
  }
  bool schlecht = false;
  for (int i = 0; i < habits_anzahl(); i++) {
    const Habit *h = habits_nr(i);
    int zahl, ziel;
    logbuch_laden(lb, habits_platz(h->id), h->id);
    if (!planer_bilanz(h, lb, heute, &r, &zahl, &ziel)) continue;
    b->id[b->n] = h->id;
    b->zahl[b->n] = (uint8_t)(zahl > 255 ? 255 : zahl);
    b->ziel[b->n] = (uint8_t)ziel;
    b->n++;
    schlecht |= h->type == HABIT_BAD;
  }
  free(lb);
  persist_write_int(FACH_BILANZ, heute);
  if (!b->n) {
    free(b);
    return false;
  }
  liste_zeigen(&s_bilanz, "LAST PERIOD", b);
  if (!still) rueckmeldung(schlecht ? HABIT_BAD : HABIT_GOOD, false);
  return true;
}
