#pragma once

// Faecher im Speicher der Uhr (persist). Ein Fach fasst hoechstens 256 Byte;
// eine App hat auf den Uhren von Core Devices 1 MiB.
//
// DIE NUMMERN SIND FEST: sie stehen so auf den Uhren. Neue Faecher bekommen
// neue Nummern, alte werden nie anders verwendet.

#define FACH_NAECHSTE_ID   1   //< int: id der naechsten neuen Gewohnheit
#define FACH_EINSTELLUNGEN 2   //< Einstellungen (einstellungen.c)
#define FACH_BILANZ        3   //< int: Tag der letzten Morgenbilanz (planer.c)

// Die Gewohnheiten: ein Fach je Platz.
#define FACH_HABIT(platz)  (10 + (platz))

// Das Logbuch eines Platzes: Kopf mit den Stunden der letzten Tage, dahinter
// drei Faecher mit den Tagen (logbuch.c).
#define FACH_LOG(platz, k) (100 + (platz) * 4 + (k))
