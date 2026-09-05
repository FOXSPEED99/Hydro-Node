# PNG Icon System Guide

## Overview
This guide explains the PNG-based icon system using LittleFS and PNGdec, replacing the hardcoded pixel arrays in DisplayUI.cpp.

## TFT_eSPI State Requirements

### setSwapBytes()
**Critical for correct color display:**

```cpp
tft.setSwapBytes(true);  // Required for RGB565 PNG data
```

- **Location**: Already set in `setupPanel()` at line 2526
- **Purpose**: Swaps byte order for RGB565 color data
- **When needed**: 
  - Before calling `pushImage()` with RGB565 data
  - Before calling `pushSprite()` with sprites containing RGB565 data
  - PNGdec outputs RGB565_BIG_ENDIAN, which requires swapBytes=true on ESP32

### State Management
The icon system automatically manages TFT_eSPI state:

1. **Sprite Creation**: `iconSprite->createSprite()` sets up sprite buffer
2. **PNG Decoding**: PNGdec writes RGB565 data to sprite
3. **Sprite Drawing**: `pushSprite()` or `pushRotateZoom()` draws to TFT
4. **Sprite Cleanup**: `deleteSprite()` frees memory

**No manual state changes needed** - the system handles it internally.

### Color Format
- **PNGdec Output**: RGB565_BIG_ENDIAN
- **TFT_eSPI Expected**: RGB565 (with swapBytes flipped)
- **Alpha Handling**: Transparent pixels (alpha=0) are skipped during drawing

## Upload Instructions

### Two-Step Upload Process

#### Step 1: Upload PNG Files to LittleFS
1. Place your PNG files in the `/data` directory:
   - `water_drop.png` (60x60 recommended)
   - `pump.png` (145x145 recommended)
   - `power_switch.png` (32x32 recommended)

2. In Arduino IDE 2.x:
   - Go to **Tools** menu
   - Select **ESP32 LittleFS Data Upload**
   - Wait for upload to complete (check serial monitor for success message)

3. **Important**: This only uploads the files to flash, NOT the sketch code

#### Step 2: Upload Sketch Code
1. After LittleFS upload completes:
   - Go to **Sketch** menu
   - Select **Upload** (or press Ctrl+U)
   - Wait for compilation and upload

2. The sketch will now use the PNG files from LittleFS

### Upload Order Matters
**Always upload LittleFS data FIRST, then the sketch.**

If you upload the sketch first:
- The code will try to load PNG files that don't exist yet
- Fallback icons will be displayed instead
- You'll need to re-upload the sketch after LittleFS upload

### Re-uploading Icons
To change icon artwork:
1. Replace PNG files in `/data` directory
2. Run **ESP32 LittleFS Data Upload** again
3. **No sketch re-upload needed** (unless you changed config table)

### Re-uploading Code Only
To change icon positions/sizes:
1. Edit the `iconConfigs` table in DisplayUI.cpp (lines 46-50)
2. Run normal **Upload** (sketch only)
3. **No LittleFS re-upload needed**

## Icon Configuration Table

Edit this table in DisplayUI.cpp to change icon properties:

```cpp
static constexpr IconConfig iconConfigs[ICON_COUNT] = {
  { "/water_drop.png", 195, 60, 30, 60, 60 },    // Water drop
  { "/pump.png", 285, 82, 145, 145, 145 },       // Pump
  { "/power_switch.png", 0, 0, 32, 32, 32 }      // Power switch
};
```

**Fields:**
- `file`: PNG filename in LittleFS (must start with `/`)
- `x, y`: Draw position on screen
- `size`: Target draw size (will scale from native)
- `nativeW, nativeH`: Original PNG dimensions

**Example changes:**
- Move water drop: `{ "/water_drop.png", 200, 70, 30, 60, 60 }`
- Scale pump larger: `{ "/pump.png", 285, 82, 160, 145, 145 }`
- Change file: `{ "/water_drop.png", 195, 60, 30, 64, 64 }` (if you use 64x64 PNG)

## Fallback System

If LittleFS fails to mount or PNG files are missing:
- Simple fallback icons are drawn (rectangles with basic shapes)
- Screen never goes blank
- Check serial monitor for error messages:
  - `LittleFS mount failed, using fallback icons`
  - `Failed to open PNG: <filename>`
  - `PNG file not found: <filename>`

## Troubleshooting

### Icons not appearing
1. Check serial monitor for LittleFS mount errors
2. Verify PNG files are in `/data` directory
3. Ensure LittleFS upload completed successfully
4. Check PNG file format (must be PNG with alpha channel)

### Wrong colors
1. Verify `tft.setSwapBytes(true)` is called in setupPanel()
2. Check PNG is RGB mode, not grayscale
3. Verify PNG has alpha channel enabled

### Scaling artifacts
1. Use higher resolution native PNG (e.g., 2x target size)
2. Update `nativeW, nativeH` in config table
3. Re-upload LittleFS data

### Memory issues
1. Large PNGs may cause sprite allocation failures
2. Keep native PNGs under 200x200 pixels
3. Check serial monitor for sprite creation errors

## Library Requirements

Add these to your `platformio.ini` or install via Arduino Library Manager:

```ini
lib_deps =
    bodmer/TFT_eSPI
    bitbank2/PNGdec
    lorol/LittleFS
```

Or in Arduino IDE:
- **TFT_eSPI** by Bodmer
- **PNGdec** by bitbank2
- **LittleFS** (built into ESP32 Arduino core)

## Performance Notes

- PNG decoding happens once per icon per draw call
- Sprite scaling uses hardware acceleration via `pushRotateZoom()`
- Total decode+draw time: ~50-100ms per icon (typical)
- Consider caching sprites if performance is critical
