#include "eingabe.h"
#include "einstellungen.h"
#include "eintragen.h"
#include "fenster.h"
#include "liste.h"
#include "rueckmeldung.h"
#include "wecker.h"

// Die Einstellungen (Abschnitt 4.5) und die aktiven Stunden. Mitte schaltet
// weiter und spielt die neue Vibration oder den Ton gleich vor. Auf einer Uhr
// ohne Lautsprecher (gabbro) fehlen die Zeilen fuer den Ton.

typedef enum { E_HAPTIK_GUT, E_TON_GUT, E_HAPTIK_SCHLECHT, E_TON_SCHLECHT, E_THEMA, E_VON, E_BIS, E_ANZAHL } Zeilen;

static Zeilen prv_art(uint16_t i) {
  if (rueckmeldung_hat_ton()) return (Zeilen)i;
  static const Zeilen ohne_ton[] = {E_HAPTIK_GUT, E_HAPTIK_SCHLECHT, E_THEMA, E_VON, E_BIS};
  return ohne_ton[i];
}

static uint16_t prv_anzahl(Liste *l) {
  return rueckmeldung_hat_ton() ? E_ANZAHL : E_ANZAHL - 2;
}

static void prv_zeile(Liste *l, uint16_t i, Zeile *z) {
  const Einstellungen *e = einstellungen();
  const size_t tn = sizeof(z->titel), un = sizeof(z->untertitel);
  switch (prv_art(i)) {
    case E_HAPTIK_GUT:
      strncpy(z->titel, "Good: vibration", tn - 1);
      strncpy(z->untertitel, einstellungen_haptik_name(e->haptik[HABIT_GOOD]), un - 1);
      z->marke = 1;
      break;
    case E_TON_GUT:
      strncpy(z->titel, "Good: sound", tn - 1);
      strncpy(z->untertitel, einstellungen_ton_name(e->ton[HABIT_GOOD]), un - 1);
      z->marke = 1;
      break;
    case E_HAPTIK_SCHLECHT:
      strncpy(z->titel, "Bad: vibration", tn - 1);
      strncpy(z->untertitel, einstellungen_haptik_name(e->haptik[HABIT_BAD]), un - 1);
      z->marke = 2;
      break;
    case E_TON_SCHLECHT:
      strncpy(z->titel, "Bad: sound", tn - 1);
      strncpy(z->untertitel, einstellungen_ton_name(e->ton[HABIT_BAD]), un - 1);
      z->marke = 2;
      break;
    case E_THEMA:
      strncpy(z->titel, "Menu theme", tn - 1);
      strncpy(z->untertitel, e->thema == THEMA_DUNKEL ? "Dark" : "Light", un - 1);
      break;
    case E_VON:
      strncpy(z->titel, "Reminders from", tn - 1);
      eintragen_uhrzeit(e->aktiv_von, z->untertitel, un);
      break;
    case E_BIS:
      strncpy(z->titel, "Reminders until", tn - 1);
      eintragen_uhrzeit(e->aktiv_bis % 1440, z->untertitel, un);
      break;
    default:
      break;
  }
}

static void prv_von(void *daten, const int *werte) {
  Einstellungen e = *einstellungen();
  e.aktiv_von = (uint16_t)werte[0];
  einstellungen_setzen(&e);
  wecker_planen();
}

static void prv_bis(void *daten, const int *werte) {
  Einstellungen e = *einstellungen();
  // Mitternacht heisst bis zum Ende des Tages.
  e.aktiv_bis = (uint16_t)(werte[0] ? werte[0] : 1440);
  einstellungen_setzen(&e);
  wecker_planen();
}

static void prv_waehlen(Liste *l, uint16_t i) {
  Einstellungen e = *einstellungen();
  switch (prv_art(i)) {
    case E_HAPTIK_GUT:
    case E_HAPTIK_SCHLECHT: {
      const int typ = prv_art(i) == E_HAPTIK_SCHLECHT;
      e.haptik[typ] = (e.haptik[typ] + 1) % HAPTIK_ANZAHL;
      rueckmeldung_haptik(e.haptik[typ]);
      break;
    }
    case E_TON_GUT:
    case E_TON_SCHLECHT: {
      const int typ = prv_art(i) == E_TON_SCHLECHT;
      e.ton[typ] = (e.ton[typ] + 1) % TON_ANZAHL;
      rueckmeldung_ton(e.ton[typ]);
      break;
    }
    case E_THEMA:
      e.thema = (e.thema + 1) % THEMA_ANZAHL;
      einstellungen_setzen(&e);
      // Neu aufbauen, damit das Fenster selbst das neue Thema traegt.
      liste_schliessen(l);
      einstellungen_zeigen();
      return;
    case E_VON:
      uhrzeit_zeigen("Reminders from", e.aktiv_von, prv_von, NULL);
      return;
    case E_BIS:
      uhrzeit_zeigen("Reminders until", e.aktiv_bis % 1440, prv_bis, NULL);
      return;
    default:
      return;
  }
  einstellungen_setzen(&e);
  liste_auffrischen(l);
}

static const ListeArt s_art = {
  .anzahl = prv_anzahl,
  .zeile = prv_zeile,
  .waehlen = prv_waehlen,
};

void einstellungen_zeigen(void) {
  liste_zeigen(&s_art, "SETTINGS", NULL);
}
