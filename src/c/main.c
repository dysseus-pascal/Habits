#include "einstellungen.h"
#include "eintragen.h"
#include "fenster.h"
#include "habit.h"
#include "logbuch.h"
#include "planer.h"
#include "pruef.h"
#include "wecker.h"

// Habits: Gewohnheiten auf der Uhr anlegen, eintragen und ansehen, mit
// Erinnerungen nach Nachdruck (habit_app.md).
//
// DREI WEGE HINEIN:
//   aus dem Menue der Uhr    das Hauptmenue
//   per Quick Launch         gleich die Liste zum Eintragen; danach zurueck
//                            zum Zifferblatt (Abschnitt 4.4)
//   durch einen Wecker       nur, was faellig ist; ist nichts faellig, geht
//                            die App gleich wieder

// Zeigt alles, was jetzt faellig ist. Rueckgabe: wie viele Fenster.
static int prv_faelliges(void) {
  const time_t jetzt = time(NULL);
  const Rahmen r = einstellungen_rahmen();
  const bool ruhe = quiet_time_is_active();
  int gezeigt = bilanz_zeigen(ruhe) ? 1 : 0;
  Logbuch *lb = malloc(sizeof(Logbuch));
  if (!lb) return gezeigt;
  for (int i = 0; i < habits_anzahl(); i++) {
    const Habit *h = habits_nr(i);
    if (h->insistence == INSIST_NONE) continue;
    logbuch_laden(lb, habits_platz(h->id), h->id);
    if (!planer_faellig(h, lb, jetzt, &r)) continue;
    eintragen_erinnert(h->id, jetzt + PLANER_VORGRIFF_S);
    // Critical kommt auch in der Ruhezeit; Heavy bleibt dann ganz still,
    // Light zeigt sich ohne Vibration.
    if (ruhe && h->insistence == INSIST_HEAVY) continue;
    erinnerung_zeigen(h->id, ruhe && h->insistence != INSIST_CRITICAL);
    gezeigt++;
  }
  free(lb);
  return gezeigt;
}

// Die Zeile unter dem Namen im Starter: wie viele heute erledigt sind, wie
// viele schlechte ueber der Grenze. Sie gilt bis Mitternacht; danach steht
// dort nichts, bis die App wieder laeuft.
static void prv_glance(AppGlanceReloadSession *sitzung, size_t grenze, void *ctx) {
  if (grenze < 1) return;
  const time_t jetzt = time(NULL);
  int erledigt = 0, faellig = 0, ueber = 0;
  for (int i = 0; i < habits_anzahl(); i++) {
    const Habit *h = habits_nr(i);
    const Lage l = eintragen_lage(h, jetzt);
    if (h->type == HABIT_BAD) {
      ueber += l.stand == STAND_UEBER;
    } else if (l.stand != STAND_FREI) {
      faellig++;
      erledigt += l.stand == STAND_ERFUELLT;
    }
  }
  if (!faellig && !ueber) return;
  char text[40];
  if (ueber) snprintf(text, sizeof(text), "%d of %d done, %d over", erledigt, faellig, ueber);
  else snprintf(text, sizeof(text), "%d of %d done", erledigt, faellig);
  const AppGlanceSlice zeile = {
    .layout = {.icon = APP_GLANCE_SLICE_DEFAULT_ICON, .subtitle_template_string = text},
    .expiration_time = kalender_zeit(kalender_tag(jetzt) + 1, 0),
  };
  app_glance_add_slice(sitzung, zeile);
}

static void prv_wecker(WakeupId id, int32_t keks) {
  // Die App war offen: die Erinnerung legt sich obenauf.
  prv_faelliges();
  wecker_planen();
}

#ifdef HABITS_DEMO
void demo_anlegen(void);
#endif

int main(void) {
  einstellungen_laden();
  habits_laden();
#ifdef HABITS_DEMO
  demo_anlegen();
#endif
  wakeup_service_subscribe(prv_wecker);
  switch (launch_reason()) {
    case APP_LAUNCH_QUICK_LAUNCH:
      auswahl_zeigen(AUSWAHL_EINTRAGEN);
      break;
    case APP_LAUNCH_WAKEUP:
      // Ohne Fenster beendet sich die App nach dem ersten Ereignis.
      prv_faelliges();
      break;
    default:
      hauptmenue_zeigen();
      break;
  }
  wecker_planen();
  app_event_loop();
  app_glance_reload(prv_glance, NULL);
  return 0;
}
