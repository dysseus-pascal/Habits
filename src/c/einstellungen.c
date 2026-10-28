#include "einstellungen.h"
#include "speicher.h"

#define FASSUNG 1

static Einstellungen s_e;

static void prv_vorgabe(Einstellungen *e) {
  *e = (Einstellungen){
    .fassung = FASSUNG,
    .haptik = {HAPTIK_KURZ, HAPTIK_DOPPELT},
    .ton = {TON_AUS, TON_AUS},
    .thema = THEMA_HELL,
    .aktiv_von = 8 * 60,
    .aktiv_bis = 21 * 60,
  };
}

static void prv_bereinigen(Einstellungen *e) {
  e->fassung = FASSUNG;
  for (int i = 0; i < 2; i++) {
    if (e->haptik[i] >= HAPTIK_ANZAHL) e->haptik[i] = HAPTIK_KURZ;
    if (e->ton[i] >= TON_ANZAHL) e->ton[i] = TON_AUS;
  }
  if (e->thema >= THEMA_ANZAHL) e->thema = THEMA_HELL;
  // Die aktiven Stunden liegen in einem Tag: der Planer rechnet sie je Tag.
  if (e->aktiv_von > 23 * 60) e->aktiv_von = 23 * 60;
  if (e->aktiv_bis > 24 * 60) e->aktiv_bis = 24 * 60;
  if (e->aktiv_bis <= e->aktiv_von) e->aktiv_bis = e->aktiv_von + 60;
}

void einstellungen_laden(void) {
  prv_vorgabe(&s_e);
  if (persist_exists(FACH_EINSTELLUNGEN)) {
    persist_read_data(FACH_EINSTELLUNGEN, &s_e, sizeof(s_e));
  }
  prv_bereinigen(&s_e);
}

const Einstellungen *einstellungen(void) {
  return &s_e;
}

void einstellungen_setzen(const Einstellungen *e) {
  s_e = *e;
  prv_bereinigen(&s_e);
  persist_write_data(FACH_EINSTELLUNGEN, &s_e, sizeof(s_e));
}

Rahmen einstellungen_rahmen(void) {
  return (Rahmen){
    .aktiv_von = s_e.aktiv_von,
    .aktiv_bis = s_e.aktiv_bis,
    .erster_wochentag = kalender_erster_wochentag(),
  };
}

const char *einstellungen_haptik_name(int haptik) {
  static const char *const namen[HAPTIK_ANZAHL] = {"None", "Short", "Double", "Long", "Triple",
                                                   "Escalating"};
  return haptik >= 0 && haptik < HAPTIK_ANZAHL ? namen[haptik] : "";
}

const char *einstellungen_ton_name(int ton) {
  static const char *const namen[TON_ANZAHL] = {"Off", "Beep", "Chime", "Rise"};
  return ton >= 0 && ton < TON_ANZAHL ? namen[ton] : "";
}
