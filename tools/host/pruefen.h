// Ein kleiner Pruefrahmen: jede Pruefung zaehlt, jeder Fehler nennt Datei und
// Zeile, am Ende zaehlt das Ergebnis.
#pragma once
#include <stdio.h>
#include <time.h>

extern int pruef_n, pruef_fehler;

#define PRUEFE(bed)                                                     \
  do {                                                                  \
    pruef_n++;                                                          \
    if (!(bed)) {                                                       \
      pruef_fehler++;                                                   \
      printf("FEHLER %s:%d: %s\n", __FILE__, __LINE__, #bed);           \
    }                                                                   \
  } while (0)

#define GLEICH(a, b)                                                    \
  do {                                                                  \
    pruef_n++;                                                          \
    const long long _a = (long long)(a), _b = (long long)(b);           \
    if (_a != _b) {                                                     \
      pruef_fehler++;                                                   \
      printf("FEHLER %s:%d: %s = %lld, erwartet %s = %lld\n", __FILE__, \
             __LINE__, #a, _a, #b, _b);                                 \
    }                                                                   \
  } while (0)

// Ortszeit (Europe/Zurich, siehe host_tests.sh) als Zeitpunkt.
time_t ort(int jahr, int monat, int tag, int stunde, int minute);
// Zeitpunkt als "JJJJ-MM-TT HH:MM" in Ortszeit, fuer Meldungen.
const char *zeige(time_t t);

#define PRUEF_ENDE(name)                                                \
  do {                                                                  \
    printf("%s: %d Pruefungen, %d Fehler\n", (name), pruef_n, pruef_fehler); \
    return pruef_fehler ? 1 : 0;                                        \
  } while (0)
