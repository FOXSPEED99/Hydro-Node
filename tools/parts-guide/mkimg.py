import base64, io, json, os
from PIL import Image

SRC = "/home/user/Hydro-Node/Hydro Node Device/Hydro Node Parts & Schematic/Components Images"

MAP = {
    "BATTERY":   "IMG_7971.png",
    "REED":      "F2293658-01.jpg",
    "MAGNET":    "IMG_20260805_163158.jpg",
    "MOSFET":    "mosfet-irlz44n-1024x1024-1.jpg",
    "DIP14":     "cd4013be_4013_dip_14_cc651b9c453c41a88f1569a7df2f7cbd_1024x1024.jpg",
    "PROMINI":   "11113-01b.jpg",
    "RA02":      "images - 2026-08-01T183803.546.jpeg",
    "PIGTAIL":   "IPEX4.0-SMAF-15CM.webp",
    "RCWL":      "1-19.jpg",
    "FLOW":      "71ZoYQ6sslL.jpg",
    "CER104":    "104-capacitor-8.png",
    "CAN":       "61z6XL37UWL._SL1500.jpg",
    "R47K":      "images - 2026-08-04T181221.473.jpeg",
    "R10K":      "2237221.jpg",
    "R1M":       "resistor-1m-ohm-14w-5.jpg",
    "R1K":       "1K.webp",
    "RSMALL":    "pp_3466670_1200x1200.jpeg",
    "JST4":      "B4B-XH-A(LF)(SN).jpg",
    "JST2":      "jstxh-254mm-straight-2pin-connector-36518-99-O-1.jpg",
    "JSTHOUSE":  "jst_xh_254mm_socket_2_pin_straight_tunic_connector_36517_99_O.webp",
    "LED":       "images - 2026-08-04T175342.830.jpeg",
}

MAXDIM = 620
out = {}
total = 0
for key, fn in MAP.items():
    p = os.path.join(SRC, fn)
    im = Image.open(p)
    if im.mode in ("RGBA", "LA", "P"):
        im = im.convert("RGBA")
        bg = Image.new("RGB", im.size, (255, 255, 255))
        bg.paste(im, mask=im.split()[-1])
        im = bg
    else:
        im = im.convert("RGB")
    im.thumbnail((MAXDIM, MAXDIM), Image.LANCZOS)
    buf = io.BytesIO()
    im.save(buf, "JPEG", quality=82, optimize=True, progressive=True)
    b = buf.getvalue()
    total += len(b)
    out[key] = "data:image/jpeg;base64," + base64.b64encode(b).decode()
    print(f"{key:10s} {im.size}  {len(b)//1024} KB")

print("TOTAL raw:", total//1024, "KB   base64:", sum(len(v) for v in out.values())//1024, "KB")
json.dump(out, open("/tmp/claude-0/-home-user-Hydro-Node/50250add-e0dc-5b13-9075-5e5023ddeb40/scratchpad/pg/img.json", "w"))
