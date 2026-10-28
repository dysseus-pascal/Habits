#include "planer.h"

typedef struct {
  Termin *aus;
  int n, max;
  time_t ab, ende;
  time_t pause;
} Sammler;

// Nimmt einen Termin auf. false: genug gesammelt oder ueber den Horizont -
// die Termine kommen aufsteigend, also braucht es keine weiteren.
static bool prv_neu(Sammler *s, time_t t, uint16_t id, uint8_t art) {
  if (t > s->ende || s->n >= s->max) return false;
  if (t <= s->ab || t < s->pause) return true;
  s->aus[s->n++] = (Termin){.zeit = t, .habit_id = id, .art = art};
  return s->n < s->max;
}

// Gemischte Bits nach dem Integer-Hash von Chris Wellons: dieselben Abstaende
// bei jeder Planung, denn sie haengen nur an Gewohnheit, Tag und Nummer.
static uint32_t prv_mischen(uint32_t x) {
  x ^= x >> 16;
  x *= 0x7feb352dU;
  x ^= x >> 15;
  x *= 0x846ca68bU;
  x ^= x >> 16;
  return x;
}

static int prv_zufall_min(uint16_t id, int32_t anker, int k, int lo, int hi) {
  const uint32_t x = prv_mischen((uint32_t)anker * 2654435761u ^ ((uint32_t)id << 8) ^ (uint32_t)k);
  return lo + (int)(x % (uint32_t)(hi - lo + 1));
}

// Heavy: zufaellige Abstaende in [lo, hi] Minuten, ab `a` bis vor `b`; ein
// Eintrag schiebt den naechsten Anstoss um mindestens lo hinaus.
static bool prv_zufaellig(const Habit *h, const Logbuch *lb, Sammler *s, time_t a, time_t b, int lo,
                          int hi) {
  time_t t = a;
  for (int k = 0; k < 64; k++) {
    t += (time_t)prv_zufall_min(h->id, (int32_t)(a / 60), k, lo, hi) * 60;
    if (t >= b) return true;
    if (lb->letzter_log && t < lb->letzter_log + (time_t)lo * 60) continue;
    if (!prv_neu(s, t, h->id, TERMIN_ERINNERUNG)) return false;
  }
  return true;
}

// Critical: alle 5 Minuten ab `a` bis vor `b`.
static bool prv_kritisch(const Habit *h, Sammler *s, time_t a, time_t b) {
  // Gleich zum ersten Termin nach `ab` und nicht vor der Pause springen,
  // statt eine Woche in Schritten von 5 Minuten abzulaufen.
  time_t t = a;
  if (t <= s->ab) t += ((s->ab - t) / PLANER_KRITISCH_S + 1) * PLANER_KRITISCH_S;
  if (t < s->pause) t += ((s->pause - t + PLANER_KRITISCH_S - 1) / PLANER_KRITISCH_S) * PLANER_KRITISCH_S;
  for (; t < b; t += PLANER_KRITISCH_S) {
    if (!prv_neu(s, t, h->id, TERMIN_ERINNERUNG)) return false;
  }
  return true;
}

static void prv_heavy_abstand(const Zeitraum *z, int *lo, int *hi) {
  if (!zeitraum_tage(z)) {
    const int n = z->stunde_bis - z->stunde_von;
    *lo = 15;
    *hi = n * 20 < 20 ? 20 : (n * 20 > 60 ? 60 : n * 20);
  } else if (z->tag_bis - z->tag_von <= 1) {
    *lo = 45;
    *hi = 120;
  } else {
    *lo = 120;
    *hi = 240;
  }
}

// Die aktiven Stunden des Tages `d`, beschnitten auf den Zeitraum.
static bool prv_aktiv(const Rahmen *r, const Zeitraum *z, Tag d, time_t *a, time_t *b) {
  *a = kalender_zeit(d, r->aktiv_von);
  *b = kalender_zeit(d, r->aktiv_bis);
  if (*a < z->von) *a = z->von;
  if (*b > z->bis) *b = z->bis;
  return *a < *b;
}

// Ein unerfuellter Zeitraum ohne festen Plan.
static bool prv_zeitraum(const Habit *h, const Logbuch *lb, const Rahmen *r, const Zeitraum *z,
                         Sammler *s) {
  time_t a, b;
  switch (h->insistence) {
    case INSIST_LIGHT:
      if (zeitraum_tage(z)) return prv_neu(s, kalender_zeit(z->tag_bis, r->aktiv_von), 0, TERMIN_BILANZ);
      // Der naechste Block beginnt - nur in den aktiven Stunden.
      if (z->stunde_bis >= 24 || z->stunde_bis * 60 < r->aktiv_von ||
          z->stunde_bis * 60 > r->aktiv_bis) {
        return true;
      }
      return prv_neu(s, z->bis, h->id, TERMIN_ERINNERUNG);
    case INSIST_HEAVY: {
      int lo, hi;
      prv_heavy_abstand(z, &lo, &hi);
      for (Tag d = z->tag_von; d < z->tag_bis; d++) {
        if (prv_aktiv(r, z, d, &a, &b) && !prv_zufaellig(h, lb, s, a, b, lo, hi)) return false;
        if (kalender_zeit(d, 0) > s->ende) return false;
      }
      return true;
    }
    case INSIST_CRITICAL:
      // Ab den ersten aktiven Stunden im Zeitraum bis zu seinem Ende, auch
      // nachts: das ist Critical.
      for (Tag d = z->tag_von; d < z->tag_bis; d++) {
        if (prv_aktiv(r, z, d, &a, &b)) return prv_kritisch(h, s, a, z->bis);
        if (kalender_zeit(d, 0) > s->ende) return false;
      }
      return true;
    default:
      return true;
  }
}

static void prv_frei(const Habit *h, const Logbuch *lb, const Rahmen *r, Sammler *s) {
  time_t t = s->ab;
  for (int i = 0; i < 1000 && t <= s->ende; i++) {
    const Zeitraum z = zeitraum(h, t, r->erster_wochentag);
    if (lb->pause_bis < z.bis && zeitraum_zahl(lb, &z) < habit_ziel(h, z.tag_von) &&
        !prv_zeitraum(h, lb, r, &z, s)) {
      return;
    }
    t = z.bis;
  }
}

#define PLAN_ZEITEN_MAX (12 * HABIT_ZEITEN_MAX)

static void prv_plan(const Habit *h, const Logbuch *lb, Sammler *s) {
  // Alle geplanten Zeiten von gestern bis nach dem Horizont; die letzte hat
  // eine naechste, sobald ein Plantag in der Woche danach liegt.
  time_t zeiten[PLAN_ZEITEN_MAX];
  int n = 0;
  const Tag heute = kalender_tag(s->ab);
  for (Tag d = heute - 1; d <= heute + 10 && n < PLAN_ZEITEN_MAX; d++) {
    if (!habit_geplant_am(h, d)) continue;
    for (int i = 0; i < h->times_n && n < PLAN_ZEITEN_MAX; i++) zeiten[n++] = kalender_zeit(d, h->times[i]);
  }
  for (int i = 0; i + 1 < n; i++) {
    const time_t t = zeiten[i], naechste = zeiten[i + 1];
    if (naechste <= s->ab || lb->pause_bis >= naechste) continue;
    if (lb->letzter_log && lb->letzter_log >= t - PLANER_FRUEHER_S) continue;
    if (!prv_neu(s, t, h->id, TERMIN_ERINNERUNG)) return;
    if (h->insistence == INSIST_HEAVY) {
      const time_t b = t + PLANER_NACHPLAN_S < naechste ? t + PLANER_NACHPLAN_S : naechste;
      if (!prv_zufaellig(h, lb, s, t, b, 20, 45)) return;
    } else if (h->insistence == INSIST_CRITICAL) {
      if (!prv_kritisch(h, s, t + PLANER_KRITISCH_S, naechste)) return;
    }
  }
}

int planer_termine(const Habit *h, const Logbuch *lb, time_t ab, const Rahmen *r, Termin *aus,
                   int max) {
  Sammler s = {
    .aus = aus, .n = 0, .max = max, .ab = ab, .ende = ab + PLANER_HORIZONT_S, .pause = lb->pause_bis,
  };
  if (h->insistence == INSIST_NONE || max <= 0) return 0;
  if (h->type == HABIT_BAD) {
    // Ueber der Grenze: morgen frueh in die Bilanz. Kuenftige Tage haben noch
    // keine Eintraege.
    const Tag heute = kalender_tag(ab);
    if (h->insistence == INSIST_LIGHT && h->line && logbuch_tag(lb, heute) > h->line) {
      prv_neu(&s, kalender_zeit(heute + 1, r->aktiv_von), 0, TERMIN_BILANZ);
    }
  } else if (h->period_type == PERIOD_SCHEDULED) {
    prv_plan(h, lb, &s);
  } else {
    prv_frei(h, lb, r, &s);
  }
  return s.n;
}

void planer_einfuegen(Termin *liste, int *n, int max, const Termin *neu, int anzahl) {
  for (int i = 0; i < anzahl; i++) {
    const Termin t = neu[i];
    int k = *n;
    while (k > 0 && liste[k - 1].zeit > t.zeit) k--;
    bool doppelt = false;
    for (int j = k - 1; j >= 0 && liste[j].zeit == t.zeit; j--) {
      if (t.art == TERMIN_BILANZ && liste[j].art == TERMIN_BILANZ) doppelt = true;
    }
    if (doppelt || k >= max) continue;
    const int bleiben = *n < max ? *n : max - 1;
    memmove(&liste[k + 1], &liste[k], (size_t)(bleiben - k) * sizeof(Termin));
    liste[k] = t;
    if (*n < max) (*n)++;
  }
}

void planer_abstand(Termin *liste, int n) {
  for (int i = 1; i < n; i++) {
    if (liste[i].zeit < liste[i - 1].zeit + PLANER_ABSTAND_S) {
      liste[i].zeit = liste[i - 1].zeit + PLANER_ABSTAND_S;
    }
  }
}

bool planer_faellig(const Habit *h, const Logbuch *lb, time_t jetzt, const Rahmen *r) {
  time_t ab = jetzt - PLANER_RUECKBLICK_S;
  if (lb->erinnert > ab) ab = lb->erinnert;
  Termin t;
  // Die Bilanz zaehlt hier nicht, sie hat ihren eigenen Weg (planer_bilanz).
  Termin liste[4];
  const int n = planer_termine(h, lb, ab, r, liste, 4);
  for (int i = 0; i < n; i++) {
    t = liste[i];
    if (t.zeit > jetzt + PLANER_VORGRIFF_S) return false;
    if (t.art == TERMIN_ERINNERUNG) return true;
  }
  return false;
}

bool planer_bilanz(const Habit *h, const Logbuch *lb, Tag heute, const Rahmen *r, int *zahl,
                   int *ziel) {
  if (h->insistence != INSIST_LIGHT) return false;
  if (h->type == HABIT_BAD) {
    *zahl = logbuch_tag(lb, heute - 1);
    *ziel = h->line;
    return h->line && *zahl > h->line;
  }
  if (h->period_type == PERIOD_SCHEDULED) return false;
  const time_t mitternacht = kalender_zeit(heute, 0);
  const Zeitraum z = zeitraum(h, mitternacht - 1, r->erster_wochentag);
  if (!zeitraum_tage(&z) || z.bis != mitternacht) return false;
  *zahl = zeitraum_zahl(lb, &z);
  *ziel = habit_ziel(h, z.tag_von);
  return *zahl < *ziel;
}
