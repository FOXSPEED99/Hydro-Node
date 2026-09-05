# LittleFS assets (`data/`)

These PNGs are flashed to the ESP32-S3's LittleFS partition and loaded by the
firmware at runtime. Upload them with **Tools → ESP32 LittleFS Data Upload** in
the Arduino IDE — this is separate from uploading the sketch, so do both steps.

| File | Used for |
|------|----------|
| `Pump-No-Filling.png` | Pump body (blue frame + empty circle); the water and blade are drawn/animated on top in code |
| `pump.png` | Pump blade/gear that spins when the pump is on |
| `water_drop.png` | Water-drop icon in the level card |

All are RGB PNGs with a real alpha channel (transparent background). Keep them at
their native sizes — the firmware measures hub/circle geometry from these exact
pixels, so resizing them means re-measuring the constants in `DisplayUI.cpp`.
