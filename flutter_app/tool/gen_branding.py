"""Generate premium Star Trail / Benz-inspired branding assets.

Draws an abstract chrome/silver three-point star emblem (tri-star inspired,
NOT the trademarked Mercedes logo) on a near-black background. Produces:
  - app_icon.png           1024x1024 full icon (ring + star, dark bg)
  - app_icon_foreground.png 1024x1024 adaptive foreground (star only, padded, transparent)
  - splash_logo.png        768x768 emblem for the native splash (transparent bg)

Run:  python tool/gen_branding.py
"""
import math
import os
from PIL import Image, ImageDraw, ImageFilter

OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "branding")
os.makedirs(OUT, exist_ok=True)

BG = (10, 10, 12, 255)          # #0A0A0C near-black
SILVER_HI = (232, 233, 236)     # bright chrome highlight
SILVER = (200, 201, 204)        # #C8C9CC steel silver
SILVER_LO = (142, 145, 149)     # #8E9195 gunmetal


def _radial_silver(size):
    """A vertical chrome gradient tile used to 'fill' the star via masking."""
    grad = Image.new("RGBA", (size, size))
    px = grad.load()
    for y in range(size):
        t = y / (size - 1)
        # highlight near top, gunmetal near bottom, subtle mid band
        if t < 0.5:
            k = t / 0.5
            r = int(SILVER_HI[0] + (SILVER[0] - SILVER_HI[0]) * k)
            g = int(SILVER_HI[1] + (SILVER[1] - SILVER_HI[1]) * k)
            b = int(SILVER_HI[2] + (SILVER[2] - SILVER_HI[2]) * k)
        else:
            k = (t - 0.5) / 0.5
            r = int(SILVER[0] + (SILVER_LO[0] - SILVER[0]) * k)
            g = int(SILVER[1] + (SILVER_LO[1] - SILVER[1]) * k)
            b = int(SILVER[2] + (SILVER_LO[2] - SILVER[2]) * k)
        for x in range(size):
            px[x, y] = (r, g, b, 255)
    return grad


def _tristar_mask(size, r_outer, r_inner, ring=True, ring_w=None):
    """White-on-black mask of a three-point star inside two concentric rings."""
    ss = 4  # supersample
    S = size * ss
    cx = cy = S / 2
    mask = Image.new("L", (S, S), 0)
    d = ImageDraw.Draw(mask)
    ro = r_outer * ss
    ri = r_inner * ss

    # three filled wedges from centre to outer ring (tri-star)
    for i in range(3):
        a = -math.pi / 2 + i * 2 * math.pi / 3
        half = math.radians(15)
        p_tip = (cx + math.cos(a) * ro, cy + math.sin(a) * ro)
        p_l = (cx + math.cos(a - half) * (ri * 0.55), cy + math.sin(a - half) * (ri * 0.55))
        p_r = (cx + math.cos(a + half) * (ri * 0.55), cy + math.sin(a + half) * (ri * 0.55))
        d.polygon([p_tip, p_l, (cx, cy), p_r], fill=255)

    # centre hub
    d.ellipse([cx - ri * 0.16, cy - ri * 0.16, cx + ri * 0.16, cy + ri * 0.16], fill=255)

    if ring:
        w = (ring_w if ring_w else max(2, int(r_outer * 0.05))) * ss
        d.ellipse([cx - ro, cy - ro, cx + ro, cy + ro], outline=255, width=int(w))
        rim = ri * 1.02
        d.ellipse([cx - rim, cy - rim, cx + rim, cy + rim], outline=255,
                  width=int(w * 0.55))

    return mask.resize((size, size), Image.LANCZOS)


def build_emblem(size, bg=None, pad=0.0, ring=True):
    """Compose a silver-filled emblem, optional dark background."""
    canvas = Image.new("RGBA", (size, size), bg if bg else (0, 0, 0, 0))
    inner = int(size * (1 - pad))
    r_outer = inner * 0.44
    r_inner = inner * 0.30
    silver = _radial_silver(size)
    mask = _tristar_mask(size, r_outer, r_inner, ring=ring)

    # soft outer glow for depth
    glow = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    gmask = mask.filter(ImageFilter.GaussianBlur(size * 0.02))
    glow.paste((180, 200, 220, 90), (0, 0), gmask)
    canvas = Image.alpha_composite(canvas, glow)

    emblem = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    emblem.paste(silver, (0, 0), mask)
    canvas = Image.alpha_composite(canvas, emblem)
    return canvas


# 1) Full app icon: emblem on dark rounded bg
icon = build_emblem(1024, bg=BG, pad=0.10, ring=True)
icon.save(os.path.join(OUT, "app_icon.png"))

# 2) Adaptive foreground: star only, extra padding, transparent (bg handled by manifest)
fg = build_emblem(1024, bg=None, pad=0.28, ring=True)
fg.save(os.path.join(OUT, "app_icon_foreground.png"))

# 3) Splash logo: transparent, medium
splash = build_emblem(768, bg=None, pad=0.12, ring=True)
splash.save(os.path.join(OUT, "splash_logo.png"))

print("branding assets written to", os.path.abspath(OUT))
