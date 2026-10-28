#pragma once
#include <pebble.h>

// Stellt die Wecker der App neu: die naechsten acht Termine aller
// Gewohnheiten (planer.c). Nach jedem Start, Eintrag, Aendern und Wecker -
// so stimmen sie immer mit dem Stand ueberein, und ein Termin, der wegen
// der acht Plaetze noch keinen Wecker hat, bekommt ihn, sobald einer frei wird.
void wecker_planen(void);
