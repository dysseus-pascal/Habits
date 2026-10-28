#include "eingabe.h"
#include "eintragen.h"
#include "thema.h"

// Felder nebeneinander, das gewaehlte hervorgehoben, darueber und darunter
// ein Pfeil fuer Oben und Unten.

#define FELDER_MAX 3

typedef struct {
  FeldFertig fertig;
  void *daten;
  const char *null_text;
} Zahl;

typedef struct {
  FeldFertig fertig;
  void *daten;
} Uhr;

typedef struct {
  Window *fenster;
  Layer *flaeche;
  StatusBarLayer *leiste;
  char titel[24];
  Feld felder[FELDER_MAX];
  int werte[FELDER_MAX];
  int anzahl, aktiv;
  const char *trenner;
  FeldText text, vorschau;
  FeldFertig fertig;
  void *daten;
  // Zahl und Uhrzeit legen ihre Daten hier ab: sie leben so lange wie das
  // Fenster und brauchen kein eigenes Freigeben.
  union {
    Zahl zahl;
    Uhr uhr;
  } eigen;
} Feldwahl;

static void prv_pfeil(GContext *ctx, GPoint mitte, bool hoch) {
  const int16_t d = hoch ? -1 : 1;
  const GPoint p[] = {{mitte.x - 7, mitte.y - 3 * d}, {mitte.x + 7, mitte.y - 3 * d}, {mitte.x, mitte.y + 4 * d}};
  GPathInfo info = {.num_points = 3, .points = (GPoint *)p};
  GPath *pfad = gpath_create(&info);
  gpath_draw_filled(ctx, pfad);
  gpath_destroy(pfad);
}

static void prv_zeichnen(Layer *ebene, GContext *ctx) {
  Feldwahl *w = *(Feldwahl **)layer_get_data(ebene);
  const GRect b = layer_get_bounds(ebene);
  const Farben *f = farben();
  const GFont titel = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  const GFont gross = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  const GFont klein = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  graphics_context_set_text_color(ctx, f->text);
  graphics_draw_text(ctx, w->titel, titel, GRect(4, PBL_IF_ROUND_ELSE(14, 4), b.size.w - 8, 30),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  // Breite aller Felder, damit sie zusammen in der Mitte stehen.
  char text[FELDER_MAX][16];
  int16_t breite[FELDER_MAX];
  const char *trenner = w->trenner ? w->trenner : " ";
  const int16_t trenner_w =
      graphics_text_layout_get_content_size(trenner, gross, GRect(0, 0, 60, 40), GTextOverflowModeFill,
                                            GTextAlignmentLeft).w + 4;
  int16_t gesamt = 0;
  for (int i = 0; i < w->anzahl; i++) {
    w->text(w->daten, i, w->werte, text[i], sizeof(text[i]));
    breite[i] = graphics_text_layout_get_content_size(text[i], gross, GRect(0, 0, b.size.w, 40),
                                                      GTextOverflowModeFill, GTextAlignmentLeft).w + 14;
    gesamt += breite[i] + (i ? trenner_w : 0);
  }
  const int16_t y = b.size.h / 2 - 22;
  int16_t x = (b.size.w - gesamt) / 2;
  for (int i = 0; i < w->anzahl; i++) {
    if (i) {
      graphics_context_set_text_color(ctx, f->text);
      graphics_draw_text(ctx, trenner, gross, GRect(x, y, trenner_w, 40), GTextOverflowModeFill,
                         GTextAlignmentCenter, NULL);
      x += trenner_w;
    }
    const GRect kasten = GRect(x, y + 4, breite[i], 34);
    if (i == w->aktiv) {
      graphics_context_set_fill_color(ctx, f->mark_grund);
      graphics_fill_rect(ctx, kasten, 4, GCornersAll);
      graphics_context_set_fill_color(ctx, f->text);
      prv_pfeil(ctx, GPoint(x + breite[i] / 2, y - 6), true);
      prv_pfeil(ctx, GPoint(x + breite[i] / 2, y + 52), false);
    }
    graphics_context_set_text_color(ctx, i == w->aktiv ? f->mark_text : f->text);
    graphics_draw_text(ctx, text[i], gross, GRect(x, y, breite[i], 40), GTextOverflowModeFill,
                       GTextAlignmentCenter, NULL);
    x += breite[i];
  }
  if (w->vorschau) {
    char zeile[32] = "";
    w->vorschau(w->daten, -1, w->werte, zeile, sizeof(zeile));
    graphics_context_set_text_color(ctx, f->leise);
    graphics_draw_text(ctx, zeile, klein, GRect(4, y + 62, b.size.w - 8, 24), GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
  }
}

static void prv_aendern(Feldwahl *w, int richtung) {
  const Feld *f = &w->felder[w->aktiv];
  int v = w->werte[w->aktiv] + richtung * f->schritt;
  // Rundherum: vom groessten Wert geht es zum kleinsten weiter.
  if (v > f->max) v = f->min;
  if (v < f->min) v = f->max - (f->max - f->min) % f->schritt;
  w->werte[w->aktiv] = v;
  layer_mark_dirty(w->flaeche);
}

static void prv_hoch(ClickRecognizerRef r, void *ctx) {
  prv_aendern(ctx, 1);
}

static void prv_runter(ClickRecognizerRef r, void *ctx) {
  prv_aendern(ctx, -1);
}

static void prv_weiter(ClickRecognizerRef r, void *ctx) {
  Feldwahl *w = ctx;
  if (w->aktiv + 1 < w->anzahl) {
    w->aktiv++;
    layer_mark_dirty(w->flaeche);
    return;
  }
  // Erst das Fenster schliessen, dann melden: die Meldung darf ein neues
  // Fenster oeffnen, und beim Schliessen wird w freigegeben - darum mit
  // einer Abschrift.
  Feldwahl kopie = *w;
  if (w->daten == &w->eigen) kopie.daten = &kopie.eigen;
  window_stack_remove(w->fenster, true);
  kopie.fertig(kopie.daten, kopie.werte);
}

static void prv_zurueck(ClickRecognizerRef r, void *ctx) {
  Feldwahl *w = ctx;
  if (w->aktiv > 0) {
    w->aktiv--;
    layer_mark_dirty(w->flaeche);
  } else {
    window_stack_remove(w->fenster, true);
  }
}

static void prv_klicks(void *ctx) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 90, prv_hoch);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 90, prv_runter);
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_weiter);
  window_single_click_subscribe(BUTTON_ID_BACK, prv_zurueck);
}

static void prv_laden(Window *fenster) {
  Feldwahl *w = window_get_user_data(fenster);
  Layer *wurzel = window_get_root_layer(fenster);
  const GRect b = layer_get_bounds(wurzel);
  window_set_background_color(fenster, farben()->grund);
  w->leiste = status_bar_layer_create();
  thema_leiste(w->leiste);
  w->flaeche = layer_create_with_data(
      GRect(0, STATUS_BAR_LAYER_HEIGHT, b.size.w, b.size.h - STATUS_BAR_LAYER_HEIGHT), sizeof(Feldwahl *));
  *(Feldwahl **)layer_get_data(w->flaeche) = w;
  layer_set_update_proc(w->flaeche, prv_zeichnen);
  layer_add_child(wurzel, w->flaeche);
  layer_add_child(wurzel, status_bar_layer_get_layer(w->leiste));
}

static void prv_entladen(Window *fenster) {
  Feldwahl *w = window_get_user_data(fenster);
  layer_destroy(w->flaeche);
  status_bar_layer_destroy(w->leiste);
  window_destroy(fenster);
  free(w);
}

static Feldwahl *prv_neu(const char *titel, const Feld *felder, int anzahl, FeldText text, FeldText vorschau,
                         FeldFertig fertig, void *daten) {
  Feldwahl *w = calloc(1, sizeof(Feldwahl));
  if (!w) return NULL;
  strncpy(w->titel, titel, sizeof(w->titel) - 1);
  w->anzahl = anzahl < FELDER_MAX ? anzahl : FELDER_MAX;
  for (int i = 0; i < w->anzahl; i++) {
    w->felder[i] = felder[i];
    if (w->felder[i].schritt < 1) w->felder[i].schritt = 1;
    w->werte[i] = felder[i].wert;
  }
  w->text = text;
  w->vorschau = vorschau;
  w->fertig = fertig;
  w->daten = daten;
  w->fenster = window_create();
  window_set_user_data(w->fenster, w);
  window_set_click_config_provider_with_context(w->fenster, prv_klicks, w);
  window_set_window_handlers(w->fenster, (WindowHandlers){.load = prv_laden, .unload = prv_entladen});
  return w;
}

void feldwahl_zeigen(const char *titel, const Feld *felder, int anzahl, FeldText text, FeldText vorschau,
                     FeldFertig fertig, void *daten) {
  Feldwahl *w = prv_neu(titel, felder, anzahl, text, vorschau, fertig, daten);
  if (w) window_stack_push(w->fenster, true);
}

// --- Eine Zahl ---

static void prv_zahl_text(void *daten, int feld, const int *werte, char *puffer, size_t groesse) {
  const Zahl *z = daten;
  if (!werte[0] && z->null_text) strncpy(puffer, z->null_text, groesse - 1);
  else snprintf(puffer, groesse, "%d", werte[0]);
}

static void prv_zahl_fertig(void *daten, const int *werte) {
  const Zahl *z = daten;
  z->fertig(z->daten, werte);
}

void zahl_zeigen(const char *titel, int wert, int min, int max, const char *null_text, FeldFertig fertig,
                 void *daten) {
  const Feld feld = {.wert = wert, .min = min, .max = max, .schritt = 1};
  Feldwahl *w = prv_neu(titel, &feld, 1, prv_zahl_text, NULL, prv_zahl_fertig, NULL);
  if (!w) return;
  w->eigen.zahl = (Zahl){.fertig = fertig, .daten = daten, .null_text = null_text};
  w->daten = &w->eigen;
  window_stack_push(w->fenster, true);
}

// --- Eine Uhrzeit ---

static void prv_uhr_text(void *daten, int feld, const int *werte, char *puffer, size_t groesse) {
  snprintf(puffer, groesse, "%02d", werte[feld]);
}

static void prv_uhr_vorschau(void *daten, int feld, const int *werte, char *puffer, size_t groesse) {
  // Mit 12 Stunden steht unter den Feldern, was gemeint ist.
  if (!clock_is_24h_style()) eintragen_uhrzeit(werte[0] * 60 + werte[1], puffer, groesse);
}

static void prv_uhr_fertig(void *daten, const int *werte) {
  const Uhr *u = daten;
  const int minute = werte[0] * 60 + werte[1];
  u->fertig(u->daten, &minute);
}

void uhrzeit_zeigen(const char *titel, int minute, FeldFertig fertig, void *daten) {
  minute = (minute % 1440 + 1440) % 1440;
  const Feld felder[] = {
    {.wert = minute / 60, .min = 0, .max = 23, .schritt = 1},
    {.wert = minute % 60 / 5 * 5, .min = 0, .max = 55, .schritt = 5},
  };
  Feldwahl *w = prv_neu(titel, felder, 2, prv_uhr_text, prv_uhr_vorschau, prv_uhr_fertig, NULL);
  if (!w) return;
  w->eigen.uhr = (Uhr){.fertig = fertig, .daten = daten};
  w->daten = &w->eigen;
  w->trenner = ":";
  window_stack_push(w->fenster, true);
}
