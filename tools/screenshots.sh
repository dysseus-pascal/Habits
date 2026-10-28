#!/bin/sh
# Bilder der App im Emulator, mit Beispieldaten (src/c/demo.c).
#
#   sh tools/screenshots.sh <emery|flint|gabbro> [zielordner]
#
# Gebaut wird in einer Kopie (pruef.h bekommt HABITS_DEMO), der Quellordner
# bleibt unberuehrt. Die Uhr steht auf Mittwoch, 28.10.2026, 14:10; nach dem
# Installieren geht alles ueber tools/emu.py, das die Zeit nicht neu stellt
# (jeder `pebble`-Befehl taete das).
#
# ACHTUNG: `pebble wipe` loescht die Daten ALLER Emulatoren dieser SDK.
#
# Ohne IPv6 (etwa in einem Container) bricht pypkjs ab, weil gevent einen
# leeren Host als "::" bindet; dann PYTHONPATH auf einen Ordner mit einem
# sitecustomize.py setzen, der das auf 0.0.0.0 biegt.
export PATH=$HOME/.local/bin:$PATH
P="$1"
case "$P" in emery|flint|gabbro) ;; *) echo "Plattform: emery, flint oder gabbro"; exit 1 ;; esac
SRC="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${2:-$SRC/screenshots/$P}"
PB=$(command -v pebble) || { echo "pebble nicht gefunden"; exit 1; }
PY="$(dirname "$(readlink -f "$PB")")/python"
DEMO=1793193000 # 28.10.2026 14:10 in Zuerich
ARBEIT=$(mktemp -d)
trap 'rm -rf "$ARBEIT"' EXIT
mkdir -p "$OUT"

drv() { "$PY" "$SRC/tools/emu.py" "$P" "$@" || exit 1; }

(cd "$SRC" && tar --exclude=./build --exclude=./.git --exclude=./screenshots -cf - .) | tar -xf - -C "$ARBEIT"
printf '#define HABITS_DEMO %s\n' "$DEMO" >> "$ARBEIT/src/c/pruef.h"
(cd "$ARBEIT" && pebble build 2>&1 | grep -E "error|Build failed")
ls "$ARBEIT"/build/*.pbw >/dev/null 2>&1 || { echo "Bau fehlgeschlagen"; exit 1; }

pebble kill >/dev/null 2>&1; sleep 2
pebble wipe >/dev/null 2>&1
pebble install --emulator "$P" "$ARBEIT"/build/*.pbw >/dev/null 2>&1 \
  || pebble install --emulator "$P" "$ARBEIT"/build/*.pbw >/dev/null 2>&1 || { echo "Installieren fehlgeschlagen"; exit 1; }
sleep 4
# Die Uhr stellen und die App neu starten: sie plant ihre Wecker zu dieser
# Zeit. Ob sie nach dem Installieren schon laeuft, haengt am Emulator; darum
# zurueck bis zum Zifferblatt und von dort ueber den Starter (Habits steht dort
# oben).
drv t=2026-10-28T14:10:00 b=back w=0.6 b=back w=0.6 b=back w=0.6 b=back w=1 b=select w=1.5 b=select w=2.5

# Taste n-mal.
n() { i=0; while [ $i -lt "$2" ]; do printf 'b=%s w=0.2 ' "$1"; i=$((i + 1)); done; }
S="$OUT"
echo "== $P"
# Hauptmenue, Eintragen mit Bestaetigung, schlechte Gewohnheit mit Warnung.
drv s="$S/01-hauptmenue.png" \
    b=down w=0.4 b=select w=1 s="$S/02-eintragen.png" \
    b=select w=0.8 s="$S/03-bestaetigung.png" w=2.5 \
    b=select w=1 $(n down 2) b=select w=0.8 s="$S/04-warnung.png" w=4.5
# Diagramme: Wasser in Woche, Tag, Monat, Jahr; Kaffee in der Woche.
drv b=up w=0.4 b=select w=1 b=select w=1 b=select w=1 s="$S/05-woche.png" \
    b=up w=0.6 s="$S/06-tag.png" \
    b=down w=0.4 b=down w=0.6 s="$S/07-monat.png" \
    b=down w=0.6 s="$S/08-jahr.png" b=back w=0.8 \
    $(n down 2) b=select w=1 s="$S/09-kaffee.png" b=back w=0.8 b=back w=0.8
# Aendern (Pills, nach Plan, Critical), Anlegen mit Namen und Rad, Loeschen.
drv $(n down 2) b=select w=1 $(n down 4) b=select w=1 s="$S/10-aendern.png" b=back w=0.8 b=back w=0.8 \
    b=up w=0.4 b=select w=1 b=select w=1 s="$S/11-namen.png" \
    b=select w=1 $(n down 17) b=select w=0.3 $(n down 3) b=select w=0.3 $(n up 7) b=select w=0.6 \
    s="$S/12-rad.png" b=back w=0.6 b=back w=0.6 b=back w=0.8 \
    $(n down 2) b=select w=1 $(n down 3) b=select w=1 s="$S/13-loeschen.png" b=down w=1 b=back w=0.8 b=back w=0.8
# Einstellungen, dann dunkel: Einstellungen, Hauptmenue, Diagramm.
if [ "$P" = gabbro ]; then THEMA=2; else THEMA=4; fi
drv $(n down 2) b=select w=1 s="$S/14-einstellungen.png" \
    $(n down $THEMA) b=select w=1.2 s="$S/15-dunkel.png" b=back w=0.8 \
    s="$S/16-dunkel-menue.png" $(n up 2) b=select w=1 b=select w=1 b=select w=1 s="$S/17-dunkel-woche.png" \
    b=back w=0.6 b=back w=0.6 b=back w=0.6 b=back w=1
# Wecker: Pills um 20:00 (Critical), "Skip" bis zur naechsten Zeit, am
# naechsten Morgen die Bilanz und wieder Pills.
drv t=2026-10-28T19:59:40 w=30 s="$S/18-erinnerung.png" b=down w=1 \
    t=2026-10-29T07:59:40 w=30 s="$S/19-morgen.png" b=back w=1.2 s="$S/20-bilanz.png" b=back w=1
pebble kill >/dev/null 2>&1
echo "fertig: $OUT"
