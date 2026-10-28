#include "eingabe.h"
#include "liste.h"
#include "thema.h"

// Der Name einer Gewohnheit. Die Anfrage verlangt die einfachste verlaessliche
// Eingabe (Abschnitt 7): das sind hier Vorschlaege fuer die haeufigen
// Gewohnheiten und, fuer alles andere, Buchstabe fuer Buchstabe. Diktat geht
// nur mit verbundenem Telefon, darum steht es dazu, nicht an seiner Stelle.

static const char *const s_gute[] = {"Water", "Walk", "Workout", "Read", "Meditate", "Stretch",
                                     "Vitamins", "Journal", "Floss", "Sleep early", "Fruit & veg",
                                     "Practice"};
static const char *const s_schlechte[] = {"Smoking", "Alcohol", "Snacks", "Sweets", "Soda", "Coffee",
                                          "Social media", "Fast food", "Late night"};
#define GUTE (int)(sizeof(s_gute) / sizeof(s_gute[0]))
#define SCHLECHTE (int)(sizeof(s_schlechte) / sizeof(s_schlechte[0]))

#if defined(PBL_MICROPHONE)
#define DIKTAT 1
#else
#define DIKTAT 0
#endif

typedef struct {
  Habit *h;
  bool neu;
  Liste *liste;
#if DIKTAT
  DictationSession *diktat;
#endif
} Name;

static void prv_setzen(Habit *h, const char *text) {
  // Hoechstens HABIT_NAME_LEN - 1 Byte, und nie mitten in einem UTF-8-Zeichen
  // abgeschnitten: ein halbes Zeichen zeichnet die Uhr als Kasten.
  size_t n = strlen(text);
  if (n > HABIT_NAME_LEN - 1) {
    n = HABIT_NAME_LEN - 1;
    while (n > 0 && ((uint8_t)text[n] & 0xc0) == 0x80) n--;
  }
  memcpy(h->name, text, n);
  h->name[n] = 0;
}

// --- Buchstabe fuer Buchstabe ---

// Ein Rad zum Drehen mit Oben und Unten; Mitte nimmt das Zeichen. Vorn
// stehen Fertig, Loeschen und Leerzeichen, dann die Buchstaben, die nach
// einem Leerzeichen gross und sonst klein kommen.
static const char s_zeichen[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-.'&";
#define SONDER 3
#define RAD (SONDER + (int)sizeof(s_zeichen) - 1)

typedef struct {
  Window *fenster;
  Layer *flaeche;
  Habit *h;
  Liste *namen; //< die Namensliste darunter; sie geht mit
  char text[HABIT_NAME_LEN];
  int pos;
} Buchstaben;

static bool prv_gross(const Buchstaben *b) {
  const size_t n = strlen(b->text);
  return !n || b->text[n - 1] == ' ';
}

static void prv_eintrag(const Buchstaben *b, int i, char *puffer, size_t groesse) {
  i = (i % RAD + RAD) % RAD;
  static const char *const sonder[SONDER] = {"Done", "Delete", "Space"};
  if (i < SONDER) {
    strncpy(puffer, sonder[i], groesse - 1);
    puffer[groesse - 1] = 0;
    return;
  }
  char c = s_zeichen[i - SONDER];
  if (!prv_gross(b) && c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
  puffer[0] = c;
  puffer[1] = 0;
}

static void prv_rad_zeichnen(Layer *ebene, GContext *ctx) {
  Buchstaben *b = *(Buchstaben **)layer_get_data(ebene);
  const GRect r = layer_get_bounds(ebene);
  const Farben *f = farben();
  // Auf der kleinen Uhr (flint, 168 Punkte hoch) ein engeres Rad.
  const bool klein = r.size.h < 200;
  const GFont mittel = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  const GFont gross = fonts_get_system_font(klein ? FONT_KEY_GOTHIC_24_BOLD : FONT_KEY_GOTHIC_28_BOLD);
  const int16_t rand = PBL_IF_ROUND_ELSE(56, 6);

  // Der Name bisher, mit Schreibmarke; ist er zu lang, sein Ende.
  char zeile[HABIT_NAME_LEN + 2];
  snprintf(zeile, sizeof(zeile), "%s_", b->text);
  const GRect feld = GRect(rand, PBL_IF_ROUND_ELSE(34, 2), r.size.w - 2 * rand, 30);
  graphics_context_set_stroke_color(ctx, f->leise);
  graphics_draw_line(ctx, GPoint(feld.origin.x, feld.origin.y + 31),
                     GPoint(feld.origin.x + feld.size.w, feld.origin.y + 31));
  const char *zeigen = zeile;
  while (*zeigen && graphics_text_layout_get_content_size(zeigen, mittel, GRect(0, 0, 1000, 30),
                                                          GTextOverflowModeFill, GTextAlignmentLeft).w >
                        feld.size.w) {
    zeigen++;
    while (((uint8_t)*zeigen & 0xc0) == 0x80) zeigen++;
  }
  graphics_context_set_text_color(ctx, f->text);
  graphics_draw_text(ctx, zeigen, mittel, feld, GTextOverflowModeFill, GTextAlignmentLeft, NULL);

  // Das Rad: fuenf Eintraege, der mittlere gewaehlt.
  const int16_t hinweis = PBL_IF_ROUND_ELSE(40, 18);
  const int16_t mitte = feld.origin.y + 34 + (r.size.h - feld.origin.y - 34 - hinweis) / 2;
  const int16_t schritt = klein ? 22 : 30;
  const int16_t kasten_h = klein ? 24 : 32;
  for (int d = -2; d <= 2; d++) {
    char text[8];
    prv_eintrag(b, b->pos + d, text, sizeof(text));
    const GRect kasten = GRect(r.size.w / 2 - 44, mitte - kasten_h / 2 + d * schritt, 88, kasten_h);
    if (!d) {
      graphics_context_set_fill_color(ctx, f->mark_grund);
      graphics_fill_rect(ctx, kasten, 4, GCornersAll);
    }
    graphics_context_set_text_color(ctx, d ? f->leise : f->mark_text);
    const bool wort = strlen(text) > 1;
    graphics_draw_text(ctx, text, wort ? mittel : gross,
                       GRect(kasten.origin.x, kasten.origin.y + (klein ? -5 : (wort ? 0 : -4)), kasten.size.w, 34),
                       GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }
  graphics_context_set_text_color(ctx, f->leise);
  graphics_draw_text(ctx, "Hold select to save", fonts_get_system_font(FONT_KEY_GOTHIC_14),
                     GRect(0, r.size.h - hinweis, r.size.w, 18), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

static void prv_rad(ClickRecognizerRef r, void *ctx) {
  Buchstaben *b = ctx;
  b->pos = (b->pos + (click_recognizer_get_button_id(r) == BUTTON_ID_UP ? -1 : 1) + RAD) % RAD;
  layer_mark_dirty(b->flaeche);
}

static void prv_fertig(Buchstaben *b) {
  if (b->text[0]) prv_setzen(b->h, b->text);
  Liste *namen = b->namen;
  window_stack_remove(b->fenster, false);
  if (namen) liste_schliessen(namen);
}

static void prv_nehmen(ClickRecognizerRef r, void *ctx) {
  Buchstaben *b = ctx;
  const size_t n = strlen(b->text);
  if (b->pos == 0) {
    prv_fertig(b);
    return;
  }
  if (b->pos == 1) {
    // Loeschen: ein ganzes Zeichen, auch wenn es mehrere Byte hat.
    size_t k = n;
    while (k > 0 && ((uint8_t)b->text[k - 1] & 0xc0) == 0x80) k--;
    if (k > 0) k--;
    b->text[k] = 0;
  } else if (n + 1 < sizeof(b->text)) {
    char text[8];
    prv_eintrag(b, b->pos, text, sizeof(text));
    b->text[n] = b->pos == 2 ? ' ' : text[0];
    b->text[n + 1] = 0;
  }
  layer_mark_dirty(b->flaeche);
}

static void prv_lang(ClickRecognizerRef r, void *ctx) {
  prv_fertig(ctx);
}

static void prv_rad_klicks(void *ctx) {
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 80, prv_rad);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 80, prv_rad);
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_nehmen);
  window_long_click_subscribe(BUTTON_ID_SELECT, 600, prv_lang, NULL);
}

static void prv_rad_laden(Window *fenster) {
  Buchstaben *b = window_get_user_data(fenster);
  Layer *wurzel = window_get_root_layer(fenster);
  window_set_background_color(fenster, farben()->grund);
  b->flaeche = layer_create_with_data(layer_get_bounds(wurzel), sizeof(Buchstaben *));
  *(Buchstaben **)layer_get_data(b->flaeche) = b;
  layer_set_update_proc(b->flaeche, prv_rad_zeichnen);
  layer_add_child(wurzel, b->flaeche);
}

static void prv_rad_entladen(Window *fenster) {
  Buchstaben *b = window_get_user_data(fenster);
  layer_destroy(b->flaeche);
  window_destroy(fenster);
  free(b);
}

static void prv_buchstaben(Habit *h, Liste *namen) {
  Buchstaben *b = calloc(1, sizeof(Buchstaben));
  if (!b) return;
  b->h = h;
  b->namen = namen;
  b->pos = SONDER;
  b->fenster = window_create();
  window_set_user_data(b->fenster, b);
  window_set_click_config_provider_with_context(b->fenster, prv_rad_klicks, b);
  window_set_window_handlers(b->fenster, (WindowHandlers){.load = prv_rad_laden, .unload = prv_rad_entladen});
  window_stack_push(b->fenster, true);
}

// --- Diktat ---

#if DIKTAT
static void prv_diktiert(DictationSession *s, DictationSessionStatus status, char *text, void *ctx) {
  Name *n = ctx;
  if (status == DictationSessionStatusSuccess && text && text[0]) {
    prv_setzen(n->h, text);
    liste_schliessen(n->liste);
  }
}
#endif

// --- Die Liste ---

static uint16_t prv_anzahl(Liste *l) {
  return 1 + DIKTAT + GUTE + SCHLECHTE;
}

static void prv_zeile(Liste *l, uint16_t i, Zeile *z) {
  Name *n = l->daten;
  if (i == 0) {
    strncpy(z->titel, "Type a name", sizeof(z->titel) - 1);
    strncpy(z->untertitel, n->h->name, sizeof(z->untertitel) - 1);
    return;
  }
  if (DIKTAT && i == 1) {
    strncpy(z->titel, "Dictate", sizeof(z->titel) - 1);
    strncpy(z->untertitel, "Needs the phone", sizeof(z->untertitel) - 1);
    return;
  }
  i -= 1 + DIKTAT;
  const bool gut = i < GUTE;
  strncpy(z->titel, gut ? s_gute[i] : s_schlechte[i - GUTE], sizeof(z->titel) - 1);
  z->marke = gut ? 1 : 2;
}

static void prv_waehlen(Liste *l, uint16_t i) {
  Name *n = l->daten;
  if (i == 0) {
    prv_buchstaben(n->h, l);
    return;
  }
#if DIKTAT
  if (i == 1) {
    if (!n->diktat) n->diktat = dictation_session_create(64, prv_diktiert, n);
    if (n->diktat) {
      dictation_session_enable_confirmation(n->diktat, true);
      dictation_session_start(n->diktat);
    } else {
      vibes_double_pulse();
    }
    return;
  }
#endif
  i -= 1 + DIKTAT;
  const bool gut = i < GUTE;
  prv_setzen(n->h, gut ? s_gute[i] : s_schlechte[i - GUTE]);
  if (n->neu) n->h->type = gut ? HABIT_GOOD : HABIT_BAD;
  liste_schliessen(l);
}

static void prv_schluss(Liste *l) {
#if DIKTAT
  Name *n = l->daten;
  if (n->diktat) dictation_session_destroy(n->diktat);
#endif
  free(l->daten);
}

static const ListeArt s_art = {
  .anzahl = prv_anzahl,
  .zeile = prv_zeile,
  .waehlen = prv_waehlen,
  .schluss = prv_schluss,
};

void name_zeigen(Habit *h, bool neu) {
  Name *n = calloc(1, sizeof(Name));
  if (!n) return;
  n->h = h;
  n->neu = neu;
  n->liste = liste_zeigen(&s_art, "NAME", n);
}
