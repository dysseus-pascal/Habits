#pragma once
#include <pebble.h>
#include "kalender.h"

// Was eingetragen wurde, gezaehlt statt einzeln: je Stunde fuer die letzten
// sieben Tage, je Tag fuer gut zwei Jahre, dazu der Zeitpunkt des letzten
// Eintrags. Die Anfrage erlaubt das ("day-aggregated counts"), und es braucht
// gleich viel Platz, ob einmal am Tag eingetragen wird oder zwanzigmal.
//
// Die Stunden reichen fuer die Tagesansicht und fuer "einmal alle n Stunden",
// die Tage fuer Woche, Monat, Jahr und das Vorjahr. Ein Zaehler endet bei 255.

#define LOGBUCH_STUNDEN_TAGE 7
#define LOGBUCH_FACH_TAGE 248
#define LOGBUCH_FAECHER 3
#define LOGBUCH_TAGE (LOGBUCH_FACH_TAGE * LOGBUCH_FAECHER) //< 744: zwei Kalenderjahre

typedef struct {
  // --- Kopf (ein Fach) ---
  uint16_t habit_id;      //< wem es gehoert; ein fremdes Logbuch gilt als leer
  uint16_t fassung;
  int32_t letzter_tag;    //< juengster Tag im Ring; 0 = noch nie eingetragen
  int32_t stunden_tag;    //< Tag der Zeile stunden[0]
  int32_t letzter_log;    //< Zeitpunkt des letzten Eintrags, 0 = keiner
  int32_t vorletzter_log; //< davor; fuer genau ein Rueckgaengig
  int32_t pause_bis;      //< "Skip": vor diesem Zeitpunkt keine Erinnerung
  int32_t erinnert;       //< bis hierhin ist jede faellige Erinnerung gezeigt
  uint8_t stunden[LOGBUCH_STUNDEN_TAGE][24]; //< [0] = stunden_tag, [1] = Tag davor ...
  // --- Ring der Tage (drei Faecher) ---
  uint8_t tage[LOGBUCH_TAGE]; //< Index: Tag % LOGBUCH_TAGE
  // --- nur im Speicher der App ---
  uint8_t geaendert; //< Bit k: Fach k (0 = Kopf) muss geschrieben werden
} Logbuch;

// Aus dem Speicher: leer, wenn es fehlt oder einer anderen id gehoert.
void logbuch_laden(Logbuch *lb, int platz, uint16_t habit_id);
void logbuch_speichern(Logbuch *lb, int platz);
void logbuch_loeschen(int platz);
void logbuch_leeren(Logbuch *lb, uint16_t habit_id);

// Ein Eintrag zum Zeitpunkt t. false, wenn die Uhrzeit nicht gestellt ist
// oder t vor dem Ring liegt.
bool logbuch_eintragen(Logbuch *lb, time_t t);
// Nimmt den letzten Eintrag zurueck (nur einen). false, wenn es keinen gibt.
bool logbuch_rueckgaengig(Logbuch *lb);
// Setzt pause_bis oder erinnert und merkt den Kopf zum Schreiben vor.
void logbuch_pause(Logbuch *lb, time_t bis);
void logbuch_erinnert(Logbuch *lb, time_t bis);

int logbuch_tag(const Logbuch *lb, Tag tag);
// Summe der Tage [von, bis).
int logbuch_tage(const Logbuch *lb, Tag von, Tag bis);
// Stunden des Tages; false, wenn sie nicht mehr da sind (aelter als sieben
// Tage). Sonst steht in `stunden` je Stunde die Zahl.
bool logbuch_stunden(const Logbuch *lb, Tag tag, uint8_t stunden[24]);
