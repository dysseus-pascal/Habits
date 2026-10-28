#include "kalender.h"

// Tage vom 1.1.1970 bis zum 1.1.2000.
#define TAGE_BIS_2000 10957
#define MITTAG_S (12 * 3600)

// days_from_civil und civil_from_days nach Howard Hinnant: genau fuer jedes
// Datum, ohne Tabellen.
Tag kalender_tag_aus_datum(int jahr, int monat, int mtag) {
  int32_t j = jahr - (monat <= 2);
  const int32_t era = (j >= 0 ? j : j - 399) / 400;
  const int32_t yoe = j - era * 400;
  const int32_t doy = (153 * (monat > 2 ? monat - 3 : monat + 9) + 2) / 5 + mtag - 1;
  const int32_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468 - TAGE_BIS_2000;
}

void kalender_datum(Tag tag, int *jahr, int *monat, int *mtag) {
  const int32_t z = tag + TAGE_BIS_2000 + 719468;
  const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
  const int32_t doe = z - era * 146097;
  const int32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const int32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const int32_t mp = (5 * doy + 2) / 153;
  const int m = mp < 10 ? mp + 3 : mp - 9;
  if (jahr) *jahr = yoe + era * 400 + (m <= 2);
  if (monat) *monat = m;
  if (mtag) *mtag = doy - (153 * mp + 2) / 5 + 1;
}

Tag kalender_tag(time_t t) {
  const struct tm *lt = localtime(&t);
  return kalender_tag_aus_datum(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday);
}

int kalender_wochentag(Tag tag) {
  // Tag 0 war ein Samstag (5, wenn Montag 0 ist).
  return (int)(((tag % 7) + 7 + 5) % 7);
}

Tag kalender_wochenbeginn(Tag tag, int erster) {
  return tag - (kalender_wochentag(tag) - erster + 7) % 7;
}

int32_t kalender_monat(Tag tag) {
  int jahr, monat;
  kalender_datum(tag, &jahr, &monat, NULL);
  return (int32_t)jahr * 12 + monat - 1;
}

Tag kalender_monatsbeginn(int32_t monat) {
  return kalender_tag_aus_datum((int)(monat / 12), (int)(monat % 12) + 1, 1);
}

int kalender_monatstage(int32_t monat) {
  return (int)(kalender_monatsbeginn(monat + 1) - kalender_monatsbeginn(monat));
}

int kalender_minute(time_t t) {
  const struct tm *lt = localtime(&t);
  return lt->tm_hour * 60 + lt->tm_min;
}

int kalender_stunde(time_t t) {
  return localtime(&t)->tm_hour;
}

// Sekunden seit dem 1.1.2000 00:00 auf der Wanduhr.
static int32_t prv_wand(time_t t) {
  const struct tm *lt = localtime(&t);
  const Tag tag = kalender_tag_aus_datum(lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday);
  return tag * 86400 + lt->tm_hour * 3600 + lt->tm_min * 60 + lt->tm_sec;
}

// Der lokale Mittag: ihn gibt es an jedem Tag genau einmal. Vom Mittag in UTC
// aus fehlt nur der Versatz der Zeitzone; ein zweiter Schritt faengt den Fall,
// dass zwischen beiden Zeitpunkten die Sommerzeit wechselt.
static time_t prv_mittag(Tag tag) {
  const int32_t ziel = tag * 86400 + MITTAG_S;
  time_t t = (time_t)(tag + TAGE_BIS_2000) * 86400 + MITTAG_S;
  for (int i = 0; i < 3; i++) {
    const int32_t ab = ziel - prv_wand(t);
    if (ab == 0) break;
    t += ab;
  }
  return t;
}

// Vom Mittag zurueckgerechnet stimmt jede Zeit nach der Umstellung; eine davor
// liegt eine Stunde daneben und wird um die Abweichung der Wanduhr
// nachgezogen. Fuer eine Zeit in der ausgefallenen Stunde zeigt die Wanduhr
// danach eine Stunde mehr - sie bleibt so, also eine Stunde spaeter.
time_t kalender_zeit(Tag tag, int minute) {
  if (minute >= 1440) {
    tag += minute / 1440;
    minute %= 1440;
  }
  const time_t grob = prv_mittag(tag) - MITTAG_S + (time_t)minute * 60;
  int32_t ab = (int32_t)(tag * 86400 + minute * 60) - prv_wand(grob);
  if (ab > MITTAG_S) ab -= 2 * MITTAG_S;
  if (ab < -MITTAG_S) ab += 2 * MITTAG_S;
  return grob + ab;
}

int kalender_erster_wochentag(void) {
  const char *sprache = i18n_get_system_locale();
  return (sprache && strncmp(sprache, "en_US", 5) == 0) ? 6 : 0;
}
