#include "diagramm_daten.h"

static const char *const s_monate[] = {"January", "February", "March", "April", "May", "June", "July",
                                       "August", "September", "October", "November", "December"};
static const char *const s_tage[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

// Gilt `line` je Tag? Fuer schlechte immer, fuer gute ohne "einmal alle n"
// oder mit "einmal am Tag".
static bool prv_taeglich(const Habit *h) {
  if (h->type == HABIT_BAD || h->period_type != PERIOD_ONCE_PER) return true;
  if (h->period_unit == UNIT_DAYS) return h->period_count == 1;
  return h->period_unit == UNIT_HOURS && h->period_count >= 24;
}

static bool prv_stuendlich(const Habit *h) {
  return h->type == HABIT_GOOD && h->period_type == PERIOD_ONCE_PER && h->period_unit == UNIT_HOURS &&
         h->period_count < 24;
}

// Die Tageslinie: nach Plan ohne eigenes Ziel wechselt es mit dem Tag, dann
// keine.
static int16_t prv_tageslinie(const Habit *h) {
  return prv_taeglich(h) && h->line ? (int16_t)h->line : -1;
}

void diagramm_rechnen(const Habit *h, const Logbuch *lb, Ansicht a, int zurueck, time_t jetzt,
                      int erster_wochentag, Diagramm *d) {
  memset(d, 0, sizeof(*d));
  d->ansicht = a;
  d->linie = -1;
  d->erster_wochentag = erster_wochentag;
  const Tag heute = kalender_tag(jetzt);
  switch (a) {
    case ANSICHT_TAG: {
      const Tag tag = heute - zurueck;
      uint8_t st[24];
      d->tag_von = tag;
      d->n = 24;
      d->tage = 1;
      if (!logbuch_stunden(lb, tag, st)) {
        d->ohne_stunden = true;
        d->summe = logbuch_tag(lb, tag);
        return;
      }
      d->laufend = !prv_stuendlich(h);
      int laufend = 0;
      for (int i = 0; i < 24; i++) {
        laufend += st[i];
        d->wert[i] = (uint16_t)(d->laufend ? laufend : st[i]);
        if (tag == heute && i > kalender_stunde(jetzt)) d->zukunft |= 1u << i;
      }
      d->summe = laufend;
      if (d->laufend) d->linie = prv_tageslinie(h);
      else if (h->period_count == 1 && h->line) d->linie = (int16_t)h->line;
      return;
    }
    case ANSICHT_WOCHE:
    case ANSICHT_MONAT: {
      Tag von;
      if (a == ANSICHT_WOCHE) {
        von = kalender_wochenbeginn(heute, erster_wochentag) - 7 * zurueck;
        d->n = 7;
      } else {
        const int32_t monat = kalender_monat(heute) - zurueck;
        von = kalender_monatsbeginn(monat);
        d->n = (uint8_t)kalender_monatstage(monat);
      }
      d->tag_von = von;
      for (int i = 0; i < d->n; i++) {
        d->wert[i] = (uint16_t)logbuch_tag(lb, von + i);
        d->summe += d->wert[i];
        if (von + i > heute) d->zukunft |= 1u << i;
        else d->tage++;
      }
      d->linie = prv_tageslinie(h);
      return;
    }
    case ANSICHT_JAHR:
    default: {
      int jahr;
      kalender_datum(heute, &jahr, NULL, NULL);
      jahr -= zurueck;
      d->n = 12;
      d->tag_von = kalender_tag_aus_datum(jahr, 1, 1);
      d->zehntel = prv_taeglich(h);
      for (int i = 0; i < 12; i++) {
        const int32_t monat = (int32_t)jahr * 12 + i;
        const Tag von = kalender_monatsbeginn(monat);
        if (von > heute) {
          d->zukunft |= 1u << i;
          continue;
        }
        Tag bis = kalender_monatsbeginn(monat + 1);
        if (bis > heute + 1) bis = heute + 1;
        const int summe = logbuch_tage(lb, von, bis);
        d->summe += summe;
        d->tage += (int16_t)(bis - von);
        d->wert[i] = (uint16_t)(d->zehntel ? (summe * 10 + (bis - von) / 2) / (bis - von) : summe);
      }
      if (d->zehntel && prv_tageslinie(h) >= 0) d->linie = (int16_t)(prv_tageslinie(h) * 10);
      return;
    }
  }
}

int diagramm_zurueck_max(Ansicht a) {
  switch (a) {
    case ANSICHT_TAG: return LOGBUCH_STUNDEN_TAGE - 1;
    case ANSICHT_WOCHE: return (LOGBUCH_TAGE - 7) / 7;
    case ANSICHT_MONAT: return (LOGBUCH_TAGE - 31) / 31;
    default: return 1;
  }
}

void diagramm_titel(const Diagramm *d, Tag heute, char *puffer, size_t groesse) {
  int j, m, t;
  kalender_datum(d->tag_von, &j, &m, &t);
  switch (d->ansicht) {
    case ANSICHT_TAG:
      if (d->tag_von == heute) snprintf(puffer, groesse, "Today");
      else if (d->tag_von == heute - 1) snprintf(puffer, groesse, "Yesterday");
      else snprintf(puffer, groesse, "%s %d %.3s", s_tage[kalender_wochentag(d->tag_von)], t, s_monate[m - 1]);
      return;
    case ANSICHT_WOCHE: {
      const Tag diese = kalender_wochenbeginn(heute, d->erster_wochentag);
      if (d->tag_von == diese) {
        snprintf(puffer, groesse, "This week");
      } else if (d->tag_von == diese - 7) {
        snprintf(puffer, groesse, "Last week");
      } else {
        int j2, m2, t2;
        kalender_datum(d->tag_von + 6, &j2, &m2, &t2);
        if (m2 == m) snprintf(puffer, groesse, "%d-%d %.3s", t, t2, s_monate[m - 1]);
        else snprintf(puffer, groesse, "%d %.3s - %d %.3s", t, s_monate[m - 1], t2, s_monate[m2 - 1]);
      }
      return;
    }
    case ANSICHT_MONAT: snprintf(puffer, groesse, "%s %d", s_monate[m - 1], j); return;
    default: snprintf(puffer, groesse, "%d", j); return;
  }
}

void diagramm_marke(const Diagramm *d, int i, bool h24, char *puffer, size_t groesse) {
  puffer[0] = 0;
  switch (d->ansicht) {
    case ANSICHT_TAG:
      if (i % 6) return;
      if (h24) snprintf(puffer, groesse, "%d", i);
      else snprintf(puffer, groesse, "%d%s", (i + 11) % 12 + 1, i < 12 ? "a" : "p");
      return;
    case ANSICHT_WOCHE:
      snprintf(puffer, groesse, "%.1s", s_tage[(d->erster_wochentag + i) % 7]);
      return;
    case ANSICHT_MONAT:
      if (i % 7 == 0) snprintf(puffer, groesse, "%d", i + 1);
      return;
    default:
      snprintf(puffer, groesse, "%.1s", s_monate[i]);
      return;
  }
}
