#include "wecker.h"
#include "einstellungen.h"
#include "habit.h"
#include "logbuch.h"
#include "planer.h"

// Die Uhr nimmt keinen Wecker naeher als eine Minute an einem anderen, auch
// nicht an dem einer anderen App. Dann etwas spaeter versuchen; die eigenen
// liegen PLANER_ABSTAND_S auseinander, die Versuche bleiben davor.
static bool prv_stellen(const Termin *t) {
  static const int versuche[] = {0, 80, 160};
  const int32_t keks = (int32_t)t->art << 16 | t->habit_id;
  for (unsigned i = 0; i < sizeof(versuche) / sizeof(versuche[0]); i++) {
    const WakeupId id = wakeup_schedule(t->zeit + versuche[i], keks, false);
    if (id >= 0) return true;
    if (id != E_RANGE) break;
  }
  APP_LOG(APP_LOG_LEVEL_WARNING, "Wecker %d nicht gestellt", (int)t->zeit);
  return false;
}

void wecker_planen(void) {
  const time_t jetzt = time(NULL);
  const Rahmen r = einstellungen_rahmen();
  Termin liste[PLANER_WECKER_MAX];
  int n = 0;
  Logbuch *lb = malloc(sizeof(Logbuch));
  if (!lb) return;
  for (int i = 0; i < habits_anzahl(); i++) {
    const Habit *h = habits_nr(i);
    if (h->insistence == INSIST_NONE) continue;
    logbuch_laden(lb, habits_platz(h->id), h->id);
    time_t ab = jetzt + PLANER_VORLAUF_S;
    if (lb->erinnert > ab) ab = lb->erinnert;
    Termin neu[PLANER_WECKER_MAX];
    const int k = planer_termine(h, lb, ab, &r, neu, PLANER_WECKER_MAX);
    planer_einfuegen(liste, &n, PLANER_WECKER_MAX, neu, k);
  }
  free(lb);
  planer_abstand(liste, n);
  wakeup_cancel_all();
  for (int i = 0; i < n; i++) prv_stellen(&liste[i]);
}
