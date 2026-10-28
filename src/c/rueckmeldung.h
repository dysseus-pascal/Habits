#pragma once
#include <pebble.h>
#include "habit.h"

// Vibration, Ton und Licht nach den Einstellungen des Typs (Abschnitt 4.5).
// In der Ruhezeit der Uhr still, ausser `trotz_ruhe` (Critical). Den Ton kann
// die Uhr trotzdem stumm schalten (Einstellung "Ton in der Ruhezeit").
void rueckmeldung(HabitType typ, bool trotz_ruhe);

// Zum Ausprobieren in den Einstellungen, ohne Ruhezeit.
void rueckmeldung_haptik(int haptik);
void rueckmeldung_ton(int ton);

// Hat die Uhr einen Lautsprecher? Ohne ihn zeigen die Einstellungen keinen Ton.
bool rueckmeldung_hat_ton(void);
