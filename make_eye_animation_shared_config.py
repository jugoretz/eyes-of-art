"""
make_eye_animation.py

STEP 2 of the eye-animation workflow.

Reads the shared landmark values from eye_config.py and produces an
animated GIF: subtle gaze shifts left/right and two blinks, with the
eyelid following the real curve of the painted/photographed lid.

Usage:
    python3 make_eye_animation.py
"""

from PIL import Image, ImageDraw, ImageFilter
import numpy as np

from eye_config import (
    SOURCE_IMAGE,
    OUTPUT_GIF,
    GUESS_IRIS_CX,
    GUESS_IRIS_CY,
    GUESS_IRIS_R,
    GUESS_LID_LEFT,
    GUESS_LID_RIGHT,
    GUESS_LID_CTRL_Y,
    LOWER_LID_Y,
    GAZE_SHIFT_X,
    GAZE_SHIFT_Y_ON_LEFT,
    #EYELID_SAMPLE_X,
    #EYELID_SAMPLE_Y,
)

# Use shorter internal names in the animation code.
EYE_CX, EYE_CY = GUESS_IRIS_CX, GUESS_IRIS_CY
IRIS_R = GUESS_IRIS_R
LID_L = GUESS_LID_LEFT
LID_R = GUESS_LID_RIGHT
LID_CTRL_Y = GUESS_LID_CTRL_Y

src = Image.open(SOURCE_IMAGE).convert("RGB")
W, H = src.size


def lid_y_at(x):
    """Resting/open-eye upper lid y-coordinate at pixel column x."""
    if x < LID_L[0] or x > LID_R[0]:
        return None
    t = (x - LID_L[0]) / (LID_R[0] - LID_L[0])
    y = (1 - t) ** 2 * LID_L[1] + 2 * (1 - t) * t * LID_CTRL_Y + t ** 2 * LID_R[1]
    return y


# Grab the iris patch once, to paste at shifted positions later.
# PATCH_R should be only a little larger than the iris radius. If the iris
# circle in eye_config.py is too large, the animation will move too much skin/eye.
PATCH_R = IRIS_R + 5
iris_box = (EYE_CX - PATCH_R, EYE_CY - PATCH_R, EYE_CX + PATCH_R, EYE_CY + PATCH_R)
orig_iris = src.crop(iris_box)


def paste_iris(base_img, cx, cy):
    out = base_img.copy()
    pw, ph = orig_iris.size
    mask = Image.new("L", (pw, ph), 0)
    draw = ImageDraw.Draw(mask)
    draw.ellipse([3, 3, pw - 3, ph - 3], fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(3))
    paste_x = cx - pw // 2
    paste_y = cy - ph // 2
    out.paste(orig_iris, (paste_x, paste_y), mask)
    return out


# Tunable eyelid texture settings.
# This samples a thin strip ABOVE the resting eyelid curve and stretches that
# strip downward as the eyelid closes.
EYELID_STRIP_HEIGHT = 18   # taller = more varied texture
EYELID_STRIP_GAP = 4       # distance above lid before sampling starts


def draw_eyelid(img, close_frac):
    """close_frac: 0.0 = open, 1.0 = fully closed."""
    if close_frac <= 0:
        return img.copy()

    out = img.copy()
    draw = ImageDraw.Draw(out)
    arr = np.array(img)

    for x in range(LID_L[0], LID_R[0] + 1):
        rest_y = lid_y_at(x)
        if rest_y is None:
            continue

        rest_y = int(rest_y)

        travel = LOWER_LID_Y - rest_y
        current_lid_bottom = rest_y + int(travel * close_frac)

        if current_lid_bottom <= rest_y:
            continue

        # Sample a vertical strip just above the upper eyelid.
        # This preserves local color variation across the painting instead of
        # using one flat color for the whole eyelid.
        sample_bottom = max(0, rest_y - EYELID_STRIP_GAP)
        sample_top = max(0, sample_bottom - EYELID_STRIP_HEIGHT)

        if sample_bottom <= sample_top:
            continue

        strip = arr[sample_top:sample_bottom, x]

        for y in range(rest_y, current_lid_bottom + 1):
            if y < 0 or y >= H:
                continue

            # Map the closing-lid vertical area onto the sampled strip.
            # This stretches the strip downward as the eyelid closes.
            t = (y - rest_y) / max(current_lid_bottom - rest_y, 1)
            strip_index = int(t * (len(strip) - 1))

            color = tuple(int(v) for v in strip[strip_index])
            draw.point((x, y), fill=color)

    return out


def make_frame(shift_x=0, shift_y=0, blink=0.0):
    frame = src.copy()
    frame = paste_iris(frame, EYE_CX + shift_x, EYE_CY + shift_y)
    frame = draw_eyelid(frame, blink)
    return frame


# ── Keyframes: (shift_x, shift_y, blink, duration_ms) ─────────────────────────
keyframes = [
    (0, 0, 0.0, 120),
    (0, 0, 0.0, 120),
    (0, 0, 0.0, 120),
    (-GAZE_SHIFT_X, GAZE_SHIFT_Y_ON_LEFT, 0.0, 70),
    (-GAZE_SHIFT_X, GAZE_SHIFT_Y_ON_LEFT, 0.0, 130),
    (-GAZE_SHIFT_X, GAZE_SHIFT_Y_ON_LEFT, 0.0, 130),
    (0, 0, 0.0, 70),
    (0, 0, 0.0, 120),
    (GAZE_SHIFT_X, 0, 0.0, 70),
    (GAZE_SHIFT_X, 0, 0.0, 130),
    (GAZE_SHIFT_X, 0, 0.0, 130),
    (0, 0, 0.0, 70),
    (0, 0, 0.0, 100),

    # First quick blink
    (0, 0, 0.55, 25),
    (0, 0, 1.00, 30),
    (0, 0, 0.55, 25),
    (0, 0, 0.00, 30),

    # Second quick blink
    (0, 0, 0.60, 25),
    (0, 0, 1.00, 30),
    (0, 0, 0.60, 25),
    (0, 0, 0.00, 30),

    (0, 0, 0.0, 120),
]

frames = []
durations = []
for sx, sy, bl, dur in keyframes:
    frames.append(make_frame(sx, sy, bl))
    durations.append(dur)

print(f"Generated {len(frames)} frames")

# Reduce each frame to a 64-color adaptive palette.

# This often makes the GIF smaller and may reduce the amount of work

# the ESP32 has to do when decoding.

frames = [

    f.convert("P", palette=Image.ADAPTIVE, colors=64)

    for f in frames

]

frames[0].save(
    OUTPUT_GIF,
    save_all=True,
    append_images=frames[1:],
    duration=durations,
    loop=0,
    optimize=True,
    disposal=2,
)
print(f"Saved: {OUTPUT_GIF}")
