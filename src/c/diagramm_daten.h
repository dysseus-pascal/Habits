#pragma once
#include <pebble.h>
#include "habit.h"
#include "logbuch.h"

// Was ein Diagramm zeigt (Abschnitt 4.1), gerechnet ohne Grafik, damit es
// auf dem Rechner geprueft wird (tools/test_diagramm.c).
//
//   Tag    24 Stunden: die laufende Summe, damit die Tageslinie (Ziel oder
//          Grenze) passt; bei Stunden-Bloecken die Zahl je Stunde
//   Woche  7 Tage, Monat 28-31 Tage: je Tag
//   Jahr   12 Monate: bei Gewohnheiten mit Tageszeitraum der Schnitt je Tag,
//          damit die Linie passt, sonst die Summe des Monats
//
// Die Linie steht nur dort, wo ein Balken dasselbe misst wie `line`: ein Ziel
// "einmal alle 2 Wochen" hat in einem Tagesbalken keinen Platz.

typedef enum { ANSICHT_TAG, ANSICHT_WOCHE, ANSICHT_MONAT, ANSICHT_JAHR, ANSICHT_ANZAHL } Ansicht;

#define DIAGRAMM_MAX 31

typedef struct {
  Ansicht ansicht;
  uint8_t n;
  uint16_t wert[DIAGRAMM_MAX]; //< in Zehnteln, wenn `zehntel`
  bool zehntel;
  uint32_t zukunft;            //< Bit i: Balken i liegt nach jetzt
  int16_t linie;               //< wie `wert`; -1 = keine
  int32_t summe;               //< Eintraege im ganzen Ausschnitt
  int16_t tage;                //< davon vergangene Tage, fuer den Schnitt
  bool laufend;                //< Tag: laufende Summe
  bool ohne_stunden;           //< Tag: aelter als sieben Tage, nur die Summe
  Tag tag_von;                 //< erster Tag (Jahr: 1. Januar)
  int erster_wochentag;
} Diagramm;

void diagramm_rechnen(const Habit *h, const Logbuch *lb, Ansicht a, int zurueck, time_t jetzt,
                      int erster_wochentag, Diagramm *d);
// So weit reichen die Daten zurueck: Stunden sieben Tage, Tage gut zwei Jahre.
int diagramm_zurueck_max(Ansicht a);
// "Today", "Last week", "October 2026" ...
void diagramm_titel(const Diagramm *d, Tag heute, char *puffer, size_t groesse);
// Beschriftung unter Balken i, leer wo keine hingehoert.
void diagramm_marke(const Diagramm *d, int i, bool h24, char *puffer, size_t groesse);
