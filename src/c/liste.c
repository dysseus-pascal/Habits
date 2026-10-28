#include "liste.h"
#include "thema.h"

#define KOPF_H 22
#define ZEILE_H PBL_IF_ROUND_ELSE(56, (PBL_DISPLAY_WIDTH >= 200 ? 52 : 46))
#define ZEILE_EIN_H PBL_IF_ROUND_ELSE(44, (PBL_DISPLAY_WIDTH >= 200 ? 44 : 38))

static uint16_t prv_anzahl(MenuLayer *m, uint16_t abschnitt, void *ctx) {
  Liste *l = ctx;
  return l->art->anzahl(l);
}

static uint16_t prv_abschnitte(MenuLayer *m, void *ctx) {
  return 1;
}

static int16_t prv_kopf_h(MenuLayer *m, uint16_t abschnitt, void *ctx) {
  Liste *l = ctx;
  return l->kopf ? KOPF_H : 0;
}

static void prv_kopf(GContext *ctx, const Layer *zelle, uint16_t abschnitt, void *daten) {
  Liste *l = daten;
  const GRect b = layer_get_bounds(zelle);
  graphics_context_set_text_color(ctx, farben()->leise);
  graphics_draw_text(ctx, l->kopf, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(PBL_IF_ROUND_ELSE(0, 8), 2, b.size.w - PBL_IF_ROUND_ELSE(0, 16), 18),
                     GTextOverflowModeTrailingEllipsis, PBL_IF_ROUND_ELSE(GTextAlignmentCenter, GTextAlignmentLeft),
                     NULL);
}

static int16_t prv_zeile_h(MenuLayer *m, MenuIndex *i, void *ctx) {
  Liste *l = ctx;
  Zeile z = {.haken = -1};
  l->art->zeile(l, i->row, &z);
  return z.untertitel[0] ? ZEILE_H : ZEILE_EIN_H;
}

// Punkt fuer gut, Ring fuer schlecht: auch ohne Farbe zu unterscheiden.
static void prv_marke(GContext *ctx, GPoint p, uint8_t marke, bool markiert) {
  const Farben *f = farben();
  const GColor farbe = markiert ? f->mark_text : (marke == 2 ? f->schlecht : f->gut);
  graphics_context_set_fill_color(ctx, farbe);
  graphics_context_set_stroke_color(ctx, farbe);
  if (marke == 1) {
    graphics_fill_circle(ctx, p, 5);
  } else {
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_circle(ctx, p, 4);
    graphics_context_set_stroke_width(ctx, 1);
  }
}

static void prv_haken(GContext *ctx, GRect r, bool gesetzt, GColor farbe) {
  graphics_context_set_stroke_color(ctx, farbe);
  graphics_draw_round_rect(ctx, r, 2);
  if (!gesetzt) return;
  graphics_context_set_stroke_width(ctx, 3);
  const int16_t x = r.origin.x, y = r.origin.y, w = r.size.w;
  graphics_draw_line(ctx, GPoint(x + 4, y + w / 2), GPoint(x + w * 2 / 5, y + w - 5));
  graphics_draw_line(ctx, GPoint(x + w * 2 / 5, y + w - 5), GPoint(x + w - 4, y + 4));
  graphics_context_set_stroke_width(ctx, 1);
}

void liste_zeile_zeichnen(GContext *ctx, const Layer *zelle, const Zeile *z) {
  const GRect b = layer_get_bounds(zelle);
  const bool markiert = menu_cell_layer_is_highlighted(zelle);
  const Farben *f = farben();
  const GColor text = markiert ? f->mark_text : f->text;
  const GFont gross = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  const GFont klein = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  const bool zwei = z->untertitel[0] != 0;
  const int16_t haken_w = z->haken >= 0 ? 26 : 0;
#if defined(PBL_ROUND)
  const int16_t rand = 14;
  const GTextAlignment ausrichtung = GTextAlignmentCenter;
  int16_t x = rand;
#else
  const int16_t rand = 8;
  const GTextAlignment ausrichtung = GTextAlignmentLeft;
  int16_t x = rand;
  if (z->marke) {
    prv_marke(ctx, GPoint(x + 5, zwei ? 18 : b.size.h / 2), z->marke, markiert);
    x += 16;
  }
#endif
  const int16_t w = b.size.w - x - rand - haken_w;
  const int16_t oben = zwei ? 0 : (b.size.h - 30) / 2;
  graphics_context_set_text_color(ctx, text);
  graphics_draw_text(ctx, z->titel, gross, GRect(x, oben, w, 28), GTextOverflowModeTrailingEllipsis,
                     ausrichtung, NULL);
  if (zwei) {
    graphics_context_set_text_color(ctx, markiert ? f->mark_text : f->leise);
    graphics_draw_text(ctx, z->untertitel, klein, GRect(x, 25, w, 22),
                       GTextOverflowModeTrailingEllipsis, ausrichtung, NULL);
  }
#if defined(PBL_ROUND)
  if (z->marke) {
    // Rund: der Punkt vor dem mittigen Titel.
    const GSize g = graphics_text_layout_get_content_size(z->titel, gross, GRect(0, 0, w, 28),
                                                          GTextOverflowModeTrailingEllipsis, ausrichtung);
    prv_marke(ctx, GPoint(x + (w - g.w) / 2 - 10, oben + 16), z->marke, markiert);
  }
#endif
  if (z->haken >= 0) {
    prv_haken(ctx, GRect(b.size.w - rand - 20, (b.size.h - 18) / 2, 18, 18), z->haken == 1, text);
  }
}

static void prv_zeichnen(GContext *ctx, const Layer *zelle, MenuIndex *i, void *daten) {
  Liste *l = daten;
  Zeile z = {.haken = -1};
  l->art->zeile(l, i->row, &z);
  liste_zeile_zeichnen(ctx, zelle, &z);
}

static void prv_waehlen(MenuLayer *m, MenuIndex *i, void *daten) {
  Liste *l = daten;
  l->art->waehlen(l, i->row);
}

static void prv_lang(MenuLayer *m, MenuIndex *i, void *daten) {
  Liste *l = daten;
  if (l->art->lang) l->art->lang(l, i->row);
}

static void prv_laden(Window *fenster) {
  Liste *l = window_get_user_data(fenster);
  Layer *wurzel = window_get_root_layer(fenster);
  const GRect b = layer_get_bounds(wurzel);
  l->leiste = status_bar_layer_create();
  thema_leiste(l->leiste);
  const GRect platz = GRect(0, STATUS_BAR_LAYER_HEIGHT, b.size.w, b.size.h - STATUS_BAR_LAYER_HEIGHT);
  l->menue = menu_layer_create(platz);
  menu_layer_set_callbacks(l->menue, l, (MenuLayerCallbacks){
    .get_num_sections = prv_abschnitte,
    .get_num_rows = prv_anzahl,
    .get_header_height = prv_kopf_h,
    .draw_header = prv_kopf,
    .get_cell_height = prv_zeile_h,
    .draw_row = prv_zeichnen,
    .select_click = prv_waehlen,
    .select_long_click = prv_lang,
  });
  thema_menue(l->menue);
  window_set_background_color(fenster, farben()->grund);
  menu_layer_set_click_config_onto_window(l->menue, fenster);
  layer_add_child(wurzel, menu_layer_get_layer(l->menue));
  layer_add_child(wurzel, status_bar_layer_get_layer(l->leiste));
}

static void prv_erscheint(Window *fenster) {
  Liste *l = window_get_user_data(fenster);
  // Das Thema kann sich in den Einstellungen geaendert haben.
  thema_menue(l->menue);
  thema_leiste(l->leiste);
  window_set_background_color(fenster, farben()->grund);
  if (l->art->erscheint) l->art->erscheint(l);
  menu_layer_reload_data(l->menue);
}

static void prv_entladen(Window *fenster) {
  Liste *l = window_get_user_data(fenster);
  if (l->art->schluss) l->art->schluss(l);
  menu_layer_destroy(l->menue);
  status_bar_layer_destroy(l->leiste);
  window_destroy(fenster);
  free(l);
}

Liste *liste_zeigen(const ListeArt *art, const char *kopf, void *daten) {
  Liste *l = malloc(sizeof(Liste));
  if (!l) return NULL;
  *l = (Liste){.art = art, .kopf = kopf, .daten = daten};
  l->fenster = window_create();
  window_set_user_data(l->fenster, l);
  window_set_window_handlers(l->fenster, (WindowHandlers){
    .load = prv_laden,
    .appear = prv_erscheint,
    .unload = prv_entladen,
  });
  window_stack_push(l->fenster, true);
  return l;
}

void liste_auffrischen(Liste *l) {
  menu_layer_reload_data(l->menue);
}

void liste_schliessen(Liste *l) {
  window_stack_remove(l->fenster, true);
}

void liste_auswahl_setzen(Liste *l, uint16_t i) {
  menu_layer_set_selected_index(l->menue, (MenuIndex){0, i}, MenuRowAlignCenter, false);
}
