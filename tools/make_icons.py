#!/usr/bin/env python3
"""Symbole der App, gezeichnet statt gemalt.

Aufruf: make_icons.py <zielordner>

  menu_icon.png   25x25  Starter: schwarzer Strich auf durchsichtigem Grund,
                         wie die Systemsymbole (Linien 2 bis 3 Punkte stark)
  icon_check.png  20x20  Aktionsleiste: weiss auf durchsichtig, die Leiste
  icon_cross.png  20x20  ist dunkel
  icon_undo.png   20x20

Gezeichnet wird achtfach gross und dann verkleinert; danach entscheidet eine
Schwelle je Punkt zwischen Farbe und durchsichtig. Halbtransparente Raender
wuerde die Uhr ohnehin auf ihre Palette runden - so sieht man vorher, was sie
zeigt.
"""
import math
import os
import sys

from PIL import Image, ImageDraw

S = 8  # Ueberabtastung


def leinwand(n):
    return Image.new('L', (n * S, n * S), 0)


def fertig(maske, n, farbe, ziel):
    klein = maske.resize((n, n), Image.LANCZOS)
    bild = Image.new('RGBA', (n, n), (0, 0, 0, 0))
    for y in range(n):
        for x in range(n):
            if klein.getpixel((x, y)) >= 110:
                bild.putpixel((x, y), farbe + (255,))
    bild.save(ziel)


def linie(d, punkte, breite):
    d.line([(x * S, y * S) for x, y in punkte], fill=255, width=int(breite * S), joint='curve')
    for x, y in (punkte[0], punkte[-1]):
        r = breite * S / 2
        d.ellipse([x * S - r, y * S - r, x * S + r, y * S + r], fill=255)


def menu_icon(ziel):
    # Ein Ring aus Tagen mit einem Haken: eine Gewohnheit, die haelt.
    n = 25
    m = leinwand(n)
    d = ImageDraw.Draw(m)
    c, r, w = 12.5, 11.0, 2.6
    d.ellipse([(c - r) * S, (c - r) * S, (c + r) * S, (c + r) * S], outline=255, width=int(w * S))
    # Eine Luecke oben rechts, wie ein Kalenderring, der sich schliesst.
    luecke = Image.new('L', m.size, 0)
    ImageDraw.Draw(luecke).pieslice([0, 0, n * S, n * S], -70, -40, fill=255)
    m.paste(0, mask=luecke)
    linie(d, [(7.5, 13.0), (11.0, 16.5), (18.0, 8.5)], 2.8)
    fertig(m, n, (0, 0, 0), ziel)


def haken(ziel):
    n = 20
    m = leinwand(n)
    linie(ImageDraw.Draw(m), [(3.5, 10.5), (8.0, 15.0), (16.5, 5.0)], 3.0)
    fertig(m, n, (255, 255, 255), ziel)


def kreuz(ziel):
    n = 20
    m = leinwand(n)
    d = ImageDraw.Draw(m)
    linie(d, [(4.5, 4.5), (15.5, 15.5)], 3.0)
    linie(d, [(15.5, 4.5), (4.5, 15.5)], 3.0)
    fertig(m, n, (255, 255, 255), ziel)


def zurueck(ziel):
    # Ein Pfeil im Bogen nach links: rueckgaengig.
    n = 20
    m = leinwand(n)
    d = ImageDraw.Draw(m)
    bogen = [(10 + 6 * math.cos(math.radians(a)), 11 + 6 * math.sin(math.radians(a)))
             for a in range(-200, 91, 10)]
    linie(d, bogen, 2.6)
    sx, sy = bogen[0]
    d.polygon([((sx - 4.2) * S, (sy - 0.5) * S), ((sx + 3.8) * S, (sy - 3.0) * S),
               ((sx + 1.2) * S, (sy + 4.6) * S)], fill=255)
    fertig(m, n, (255, 255, 255), ziel)


def main():
    ziel = sys.argv[1]
    os.makedirs(ziel, exist_ok=True)
    menu_icon(os.path.join(ziel, 'menu_icon.png'))
    haken(os.path.join(ziel, 'icon_check.png'))
    kreuz(os.path.join(ziel, 'icon_cross.png'))
    zurueck(os.path.join(ziel, 'icon_undo.png'))


if __name__ == '__main__':
    main()
