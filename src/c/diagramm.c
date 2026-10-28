#include "diagramm_daten.h"
#include "fenster.h"
#include "kalender.h"
#include "thema.h"

// Das Diagramm einer Gewohnheit (Abschnitt 4.1). Die Anfrage nennt Links und
// Rechts fuer Tag, Woche, Monat, Jahr; die Uhr hat dafuer Oben und Unten.
// Mitte geht einen Ausschnitt zurueck (gestern, letzte Woche ...), lang
// gedrueckt wieder zu heute.

typedef struct {
  Window *fenster;
  Layer *flaeche;
  StatusBarLayer *leiste;
  uint16_t id;
  Logbuch *lb;
  Ansicht ansicht;
  int zurueck;
  Diagramm d;
} Fenster;

static void prv_rechnen(Fenster *f) {
  const Habit *h = habits_mit_id(f->id);
  if (!h) return;
  diagramm_rechnen(h, f->lb, f->ansicht, f->zurueck, time(NULL), kalender_erster_wochentag(), &f->d);
}

static void prv_zahl(const Diagramm *d, int wert, char *puffer, size_t groesse) {
  if (d->zehntel) snprintf(puffer, groesse, "%d.%d", wert / 10, wert % 10);
  else snprintf(puffer, groesse, "%d", wert);
}

// Gestrichelt, damit sie auch auf Schwarz-Weiss neben schwarzen Balken
// erkennbar bleibt.
static void prv_linie(GContext *ctx, int16_t x0, int16_t x1, int16_t y) {
  for (int16_t x = x0; x < x1; x += 9) {
    const int16_t ende = x + 5 < x1 ? x + 5 : x1;
    graphics_fill_rect(ctx, GRect(x, y - 1, ende - x, 2), 0, GCornerNone);
  }
}

static void prv_zeichnen(Layer *ebene, GContext *ctx) {
  Fenster *f = *(Fenster **)layer_get_data(ebene);
  const Habit *h = habits_mit_id(f->id);
  if (!h) return;
  const Diagramm *d = &f->d;
  const Farben *fa = farben();
  const GRect b = layer_get_bounds(ebene);
  const bool rund = PBL_IF_ROUND_ELSE(true, false);
  // Auf der schmalen Uhr (flint) stehen D W M Y neben dem Namen, sonst fehlte
  // dem Ausschnitt ("October 2026") der Platz.
  const bool schmal = b.size.w < 180;
  // Rund: unten ist der Kreis schmal, Balken und Beschriftung bleiben innen.
  const int16_t rand = rund ? 44 : 6;
  const int16_t breite = b.size.w - 2 * rand;
  int16_t y = rund ? 6 : 0;
  static const char *const ansichten[] = {"D", "W", "M", "Y"};
  const int16_t pille = 15;

  // Kopf: Name, Ausschnitt und D W M Y mit dem gewaehlten hervorgehoben.
  graphics_context_set_text_color(ctx, fa->leise);
  graphics_draw_text(ctx, h->name, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
                     GRect(rand, y, schmal ? breite - 4 * pille - 4 : breite, 22), GTextOverflowModeTrailingEllipsis,
                     rund ? GTextAlignmentCenter : GTextAlignmentLeft, NULL);
  y += 20;
  const int16_t pillen_x = rund ? (b.size.w - 4 * pille) / 2 : b.size.w - rand - 4 * pille;
  const int16_t pillen_y = rund ? y + 26 : (schmal ? y - 17 : y + 6);
  for (int i = 0; i < ANSICHT_ANZAHL; i++) {
    const GRect r = GRect(pillen_x + i * pille, pillen_y, pille - 1, 16);
    if (i == (int)f->ansicht) {
      graphics_context_set_fill_color(ctx, fa->mark_grund);
      graphics_fill_rect(ctx, r, 3, GCornersAll);
    }
    graphics_context_set_text_color(ctx, i == (int)f->ansicht ? fa->mark_text : fa->leise);
    graphics_draw_text(ctx, ansichten[i], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(r.origin.x, r.origin.y - 2, r.size.w, 16), GTextOverflowModeFill, GTextAlignmentCenter,
                       NULL);
  }
  char titel[24];
  diagramm_titel(d, kalender_tag(time(NULL)), titel, sizeof(titel));
  graphics_context_set_text_color(ctx, fa->text);
  graphics_draw_text(ctx, titel, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                     GRect(rand, y - 2, rund || schmal ? breite : breite - 4 * pille - 4, 28), GTextOverflowModeTrailingEllipsis,
                     rund ? GTextAlignmentCenter : GTextAlignmentLeft, NULL);
  y += rund ? 44 : 26;

  // Eine Zeile Summe: wie viel, und je nach Ansicht das Ziel oder der Schnitt.
  char zeile[40];
  const bool schlecht = h->type == HABIT_BAD;
  if (d->ansicht == ANSICHT_TAG) {
    if (d->linie >= 0 && d->laufend) {
      snprintf(zeile, sizeof(zeile), "%d logged, %s %d", (int)d->summe, schlecht ? "max" : "goal", d->linie);
    } else {
      snprintf(zeile, sizeof(zeile), "%d logged", (int)d->summe);
    }
  } else {
    const int schnitt = d->tage ? (int)((d->summe * 10 + d->tage / 2) / d->tage) : 0;
    snprintf(zeile, sizeof(zeile), "%d logged, %d.%d/day", (int)d->summe, schnitt / 10, schnitt % 10);
  }
  graphics_context_set_text_color(ctx, fa->leise);
  graphics_draw_text(ctx, zeile, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(rand, y, breite, 22),
                     GTextOverflowModeTrailingEllipsis, rund ? GTextAlignmentCenter : GTextAlignmentLeft, NULL);
  y += 26;

  // Die Balken. Unten bleibt Platz fuer die Beschriftung.
  const GFont klein = fonts_get_system_font(FONT_KEY_GOTHIC_14);
  const int16_t unten = b.size.h - (rund ? 50 : 18);
  const int16_t hoehe = unten - y - 10;
  if (d->ohne_stunden) {
    snprintf(zeile, sizeof(zeile), "No hourly detail\nany more: %d logged", (int)d->summe);
    graphics_draw_text(ctx, zeile, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(rand, y + hoehe / 3, breite, 44),
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    return;
  }
  int groesste = d->linie > 0 ? d->linie : 0;
  for (int i = 0; i < d->n; i++) {
    if (d->wert[i] > groesste) groesste = d->wert[i];
  }
  if (!groesste) groesste = d->zehntel ? 10 : 1;
  // Etwas Luft ueber dem hoechsten Balken, damit die Linie oben nicht klebt.
  const int oben = groesste + (groesste + 9) / 10;
  const int16_t platz = breite / d->n;
  const int16_t x0 = rand + (breite - platz * d->n) / 2;
  const int16_t luecke = platz >= 6 ? (platz >= 12 ? 3 : 1) : 0;
  for (int i = 0; i < d->n; i++) {
    const int16_t x = x0 + i * platz;
    const bool zukunft = (d->zukunft >> i) & 1;
    const int16_t bh = (int16_t)((int32_t)d->wert[i] * hoehe / oben);
    if (!zukunft && bh > 0) {
      // Schlecht: ueber der Grenze rot, darunter grau; ohne Farbe darunter
      // nur der Umriss.
      const bool ueber = schlecht && d->linie >= 0 && d->wert[i] > d->linie;
      const GRect balken = GRect(x, unten - bh, platz - luecke > 0 ? platz - luecke : 1, bh);
      if (schlecht && !ueber && !PBL_IF_COLOR_ELSE(true, false)) {
        graphics_context_set_stroke_color(ctx, fa->text);
        graphics_draw_rect(ctx, balken);
      } else {
        graphics_context_set_fill_color(ctx, !schlecht ? fa->gut : (ueber ? fa->schlecht : fa->leise));
        graphics_fill_rect(ctx, balken, 0, GCornerNone);
      }
    } else if (zukunft && d->ansicht != ANSICHT_TAG) {
      // Kommende Tage als Strich auf der Grundlinie.
      graphics_context_set_fill_color(ctx, fa->balken_leer);
      graphics_fill_rect(ctx, GRect(x, unten - 2, platz - luecke > 0 ? platz - luecke : 1, 2), 0, GCornerNone);
    }
    char marke[6];
    diagramm_marke(d, i, clock_is_24h_style(), marke, sizeof(marke));
    if (marke[0]) {
      graphics_context_set_text_color(ctx, fa->leise);
      graphics_draw_text(ctx, marke, klein, GRect(x - 8, unten, platz + 16, 16), GTextOverflowModeFill,
                         GTextAlignmentCenter, NULL);
    }
  }
  graphics_context_set_fill_color(ctx, fa->leise);
  graphics_fill_rect(ctx, GRect(x0, unten, platz * d->n, 1), 0, GCornerNone);

  // Ziel oder Grenze als Linie mit ihrem Wert.
  if (d->linie > 0) {
    const int16_t ly = unten - (int16_t)((int32_t)d->linie * hoehe / oben);
    graphics_context_set_fill_color(ctx, fa->linie);
    prv_linie(ctx, x0, x0 + platz * d->n, ly);
    char wert[12];
    prv_zahl(d, d->linie, wert, sizeof(wert));
    graphics_context_set_text_color(ctx, fa->linie);
    graphics_draw_text(ctx, wert, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                       GRect(x0, ly - 17, platz * d->n, 16), GTextOverflowModeFill, GTextAlignmentRight, NULL);
  }
}

static void prv_neu(Fenster *f) {
  prv_rechnen(f);
  layer_mark_dirty(f->flaeche);
}

static void prv_hoch(ClickRecognizerRef r, void *ctx) {
  Fenster *f = ctx;
  f->ansicht = (Ansicht)((f->ansicht + ANSICHT_ANZAHL - 1) % ANSICHT_ANZAHL);
  f->zurueck = 0;
  prv_neu(f);
}

static void prv_runter(ClickRecognizerRef r, void *ctx) {
  Fenster *f = ctx;
  f->ansicht = (Ansicht)((f->ansicht + 1) % ANSICHT_ANZAHL);
  f->zurueck = 0;
  prv_neu(f);
}

static void prv_frueher(ClickRecognizerRef r, void *ctx) {
  Fenster *f = ctx;
  f->zurueck = f->zurueck < diagramm_zurueck_max(f->ansicht) ? f->zurueck + 1 : 0;
  prv_neu(f);
}

static void prv_jetzt(ClickRecognizerRef r, void *ctx) {
  Fenster *f = ctx;
  f->zurueck = 0;
  prv_neu(f);
}

static void prv_klicks(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_UP, prv_hoch);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_runter);
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_frueher);
  window_long_click_subscribe(BUTTON_ID_SELECT, 600, prv_jetzt, NULL);
}

static void prv_laden(Window *fenster) {
  Fenster *f = window_get_user_data(fenster);
  Layer *wurzel = window_get_root_layer(fenster);
  const GRect b = layer_get_bounds(wurzel);
  window_set_background_color(fenster, farben()->grund);
  f->leiste = status_bar_layer_create();
  thema_leiste(f->leiste);
  f->flaeche = layer_create_with_data(GRect(0, STATUS_BAR_LAYER_HEIGHT, b.size.w, b.size.h - STATUS_BAR_LAYER_HEIGHT),
                                      sizeof(Fenster *));
  *(Fenster **)layer_get_data(f->flaeche) = f;
  layer_set_update_proc(f->flaeche, prv_zeichnen);
  layer_add_child(wurzel, f->flaeche);
  layer_add_child(wurzel, status_bar_layer_get_layer(f->leiste));
  prv_rechnen(f);
}

static void prv_entladen(Window *fenster) {
  Fenster *f = window_get_user_data(fenster);
  layer_destroy(f->flaeche);
  status_bar_layer_destroy(f->leiste);
  window_destroy(fenster);
  free(f->lb);
  free(f);
}

void diagramm_zeigen(uint16_t id) {
  const Habit *h = habits_mit_id(id);
  if (!h) return;
  Fenster *f = calloc(1, sizeof(Fenster));
  Logbuch *lb = malloc(sizeof(Logbuch));
  if (!f || !lb) {
    free(f);
    free(lb);
    return;
  }
  logbuch_laden(lb, habits_platz(id), id);
  f->id = id;
  f->lb = lb;
  f->ansicht = ANSICHT_WOCHE;
  f->fenster = window_create();
  window_set_user_data(f->fenster, f);
  window_set_click_config_provider_with_context(f->fenster, prv_klicks, f);
  window_set_window_handlers(f->fenster, (WindowHandlers){.load = prv_laden, .unload = prv_entladen});
  window_stack_push(f->fenster, true);
}
