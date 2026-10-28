#include "eintragen.h"
#include "fenster.h"
#include "liste.h"

// Die Liste der Gewohnheiten, fuer Diagramme, Aendern, Loeschen und Eintragen
// (Abschnitte 4.1, 4.3, 4.4). Jede Zeile zeigt den Stand im laufenden
// Zeitraum; er wird beim Erscheinen einmal gerechnet, nicht bei jedem
// Zeichnen - dafuer muesste jedes Mal das Logbuch geladen werden.

typedef struct {
  AuswahlZweck zweck;
  uint8_t n;
  uint16_t id[HABITS_MAX];
  char text[HABITS_MAX][32];
} Auswahl;

static const char *const s_kopf[] = {"SEE CHARTS", "EDIT HABIT", "DELETE HABIT", "LOG A HABIT"};

static void prv_erscheint(Liste *l) {
  Auswahl *a = l->daten;
  const time_t jetzt = time(NULL);
  a->n = (uint8_t)habits_anzahl();
  for (int i = 0; i < a->n; i++) {
    const Habit *h = habits_nr(i);
    a->id[i] = h->id;
    const Lage lage = eintragen_lage(h, jetzt);
    eintragen_text(h, &lage, jetzt, a->text[i], sizeof(a->text[i]));
  }
}

static uint16_t prv_anzahl(Liste *l) {
  Auswahl *a = l->daten;
  // Ohne Gewohnheit eine Zeile, die zum Anlegen fuehrt; beim Eintragen dazu
  // "Cancel" (Abschnitt 4.4).
  if (!a->n) return 1;
  return a->n + (a->zweck == AUSWAHL_EINTRAGEN);
}

static void prv_zeile(Liste *l, uint16_t i, Zeile *z) {
  Auswahl *a = l->daten;
  if (!a->n) {
    strncpy(z->titel, "No habits yet", sizeof(z->titel) - 1);
    strncpy(z->untertitel, "Select to add one", sizeof(z->untertitel) - 1);
    return;
  }
  if (i >= a->n) {
    strncpy(z->titel, "Cancel", sizeof(z->titel) - 1);
    return;
  }
  const Habit *h = habits_mit_id(a->id[i]);
  if (!h) return;
  strncpy(z->titel, h->name, sizeof(z->titel) - 1);
  strncpy(z->untertitel, a->text[i], sizeof(z->untertitel) - 1);
  z->marke = h->type == HABIT_BAD ? 2 : 1;
}

static void prv_waehlen(Liste *l, uint16_t i) {
  Auswahl *a = l->daten;
  if (!a->n) {
    // Erst das Formular, dann die Liste weg: ohne Fenster dazwischen ginge
    // die App sonst zu (nach Quick Launch ist die Liste das einzige).
    formular_zeigen(0);
    liste_schliessen(l);
    return;
  }
  if (i >= a->n) {
    // Cancel: zurueck, nach Quick Launch zum Zifferblatt.
    liste_schliessen(l);
    return;
  }
  const uint16_t id = a->id[i];
  switch (a->zweck) {
    case AUSWAHL_DIAGRAMM: diagramm_zeigen(id); break;
    case AUSWAHL_AENDERN: formular_zeigen(id); break;
    case AUSWAHL_LOESCHEN: loeschen_fragen(id); break;
    case AUSWAHL_EINTRAGEN:
      // Die Liste geht, die Bestaetigung kommt: danach steht wieder das
      // vorige Menue da (Abschnitt 4.4).
      bestaetigung_zeigen(id);
      liste_schliessen(l);
      break;
  }
}

static void prv_schluss(Liste *l) {
  free(l->daten);
}

static const ListeArt s_art = {
  .anzahl = prv_anzahl,
  .zeile = prv_zeile,
  .waehlen = prv_waehlen,
  .erscheint = prv_erscheint,
  .schluss = prv_schluss,
};

void auswahl_zeigen(AuswahlZweck zweck) {
  Auswahl *a = calloc(1, sizeof(Auswahl));
  if (!a) return;
  a->zweck = zweck;
  liste_zeigen(&s_art, s_kopf[zweck], a);
}
