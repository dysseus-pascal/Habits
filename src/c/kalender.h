#pragma once
#include <pebble.h>

// Kalenderzeit der Uhr, immer Ortszeit, so wie die Uhr sie zeigt: ein Tag geht
// von Mitternacht bis Mitternacht, eine Woche von ihrem ersten Tag bis zum
// naechsten, Monat und Jahr wie im Kalender.
//
// TAGE SIND NUMMERN, gezaehlt ab dem 1.1.2000 (Tag 0, ein Samstag). Abstaende,
// Wochentage und der Ringpuffer des Logbuchs sind damit Ganzzahlrechnung, und
// ein Umstellungstag mit 23 oder 25 Stunden bleibt ein Tag.
//
// OHNE mktime: der Weg von Tag und Uhrzeit zurueck zur Sekunde geht ueber den
// lokalen Mittag (kalender_zeit), wie in Drinktervall. Er braucht nur
// localtime und ist am Umstellungstag geprueft (tools/test_kalender.c).

typedef int32_t Tag;

// Vor diesem Tag (1.1.2025) gilt die Uhrzeit als nicht gestellt: nach einem
// Zuruecksetzen steht die Uhr kurz auf einem alten Datum, bis das Telefon sie
// stellt. Ein Eintrag dann landete an einem falschen Tag.
#define KALENDER_TAG_GUELTIG 9132

Tag kalender_tag(time_t t);
Tag kalender_tag_aus_datum(int jahr, int monat, int mtag);
void kalender_datum(Tag tag, int *jahr, int *monat, int *mtag);

// 0 = Montag ... 6 = Sonntag.
int kalender_wochentag(Tag tag);
// Erster Tag der Woche, in der `tag` liegt; `erster` ist ihr Wochentag
// (0 = Montag, 6 = Sonntag).
Tag kalender_wochenbeginn(Tag tag, int erster);

// Monate als fortlaufende Nummer: jahr * 12 + monat - 1.
int32_t kalender_monat(Tag tag);
Tag kalender_monatsbeginn(int32_t monat);
int kalender_monatstage(int32_t monat);

// Minute des Tages (0..1439) und Stunde, wie die Uhr sie zeigt.
int kalender_minute(time_t t);
int kalender_stunde(time_t t);

// Der Zeitpunkt, an dem die Uhr am Tag `tag` die Minute `minute` zeigt.
// `minute` darf 1440 sein (Mitternacht danach). Eine Uhrzeit, die es am
// Umstellungstag nicht gibt (02:30 im Fruehling), kommt eine Stunde spaeter.
time_t kalender_zeit(Tag tag, int minute);

// Erster Wochentag nach der Sprache der Uhr: Sonntag fuer US-Englisch,
// sonst Montag. Pebble kennt keine eigene Einstellung dafuer.
int kalender_erster_wochentag(void);
