#!/usr/bin/env python3
"""Generate boot/wallpaper.bmp for the Limine boot menu.

v0.4.0 composition: calm deep-sea gradient, low-contrast kelp silhouettes
anchored at the bottom, a clean wordmark with an accent underline.
Plain 24-bit BMP, 1280x800. Requires Pillow (only when regenerating).
"""
from PIL import Image, ImageDraw, ImageFilter, ImageFont
import math
import os

W, H = 1280, 800
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ── background: deep-sea vertical gradient ────────────────────────────
top = (9, 15, 27)
bot = (13, 36, 50)
img = Image.new("RGB", (W, H))
d = ImageDraw.Draw(img)
for y in range(H):
    t = y / (H - 1)
    # ease out so the bottom isn't a hard band
    t = t ** 1.25
    r = int(top[0] + (bot[0] - top[0]) * t)
    g = int(top[1] + (bot[1] - top[1]) * t)
    b = int(top[2] + (bot[2] - top[2]) * t)
    d.line([(0, y), (W, y)], fill=(r, g, b))

# ── kelp silhouettes on a soft overlay ────────────────────────────────
ov = Image.new("RGBA", (W, H), (0, 0, 0, 0))
od = ImageDraw.Draw(ov)


def strand(x0, base_amp, phase, width, alpha, sway=110):
    """One kelp blade: a tapered sine curve rising from the bottom."""
    pts = []
    n = 60
    for i in range(n + 1):
        t = i / n                       # 0 = bottom, 1 = top
        y = H - t * (H * 0.78)
        # amplitude shrinks toward the tip
        amp = base_amp * (0.25 + 0.75 * (1 - t))
        x = x0 + amp * math.sin(t * 6.0 + phase) \
            + sway * math.sin(t * 2.2 + phase * 0.5) * t
        pts.append((x, y))
    for i in range(n):
        t = i / n
        w = max(1, int(width * (1 - t) + 1))
        od.line([pts[i], pts[i + 1]], fill=(16, 42, 40, alpha), width=w)


for x0, amp, ph, w, a in [
    (70, 55, 0.0, 9, 120),
    (130, 70, 1.9, 7, 90),
    (190, 40, 3.7, 5, 70),
    (1120, 60, 1.2, 8, 100),
    (1180, 45, 4.4, 6, 80),
    (1240, 70, 2.6, 5, 60),
]:
    strand(x0, amp, ph, w, a)

ov = ov.filter(ImageFilter.GaussianBlur(1.2))
img = Image.alpha_composite(img.convert("RGBA"), ov).convert("RGB")
d = ImageDraw.Draw(img)

# ── typography ────────────────────────────────────────────────────────
text = "KelpOS"
sub = "The Kelp Kernel"  # Leave this alone, no changing
font = None
for path in ("/usr/share/fonts/google-noto/NotoSans-Bold.ttf",
             "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"):
    try:
        font = ImageFont.truetype(path, 110)
        break
    except Exception:
        continue
if font is None:
    font = ImageFont.load_default()
try:
    small = ImageFont.truetype(font.path, 34)
except Exception:
    small = font

bbox = d.textbbox((0, 0), text, font=font)
tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
x = W - tw - 120
y = 128

# soft shadow, then the wordmark
d.text((x + 3, y + 4), text, font=font, fill=(4, 8, 14))
d.text((x, y), text, font=font, fill=(214, 236, 222))

# accent underline: thin kelp-green bar under the wordmark
uy = y + th + 26
d.rectangle([x, uy, x + tw, uy + 5], fill=(63, 163, 77))
d.rectangle([x, uy, x + 90, uy + 5], fill=(160, 224, 170))

d.text((x + 5, uy + 22), sub, font=small, fill=(140, 178, 205))

img.save(os.path.join(ROOT, "boot", "wallpaper.bmp"))
print(f"boot/wallpaper.bmp written ({W}x{H}, BMP24)")
