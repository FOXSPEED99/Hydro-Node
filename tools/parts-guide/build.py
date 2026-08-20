#!/usr/bin/env python3
"""Assemble parts-guide.html: inline the component photos and the four line drawings."""
import json, os, re

BASE = os.path.dirname(os.path.abspath(__file__))
imgs = json.load(open(os.path.join(BASE, "img.json")))
tpl = open(os.path.join(BASE, "template.html")).read()

LBL = 'font-family="JetBrains Mono, ui-monospace, monospace" font-size="9" letter-spacing="1" fill="#6B7A84"'

SVG = {}

SVG["SVG_DIODE"] = f'''<svg viewBox="0 0 200 200" role="img" aria-label="A 1N5819 axial diode: dark cylindrical body with a silver stripe near one end and a wire lead out of each end">
  <line x1="6" y1="100" x2="62" y2="100" stroke="#9AA6AD" stroke-width="4"/>
  <line x1="138" y1="100" x2="194" y2="100" stroke="#9AA6AD" stroke-width="4"/>
  <rect x="62" y="80" width="76" height="40" rx="7" fill="#2E3A42"/>
  <rect x="62" y="80" width="76" height="14" rx="7" fill="#3E4B54"/>
  <rect x="124" y="80" width="10" height="40" fill="#E9EDEF"/>
  <text x="72" y="105" font-family="JetBrains Mono, ui-monospace, monospace" font-size="11" fill="#DDE4E8">1N5819</text>
  <text x="30" y="140" {LBL} text-anchor="middle">PLAIN</text>
  <text x="168" y="140" {LBL} text-anchor="middle">STRIPE</text>
  <text x="100" y="164" {LBL} text-anchor="middle">CURRENT FLOWS THIS WAY →</text>
</svg>'''

SVG["SVG_FUSE"] = f'''<svg viewBox="0 0 200 200" role="img" aria-label="A glass cartridge fuse: transparent tube with metal end caps and a thin wire running through it">
  <line x1="10" y1="100" x2="40" y2="100" stroke="#9AA6AD" stroke-width="4"/>
  <line x1="160" y1="100" x2="190" y2="100" stroke="#9AA6AD" stroke-width="4"/>
  <rect x="52" y="78" width="96" height="44" rx="4" fill="#DCE6EA" stroke="#A9B8C0" stroke-width="1.5"/>
  <rect x="40" y="74" width="18" height="52" rx="3" fill="#AFBAC1" stroke="#8B979E" stroke-width="1"/>
  <rect x="142" y="74" width="18" height="52" rx="3" fill="#AFBAC1" stroke="#8B979E" stroke-width="1"/>
  <path d="M58 100 L92 100 L100 96 L108 104 L142 100" fill="none" stroke="#8A6A2E" stroke-width="1.8"/>
  <text x="100" y="150" {LBL} text-anchor="middle">0.5 A · FAST-BLOW</text>
  <text x="100" y="166" {LBL} text-anchor="middle">INSIDE THE BATTERY PACK</text>
</svg>'''

SVG["SVG_DS18B20"] = f'''<svg viewBox="0 0 200 200" role="img" aria-label="A DS18B20 probe: stainless steel tube with black heatshrink and three wires, red, black and yellow">
  <rect x="14" y="82" width="70" height="34" rx="16" fill="#C6CFD4" stroke="#9AA6AD" stroke-width="1.5"/>
  <rect x="46" y="82" width="38" height="34" fill="#BAC4CA"/>
  <rect x="80" y="84" width="30" height="30" rx="5" fill="#23292D"/>
  <path d="M108 92 C132 92 138 66 168 66" fill="none" stroke="#C0392B" stroke-width="4.5" stroke-linecap="round"/>
  <path d="M108 99 C136 99 142 99 168 99" fill="none" stroke="#22282C" stroke-width="4.5" stroke-linecap="round"/>
  <path d="M108 106 C132 106 138 132 168 132" fill="none" stroke="#D8B32A" stroke-width="4.5" stroke-linecap="round"/>
  <text x="174" y="70" {LBL}>VCC</text>
  <text x="174" y="103" {LBL}>GND</text>
  <text x="174" y="136" {LBL}>DATA</text>
  <text x="50" y="152" {LBL} text-anchor="middle">STAINLESS TIP</text>
  <text x="100" y="176" {LBL} text-anchor="middle">CHECK COLOURS WITH A METER</text>
</svg>'''

SVG["SVG_TANT"] = f'''<svg viewBox="0 0 200 200" role="img" aria-label="A tantalum capacitor: orange rectangular block with a dark bar across one end marking the plus leg, and two legs">
  <rect x="46" y="62" width="108" height="62" rx="9" fill="#D89A2A"/>
  <rect x="46" y="62" width="108" height="20" rx="9" fill="#E4AC42"/>
  <rect x="46" y="62" width="20" height="62" fill="#3A3026"/>
  <path d="M52 86 h8 M56 82 v8" stroke="#F2E7D2" stroke-width="2.6" stroke-linecap="round"/>
  <text x="104" y="102" font-family="JetBrains Mono, ui-monospace, monospace" font-size="15" fill="#3A3026" text-anchor="middle">107</text>
  <line x1="60" y1="124" x2="60" y2="164" stroke="#9AA6AD" stroke-width="4"/>
  <line x1="140" y1="124" x2="140" y2="164" stroke="#9AA6AD" stroke-width="4"/>
  <text x="60" y="182" {LBL} text-anchor="middle">PLUS</text>
  <text x="140" y="182" {LBL} text-anchor="middle">MINUS</text>
  <text x="100" y="44" {LBL} text-anchor="middle">THE BAR MARKS PLUS</text>
</svg>'''

out = tpl
for k, v in imgs.items():
    out = out.replace("{{" + k + "}}", v)
for k, v in SVG.items():
    out = out.replace("{{" + k + "}}", v)

left = re.findall(r"\{\{[A-Z0-9_]+\}\}", out)
if left:
    raise SystemExit("unfilled placeholders: " + ", ".join(sorted(set(left))))

dest = "/home/user/Hydro-Node/parts-guide.html"
open(dest, "w").write(out)
print(f"wrote {dest}  {len(out)//1024} KB")
