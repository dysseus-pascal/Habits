// Attrappe fuer die Host-Tests: Log, Sprache, Persist, Wecker.
#include <pebble.h>
#include <stdarg.h>

time_t stub_jetzt;
const char *attrappe_sprache = "en_GB";

const char *i18n_get_system_locale(void) {
  return attrappe_sprache;
}

void attrappe_log(int level, const char *fmt, ...) {
  (void)level;
  (void)fmt;
}

// --- Persist ---
#define FAECHER 512
static struct {
  bool da;
  uint8_t daten[PERSIST_DATA_MAX_LENGTH];
  size_t laenge;
} s_persist[FAECHER];
static int s_schreiben;

bool persist_exists(uint32_t key) {
  return key < FAECHER && s_persist[key].da;
}

int persist_read_data(uint32_t key, void *buf, size_t size) {
  if (!persist_exists(key)) return -1;
  const size_t n = s_persist[key].laenge < size ? s_persist[key].laenge : size;
  memcpy(buf, s_persist[key].daten, n);
  return (int)n;
}

int32_t persist_read_int(uint32_t key) {
  int32_t v = 0;
  if (persist_exists(key)) memcpy(&v, s_persist[key].daten, sizeof(v));
  return v;
}

int persist_write_data(uint32_t key, const void *data, size_t size) {
  if (key >= FAECHER || size > PERSIST_DATA_MAX_LENGTH) {
    fprintf(stderr, "persist_write_data: Fach %u mit %u Byte geht auf der Uhr nicht\n",
            (unsigned)key, (unsigned)size);
    abort();
  }
  s_persist[key].da = true;
  s_persist[key].laenge = size;
  memcpy(s_persist[key].daten, data, size);
  s_schreiben++;
  return (int)size;
}

int persist_write_int(uint32_t key, int32_t value) {
  return persist_write_data(key, &value, sizeof(value));
}

int persist_delete(uint32_t key) {
  if (key < FAECHER) s_persist[key].da = false;
  return 0;
}

void attrappe_persist_leeren(void) {
  memset(s_persist, 0, sizeof(s_persist));
  s_schreiben = 0;
}

int attrappe_schreibvorgaenge(void) {
  return s_schreiben;
}

// --- Wecker ---
#define WECKER_MAX 8
static struct {
  time_t t;
  int32_t cookie;
} s_wecker[WECKER_MAX];
static int s_wecker_n;
static time_t s_fremd[4];
static int s_fremd_n;

static bool prv_zu_nah(time_t t, time_t u) {
  return (t > u ? t - u : u - t) < 60;
}

WakeupId wakeup_schedule(time_t t, int32_t cookie, bool notify) {
  (void)notify;
  if (t <= stub_jetzt) return E_RANGE;
  if (s_wecker_n >= WECKER_MAX) return E_OUT_OF_RESOURCES;
  for (int i = 0; i < s_wecker_n; i++) {
    if (prv_zu_nah(t, s_wecker[i].t)) return E_RANGE;
  }
  for (int i = 0; i < s_fremd_n; i++) {
    if (prv_zu_nah(t, s_fremd[i])) return E_RANGE;
  }
  s_wecker[s_wecker_n].t = t;
  s_wecker[s_wecker_n].cookie = cookie;
  return ++s_wecker_n;
}

void wakeup_cancel_all(void) {
  s_wecker_n = 0;
}

int attrappe_wecker_anzahl(void) {
  return s_wecker_n;
}

time_t attrappe_wecker_zeit(int nummer) {
  return s_wecker[nummer].t;
}

int32_t attrappe_wecker_cookie(int nummer) {
  return s_wecker[nummer].cookie;
}

void attrappe_fremder_wecker(time_t t) {
  if (s_fremd_n < 4) s_fremd[s_fremd_n++] = t;
}
