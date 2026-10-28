#pragma once
#include <pebble.h>
#include "planer.h"

// Die Einstellungen der Anfrage (Abschnitt 4.5): Vibration und Ton je Typ,
// das Thema der Menues. Dazu die aktiven Stunden: in ihnen erinnern Light und
// Heavy, mit ihrem Beginn kommt die Morgenbilanz.

typedef enum {
  HAPTIK_KEINE = 0,
  HAPTIK_KURZ,
  HAPTIK_DOPPELT,
  HAPTIK_LANG,
  HAPTIK_DREIFACH,
  HAPTIK_STEIGEND,
  HAPTIK_ANZAHL,
} Haptik;

typedef enum { TON_AUS = 0, TON_PIEP, TON_GLOCKE, TON_AUFSTIEG, TON_ANZAHL } Ton;
typedef enum { THEMA_HELL = 0, THEMA_DUNKEL, THEMA_ANZAHL } ThemaArt;

typedef struct {
  uint8_t fassung;
  uint8_t haptik[2];  //< [HABIT_GOOD], [HABIT_BAD]
  uint8_t ton[2];
  uint8_t thema;
  uint16_t aktiv_von; //< Minuten des Tages
  uint16_t aktiv_bis;
} Einstellungen;

void einstellungen_laden(void);
const Einstellungen *einstellungen(void);
// Bereinigt und speichert.
void einstellungen_setzen(const Einstellungen *e);
Rahmen einstellungen_rahmen(void);

const char *einstellungen_haptik_name(int haptik);
const char *einstellungen_ton_name(int ton);
