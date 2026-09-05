import os
import sys
from PIL import Image, ImageDraw, ImageFont
import arabic_reshaper
from bidi.algorithm import get_display

def text_to_xbm(text, font_path, font_size, height=16):
    # 1. Reshape and reorder Arabic text
    reshaped_text = arabic_reshaper.reshape(text)
    bidi_text = get_display(reshaped_text)
    
    # 2. Load font
    try:
        font = ImageFont.truetype(font_path, font_size)
    except IOError:
        print(f"Error: Could not load font from {font_path}", file=sys.stderr)
        return None
        
    # 3. Determine text size
    # In newer Pillow versions, use getbbox
    mask = font.getmask(bidi_text, mode='1')
    width, _ = mask.size
    
    # Add a bit of padding
    width += 4
    
    # 4. Create image
    # XBM uses 1-bit monochrome pixels
    img = Image.new('1', (width, height), 0)
    draw = ImageDraw.Draw(img)
    
    # Calculate vertical offset to center the text
    bbox = draw.textbbox((0, 0), bidi_text, font=font)
    text_height = bbox[3] - bbox[1]
    y_offset = (height - text_height) // 2 - bbox[1]
    
    # Draw text (white on black background)
    draw.text((2, y_offset), bidi_text, font=font, fill=1)
    
    # 5. Convert to XBM byte format manually
    # XBM format: LSB-first (least significant bit is the leftmost pixel of a byte)
    xbm_bytes = []
    for y in range(height):
        byte = 0
        for x in range(width):
            pixel = img.getpixel((x, y))
            # If pixel is set (white), set the corresponding bit
            if pixel:
                bit_index = x % 8
                byte |= (1 << bit_index)
            
            # Write byte every 8 pixels, or at the end of the row
            if (x % 8 == 7) or (x == width - 1):
                xbm_bytes.append(byte)
                byte = 0
                
    return width, xbm_bytes

def generate_cpp_array(name, text, font_path, font_size, height=16):
    result = text_to_xbm(text, font_path, font_size, height)
    if not result:
        return ""
    width, xbm_bytes = result
    
    out = []
    out.append(f"// {text} (width: {width}px, height: {height}px)")
    out.append(f"static constexpr int {name}_width = {width};")
    out.append(f"static constexpr int {name}_height = {height};")
    out.append(f"static const uint8_t {name}_bits[] PROGMEM = {{")
    
    # Format bytes nicely
    bytes_str = []
    for i, b in enumerate(xbm_bytes):
        bytes_str.append(f"0x{b:02X}")
        if (i + 1) % 12 == 0:
            out.append("  " + ", ".join(bytes_str) + ",")
            bytes_str = []
    if bytes_str:
        out.append("  " + ", ".join(bytes_str))
    
    out.append("};")
    out.append("")
    return "\n".join(out)

def main():
    font_path = "C:\\Windows\\Fonts\\tahoma.ttf"
    if not os.path.exists(font_path):
        # Fallback to another common font if tahoma doesn't exist
        font_path = "C:\\Windows\\Fonts\\arial.ttf"
        
    labels = [
        # (variable_name, text, font_size, height)
        # You can add, edit, or remove rows here!
        ("label_smart_water_monitor", "مراقب المياه الذكي", 20, 26),
        ("label_tank_level", "مستوى الخزان", 13, 16),
        ("label_volume", "الكمية", 13, 16),
        ("label_flow", "التدفق", 13, 16),
        ("label_eta_filling", "الوقت والتعبئة", 13, 16),
        ("label_liters", "لتر", 12, 16),
        ("label_lpm", "لتر/دقيقة", 11, 16),
        ("label_filling", "جاري التعبئة", 12, 16),
        ("label_idle", "متوقف", 12, 16)
    ]
    
    cpp_output = []
    cpp_output.append("#pragma once")
    cpp_output.append("#include <Arduino.h>")
    cpp_output.append("")
    
    for name, text, font_size, height in labels:
        cpp_array = generate_cpp_array(name, text, font_path, font_size, height)
        cpp_output.append(cpp_array)
        
    output_file = os.path.join(os.path.dirname(__file__), "ArabicLabels.h")
    with open(output_file, "w", encoding="utf-8") as f:
        f.write("\n".join(cpp_output))
    print(f"Generated successfully: {output_file}")

if __name__ == "__main__":
    main()
