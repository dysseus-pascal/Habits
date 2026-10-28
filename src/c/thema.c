#include "thema.h"
#include "einstellungen.h"

static const Farben s_hell = {
  .grund = GColorWhite,
  .text = GColorBlack,
  .leise = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack),
  .mark_grund = PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorBlack),
  .mark_text = GColorWhite,
  .gut = PBL_IF_COLOR_ELSE(GColorIslamicGreen, GColorBlack),
  .schlecht = PBL_IF_COLOR_ELSE(GColorRed, GColorBlack),
  .linie = PBL_IF_COLOR_ELSE(GColorOrange, GColorBlack),
  .balken_leer = PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack),
};

static const Farben s_dunkel = {
  .grund = GColorBlack,
  .text = GColorWhite,
  .leise = PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite),
  .mark_grund = PBL_IF_COLOR_ELSE(GColorVividCerulean, GColorWhite),
  .mark_text = GColorBlack,
  .gut = PBL_IF_COLOR_ELSE(GColorMalachite, GColorWhite),
  .schlecht = PBL_IF_COLOR_ELSE(GColorFolly, GColorWhite),
  .linie = PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite),
  .balken_leer = PBL_IF_COLOR_ELSE(GColorDarkGray, GColorWhite),
};

const Farben *farben(void) {
  return einstellungen()->thema == THEMA_DUNKEL ? &s_dunkel : &s_hell;
}

void thema_menue(MenuLayer *menue) {
  const Farben *f = farben();
  menu_layer_set_normal_colors(menue, f->grund, f->text);
  menu_layer_set_highlight_colors(menue, f->mark_grund, f->mark_text);
}

void thema_leiste(StatusBarLayer *leiste) {
  const Farben *f = farben();
  status_bar_layer_set_colors(leiste, f->grund, f->text);
  status_bar_layer_set_separator_mode(leiste, StatusBarLayerSeparatorModeNone);
}
