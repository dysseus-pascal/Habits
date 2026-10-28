#pragma once
#include <pebble.h>
#include "habit.h"

// Ein Fenster wie eine Karte: oben ein Kreis mit Zeichen, darunter eine kleine
// Zeile, der Name gross und ein Satz. Rechts eine Aktionsleiste, wenn Tasten
// etwas tun. Fuer Bestaetigung, Rueckfrage, Erinnerung.

typedef enum { ZEICHEN_HAKEN = 1, ZEICHEN_ACHTUNG, ZEICHEN_GLOCKE, ZEICHEN_FRAGE } Zeichen;

typedef struct Karte Karte;
typedef void (*KarteTaste)(Karte *k, ButtonId taste);

struct Karte {
  Window *fenster;
  Layer *flaeche;
  ActionBarLayer *leiste;
  GBitmap *symbole[NUM_BUTTONS];
  char oben[32];
  char titel[HABIT_NAME_LEN + 8];
  char text[64];
  GColor farbe;
  uint8_t zeichen;
  KarteTaste taste;
  AppTimer *zeitgeber;
  uint16_t id;
  int wert;
};

// `symbole`: Ressourcen fuer Oben, Mitte, Unten (0 = keins); ohne jedes
// Symbol keine Leiste. Zurueck ruft `taste` mit BUTTON_ID_BACK.
Karte *karte_zeigen(const uint32_t symbole[3], KarteTaste taste);
void karte_neu_zeichnen(Karte *k);
void karte_schliessen(Karte *k);
// Schliesst die Karte nach `ms`; ein Tastendruck haelt das auf.
void karte_zeitgeber(Karte *k, uint32_t ms);
