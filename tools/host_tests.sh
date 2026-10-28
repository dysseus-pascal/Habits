#!/bin/sh
# Die Rechnung der App (src/c ohne Oberflaeche) mit dem C-Compiler des
# Rechners pruefen - pebble.h kommt als Attrappe aus tools/host.
#
#   sh tools/host_tests.sh
#
# Exitcode 0 = alles bestanden. TZ=Europe/Zurich, damit die Tage der
# Umstellung auf und von der Sommerzeit mitgeprueft werden.
DIR=$(cd "$(dirname "$0")/.." && pwd)
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT
LOGIK="kalender.c logbuch.c habit.c zeitraum.c planer.c diagramm_daten.c"
QUELLEN=""
for f in $LOGIK; do QUELLEN="$QUELLEN $DIR/src/c/$f"; done
FEHLER=0
for t in kalender logbuch habit zeitraum planer diagramm; do
  cc -std=gnu99 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
     -I "$DIR/tools/host" -I "$DIR/src/c" -I "$DIR/tools" \
     "$DIR/tools/test_$t.c" $QUELLEN "$DIR/tools/host/attrappe.c" "$DIR/tools/host/pruefen.c" \
     -o "$OUT/test_$t" || { FEHLER=1; continue; }
  TZ=Europe/Zurich "$OUT/test_$t" || FEHLER=1
done
exit $FEHLER
