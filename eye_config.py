"""
eye_config.py

Shared settings for the eye landmark/animation workflow.

Edit this file for each cropped eye image. Both find_eye_landmarks.py
and make_eye_animation.py read these same values, so you do not have to
copy numbers from one script to the other.
"""

# Source/output files
SOURCE_IMAGE = "vanderweyden_3.png"
OUTPUT_GIF   = "vanderweyden_3.gif"
LANDMARK_CHECK_IMAGE = "landmarks_check.png"

# Iris center and radius
GUESS_IRIS_CX = 120
GUESS_IRIS_CY = 112
GUESS_IRIS_R  = 22

# Eyelid corners: where the upper lid crease meets the eye at left & right
GUESS_LID_LEFT  = (35, 125)
GUESS_LID_RIGHT = (200, 95)

# Bezier control point y: pulls the lid arc upward/downward.
# The curve does NOT pass exactly through this y value.
# Lower number = higher arc.
GUESS_LID_CTRL_Y = 20

# Lower lid approximation used by the blink animation
LOWER_LID_Y = 160

# Gaze shift amount in pixels
GAZE_SHIFT_X = 8
GAZE_SHIFT_Y_ON_LEFT = 1

# Sample point used to determine the eyelid fill color.
# Pick a representative patch of upper eyelid skin/paint.
# We will average the area around that spot
EYELID_SAMPLE_X = 110
EYELID_SAMPLE_Y = 65

# Grid overlay settings for find_eye_landmarks.py
SHOW_GRID = True
GRID_SPACING = 20
GRID_MINOR_COLOR = (255, 255, 255, 90)
GRID_MAJOR_COLOR = (0, 255, 255, 130)
GRID_LABEL_COLOR = (0, 255, 255, 220)
GRID_MAJOR_EVERY = 5
