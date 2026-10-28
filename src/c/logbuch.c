#include "logbuch.h"
#include "speicher.h"

#define LOGBUCH_FASSUNG 1
// Der Kopf liegt vorn im Logbuch, bis zu den Tagen.
#define KOPF ((int)offsetof(Logbuch, tage))

_Static_assert(offsetof(Logbuch, tage) <= 256, "Kopf passt nicht in ein Fach");

void logbuch_leeren(Logbuch *lb, uint16_t habit_id) {
  memset(lb, 0, sizeof(*lb));
  lb->habit_id = habit_id;
  lb->fassung = LOGBUCH_FASSUNG;
}

void logbuch_laden(Logbuch *lb, int platz, uint16_t habit_id) {
  logbuch_leeren(lb, habit_id);
  if (persist_read_data(FACH_LOG(platz, 0), lb, KOPF) != KOPF || lb->habit_id != habit_id ||
      lb->fassung != LOGBUCH_FASSUNG) {
    logbuch_leeren(lb, habit_id);
    return;
  }
  for (int k = 0; k < LOGBUCH_FAECHER; k++) {
    persist_read_data(FACH_LOG(platz, k + 1), &lb->tage[k * LOGBUCH_FACH_TAGE], LOGBUCH_FACH_TAGE);
  }
  lb->geaendert = 0;
}

void logbuch_speichern(Logbuch *lb, int platz) {
  if (lb->geaendert & 1) persist_write_data(FACH_LOG(platz, 0), lb, KOPF);
  for (int k = 0; k < LOGBUCH_FAECHER; k++) {
    if (lb->geaendert & (2 << k)) {
      persist_write_data(FACH_LOG(platz, k + 1), &lb->tage[k * LOGBUCH_FACH_TAGE],
                         LOGBUCH_FACH_TAGE);
    }
  }
  lb->geaendert = 0;
}

void logbuch_loeschen(int platz) {
  for (int k = 0; k <= LOGBUCH_FAECHER; k++) persist_delete(FACH_LOG(platz, k));
}

static uint8_t *prv_feld(Logbuch *lb, Tag tag) {
  const int i = (int)(tag % LOGBUCH_TAGE);
  lb->geaendert |= (uint8_t)(2 << (i / LOGBUCH_FACH_TAGE));
  return &lb->tage[i];
}

// Rueckt den Ring bis `tag` vor; die Tage dazwischen beginnen bei 0. Ein Tag,
// der vor dem Ring liegt, hat keinen Platz mehr: false.
static bool prv_ring_bis(Logbuch *lb, Tag tag) {
  if (!lb->letzter_tag || tag - lb->letzter_tag >= LOGBUCH_TAGE) {
    memset(lb->tage, 0, sizeof(lb->tage));
    lb->geaendert |= 0x0e;
    lb->letzter_tag = tag;
    return true;
  }
  for (Tag d = lb->letzter_tag + 1; d <= tag; d++) *prv_feld(lb, d) = 0;
  if (tag > lb->letzter_tag) lb->letzter_tag = tag;
  return tag > lb->letzter_tag - LOGBUCH_TAGE;
}

// Schiebt die Zeilen der Stunden, bis stunden[0] der Tag `tag` ist.
static void prv_stunden_bis(Logbuch *lb, Tag tag) {
  if (!lb->stunden_tag || tag - lb->stunden_tag >= LOGBUCH_STUNDEN_TAGE) {
    memset(lb->stunden, 0, sizeof(lb->stunden));
    lb->stunden_tag = tag;
  } else if (tag > lb->stunden_tag) {
    const int n = (int)(tag - lb->stunden_tag);
    memmove(lb->stunden[n], lb->stunden[0], (size_t)(LOGBUCH_STUNDEN_TAGE - n) * 24);
    memset(lb->stunden[0], 0, (size_t)n * 24);
    lb->stunden_tag = tag;
  }
}

static uint8_t *prv_stunde(Logbuch *lb, time_t t) {
  const int32_t r = lb->stunden_tag - kalender_tag(t);
  if (!lb->stunden_tag || r < 0 || r >= LOGBUCH_STUNDEN_TAGE) return NULL;
  return &lb->stunden[r][kalender_stunde(t)];
}

bool logbuch_eintragen(Logbuch *lb, time_t t) {
  const Tag tag = kalender_tag(t);
  if (tag < KALENDER_TAG_GUELTIG || !prv_ring_bis(lb, tag)) return false;
  uint8_t *z = prv_feld(lb, tag);
  if (*z < 255) (*z)++;
  prv_stunden_bis(lb, tag);
  uint8_t *s = prv_stunde(lb, t);
  if (s && *s < 255) (*s)++;
  lb->vorletzter_log = lb->letzter_log;
  lb->letzter_log = (int32_t)t;
  lb->geaendert |= 1;
  return true;
}

bool logbuch_rueckgaengig(Logbuch *lb) {
  if (!lb->letzter_log) return false;
  const time_t t = lb->letzter_log;
  const Tag tag = kalender_tag(t);
  if (logbuch_tag(lb, tag) > 0) (*prv_feld(lb, tag))--;
  uint8_t *s = prv_stunde(lb, t);
  if (s && *s) (*s)--;
  lb->letzter_log = lb->vorletzter_log;
  lb->vorletzter_log = 0;
  lb->geaendert |= 1;
  return true;
}

void logbuch_pause(Logbuch *lb, time_t bis) {
  lb->pause_bis = (int32_t)bis;
  lb->geaendert |= 1;
}

void logbuch_erinnert(Logbuch *lb, time_t bis) {
  lb->erinnert = (int32_t)bis;
  lb->geaendert |= 1;
}

int logbuch_tag(const Logbuch *lb, Tag tag) {
  if (!lb->letzter_tag || tag < 0 || tag > lb->letzter_tag || tag <= lb->letzter_tag - LOGBUCH_TAGE) {
    return 0;
  }
  return lb->tage[tag % LOGBUCH_TAGE];
}

int logbuch_tage(const Logbuch *lb, Tag von, Tag bis) {
  int summe = 0;
  if (lb->letzter_tag && von <= lb->letzter_tag - LOGBUCH_TAGE) von = lb->letzter_tag - LOGBUCH_TAGE + 1;
  if (bis > lb->letzter_tag + 1) bis = lb->letzter_tag + 1;
  for (Tag d = von; d < bis; d++) summe += logbuch_tag(lb, d);
  return summe;
}

bool logbuch_stunden(const Logbuch *lb, Tag tag, uint8_t stunden[24]) {
  const int32_t r = lb->stunden_tag - tag;
  if (!lb->stunden_tag || r < 0) {
    // Nach dem letzten Eintrag: nichts eingetragen.
    memset(stunden, 0, 24);
    return true;
  }
  if (r >= LOGBUCH_STUNDEN_TAGE) return false;
  memcpy(stunden, lb->stunden[r], 24);
  return true;
}
