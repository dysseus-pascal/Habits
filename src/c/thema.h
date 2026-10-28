#pragma once
#include <pebble.h>

// Farben der Oberflaeche nach dem Thema (Einstellungen: hell oder dunkel).
// Auf der Schwarz-Weiss-Uhr (flint) stehen Gut, Schlecht und Linie in der
// Schriftfarbe; der Unterschied liegt dort in der Form (Punkt oder Ring).
typedef struct {
  GColor grund, text, leise;      //< Hintergrund, Schrift, Nebensaechliches
  GColor mark_grund, mark_text;   //< die gewaehlte Zeile
  GColor gut, schlecht, linie;    //< Gewohnheiten, Ziel- und Grenzlinie
  GColor balken_leer;             //< Balken in der Zukunft
} Farben;

const Farben *farben(void);
void thema_menue(MenuLayer *menue);
void thema_leiste(StatusBarLayer *leiste);
