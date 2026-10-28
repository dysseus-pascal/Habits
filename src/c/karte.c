#include "karte.h"
#include "einstellungen.h"
#include "thema.h"

static void prv_zeichen(GContext *ctx, GPoint m, int16_t r, uint8_t zeichen, GColor farbe) {
  graphics_context_set_stroke_color(ctx, farbe);
  graphics_context_set_fill_color(ctx, farbe);
  graphics_context_set_stroke_width(ctx, 3);
  switch (zeichen) {
    case ZEICHEN_HAKEN:
      graphics_draw_line(ctx, GPoint(m.x - r / 2, m.y), GPoint(m.x - r / 8, m.y + r * 2 / 5));
      graphics_draw_line(ctx, GPoint(m.x - r / 8, m.y + r * 2 / 5), GPoint(m.x + r / 2, m.y - r * 2 / 5));
      break;
    case ZEICHEN_ACHTUNG:
      graphics_fill_rect(ctx, GRect(m.x - 2, m.y - r / 2, 5, r * 3 / 5), 1, GCornersAll);
      graphics_fill_rect(ctx, GRect(m.x - 2, m.y + r / 3, 5, 5), 1, GCornersAll);
      break;
    case ZEICHEN_GLOCKE:
      graphics_fill_circle(ctx, GPoint(m.x, m.y - 1), r / 2 - 1);
      graphics_fill_rect(ctx, GRect(m.x - r / 2 + 1, m.y - 1, r - 1, r / 3), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(m.x - r * 3 / 5, m.y + r / 3 - 1, r * 6 / 5 + 1, 3), 1, GCornersAll);
      graphics_fill_circle(ctx, GPoint(m.x, m.y + r / 2 + 1), 3);
      break;
    case ZEICHEN_FRAGE:
      graphics_draw_text(ctx, "?", fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD),
                         GRect(m.x - 15, m.y - 20, 30, 34), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
      break;
  }
  graphics_context_set_stroke_width(ctx, 1);
}

static void prv_zeichnen(Layer *ebene, GContext *ctx) {
  Karte *k = *(Karte **)layer_get_data(ebene);
  GRect b = layer_get_bounds(ebene);
  if (k->leiste) b.size.w -= ACTION_BAR_WIDTH;
  const Farben *f = farben();
  const bool breit = b.size.w >= 160;
  const int16_t r = breit ? 24 : 18;
  const int16_t rand = PBL_IF_ROUND_ELSE(26, 6);
  int16_t y = PBL_IF_ROUND_ELSE(30, breit ? 14 : 8);
  if (k->zeichen) {
    const GPoint m = GPoint(b.size.w / 2 + PBL_IF_ROUND_ELSE(k->leiste ? 10 : 0, 0), y + r);
    graphics_context_set_fill_color(ctx, k->farbe);
    graphics_fill_circle(ctx, m, r);
    graphics_context_set_text_color(ctx, f->grund);
    prv_zeichen(ctx, m, r, k->zeichen, f->grund);
    y += 2 * r + 4;
  }
  const GRect spalte = GRect(rand + PBL_IF_ROUND_ELSE(k->leiste ? 10 : 0, 0), y, b.size.w - 2 * rand, b.size.h - y);
  graphics_context_set_text_color(ctx, f->leise);
  graphics_draw_text(ctx, k->oben, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(spalte.origin.x, y, spalte.size.w, 22),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  y += k->oben[0] ? 20 : 0;
  const GFont gross = fonts_get_system_font(breit ? FONT_KEY_GOTHIC_28_BOLD : FONT_KEY_GOTHIC_24_BOLD);
  const GRect titel = GRect(spalte.origin.x, y, spalte.size.w, breit ? 64 : 56);
  const GSize g = graphics_text_layout_get_content_size(k->titel, gross, titel, GTextOverflowModeTrailingEllipsis,
                                                        GTextAlignmentCenter);
  graphics_context_set_text_color(ctx, f->text);
  graphics_draw_text(ctx, k->titel, gross, titel, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  y += g.h + 4;
  graphics_draw_text(ctx, k->text, fonts_get_system_font(breit ? FONT_KEY_GOTHIC_24 : FONT_KEY_GOTHIC_18),
                     GRect(spalte.origin.x, y, spalte.size.w, b.size.h - y), GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
}

static void prv_taste(ClickRecognizerRef r, void *ctx) {
  Karte *k = ctx;
  if (k->zeitgeber) {
    app_timer_cancel(k->zeitgeber);
    k->zeitgeber = NULL;
  }
  k->taste(k, click_recognizer_get_button_id(r));
}

static void prv_klicks(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_UP, prv_taste);
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_taste);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_taste);
  window_single_click_subscribe(BUTTON_ID_BACK, prv_taste);
}

static void prv_laden(Window *fenster) {
  Karte *k = window_get_user_data(fenster);
  Layer *wurzel = window_get_root_layer(fenster);
  window_set_background_color(fenster, farben()->grund);
  k->flaeche = layer_create_with_data(layer_get_bounds(wurzel), sizeof(Karte *));
  *(Karte **)layer_get_data(k->flaeche) = k;
  layer_set_update_proc(k->flaeche, prv_zeichnen);
  layer_add_child(wurzel, k->flaeche);
  bool leiste = false;
  for (int i = 0; i < NUM_BUTTONS; i++) leiste |= k->symbole[i] != NULL;
  if (leiste) {
    k->leiste = action_bar_layer_create();
    // Die Symbole sind weiss: die Leiste bleibt dunkel, auch im dunklen Thema.
    action_bar_layer_set_background_color(
        k->leiste, PBL_IF_COLOR_ELSE(einstellungen()->thema == THEMA_DUNKEL ? GColorDarkGray : GColorBlack,
                                     GColorBlack));
    for (int i = BUTTON_ID_UP; i <= BUTTON_ID_DOWN; i++) {
      if (k->symbole[i]) action_bar_layer_set_icon(k->leiste, (ButtonId)i, k->symbole[i]);
    }
    action_bar_layer_set_context(k->leiste, k);
    action_bar_layer_set_click_config_provider(k->leiste, prv_klicks);
    action_bar_layer_add_to_window(k->leiste, fenster);
  } else {
    window_set_click_config_provider_with_context(fenster, prv_klicks, k);
  }
}

static void prv_entladen(Window *fenster) {
  Karte *k = window_get_user_data(fenster);
  if (k->zeitgeber) app_timer_cancel(k->zeitgeber);
  if (k->leiste) action_bar_layer_destroy(k->leiste);
  for (int i = 0; i < NUM_BUTTONS; i++) {
    if (k->symbole[i]) gbitmap_destroy(k->symbole[i]);
  }
  layer_destroy(k->flaeche);
  window_destroy(fenster);
  free(k);
}

Karte *karte_zeigen(const uint32_t symbole[3], KarteTaste taste) {
  Karte *k = calloc(1, sizeof(Karte));
  if (!k) return NULL;
  k->taste = taste;
  k->farbe = farben()->gut;
  for (int i = 0; i < 3; i++) {
    if (symbole && symbole[i]) k->symbole[BUTTON_ID_UP + i] = gbitmap_create_with_resource(symbole[i]);
  }
  k->fenster = window_create();
  window_set_user_data(k->fenster, k);
  window_set_window_handlers(k->fenster, (WindowHandlers){.load = prv_laden, .unload = prv_entladen});
  window_stack_push(k->fenster, true);
  return k;
}

void karte_neu_zeichnen(Karte *k) {
  layer_mark_dirty(k->flaeche);
}

void karte_schliessen(Karte *k) {
  window_stack_remove(k->fenster, true);
}

static void prv_abgelaufen(void *ctx) {
  Karte *k = ctx;
  k->zeitgeber = NULL;
  karte_schliessen(k);
}

void karte_zeitgeber(Karte *k, uint32_t ms) {
  if (k->zeitgeber) app_timer_cancel(k->zeitgeber);
  k->zeitgeber = app_timer_register(ms, prv_abgelaufen, k);
}
