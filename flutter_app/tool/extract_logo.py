"""Extract the Mercedes tri-star emblem from logo.jpg onto transparency.

The source is a chrome emblem on a black background. The star's own 3D facets
contain dark tones, so a naive black key would punch holes in it. Instead we
flood-fill from the image corners: only the OUTER background that is connected
to a corner and dark becomes transparent. The emblem — including its shaded
facets and the black gaps enclosed by the ring — is preserved (those enclosed
gaps read as authentic black on the app's dark surfaces).

Outputs into assets/branding, reusing the same filenames the app already wires:
  - app_icon.png            (emblem on dark rounded bg — launcher)
  - app_icon_foreground.png (emblem only, padded, transparent — adaptive fg)
  - splash_logo.png         (emblem only, transparent — native splash)
  - emblem.png              (tight transparent crop, for in-app use)

Run: python tool/extract_logo.py
"""
import os
from collections import deque
import numpy as np
from PIL import Image, ImageFilter

SRC = os.path.join(os.path.dirname(__file__), "..", "..", "..", "..", "..", "logo.jpg")
# logo.jpg lives at the repo root; resolve robustly relative to this file.
SRC = os.environ.get("LOGO_SRC", SRC)
OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "branding")
os.makedirs(OUT, exist_ok=True)

BG = (10, 10, 12, 255)  # #0A0A0C app cockpit background

img = Image.open(SRC).convert("RGB")
arr = np.asarray(img).astype(np.int16)
h, w, _ = arr.shape

# Brightness map; background is near-black, emblem chrome is bright.
lum = arr.max(axis=2)  # use max channel so any coloured highlight survives
DARK = 60  # pixels dimmer than this are candidates for "background"

dark = lum < DARK

# Flood fill from all border pixels through connected dark regions.
bg = np.zeros((h, w), dtype=bool)
dq = deque()
for x in range(w):
    for y in (0, h - 1):
        if dark[y, x] and not bg[y, x]:
            bg[y, x] = True
            dq.append((y, x))
for y in range(h):
    for x in (0, w - 1):
        if dark[y, x] and not bg[y, x]:
            bg[y, x] = True
            dq.append((y, x))

while dq:
    y, x = dq.popleft()
    for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        ny, nx = y + dy, x + dx
        if 0 <= ny < h and 0 <= nx < w and dark[ny, nx] and not bg[ny, nx]:
            bg[ny, nx] = True
            dq.append((ny, nx))

# Alpha: opaque everywhere except the connected outer background.
alpha = np.where(bg, 0, 255).astype(np.uint8)
alpha_img = Image.fromarray(alpha, mode="L")
# Feather the edge slightly so the chrome rim doesn't look jagged.
alpha_img = alpha_img.filter(ImageFilter.GaussianBlur(0.8))

emblem = img.convert("RGBA")
emblem.putalpha(alpha_img)

# Tight crop to the emblem bounding box.
bbox = emblem.getbbox()
emblem = emblem.crop(bbox)


def squared(im, pad_frac):
    """Center `im` on a transparent square canvas with padding."""
    side = int(max(im.size) * (1 + pad_frac * 2))
    canvas = Image.new("RGBA", (side, side), (0, 0, 0, 0))
    canvas.paste(im, ((side - im.width) // 2, (side - im.height) // 2), im)
    return canvas


emblem_sq = squared(emblem, 0.06)
emblem_sq.save(os.path.join(OUT, "emblem.png"))
emblem_sq.save(os.path.join(OUT, "splash_logo.png"))
squared(emblem, 0.22).save(os.path.join(OUT, "app_icon_foreground.png"))

# Full launcher icon: emblem on the dark rounded cockpit background.
icon_bg = Image.new("RGBA", emblem_sq.size, BG)
icon = Image.alpha_composite(icon_bg, emblem_sq)
icon.save(os.path.join(OUT, "app_icon.png"))

print("wrote emblem + icon assets to", os.path.abspath(OUT))
print("emblem crop size:", emblem.size)
