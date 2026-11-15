<img width="1024" height="768" alt="001 2026-07-13 12 30 40" src="https://github.com/user-attachments/assets/f6402e24-87a8-4fb8-baac-bcdc2a7972d2" />

# Eyes of Art

https://youtu.be/fsqjK1riPuA

(buy me a coffee, if you'd like to! https://buymeacoffee.com/jugoretz)

An ESP32-S3 artwork display by **Joseph Ugoretz** (inspired by Beth Harris). A round TFT plays animated eyes from artworks while a small OLED shows the artist, title, and date. A button advances to the next artwork; automatic advance runs every **five minutes** on an independent schedule.

The project is assembled in a shadowbox, with wiring and processor exposed and visible.  It adds to the "Mad Scientist" vibe.

## Full list of Artworks

For this project, we used 36 works of art.  Artists, titles, and dates are recorded in the file "Eye_Playlist_Catalog.pdf" above.

## Hardware and wiring

The build uses an **ESP32-S3 N16R8** (16 MB flash, 8 MB PSRAM), **1.28-inch GC9A01 TFT displays**, and a **0.96-inch SSD1306 I²C OLED (128 × 64 pixels)**. The supplied sketch drives three 240 × 240 round TFTs and one OLED. It also uses a microSD SPI module, a normally open button, and one onboard RGB NeoPixel. 

| Connection | ESP32-S3 GPIO | Notes |
| --- | --- | --- |
| TFT SCLK / SD SCK | 8 | Shared SPI clock |
| TFT MOSI (sometimes SDA) / SD MOSI | 9 | Shared SPI data |
| SD MISO | 7 | SD return data |
| TFT CS | 39 | Controlled manually by the sketch (all displays are identical) |
| TFT DC | 2 | From supplied TFT setup |
| TFT reset | 4 | From supplied TFT setup |
| SD CS | 21 | Separate from TFT CS |
| OLED SDA | 17 | I²C |
| OLED SCL | 18 | I²C |
| Advance button | 6 | Button between GPIO and GND; internal pull-up |
| Onboard RGB LED | 48 | Board-dependent; GRB NeoPixel |

All modules need a common ground. Use 3.3 V-compatible logic and consult each module's power specifications. The original TFT has no separate backlight pin. Do not use GPIO 7, 8, or 9 for the OLED.

## Arduino setup

1. Install version **2.0.14** of the Espressif **esp32** board package in Arduino IDE (the supplied display setup explicitly calls for this version) and select the exact ESP32-S3 board and upload port. The hardware is N16R8 (16 MB flash, 8 MB PSRAM); confirm the matching board settings. The tested PSRAM mode, partition scheme, and USB settings are not recorded yet.
2. Install these Arduino Library Manager dependencies:
   - [TFT_eSPI by Bodmer](https://github.com/Bodmer/TFT_eSPI)
   - [AnimatedGIF by Larry Bank](https://github.com/bitbank2/AnimatedGIF)
   - [U8g2 by olikraus](https://github.com/olikraus/u8g2)
   - [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
   `Arduino`, `SPI`, `SD`, and `Wire` come with the board package. Tested library versions are still to be recorded.
3. Configure TFT_eSPI at library build time. Copy the supplied `three_eyes_sdcard_esp32s3_GC9A01.h` into TFT_eSPI's `User_Setups` folder. In `User_Setup_Select.h`, disable other setup includes and select:

   ```cpp
   #include <User_Setups/three_eyes_sdcard_esp32s3_GC9A01.h>
   ```

   The supplied setup selects GC9A01 at 240 × 240, RGB color order, inversion on, and a **60 MHz TFT SPI clock**. It leaves TFT MISO undefined and `TFT_CS` at `-1`; the sketch controls GPIO 39 itself and configures SD MISO as GPIO 7. Preserve those settings for the shared SPI wiring. The setup's original compatibility note requires ESP32 core **2.0.14**; newer core versions have not been validated in this cleanup. Defining these settings only in the sketch does not configure the separately compiled library. Library updates may overwrite configuration.
4. Arduino requires the sketch folder to match its primary `.ino` filename. For this repository's `eyes-of-art` folder, copy `eyes_of_art.ino` into a separate folder named `eyes_of_art` and open that copy. Do not include legacy `playlist.h` in the sketch.
5. Prepare the SD card as below, insert it, upload, and open Serial Monitor at **115200 baud**.

## SD card and playlist

Use an SD card with a filesystem supported by the ESP32 SD library; FAT32 is a practical starting point. Create an `eyes` folder at the card root. Copy **the contents** of `eye_GIFs_ready_for_SD_card/` and the repository's `playlist.txt` into it:

```text
/eyes/
  playlist.txt
  ringgold.gif
  renoir.gif
  ...all remaining GIFs...
```

The UTF-8 playlist has one record per line, with four pipe-separated fields:

```text
filename.gif|Artist Name|Artwork Title|Date
```

Blank lines and lines beginning with `#` after trimming are ignored. Whitespace around fields is trimmed. There is no quoting or escaping for pipe characters; keep them out of field values. Files are relative to `/eyes/` and filenames must match the card. The loader accepts at most 80 records. Paths must fit in 63 bytes including `/eyes/`; artist, title, and date are truncated to 95, 159, and 47 bytes. UTF-8 uses multiple bytes for some characters, and glyph availability depends on the selected OLED font.

The supplied playlist contains 37 entries, adapted from the legacy header and matching the 37 GIF filenames. The original tab-separated `playlist.txt` is preserved as `artwork-catalog.tsv`; it has 36 artwork rows and is not a firmware playlist. `playlist.h` is an unused older compiled-in playlist, retained for reference; its metadata differs slightly from the catalog. Review art labels before publication.

## Operation and tuning

At startup the sketch mounts the card, loads the playlist, initializes the displays, and opens the first GIF. A short button press advances once, wrapping at the end. Each GIF loops until the next advance. Long OLED text scrolls; short text is centered. The onboard pixel stays dim purple.

`AUTO_ADVANCE_INTERVAL_MS` is currently five minutes. Button presses do not reset that schedule. The automatic timer is checked between decoded frames, so it may run slightly late. GIF delays are clamped to 10–80 ms; OLED and button updates continue during those waits. The TFT is 240 × 240, and GIFs should be prepared for that canvas. The renderer draws transparent runs over the existing screen; it does not implement every GIF disposal/compositing mode.

SD mounting tries 25 MHz, then 10 MHz, then 4 MHz. The older `SD_SPI_SPEED` constant is currently unused; changing it does not change those attempts. Keep TFT and SD chip selects separate because they share SPI.

## Optional animation-creation workflow

The Python tools locate eye landmarks and generate a looping GIF with left/right gaze shifts and two blinks. They are not needed for playback of the supplied GIFs.

```sh
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
python3 find_eye_landmarks_shared_config.py path/to/eye.png
python3 find_eye_landmarks_shared_config.py path/to/eye.png done
```

Run from the repository directory. Edit `eye_config.py` to adjust the iris, lid corners, curve, and sample point. The tool writes `landmarks_check.png`; `done` also saves a `<source-stem>_config.txt` snapshot. That snapshot contains selected landmark/grid settings, not every animation setting. The configured source PNG is not supplied. Provide your own 240 × 240 cropped eye image and set `SOURCE_IMAGE` and `OUTPUT_GIF` in `eye_config.py`. The generator reads that configuration directly; it does not read the saved `_config.txt` snapshot or take the landmark tool's image override. After checking the landmarks, run:

```sh
python3 make_eye_animation_shared_config.py
```

The generator preserves the source dimensions; it does not resize the image. Keep lid x coordinates inside the image and the left corner strictly left of the right. It creates 22 keyframes, quantizes each to 64 colors, and saves an optimized looping GIF (the encoder may merge identical frames). Blinks stretch strips sampled above the lid; `EYELID_SAMPLE_X/Y` are shown by the landmark tool but are not used by this generator. Adjust gaze, lower-lid position, and the generator's strip/keyframe settings as needed. Copy the generated GIF into `/eyes/` and add its metadata to the SD playlist. Firmware clamps the encoded frame delays, so on-device timing may differ from desktop playback.

The generator passed a smoke check with a synthetic 240 × 240 image using Pillow 12.2.0 and NumPy 2.4.6. Visual quality still needs checking with each real source image.

## Troubleshooting and validation

- **No SD mount:** check power, ground, SPI connections, CS, card formatting, and the serial speed-attempt messages.
- **No playlist items:** ensure `/eyes/playlist.txt` uses pipes, includes filenames, and is not the TSV catalog.
- **GIF will not open:** check its exact filename and that it was copied inside `/eyes/`. Failure on the first GIF stops startup; failures on later advances are logged but not skipped automatically.
- **Blank TFT:** check the selected TFT_eSPI setup, GC9A01 driver, pins, and power. Use the supplied `three_eyes_sdcard_esp32s3_GC9A01.h` and the core version noted above.
- **Blank OLED:** check SSD1306 compatibility, I²C address, SDA/SCL, and power. The sketch uses U8g2's default address.

Before publishing a tested release, record board/library versions and verify startup, GIF looping, long labels, button debounce, playlist wraparound, five-minute advance, and button presses near an automatic advance. Missing-card and missing-file behavior should also be checked. No hardware validation or successful firmware build has yet been performed during this cleanup.

## Credits and licensing

Project authors: Joseph Ugoretz and Beth Harris. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for dependency and artwork attribution status. Original project code and accompanying software documentation are licensed under the [MIT License](LICENSE). Third-party dependencies retain their own licenses. The authors report that all source artworks are public domain and that they created the GIF animations using their eye-landmark and animation scripts. Artwork images, GIFs, and the PDF catalog are outside the scope of the code license; no separate license for the animation contributions is asserted here. Source-image URLs and institutional credits have not yet been recorded.
