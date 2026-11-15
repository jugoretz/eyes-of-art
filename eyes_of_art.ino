/*
  eyes_of_art.ino

  Authors:
    Joseph Ugoretz
    Beth Harris

  Project:
    Eyes of Art

  Description:
    Reads an artwork playlist from the SD card, plays the matching eye GIF on
    the round TFT display, and shows the matching artist/title/date on a small
    SSD1306 OLED wall-label display.

  SD card expected structure:

    /eyes/playlist.txt
    /eyes/ringgold.gif
    /eyes/renoir.gif
    etc.

  playlist.txt format:

    filename.gif|Artist Name|Artwork Title|Date

  Lines beginning with # are ignored.
*/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include <AnimatedGIF.h>
#include <U8g2lib.h>
#include <Adafruit_NeoPixel.h>



// ============================================================================
// HARDWARE PIN DEFINITIONS
// ============================================================================

// TFT chip select.

// Other TFT SPI pins are configured in TFT_eSPI's User_Setup file.
//we're using <User_Setups/three_eyes_sdcard_esp32s3_GC9A01.h>
//which has:
//#define TFT_RST 4	// Reset
//#define TFT_CS   -1 // Chip Select (-1 because we're defining it below)
//#define TFT_BL -1 //Backlight (-1 because these displays don't have that pin)
//#define TFT_DC   2 // Data/Command
//#define TFT_MOSI 9 // SPI Data/SDA
//#define TFT_SCLK 8	// SPI Clock/CLK

#define TFT_CS   39

// SD card SPI pins.
// These are custom SPI pins passed to SPI.begin().
#define SD_SCLK  8
#define SD_MISO  7
#define SD_MOSI  9
#define SD_CS    21

// Main advance button.
// Wired between GPIO 6 and GND.
// Uses INPUT_PULLUP, so pressed = LOW.
#define BUTTON_PIN 6

// OLED I2C pins.
// Important: do NOT use GPIO 7, 8, or 9 here.
// Those are already used by the SD card.
#define OLED_SDA 17
#define OLED_SCL 18

// ============================================================================
// ONBOARD LED SETUP
// ============================================================================

#define LED_PIN   48
#define NUM_LEDS  1

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ============================================================================
// ONBOARD LED BRIGHTNESS
// ============================================================================
// This is the global NeoPixel brightness limit.
// ============================================================================

const uint8_t MASTER_BRIGHTNESS = 10;



// ============================================================================
// PLAYLIST LIMITS
// ============================================================================

// Maximum number of artwork records to load from /eyes/playlist.txt.
// Increase only if needed. Each item uses RAM.
#define MAX_ITEMS      80

// Maximum stored path length, including "/eyes/".
// Example: "/eyes/ringgold.gif"
#define MAX_PATH_LEN   64

// These lengths are intentionally generous because art-history wall labels
// often get long, especially titles.
#define MAX_ARTIST_LEN 96
#define MAX_TITLE_LEN  160
#define MAX_DATE_LEN   48

struct Artwork {
  char gifPath[MAX_PATH_LEN];
  char artist[MAX_ARTIST_LEN];
  char title[MAX_TITLE_LEN];
  char date[MAX_DATE_LEN];
};

Artwork playlist[MAX_ITEMS];

uint8_t playlistCount = 0;
uint8_t currentItem = 0;

// ============================================================================
// DISPLAY OBJECTS
// ============================================================================

TFT_eSPI tft = TFT_eSPI();
AnimatedGIF gif;

// AnimatedGIF uses callback functions for file access.
// This File object is used by those callbacks.
File gifFile;

// 128x64 SSD1306 OLED using hardware I2C.
// Wire.begin(OLED_SDA, OLED_SCL) in setup() assigns the actual pins.
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

// ============================================================================
// OLED WALL-LABEL TUNING
// ============================================================================

const int OLED_SCREEN_WIDTH = 128;

// Font choices.
// These are good starting fonts for a museum-wall-label look.
// Larger fonts are easier to read but leave less vertical room.
const uint8_t *smallFont = u8g2_font_ncenB08_tf;
const uint8_t *titleFont = u8g2_font_ncenB10_tf;

// Baseline positions for the three OLED lines.
// These are y baseline values, not top-left y positions.
const int ARTIST_Y = 10;
const int TITLE_Y  = 36;
const int DATE_Y   = 62;

// How long the label pauses at the beginning before scrolling.
// Increase for a calmer feel.
const unsigned long START_PAUSE_MS = 1500;

// How long the label pauses at the fully scrolled end position.
// Increase if you want the final readable position to linger longer.
const unsigned long END_PAUSE_MS = 1200;

// OLED animation frame interval.
// Smaller = faster scrolling.
// Larger = slower scrolling.
// 30 ms is about 33 updates/second.
const unsigned long OLED_FRAME_MS = 30;

// OLED scroll state machine.
enum LabelState {
  START_PAUSE,
  SCROLLING,
  END_PAUSE
};

LabelState labelState = START_PAUSE;

unsigned long stateStartTime = 0;
unsigned long lastOLEDFrameTime = 0;

// Final x positions for each line.
// A value of 0 means no scroll is needed.
// A negative value means the text scrolls left until its right edge is visible.
int artistFinalX = 0;
int titleFinalX = 0;
int dateFinalX = 0;

// The longest needed scroll distance among artist/title/date.
// All long lines are mapped to this same scroll duration so they start and
// finish together.
int longestDistance = 0;

int scrollStep = 0;

// ============================================================================
// GIF PLAYBACK TUNING
// ============================================================================

// Cap very long GIF frame delays.
// This keeps the embedded display from feeling sluggish even if the source GIF
// contains long pauses.
const int MAX_GIF_DELAY_MS = 80;

// Minimum delay between GIF frames.
// If this is too low, playback may run too fast or starve other tasks.
const int MIN_GIF_DELAY_MS = 10;

// SD card SPI speed.
// 25 MHz has tested better than 8 MHz while remaining stable in this setup.
// If SD mounting becomes unreliable, try 16000000, 12000000, or 8000000.
const uint32_t SD_SPI_SPEED = 25000000;

// ============================================================================
// BUTTON DEBOUNCE
// ============================================================================

// Debounce time for the physical button.
// 30 ms is a normal starting value.
const unsigned long DEBOUNCE_MS = 30;

bool buttonPressedEvent() {
  static bool lastReading = HIGH;
  static bool stableState = HIGH;
  static unsigned long lastChangeTime = 0;

  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastReading) {
    lastChangeTime = millis();
    lastReading = reading;
  }

  if ((millis() - lastChangeTime) > DEBOUNCE_MS) {
    if (reading != stableState) {
      stableState = reading;

      if (stableState == LOW) {
        return true;
      }
    }
  }

  return false;
}

// ============================================================================
// SMALL UTILITY FUNCTIONS
// ============================================================================

void copyToCharArray(char *dest, size_t destSize, const String &src) {
  strncpy(dest, src.c_str(), destSize - 1);
  dest[destSize - 1] = '\0';
}

// ============================================================================
// PLAYLIST LOADING
// ============================================================================

bool loadPlaylist() {
  File playlistFile = SD.open("/eyes/playlist.txt");

  if (!playlistFile) {
    Serial.println("Could not open /eyes/playlist.txt");
    return false;
  }

  playlistCount = 0;

  while (playlistFile.available() && playlistCount < MAX_ITEMS) {
    String line = playlistFile.readStringUntil('\n');
    line.trim();

    if (line.length() == 0) continue;
    if (line.startsWith("#")) continue;

    int pipe1 = line.indexOf('|');
    int pipe2 = line.indexOf('|', pipe1 + 1);
    int pipe3 = line.indexOf('|', pipe2 + 1);

    if (pipe1 < 0 || pipe2 < 0 || pipe3 < 0) {
      Serial.print("Skipping malformed line: ");
      Serial.println(line);
      continue;
    }

    String filename = line.substring(0, pipe1);
    String artist   = line.substring(pipe1 + 1, pipe2);
    String title    = line.substring(pipe2 + 1, pipe3);
    String date     = line.substring(pipe3 + 1);

    filename.trim();
    artist.trim();
    title.trim();
    date.trim();

    if (filename.length() == 0) continue;

    String fullPath = "/eyes/" + filename;

    if (fullPath.length() >= MAX_PATH_LEN) {
      Serial.print("Skipping too-long GIF path: ");
      Serial.println(fullPath);
      continue;
    }

    copyToCharArray(playlist[playlistCount].gifPath, MAX_PATH_LEN, fullPath);
    copyToCharArray(playlist[playlistCount].artist, MAX_ARTIST_LEN, artist);
    copyToCharArray(playlist[playlistCount].title, MAX_TITLE_LEN, title);
    copyToCharArray(playlist[playlistCount].date, MAX_DATE_LEN, date);

    Serial.print("Loaded: ");
    Serial.print(playlist[playlistCount].gifPath);
    Serial.print(" | ");
    Serial.print(playlist[playlistCount].artist);
    Serial.print(" | ");
    Serial.print(playlist[playlistCount].title);
    Serial.print(" | ");
    Serial.println(playlist[playlistCount].date);

    playlistCount++;
  }

  playlistFile.close();

  Serial.print("Total playlist items loaded: ");
  Serial.println(playlistCount);

  return playlistCount > 0;
}

// ============================================================================
// SHARED SPI BUS SELECTION
// ============================================================================

void selectDisplay() {
  digitalWrite(SD_CS, HIGH);
  digitalWrite(TFT_CS, LOW);
}

void deselectDisplay() {
  digitalWrite(TFT_CS, HIGH);
}

// ============================================================================
// AnimatedGIF FILE CALLBACKS
// ============================================================================

void *GIFOpenFile(const char *fname, int32_t *pSize) {
  gifFile = SD.open(fname);

  if (gifFile) {
    *pSize = gifFile.size();
    return (void *)&gifFile;
  }

  return NULL;
}

void GIFCloseFile(void *pHandle) {
  File *f = static_cast<File *>(pHandle);
  if (f) f->close();
}

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  File *f = static_cast<File *>(pFile->fHandle);
  if (!f) return 0;

  int32_t bytesRemaining = pFile->iSize - pFile->iPos;

  if (iLen > bytesRemaining) {
    iLen = bytesRemaining;
  }

  if (iLen <= 0) return 0;

  int32_t bytesRead = f->read(pBuf, iLen);
  pFile->iPos = f->position();

  return bytesRead;
}

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
  File *f = static_cast<File *>(pFile->fHandle);
  if (!f) return 0;

  f->seek(iPosition);
  pFile->iPos = f->position();

  return pFile->iPos;
}

// ============================================================================
// GIF DRAWING TO TFT
// ============================================================================

void GIFDraw(GIFDRAW *pDraw) {
  uint8_t *src = pDraw->pPixels;
  uint16_t *palette = pDraw->pPalette;

  int16_t x = pDraw->iX;
  int16_t y = pDraw->iY + pDraw->y;
  int16_t w = pDraw->iWidth;

  static uint16_t lineBuf[240];

  if (y < 0 || y >= 240) return;
  if (x >= 240) return;
  if (x + w > 240) w = 240 - x;

  if (!pDraw->ucHasTransparency) {
    for (int i = 0; i < w; i++) {
      lineBuf[i] = palette[src[i]];
    }

    selectDisplay();
    tft.pushImage(x, y, w, 1, lineBuf);
    deselectDisplay();
    return;
  }

  uint8_t transparent = pDraw->ucTransparent;
  int runStart = -1;
  int runLength = 0;

  for (int i = 0; i < w; i++) {
    if (src[i] == transparent) {
      if (runLength > 0) {
        selectDisplay();
        tft.pushImage(x + runStart, y, runLength, 1, lineBuf);
        deselectDisplay();

        runStart = -1;
        runLength = 0;
      }
    } else {
      if (runStart < 0) {
        runStart = i;
      }

      lineBuf[runLength] = palette[src[i]];
      runLength++;
    }
  }

  if (runLength > 0) {
    selectDisplay();
    tft.pushImage(x + runStart, y, runLength, 1, lineBuf);
    deselectDisplay();
  }
}

bool openCurrentGIF() {
  gif.close();

  Serial.print("Opening GIF: ");
  Serial.println(playlist[currentItem].gifPath);

  if (!gif.open(playlist[currentItem].gifPath,
                GIFOpenFile,
                GIFCloseFile,
                GIFReadFile,
                GIFSeekFile,
                GIFDraw)) {
    Serial.println("Could not open GIF.");
    return false;
  }

  gif.reset();
  return true;
}

// ============================================================================
// OLED WALL-LABEL DRAWING
// ============================================================================

void drawSmartLine(const char *text, int y, const uint8_t *font, int scrollX) {
  u8g2.setFont(font);

  int w = u8g2.getUTF8Width(text);
  int x;

  if (w <= OLED_SCREEN_WIDTH) {
    x = (OLED_SCREEN_WIDTH - w) / 2;
  } else {
    x = scrollX;
  }

  u8g2.drawUTF8(x, y, text);
}

void drawLabel(int artistX, int titleX, int dateX) {
  const Artwork &art = playlist[currentItem];

  u8g2.clearBuffer();

  drawSmartLine(art.artist, ARTIST_Y, smallFont, artistX);
  drawSmartLine(art.title, TITLE_Y, titleFont, titleX);
  drawSmartLine(art.date, DATE_Y, smallFont, dateX);

  u8g2.sendBuffer();
}

void prepareCurrentArtworkLabel() {
  const Artwork &art = playlist[currentItem];

  u8g2.setFont(smallFont);
  int artistWidth = u8g2.getUTF8Width(art.artist);

  u8g2.setFont(titleFont);
  int titleWidth = u8g2.getUTF8Width(art.title);

  u8g2.setFont(smallFont);
  int dateWidth = u8g2.getUTF8Width(art.date);

  artistFinalX = min(0, OLED_SCREEN_WIDTH - artistWidth);
  titleFinalX  = min(0, OLED_SCREEN_WIDTH - titleWidth);
  dateFinalX   = min(0, OLED_SCREEN_WIDTH - dateWidth);

  longestDistance = max(
    abs(artistFinalX),
    max(abs(titleFinalX), abs(dateFinalX))
  );

  scrollStep = 0;
  labelState = START_PAUSE;
  stateStartTime = millis();
  lastOLEDFrameTime = millis();

  drawLabel(0, 0, 0);
}

void updateLabelAnimation() {
  unsigned long now = millis();

  if (labelState == START_PAUSE) {
    if (now - stateStartTime >= START_PAUSE_MS) {
      if (longestDistance == 0) {
        stateStartTime = now;
        drawLabel(0, 0, 0);
      } else {
        labelState = SCROLLING;
        scrollStep = 0;
        lastOLEDFrameTime = now;
      }
    }

    return;
  }

  if (labelState == SCROLLING) {
    if (now - lastOLEDFrameTime >= OLED_FRAME_MS) {
      lastOLEDFrameTime = now;

      int artistX = map(scrollStep, 0, longestDistance, 0, artistFinalX);
      int titleX  = map(scrollStep, 0, longestDistance, 0, titleFinalX);
      int dateX   = map(scrollStep, 0, longestDistance, 0, dateFinalX);

      drawLabel(artistX, titleX, dateX);

      scrollStep++;

      if (scrollStep > longestDistance) {
        labelState = END_PAUSE;
        stateStartTime = now;
      }
    }

    return;
  }

  if (labelState == END_PAUSE) {
    if (now - stateStartTime >= END_PAUSE_MS) {
      labelState = START_PAUSE;
      stateStartTime = now;
      scrollStep = 0;
      drawLabel(0, 0, 0);
    }

    return;
  }
}

// ============================================================================
// ARTWORK ADVANCE
// ============================================================================

void advanceArtwork() {
  currentItem++;

  if (currentItem >= playlistCount) {
    currentItem = 0;
  }

  prepareCurrentArtworkLabel();
  openCurrentGIF();
}

// During GIF frame delays, keep the OLED animation and button responsive.
void waitWithOLED(unsigned long waitMs) {
  unsigned long start = millis();

  while (millis() - start < waitMs) {
    if (buttonPressedEvent()) {
      advanceArtwork();
      return;
    }

    updateLabelAnimation();
    delay(1);
  }
}

// ============================================================================
// AUTOMATIC ARTWORK ADVANCE
// ============================================================================

//uncomment the interval we want or devise a different one here
// The UL suffix keeps the arithmetic in unsigned long range.

const unsigned long AUTO_ADVANCE_INTERVAL_MS =
//10UL * 1000UL;                    //10 seconds
// 30UL * 1000UL;                  // 30 seconds
   5UL * 60UL * 1000UL;           //5 minutes
//24UL * 60UL * 60UL * 1000UL;   //24 hours

// Records the scheduled automatic-advance timeline.
// Button presses do NOT alter this value.
unsigned long lastAutoAdvanceTime = 0;


// ============================================================================
// DEBUGGING SD MOUNT
// ============================================================================
void printSDDebug() {
  Serial.println("\n--- SD DEBUG ---");
  Serial.printf("SCLK=%d MISO=%d MOSI=%d CS=%d\n", SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  Serial.printf("TFT_CS=%d\n", TFT_CS);

  Serial.println("Trying SD at 25 MHz...");
  if (!SD.begin(SD_CS, SPI, 25000000)) {
    Serial.println("25 MHz failed.");

    Serial.println("Trying SD at 10 MHz...");
    if (!SD.begin(SD_CS, SPI, 10000000)) {
      Serial.println("10 MHz failed.");

      Serial.println("Trying SD at 4 MHz...");
      if (!SD.begin(SD_CS, SPI, 4000000)) {
        Serial.println("4 MHz failed.");
        Serial.println("SD mount failed at all tested speeds.");
        return;
      }
    }
  }

  Serial.println("SD mounted.");

  uint8_t type = SD.cardType();
  Serial.print("Card type: ");
  if (type == CARD_NONE) Serial.println("NONE");
  else if (type == CARD_MMC) Serial.println("MMC");
  else if (type == CARD_SD) Serial.println("SDSC");
  else if (type == CARD_SDHC) Serial.println("SDHC/SDXC");
  else Serial.println("UNKNOWN");

  Serial.printf("Card size: %llu MB\n", SD.cardSize() / (1024 * 1024));

  Serial.print("Checking /eyes: ");
  Serial.println(SD.exists("/eyes") ? "exists" : "missing");

  Serial.print("Checking /eyes/playlist.txt: ");
  Serial.println(SD.exists("/eyes/playlist.txt") ? "exists" : "missing");

  File root = SD.open("/");
  if (root) {
    Serial.println("Root directory:");
    File f;
    while ((f = root.openNextFile())) {
      Serial.print("  ");
      Serial.println(f.name());
      f.close();
    }
    root.close();
  }

  Serial.println("--- END SD DEBUG ---\n");
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Starting eyes_of_art with OLED wall label...");

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);

  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

printSDDebug();

if (SD.cardType() == CARD_NONE) {
  Serial.println("Stopping because no SD card mounted.");
  while (true) delay(1000);
}

  Wire.begin(OLED_SDA, OLED_SCL);
  u8g2.begin();
  u8g2.setContrast(255);

// Start the onboard RGB NeoPixel.
strip.begin();

// Global brightness limit for that onboard Neopixel.
strip.setBrightness(MASTER_BRIGHTNESS);

// Set that single onboard pixel to desired color.
strip.setPixelColor(0, strip.Color(85, 10, 255)); //purple
strip.show();

  if (!loadPlaylist()) {
    Serial.println("No playlist items loaded.");
    while (true) delay(1000);
  }
  
  selectDisplay();
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  deselectDisplay();


  gif.begin(BIG_ENDIAN_PIXELS);

  prepareCurrentArtworkLabel();

  if (!openCurrentGIF()) {
  while (true) delay(1000);
}

Serial.println("GIF opened.");

// Start the independent 24-hour timer from successful startup.
// Manual button presses will not reset this timer.
lastAutoAdvanceTime = millis();

  Serial.println("GIF opened.");
}

void checkAutomaticAdvance() {
  unsigned long now = millis();

  if (now - lastAutoAdvanceTime >= AUTO_ADVANCE_INTERVAL_MS) {
    // Advance by exactly one scheduled interval rather than resetting to now.
    // This keeps the timer anchored to the original startup schedule.
    lastAutoAdvanceTime += AUTO_ADVANCE_INTERVAL_MS;

    Serial.println("24-hour timer: advancing artwork.");
    advanceArtwork();
  }
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  //manual advance with the button.  this does not affect the timer.
  if (buttonPressedEvent()) {
    advanceArtwork();
  }

// Independent scheduled advance based only on time since startup. not affected by the button
  checkAutomaticAdvance();
  updateLabelAnimation();

  int delayMs = 0;
  int result = gif.playFrame(false, &delayMs);

  if (result == 1) {
   if (delayMs > MAX_GIF_DELAY_MS) delayMs = MAX_GIF_DELAY_MS;
    if (delayMs < MIN_GIF_DELAY_MS) delayMs = MIN_GIF_DELAY_MS;

    waitWithOLED(delayMs);
    return;
  }

  gif.reset();
  waitWithOLED(10);
}