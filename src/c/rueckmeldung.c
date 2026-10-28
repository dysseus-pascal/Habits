#include "rueckmeldung.h"
#include "einstellungen.h"

void rueckmeldung_haptik(int haptik) {
  static const uint32_t dreifach[] = {120, 100, 120, 100, 120};
  static const uint32_t steigend[] = {60, 200, 120, 200, 250, 200, 450};
  switch (haptik) {
    case HAPTIK_KURZ: vibes_short_pulse(); break;
    case HAPTIK_DOPPELT: vibes_double_pulse(); break;
    case HAPTIK_LANG: vibes_long_pulse(); break;
    case HAPTIK_DREIFACH:
      vibes_enqueue_custom_pattern((VibePattern){.durations = dreifach, .num_segments = 5});
      break;
    case HAPTIK_STEIGEND:
      vibes_enqueue_custom_pattern((VibePattern){.durations = steigend, .num_segments = 7});
      break;
    default: break;
  }
}

#if defined(PBL_SPEAKER)
// Noten nach MIDI: 84 = C6, 88 = E6, 91 = G6.
static const SpeakerNote s_piep[] = {{88, SpeakerWaveformSine, 120, 0, 0}};
static const SpeakerNote s_glocke[] = {{91, SpeakerWaveformSine, 140, 0, 0},
                                       {84, SpeakerWaveformSine, 260, 0, 0}};
static const SpeakerNote s_aufstieg[] = {{84, SpeakerWaveformTriangle, 110, 0, 0},
                                         {88, SpeakerWaveformTriangle, 110, 0, 0},
                                         {91, SpeakerWaveformTriangle, 200, 0, 0}};
#endif

void rueckmeldung_ton(int ton) {
#if defined(PBL_SPEAKER)
  switch (ton) {
    case TON_PIEP: speaker_play_notes(s_piep, 1, 70); break;
    case TON_GLOCKE: speaker_play_notes(s_glocke, 2, 70); break;
    case TON_AUFSTIEG: speaker_play_notes(s_aufstieg, 3, 70); break;
    default: break;
  }
#else
  (void)ton;
#endif
}

bool rueckmeldung_hat_ton(void) {
#if defined(PBL_SPEAKER)
  return true;
#else
  return false;
#endif
}

void rueckmeldung(HabitType typ, bool trotz_ruhe) {
  if (!trotz_ruhe && quiet_time_is_active()) return;
  const Einstellungen *e = einstellungen();
  rueckmeldung_haptik(e->haptik[typ == HABIT_BAD]);
  rueckmeldung_ton(e->ton[typ == HABIT_BAD]);
  light_enable_interaction();
}
