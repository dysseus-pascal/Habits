#include "fenster.h"
#include "liste.h"

// Das Hauptmenue der Anfrage (Abschnitt 2) und sein Untermenue "My habits".

static uint16_t prv_haupt_anzahl(Liste *l) {
  return 4;
}

static void prv_haupt_zeile(Liste *l, uint16_t i, Zeile *z) {
  static const char *const titel[] = {"My habits", "Log a habit", "Settings", "Exit"};
  strncpy(z->titel, titel[i], sizeof(z->titel) - 1);
  if (i == 0) {
    const int n = habits_anzahl();
    if (n) snprintf(z->untertitel, sizeof(z->untertitel), n == 1 ? "1 habit" : "%d habits", n);
    else strncpy(z->untertitel, "Add your first", sizeof(z->untertitel) - 1);
  }
}

static void prv_haupt_waehlen(Liste *l, uint16_t i) {
  switch (i) {
    case 0: meine_zeigen(); break;
    case 1: auswahl_zeigen(AUSWAHL_EINTRAGEN); break;
    case 2: einstellungen_zeigen(); break;
    default: app_beenden(); break;
  }
}

static const ListeArt s_haupt = {
  .anzahl = prv_haupt_anzahl,
  .zeile = prv_haupt_zeile,
  .waehlen = prv_haupt_waehlen,
};

void hauptmenue_zeigen(void) {
  liste_zeigen(&s_haupt, "HABITS", NULL);
}

static uint16_t prv_meine_anzahl(Liste *l) {
  return 4;
}

static void prv_meine_zeile(Liste *l, uint16_t i, Zeile *z) {
  static const char *const titel[] = {"See charts", "Add habit", "Edit habit", "Delete habit"};
  strncpy(z->titel, titel[i], sizeof(z->titel) - 1);
  if (i == 1 && habits_anzahl() >= HABITS_MAX) {
    snprintf(z->untertitel, sizeof(z->untertitel), "All %d places used", HABITS_MAX);
  }
}

static void prv_meine_waehlen(Liste *l, uint16_t i) {
  switch (i) {
    case 0: auswahl_zeigen(AUSWAHL_DIAGRAMM); break;
    case 1:
      if (habits_anzahl() < HABITS_MAX) formular_zeigen(0);
      break;
    case 2: auswahl_zeigen(AUSWAHL_AENDERN); break;
    default: auswahl_zeigen(AUSWAHL_LOESCHEN); break;
  }
}

static const ListeArt s_meine = {
  .anzahl = prv_meine_anzahl,
  .zeile = prv_meine_zeile,
  .waehlen = prv_meine_waehlen,
};

void meine_zeigen(void) {
  liste_zeigen(&s_meine, "MY HABITS", NULL);
}
