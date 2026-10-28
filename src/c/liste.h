#pragma once
#include <pebble.h>

// Ein Fenster mit einer Liste: Uhrzeit oben, eine Kopfzeile, darunter die
// Zeilen. Fast jede Ansicht der App ist so eine Liste (Hauptmenue, Auswahl,
// Formular, Einstellungen ...); sie unterscheiden sich nur in ihren Zeilen
// und darin, was ein Klick tut.

typedef struct Liste Liste;

typedef struct {
  char titel[40];
  char untertitel[40];
  uint8_t marke;  //< 0 keine, 1 gut (Punkt), 2 schlecht (Ring)
  int8_t haken;   //< -1 keiner, 0 leer, 1 gesetzt
} Zeile;

typedef struct {
  uint16_t (*anzahl)(Liste *l);
  void (*zeile)(Liste *l, uint16_t i, Zeile *z);
  void (*waehlen)(Liste *l, uint16_t i);
  void (*lang)(Liste *l, uint16_t i);  //< darf fehlen
  void (*erscheint)(Liste *l);         //< darf fehlen: Daten neu holen
  void (*schluss)(Liste *l);           //< darf fehlen: Daten freigeben
} ListeArt;

struct Liste {
  Window *fenster;
  MenuLayer *menue;
  StatusBarLayer *leiste;
  const ListeArt *art;
  const char *kopf;
  void *daten;
};

Liste *liste_zeigen(const ListeArt *art, const char *kopf, void *daten);
void liste_auffrischen(Liste *l);
void liste_schliessen(Liste *l);
void liste_auswahl_setzen(Liste *l, uint16_t i);

// Eine Zeile zeichnen, wie die Listen es tun - auch fuer Fenster mit eigener
// MenuLayer.
void liste_zeile_zeichnen(GContext *ctx, const Layer *zelle, const Zeile *z);
