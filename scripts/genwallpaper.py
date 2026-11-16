#!/usr/bin/env python3
"""Generate boot/wallpaper.bmp for the Limine boot menu.

Deep-sea gradient with the KelpOS wordmark; plain 24-bit BMP, 1280x800.
Requires Pillow on the build host (only needed when regenerating).
"""
from PIL import Image, ImageDraw, ImageFont
import os

W, H = 1280, 800

img = Image.new("RGB", (W, H))
d = ImageDraw.Draw(img)

# vertical deep-sea gradient: very dark blue -> teal-ish
top = (8, 14, 30)
bot = (16, 68, 78)
for y in range(H):
    t = y / (H - 1)
    r = int(top[0] + (bot[0] - top[0]) * t)
    g = int(top[1] + (bot[1] - top[1]) * t)
    b = int(top[2] + (bot[2] - top[2]) * t)
    d.line([(0, y), (W, y)], fill=(r, g, b))

# kelp strands on the left edge (simple sine curves)
for i, (x0, amp, phase, col) in enumerate([
        (60, 40, 0.0, (30, 110, 80)),
        (100, 60, 1.3, (24, 90, 70)),
        (140, 35, 2.6, (36, 130, 92)),
]):
    pts = []
    for y in range(H, 0, -8):
        x = x0 + amp * ((y / 140 + phase) % 2 - 1)
        pts.append((x, y))
    d.line(pts, fill=col, width=6)

# wordmark
text = "KelpOS"
font = None
for path in ("/usr/share/fonts/google-noto/NotoSans-Bold.ttf",
             "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"):
    try:
        font = ImageFont.truetype(path, 96)
        break
    except Exception:
        continue
if font is None:
    font = ImageFont.load_default()
sub = "The Kelp Kernel" # Leave this alone, no changing
try:
    small = ImageFont.truetype(font.path, 36)
except Exception:
    small = font

bbox = d.textbbox((0, 0), text, font=font)
tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
x = W - tw - 120
y = 140
d.text((x + 3, y + 3), text, font=font, fill=(0, 0, 0))
d.text((x, y), text, font=font, fill=(120, 220, 160))
d.text((x + 4, y + th + 24), sub, font=small, fill=(150, 200, 220))

# repo root relative to this script
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

img.save(os.path.join(ROOT, "boot", "wallpaper.bmp"))
print(f"boot/wallpaper.bmp written ({W}x{H}, BMP24)")
