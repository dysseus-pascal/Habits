// Nur fuer die Host-Tests in tools/: so viel vom Pebble-SDK, wie die Rechnung
// der App braucht (kalender, logbuch, habit, zeitraum, planer, einstellungen).
// Persist liegt im Speicher, die Uhrzeit stellt der Test, Wecker zaehlt die
// Attrappe mit (attrappe.c). localtime kommt vom Rechner: die Tests laufen mit
// TZ=Europe/Zurich, damit die Umstellung auf Sommerzeit mitgeprueft wird.
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define APP_LOG_LEVEL_ERROR 1
#define APP_LOG_LEVEL_WARNING 2
#define APP_LOG_LEVEL_INFO 3
#define APP_LOG_LEVEL_DEBUG 4
void attrappe_log(int level, const char *fmt, ...);
#define APP_LOG(level, fmt, ...) attrappe_log((level), (fmt), ##__VA_ARGS__)

extern time_t stub_jetzt;
static inline time_t stub_time(time_t *t) {
  if (t) *t = stub_jetzt;
  return stub_jetzt;
}
#define time(t) stub_time(t)

const char *i18n_get_system_locale(void);
extern const char *attrappe_sprache;

// --- Persist: wie auf der Uhr hoechstens 256 Byte je Fach ---
#define PERSIST_DATA_MAX_LENGTH 256
bool persist_exists(uint32_t key);
int persist_read_data(uint32_t key, void *buf, size_t size);
int32_t persist_read_int(uint32_t key);
int persist_write_data(uint32_t key, const void *data, size_t size);
int persist_write_int(uint32_t key, int32_t value);
int persist_delete(uint32_t key);
void attrappe_persist_leeren(void);
int attrappe_schreibvorgaenge(void); //< seit dem Leeren

// --- Wecker: hoechstens acht je App, keiner naeher als eine Minute am anderen ---
typedef int32_t WakeupId;
#define E_RANGE (-8)
#define E_OUT_OF_RESOURCES (-7)
WakeupId wakeup_schedule(time_t t, int32_t cookie, bool notify);
void wakeup_cancel_all(void);
int attrappe_wecker_anzahl(void);
time_t attrappe_wecker_zeit(int nummer);
int32_t attrappe_wecker_cookie(int nummer);
// Ein fremder Wecker (eine andere App), an dem E_RANGE greift.
void attrappe_fremder_wecker(time_t t);
