#include "eintragen.h"
#include "fenster.h"
#include "karte.h"
#include "rueckmeldung.h"
#include "thema.h"
#include "wecker.h"

// Eintragen mit Bestaetigung (Abschnitte 4.4, 5, 6) und Loeschen mit
// Rueckfrage (Abschnitt 4.3).

#define KURZ_MS 1800
#define WARNUNG_MS 4000

static void prv_satz(const Habit *h, const Lage *l, char *puffer, size_t groesse) {
  if (h->type == HABIT_BAD) {
    switch (l->stand) {
      case STAND_UEBER: snprintf(puffer, groesse, "Over the limit: %d of %d today", l->zahl, l->ziel); return;
      case STAND_ERREICHT: snprintf(puffer, groesse, "Limit reached: %d of %d", l->zahl, l->ziel); return;
      case STAND_NAHE: snprintf(puffer, groesse, "Near the limit: %d of %d", l->zahl, l->ziel); return;
      default: break;
    }
    if (l->ziel) snprintf(puffer, groesse, "%d of max %d today", l->zahl, l->ziel);
    else snprintf(puffer, groesse, "%d today", l->zahl);
    return;
  }
  if (l->stand == STAND_ERFUELLT && (h->line || h->period_type != PERIOD_FREE)) {
    snprintf(puffer, groesse, "Goal reached: %d of %d", l->zahl, l->ziel);
  } else if (h->line) {
    snprintf(puffer, groesse, "%d of %d", l->zahl, l->ziel);
  } else {
    snprintf(puffer, groesse, "%d so far", l->zahl);
  }
}

static void prv_bestaetigt(Karte *k, ButtonId taste) {
  if (taste == BUTTON_ID_DOWN && k->wert == 0) {
    // Rueckgaengig: genau dieser Eintrag.
    eintragen_rueckgaengig(k->id);
    k->wert = 1;
    k->zeichen = ZEICHEN_HAKEN;
    k->farbe = farben()->leise;
    strncpy(k->oben, "Undone", sizeof(k->oben) - 1);
    k->text[0] = 0;
    action_bar_layer_clear_icon(k->leiste, BUTTON_ID_DOWN);
    karte_neu_zeichnen(k);
    karte_zeitgeber(k, KURZ_MS / 2);
    return;
  }
  karte_schliessen(k);
}

void bestaetigung_zeigen(uint16_t id) {
  const Habit *h = habits_mit_id(id);
  if (!h) return;
  const uint32_t symbole[3] = {0, 0, RESOURCE_ID_ICON_UNDO};
  const time_t jetzt = time(NULL);
  Lage l;
  const bool ok = eintragen(id, jetzt, &l);
  Karte *k = karte_zeigen(ok ? symbole : NULL, prv_bestaetigt);
  if (!k) return;
  k->id = id;
  strncpy(k->titel, h->name, sizeof(k->titel) - 1);
  if (!ok) {
    // Die Uhr steht noch auf einem alten Datum (nach dem Zuruecksetzen).
    k->wert = 1;
    k->zeichen = ZEICHEN_ACHTUNG;
    k->farbe = farben()->schlecht;
    strncpy(k->oben, "Not logged", sizeof(k->oben) - 1);
    strncpy(k->text, "Set the watch time first", sizeof(k->text) - 1);
    karte_zeitgeber(k, WARNUNG_MS);
    return;
  }
  strncpy(k->oben, "Logged", sizeof(k->oben) - 1);
  prv_satz(h, &l, k->text, sizeof(k->text));
  const bool warnung = h->type == HABIT_BAD && l.stand >= STAND_NAHE && h->insistence >= INSIST_HEAVY;
  k->zeichen = warnung ? ZEICHEN_ACHTUNG : ZEICHEN_HAKEN;
  k->farbe = h->type == HABIT_BAD ? farben()->schlecht : farben()->gut;
  // Critical warnt auch in der Ruhezeit und bleibt stehen, bis eine Taste
  // kommt (Abschnitt 5: "persistent warning").
  const bool kritisch = warnung && h->insistence == INSIST_CRITICAL;
  rueckmeldung((HabitType)h->type, kritisch);
  if (!kritisch) karte_zeitgeber(k, warnung ? WARNUNG_MS : KURZ_MS);
}

static void prv_loeschen(Karte *k, ButtonId taste) {
  if (taste == BUTTON_ID_SELECT || taste == BUTTON_ID_UP) {
    habits_loeschen(k->id);
    wecker_planen();
  }
  karte_schliessen(k);
}

void loeschen_fragen(uint16_t id) {
  const Habit *h = habits_mit_id(id);
  if (!h) return;
  const uint32_t symbole[3] = {0, RESOURCE_ID_ICON_CHECK, RESOURCE_ID_ICON_CROSS};
  Karte *k = karte_zeigen(symbole, prv_loeschen);
  if (!k) return;
  k->id = id;
  k->zeichen = ZEICHEN_FRAGE;
  k->farbe = farben()->schlecht;
  snprintf(k->titel, sizeof(k->titel), "Delete %s?", h->name);
  strncpy(k->text, "This will erase all logged data for this habit.", sizeof(k->text) - 1);
}

void app_beenden(void) {
  window_stack_pop_all(true);
}
