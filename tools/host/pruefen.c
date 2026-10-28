#include <pebble.h>
#include "pruefen.h"

int pruef_n, pruef_fehler;

#undef time
time_t ort(int jahr, int monat, int tag, int stunde, int minute) {
  struct tm tm = {0};
  tm.tm_year = jahr - 1900;
  tm.tm_mon = monat - 1;
  tm.tm_mday = tag;
  tm.tm_hour = stunde;
  tm.tm_min = minute;
  tm.tm_isdst = -1;
  return mktime(&tm);
}

const char *zeige(time_t t) {
  static char puffer[4][24];
  static int i;
  i = (i + 1) % 4;
  strftime(puffer[i], sizeof(puffer[i]), "%Y-%m-%d %H:%M", localtime(&t));
  return puffer[i];
}
