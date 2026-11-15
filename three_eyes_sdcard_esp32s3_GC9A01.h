//#################################################################
// Original project compatibility requirement: ESP32 Arduino core 2.0.14.
// Newer core versions have not been validated for this project. 
//#################################################################

#define USER_SETUP_LOADED        // ADD THIS - must be first
#define USER_SETUP_ID 102        // ADD THIS
// === Driver and Display Dimensions ===
#define GC9A01_DRIVER         // Use GC9A01 driver
//#define CGRAM_OFFSET          // Fixes offset issue on many ST7789 boards

// Round GC9A01 display: 240 x 240 pixels.
#define TFT_WIDTH  240
#define TFT_HEIGHT 240

#define TFT_RGB_ORDER TFT_RGB // Try TFT_BGR if colors are wrong
#define TFT_INVERT_ON         // Enable inversion for this display setup.


// === Control Pins (match your wiring) ===
#define TFT_RST 4	// Reset
#define TFT_CS   -1 // Sketch manually controls chip select on GPIO 39.
#define TFT_DC   2 // Data/Command

// === SPI Pins (match your ESP32 wiring) ===
#define TFT_MOSI 9 // SPI Data/SDA
#define TFT_SCLK 8	// SPI Clock/CLK
// MISO is master in, slave out.  This is an output device (master out slave in-MOSI) so //we don't need or want MISO to be defined.  Sometimes this needs to be declared with
//the -1 definition below.  
//But on most boards, it does not, 
//so leave this next line commented out unless we find we need it.
//#define TFT_MISO -1  
#define TFT_BL   -1  // Backlight pin (PWM capable)
// VCC is 3.3V (best not 5)
// GND has to be same Ground as ESP32
// BL/BACKLIGHT is optional--on a PWM pin you can use this to control brightness
//dynamically.  Or it can just go to 3.3 v to just be on.
#define SPI_FREQUENCY  60000000
// === Fonts and Features ===
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#warning "Using three_eyes_sdcard_esp32s3_GC9A01.h (240x240 round display)"
