"""
find_eye_landmarks.py

STEP 1 of the eye-animation workflow.

Use this on any cropped eye image to visually identify:
  - iris center (x, y) and radius
  - eyelid left corner, right corner
  - eyelid arc control point (how high the lid peaks in the middle)

Edit eye_config.py and re-run until landmarks_check.png looks right.

Usage:
    python3 find_eye_landmarks.py
    
    or when you're finished, 
			python3 find_eye_landmarks_shared_config.py done
Optional image override:
    python3 find_eye_landmarks_shared_config.py another_eye.jpg

Save final config:
    python3 find_eye_landmarks_shared_config.py another_eye.jpg done
"""

import sys
from PIL import Image, ImageDraw, ImageFont
import numpy as np

from eye_config import (
    SOURCE_IMAGE,
    LANDMARK_CHECK_IMAGE,
    GUESS_IRIS_CX,
    GUESS_IRIS_CY,
    GUESS_IRIS_R,
    GUESS_LID_LEFT,
    GUESS_LID_RIGHT,
    GUESS_LID_CTRL_Y,
    SHOW_GRID,
    GRID_SPACING,
    GRID_MINOR_COLOR,
    GRID_MAJOR_COLOR,
    GRID_LABEL_COLOR,
    GRID_MAJOR_EVERY,
    EYELID_SAMPLE_X,
    EYELID_SAMPLE_Y,
)

# Command-line options:
#
#   python3 find_eye_landmarks_shared_config.py
#       Use SOURCE_IMAGE from eye_config.py
#
#   python3 find_eye_landmarks_shared_config.py done
#       Use SOURCE_IMAGE and also save a config snapshot
#
#   python3 find_eye_landmarks_shared_config.py another_eye.jpg
#       Override SOURCE_IMAGE
#
#   python3 find_eye_landmarks_shared_config.py another_eye.jpg done
#       Override SOURCE_IMAGE and save a config snapshot

IMG_PATH = SOURCE_IMAGE
DONE_MODE = False

if len(sys.argv) >= 2:
    if sys.argv[1].lower() == "done":
        DONE_MODE = True
    else:
        IMG_PATH = sys.argv[1]

if len(sys.argv) >= 3 and sys.argv[2].lower() == "done":
    DONE_MODE = True

img = Image.open(IMG_PATH).convert("RGB")
arr = np.array(img)
W, H = img.size

# ── 1. Print a brightness map to help spot the dark pupil ────────────────────
print(f"Image: {IMG_PATH}")
print(f"Image size: {W}x{H}")
print("Brightness map (lower=darker, scan step=5px). Look for the '##' cluster — that's the pupil:")
for y in range(0, H, 5):
    row = ""
    for x in range(0, W, 5):
        r, g, b = arr[y, x]
        br = (int(r) + int(g) + int(b)) // 3
        if br < 60:
            row += "##"
        elif br < 110:
            row += ".."
        else:
            row += "  "
    print(f"y={y:3d}: {row}")

# ── 2. Draw an overlay so you can check the guesses visually ─────────────────
debug = img.copy().convert("RGBA")
overlay = Image.new("RGBA", debug.size, (0, 0, 0, 0))
draw = ImageDraw.Draw(overlay)

# Optional coordinate grid
if SHOW_GRID:
    try:
        font = ImageFont.load_default()
    except Exception:
        font = None

    for x in range(0, W + 1, GRID_SPACING):
        line_num = x // GRID_SPACING
        is_major = (line_num % GRID_MAJOR_EVERY == 0)
        color = GRID_MAJOR_COLOR if is_major else GRID_MINOR_COLOR
        width = 2 if is_major else 1
        draw.line([(x, 0), (x, H)], fill=color, width=width)
        if x != 0:
            draw.text((x + 2, 2), str(x), fill=GRID_LABEL_COLOR, font=font)

    for y in range(0, H + 1, GRID_SPACING):
        line_num = y // GRID_SPACING
        is_major = (line_num % GRID_MAJOR_EVERY == 0)
        color = GRID_MAJOR_COLOR if is_major else GRID_MINOR_COLOR
        width = 2 if is_major else 1
        draw.line([(0, y), (W, y)], fill=color, width=width)
        if y != 0:
            draw.text((2, y + 2), str(y), fill=GRID_LABEL_COLOR, font=font)

# Iris
draw.ellipse(
    [GUESS_IRIS_CX - GUESS_IRIS_R, GUESS_IRIS_CY - GUESS_IRIS_R,
     GUESS_IRIS_CX + GUESS_IRIS_R, GUESS_IRIS_CY + GUESS_IRIS_R],
    outline="red", width=1
)
draw.ellipse([GUESS_IRIS_CX-3, GUESS_IRIS_CY-3, GUESS_IRIS_CX+3, GUESS_IRIS_CY+3], fill="red")

# Eyelid corners
draw.ellipse([GUESS_LID_LEFT[0]-3, GUESS_LID_LEFT[1]-3,
              GUESS_LID_LEFT[0]+3, GUESS_LID_LEFT[1]+3], fill="lime")
draw.ellipse([GUESS_LID_RIGHT[0]-3, GUESS_LID_RIGHT[1]-3,
              GUESS_LID_RIGHT[0]+3, GUESS_LID_RIGHT[1]+3], fill="lime")
              
              # Eyelid color sample point
draw.ellipse(
    [
        EYELID_SAMPLE_X - 3,
        EYELID_SAMPLE_Y - 3,
        EYELID_SAMPLE_X + 3,
        EYELID_SAMPLE_Y + 3,
    ],
    fill="cyan",
    outline="black",
)

# Eyelid arc: quadratic Bezier between corners through the control point y
pts = []
for x in range(GUESS_LID_LEFT[0], GUESS_LID_RIGHT[0] + 1):
    t = (x - GUESS_LID_LEFT[0]) / (GUESS_LID_RIGHT[0] - GUESS_LID_LEFT[0])
    y = ((1 - t) ** 2 * GUESS_LID_LEFT[1]
         + 2 * (1 - t) * t * GUESS_LID_CTRL_Y
         + t ** 2 * GUESS_LID_RIGHT[1])
    pts.append((x, int(y)))
for point in pts:
    draw.point(point, fill="yellow")

debug = Image.alpha_composite(debug, overlay).convert("RGB")
debug.save(LANDMARK_CHECK_IMAGE)
print(f"	Saved {LANDMARK_CHECK_IMAGE} — open it and check:")
print("  RED circle    = iris guess")
print("  GREEN dots    = eyelid corners")
print("  YELLOW curve  = eyelid arc")
print(f"  CYAN/WHITE grid = {GRID_SPACING}px coordinate grid")
print("Adjust eye_config.py and re-run until it lines up.")
print("Then run make_eye_animation.py; it will use the same eye_config.py values.")
print("but be sure to do")
print("python3 find_eye_landmarks_shared_config.py done")
print("once you've got it right, to create the text file with the config.")

if DONE_MODE:
    from pathlib import Path

    img_stem = Path(IMG_PATH).stem
    config_out = f"{img_stem}_config.txt"

    with open(config_out, "w", encoding="utf-8") as f:
        f.write("# Saved eye landmark configuration\n")
        f.write(f"# Source image: {IMG_PATH}\n\n")

        f.write(f'SOURCE_IMAGE = "{IMG_PATH}"\n')
        f.write(f'LANDMARK_CHECK_IMAGE = "{LANDMARK_CHECK_IMAGE}"\n\n')

        f.write(f"GUESS_IRIS_CX = {GUESS_IRIS_CX}\n")
        f.write(f"GUESS_IRIS_CY = {GUESS_IRIS_CY}\n")
        f.write(f"GUESS_IRIS_R  = {GUESS_IRIS_R}\n\n")

        f.write(f"GUESS_LID_LEFT  = {GUESS_LID_LEFT}\n")
        f.write(f"GUESS_LID_RIGHT = {GUESS_LID_RIGHT}\n")
        f.write(f"GUESS_LID_CTRL_Y = {GUESS_LID_CTRL_Y}\n\n")

        f.write(f"SHOW_GRID = {SHOW_GRID}\n")
        f.write(f"GRID_SPACING = {GRID_SPACING}\n")
        f.write(f"GRID_MINOR_COLOR = {GRID_MINOR_COLOR!r}\n")
        f.write(f"GRID_MAJOR_COLOR = {GRID_MAJOR_COLOR!r}\n")
        f.write(f"GRID_LABEL_COLOR = {GRID_LABEL_COLOR!r}\n")
        f.write(f"GRID_MAJOR_EVERY = {GRID_MAJOR_EVERY}\n")
        
        f.write(f"EYELID_SAMPLE_X = {EYELID_SAMPLE_X}\n")
        f.write(f"EYELID_SAMPLE_Y = {EYELID_SAMPLE_Y}\n\n")

    print(f"Saved final config: {config_out}")
