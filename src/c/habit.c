#include "habit.h"
#include "logbuch.h"
#include "speicher.h"

static Habit s_habits[HABITS_MAX];     //< nach Platz; id 0 = frei
static uint8_t s_folge[HABITS_MAX];    //< Plaetze nach id
static int s_anzahl;

void habit_vorgabe(Habit *h) {
  memset(h, 0, sizeof(*h));
  h->type = HABIT_GOOD;
  h->insistence = INSIST_NONE;
  h->period_type = PERIOD_FREE;
  h->period_unit = UNIT_DAYS;
  h->period_count = 1;
  h->days = 0x7f;
  h->times_n = 1;
  h->times[0] = 9 * 60;
}

void habit_bereinigen(Habit *h) {
  h->name[HABIT_NAME_LEN - 1] = 0;
  if (!h->name[0]) strncpy(h->name, "Habit", HABIT_NAME_LEN - 1);
  if (h->type >= HABIT_TYPE_COUNT) h->type = HABIT_GOOD;
  if (h->insistence >= INSIST_COUNT) h->insistence = INSIST_NONE;
  if (h->period_type >= PERIOD_COUNT) h->period_type = PERIOD_FREE;
  if (h->period_unit >= UNIT_COUNT) h->period_unit = UNIT_DAYS;
  // Stunden-Bloecke liegen im Tag: mehr als 24 Stunden waeren ein Tag.
  const int max = h->period_unit == UNIT_HOURS ? 24 : HABIT_ANZAHL_MAX;
  if (h->period_count < 1) h->period_count = 1;
  if (h->period_count > max) h->period_count = max;
  h->days &= 0x7f;
  if (!h->days) h->days = 0x7f;
  habit_zeiten_ordnen(h);
  if (h->line > HABIT_LINE_MAX) h->line = HABIT_LINE_MAX;
}

// Zeiten aufsteigend und ohne doppelte: die Planung nimmt je zwei
// aufeinanderfolgende als Zeit und naechste Zeit.
void habit_zeiten_ordnen(Habit *h) {
  if (h->times_n > HABIT_ZEITEN_MAX) h->times_n = HABIT_ZEITEN_MAX;
  int n = 0;
  for (int i = 0; i < h->times_n; i++) {
    const uint16_t z = h->times[i] % 1440;
    int k = n;
    while (k > 0 && h->times[k - 1] > z) k--;
    if (k > 0 && h->times[k - 1] == z) continue;
    memmove(&h->times[k + 1], &h->times[k], (size_t)(n - k) * sizeof(h->times[0]));
    h->times[k] = z;
    n++;
  }
  for (int i = n; i < HABIT_ZEITEN_MAX; i++) h->times[i] = 0;
  h->times_n = (uint8_t)n;
  if (!h->times_n) {
    h->times[0] = 9 * 60;
    h->times_n = 1;
  }
}

static void prv_folge(void) {
  s_anzahl = 0;
  for (int p = 0; p < HABITS_MAX; p++) {
    if (!s_habits[p].id) continue;
    int k = s_anzahl;
    while (k > 0 && s_habits[s_folge[k - 1]].id > s_habits[p].id) {
      s_folge[k] = s_folge[k - 1];
      k--;
    }
    s_folge[k] = (uint8_t)p;
    s_anzahl++;
  }
}

void habits_laden(void) {
  memset(s_habits, 0, sizeof(s_habits));
  for (int p = 0; p < HABITS_MAX; p++) {
    if (!persist_exists(FACH_HABIT(p))) continue;
    Habit h;
    memset(&h, 0, sizeof(h));
    // Ein kuerzeres Fach (aeltere Fassung) laesst die neuen Felder auf 0.
    if (persist_read_data(FACH_HABIT(p), &h, sizeof(h)) <= 0 || !h.id) continue;
    habit_bereinigen(&h);
    s_habits[p] = h;
  }
  prv_folge();
}

int habits_anzahl(void) {
  return s_anzahl;
}

const Habit *habits_nr(int i) {
  return (i >= 0 && i < s_anzahl) ? &s_habits[s_folge[i]] : NULL;
}

int habits_platz(uint16_t id) {
  if (!id) return -1;
  for (int p = 0; p < HABITS_MAX; p++) {
    if (s_habits[p].id == id) return p;
  }
  return -1;
}

const Habit *habits_mit_id(uint16_t id) {
  const int p = habits_platz(id);
  return p >= 0 ? &s_habits[p] : NULL;
}

uint16_t habits_speichern(const Habit *neu, time_t jetzt) {
  Habit h = *neu;
  habit_bereinigen(&h);
  int p = habits_platz(h.id);
  if (p < 0) {
    for (p = 0; p < HABITS_MAX && s_habits[p].id; p++) {}
    if (p >= HABITS_MAX) return 0;
    int32_t id = persist_exists(FACH_NAECHSTE_ID) ? persist_read_int(FACH_NAECHSTE_ID) : 1;
    if (id < 1 || id > 0xffff) id = 1;
    // Eine id, die noch vergeben ist, nicht ein zweites Mal (nach einem
    // Ueberlauf oder einem verlorenen Zaehler).
    while (habits_platz((uint16_t)id) >= 0) id = id % 0xffff + 1;
    h.id = (uint16_t)id;
    h.created = (int32_t)jetzt;
    persist_write_int(FACH_NAECHSTE_ID, id % 0xffff + 1);
    // Ein Platz, auf dem frueher eine geloeschte lag, beginnt leer.
    logbuch_loeschen(p);
  }
  s_habits[p] = h;
  persist_write_data(FACH_HABIT(p), &h, sizeof(h));
  prv_folge();
  return h.id;
}

void habits_loeschen(uint16_t id) {
  const int p = habits_platz(id);
  if (p < 0) return;
  persist_delete(FACH_HABIT(p));
  logbuch_loeschen(p);
  memset(&s_habits[p], 0, sizeof(s_habits[p]));
  prv_folge();
}
