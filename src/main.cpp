#include <TFT_eSPI.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include <HTTPClient.h> 
#include <ArduinoJson.h>  // Include the ArduinoJson library
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <math.h>  // For sin() function in animation
#include "credentials.h"  // WiFi and API credentials (not in version control)

// Color definitions - Enhanced for better visibility
#define BACKGROUND TFT_BLACK  // Changed from DARKGREY to BLACK for better contrast
#define TEXT_COLOR TFT_WHITE
#define TFT_GREY 0x7BEF
#define TFT_LIGHTGREY 0xC618
#define TFT_DARKGREY 0x39E7
#define TFT_YELLOW 0xFFE0
#define TFT_BLUE 0x001F
#define TFT_WHITE 0xFFFF
#define TFT_RED 0xF800
#define TFT_ORANGE 0xFD20
#define TFT_GREEN 0x07E0
#define TFT_CYAN 0x07FF  // Added for better weather visibility
#define TFT_MAGENTA 0xF81F  // Added for status indicators

// Brighter colors for status indicators (better for colorblind users)
// RGB565 format: RRRRR GGGGGG BBBBB
#define TFT_BRIGHT_GREEN 0x07E0   // Bright green (R:0, G:63, B:0) - full green
#define TFT_BRIGHT_RED 0xF800     // Bright red (R:31, G:0, B:0) - full red
#define TFT_BRIGHT_YELLOW 0xFFE0  // Bright yellow (R:31, G:63, B:0) - full yellow
#define TFT_BRIGHT_CYAN 0x07FF    // Bright cyan (R:0, G:63, B:31) - full cyan
#define TFT_BRIGHT_BLUE 0x001F    // Bright blue (R:0, G:0, B:31) - full blue
#define TFT_BRIGHT_ORANGE 0xFD20  // Bright orange (similar to existing)

// ============================================================================
// POSITION MATRIX - Easy modification of all display element positions
// ============================================================================
// All positions in pixels (x, y) with (0,0) at top-left
// Portrait mode: 240x320 (width x height)
// Landscape mode: 320x240 (width x height)

// Time display positions
struct {
    struct {
        int x;        // Portrait: 48 (timeXPos 62 - 14 offset)
        int y;        // Portrait: 10
        int xOffset;  // Offset from base timeXPos: -14
    } portrait;
    struct {
        int x;        // Landscape: 87 (timeXPos 62 + 25 offset)
        int y;        // Landscape: 30 (moved down for spacing)
        int xOffset;  // Offset from base timeXPos: +25
    } landscape;
} timePos = {
    {48, 10, -14},   // portrait
    {87, 30, 25}     // landscape (moved down 20px)
};

// Date display positions
struct {
    struct {
        int y;        // Portrait: 60
        int centered; // Portrait: centered (calculated dynamically)
    } portrait;
    struct {
        int xOffset;  // Landscape: +40 from base dateXPos
        int y;        // Landscape: 70 (moved down for spacing)
        int centered; // Landscape: centered (calculated dynamically)
    } landscape;
} datePos = {
    {60, 1},         // portrait (centered)
    {40, 70, 1}      // landscape (moved down 20px)
};

// Stock display positions
struct {
    struct {
        int x;        // Portrait: 5
        int y;        // Portrait: 160
    } portrait;
    struct {
        int x;        // Landscape: 5
        int y;        // Landscape: 90 (moved down for spacing)
    } landscape;
} stockPos = {
    {5, 160},        // portrait
    {5, 90}          // landscape (moved down 20px)
};

// Weather text positions
struct {
    struct {
        int x;        // Portrait: 5
        int y;        // Portrait: 90 (first line)
        int lineSpacing; // Portrait: 20 pixels between lines
    } portrait;
    struct {
        int x;        // Landscape: 5
        int y;        // Landscape: 115 (moved down for spacing)
    } landscape;
} weatherTextPos = {
    {5, 90, 20},     // portrait
    {5, 115}         // landscape (moved down 20px)
};

// Weather icon positions
struct {
    struct {
        int x;        // Portrait: 190 (240 - 50 icon width)
        int y;        // Portrait: 90
    } portrait;
    struct {
        int x;        // Landscape: 265
        int y;        // Landscape: 105 (moved down for spacing)
    } landscape;
} weatherIconPos = {
    {190, 90},       // portrait
    {265, 105}       // landscape (moved down 25px)
};

// Coffee machine positions
struct {
    struct {
        int x;        // Portrait: 5 (text mode)
        int y;        // Portrait: 180
    } portrait;
    struct {
        int x;        // Landscape: 280 (icon mode)
        int y;        // Landscape: 30 (moved down for spacing)
    } landscape;
} coffeePos = {
    {5, 180},        // portrait
    {280, 30}        // landscape (moved down 20px)
};

// Animation positions
struct {
    struct {
        int x;        // Portrait: 150
        int y;        // Portrait: 180
        int width;    // Portrait: 90 (240 - 150)
        int height;   // Portrait: 50
    } portrait;
    struct {
        int x;        // Landscape: 223
        int y;        // Landscape: 160
        int width;    // Landscape: 120
        int height;   // Landscape: 100
    } landscape;
} animationPos = {
    {150, 230, 90, 50},  // portrait (original position restored)
    {223, 160, 120, 100} // landscape (original position restored)
};

// Trail status positions (Y coordinates, X is always 0)
struct {
    struct {
        int mombaY;      // Portrait: 205
        int johnBryanY;   // Portrait: 225
        int caesarCreekY; // Portrait: 245
        int lineSpacing; // Portrait: 20 pixels
    } portrait;
    struct {
        int mombaY;      // Landscape: 155 (moved down for spacing)
        int johnBryanY;  // Landscape: 175 (moved down for spacing)
        int caesarCreekY;// Landscape: 195 (moved down for spacing)
        int lineSpacing;// Landscape: 20 pixels
    } landscape;
} trailPos = {
    {195,215, 235, 20}, // portrait
    {155, 175, 195, 20}  // landscape (moved down 20px each)
};

// Countdown timer positions (right bottom)
struct {
    struct {
        int x;        // Portrait: right-aligned (calculated dynamically)
        int y;        // Portrait: 300 (near bottom of 320px screen)
    } portrait;
    struct {
        int x;        // Landscape: right-aligned (calculated dynamically)
        int y;        // Landscape: 220 (near bottom of 240px screen)
    } landscape;
} countdownPos = {
    {0, 280},        // portrait (x calculated dynamically)
    {0, 180}         // landscape (x calculated dynamically)
};

// Printer status icon positions (small icons, top right area)
struct {
    struct {
        int x1;       // Portrait: first printer icon X
        int x2;       // Portrait: second printer icon X
        int y;        // Portrait: Y position
        int spacing;  // Portrait: spacing between icons
    } portrait;
    struct {
        int x1;       // Landscape: first printer icon X
        int x2;       // Landscape: second printer icon X
        int y;        // Landscape: Y position
        int spacing;  // Landscape: spacing between icons
    } landscape;
} printerPos = {
    {200, 220, 5, 20},   // portrait (top right, small icons)
    {280, 300, 5, 20}    // landscape (top right, small icons)
};

// Base positions (used as reference for calculations)
int timeXPos = 62;  // Base time X position (centered for 320px width display)
int dateXPos = 55;  // Base date X position (centered for date text)
int stockXPos = 5;  // Stock X position (left-aligned)
int weatherXPos = 5;  // Weather X position (left-aligned)
int coffeeStatusXPos = 5;  // Coffee status X position (left-aligned)
int trailStatusXPos = 0;  // Trail status X position (left edge)

void setupNTP();

// Forward declarations for display update functions
void updateTimeDisplay();
void updateWeatherDisplay();
void updateStockDisplay();
void updateCoffeeMachineDisplay();
void updateTrailDisplay();
bool refreshAllTrails();  // Force server-side refresh (POST /api/trail/refresh)
void updatePrinterDisplay();
void updateCountdownDisplay();
void drawWeatherIconStatic();

// TFT Display setup
TFT_eSPI tft = TFT_eSPI();

// Network Configuration (credentials loaded from credentials.h)
const char* serverHost = "mainPI.local";
const int serverPort = 5000;

// Alpha Vantage API configuration (free API for stock data)
// API key loaded from credentials.h
const char* alphaVantageHost = "www.alphavantage.co";

// Timing variables
unsigned long lastTimeUpdate = 0;
unsigned long lastWeatherUpdate = 0;
unsigned long lastCoffeeUpdate = 0;
unsigned long lastTrailUpdate = 0;
unsigned long lastStockUpdate = 0;
unsigned long lastPrinterUpdate = 0;
const unsigned long TIME_UPDATE_INTERVAL = 30000; // Update time every 30 seconds
const unsigned long WEATHER_UPDATE_INTERVAL = 1800000; // Update weather every 30 minutes
const unsigned long COFFEE_UPDATE_INTERVAL = 300000; // Update coffee machine status every 5 minutes
const unsigned long TRAIL_UPDATE_INTERVAL = 1800000; // Update trail status every 30 minutes (matches server cache)
const unsigned long STOCK_UPDATE_INTERVAL = 300000; // Update stock price every 5 minutes
const unsigned long PRINTER_UPDATE_INTERVAL = 30000; // Update printer status every 30 seconds

// Time structure
struct TimeInfo {
    String time;
    String seconds;
    String date;
} currentTime;

// Weather structure
struct WeatherInfo {
    String conditions;
    int temperature;
    int feels_like;
    int humidity;
    String icon;
} currentWeather;

// Forecast structure for today and tomorrow
struct ForecastDay {
    String date;
    int high;
    int low;
    String conditions;
    String icon;
};
struct ForecastInfo {
    ForecastDay today;
    ForecastDay tomorrow;
    bool valid;
} weatherForecast = {{}, {}, false};

// Coffee Machine structure
struct CoffeeMachineInfo {
    String status;
    String scheduledTime;
    String esp32Status;
} coffeeMachine;

// Trail structure
struct TrailInfo {
    String name;
    String status;
    String lastUpdate;
} mombaTrail, johnBryanTrail, caesarCreekTrail;

// Stock structure
struct StockInfo {
    String symbol;
    float price;
    float change;
    float changePercent;
} spyStock;

// Printer structure
struct PrinterInfo {
    String name;
    String host;
    String status;  // "ready", "printing", "paused", "error", "offline"
    String lastStatus;  // Track previous status to detect transitions
    bool isFlashing;  // Track if printer icon should flash
    unsigned long flashStartTime;  // When flashing started
} sovolPrinter, mandrainPrinter;

// Printer flash constants
const unsigned long PRINTER_FLASH_DURATION = 10000; // Flash for 10 seconds
const unsigned long PRINTER_FLASH_INTERVAL = 500;   // Flash every 500ms (on/off)

// Add these near the top with other constants
#define BUTTON_PIN 25   // GPIO25 for ESP32 WROOM DevKit (avoiding TFT pins)
int currentRotation = 0;  // Track current rotation state
unsigned long lastButtonPress = 0;  // Debouncing variable
const unsigned long DEBOUNCE_DELAY = 50;  // Increased to 500ms for more stability
#define SCREEN_TOGGLE_PIN 32    // GPIO22 for ESP32 WROOM DevKit
#define REFRESH_PIN 33         // GPIO5 for ESP32 WROOM DevKit
bool isScreenOn = true;        // Track screen state
unsigned long lastScreenToggle = 0;  // Debouncing for screen toggle
unsigned long lastRefreshPress = 0;  // Debouncing for refresh
#define TFT_BL 5   // GPIO32 for ESP32 WROOM DevKit backlight control

// Refresh button hold state for forecast view
bool isShowingForecast = false;  // Track if forecast view is displayed
unsigned long refreshHoldStart = 0;  // When refresh button was pressed
const unsigned long HOLD_THRESHOLD = 300;  // Hold for 300ms to trigger forecast view



// Add this near other global variables at the top
bool needRedraw = true;  // Global variable for screen updates

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -5 * 3600); // Default to EST (UTC-5), will adjust for DST automatically
unsigned long lastSecondUpdate = 0;
const unsigned long SECOND_UPDATE_INTERVAL = 1000; // Update seconds every 1 second
bool useLocalServerTime = false; // Use NTP directly (more efficient - no local server overhead)
unsigned long lastDSTCheck = 0;
const unsigned long DST_CHECK_INTERVAL = 3600000; // Check DST once per hour (offset changes are rare)
int cachedDSTOffset = -5 * 3600; // Cache current DST offset

// Add these with other global variables at the top
static String lastTime = "";
static String lastSeconds = "";
static String lastDate = "";
static String lastDisplayedDate = "";  // Track displayed date string to enable forced redraws
static String lastStockDisplay = "";  // Cache last stock display to prevent disappearing
static String lastCoffeeScheduledTime = "";  // Track last scheduled coffee time for auto-schedule
static String lastCoffeeDisplayTime = "";  // Cache last displayed coffee time to prevent disappearing
static String lastCoffeeStatus = "";  // Track coffee status to prevent unnecessary redraws
static int lastCountdownDays = -1;  // Track last countdown value to prevent unnecessary redraws

// Animation disabled - weather icon is drawn statically

void drawWeatherIcon(String icon, int x, int y) {
    // Clear the area where the icon will be drawn
    tft.fillRect(x, y, 50, 50, BACKGROUND);

    if (icon == "01d" || icon == "01n") {
        // Clear sky
        tft.fillCircle(x + 25, y + 25, 20, TFT_YELLOW);
    } else if (icon == "02d" || icon == "02n") {
        // Few clouds
        tft.fillCircle(x + 25, y + 25, 20, TFT_LIGHTGREY);
        tft.fillCircle(x + 15, y + 25, 15, TFT_LIGHTGREY);
    } else if (icon == "03d" || icon == "03n" || icon == "04d" || icon == "04n") {
        // Scattered or broken clouds
        tft.fillCircle(x + 25, y + 25, 20, TFT_GREY);
        tft.fillCircle(x + 15, y + 25, 15, TFT_GREY);
    } else if (icon == "09d" || icon == "09n" || icon == "10d" || icon == "10n") {
        // Rain
        tft.fillCircle(x + 25, y + 20, 15, TFT_BLUE);
        for (int i = 0; i < 3; i++) {
            tft.drawLine(x + 20 + (i * 5), y + 30, x + 15 + (i * 5), y + 40, TFT_BLUE);
        }
    } else if (icon == "11d" || icon == "11n") {
        // Thunderstorm
        tft.fillCircle(x + 25, y + 20, 15, TFT_DARKGREY);
        tft.drawLine(x + 20, y + 30, x + 30, y + 40, TFT_YELLOW);
        tft.drawLine(x + 30, y + 40, x + 20, y + 50, TFT_YELLOW);
    } else if (icon == "13d" || icon == "13n") {
        // Snow
        tft.fillCircle(x + 25, y + 20, 15, TFT_WHITE);
        for (int i = 0; i < 3; i++) {
            tft.drawLine(x + 20 + (i * 5), y + 30, x + 15 + (i * 5), y + 40, TFT_WHITE);
        }
    } else if (icon == "50d" || icon == "50n") {
        // Mist
        tft.fillRect(x + 10, y + 20, 30, 10, TFT_LIGHTGREY);
        tft.fillRect(x + 10, y + 35, 30, 10, TFT_LIGHTGREY);
    }
}

// Draw weather icon statically (animation disabled)
void drawWeatherIconStatic() {
    // Only draw if we have weather data
    if (currentWeather.icon.length() == 0) {
        return;
    }
    
    // Get weather icon position
    int iconX, iconY;
    if (currentRotation == 1 || currentRotation == 3) { // Landscape
        iconX = weatherIconPos.landscape.x;
        iconY = weatherIconPos.landscape.y;
    } else { // Portrait
        iconX = weatherIconPos.portrait.x;
        iconY = weatherIconPos.portrait.y;
    }
    
    // Draw weather icon at static position
    drawWeatherIcon(currentWeather.icon, iconX, iconY);
}

void drawCoffeeIcon(uint16_t color, int x, int y) {
    // Clear the area where the icon will be drawn (only icon area, not time below)
    tft.fillRect(x, y, 40, 40, BACKGROUND);
    
    // Draw a simple coffee cup icon using basic shapes
    // Cup body (rectangle/trapezoid shape)
    int cupX = x + 8;
    int cupY = y + 12;
    int cupWidth = 20;
    int cupHeight = 20;
    
    // Fill cup entirely with color (no different interior color)
    if (color == TFT_GREEN) {
        // Fill entire cup with green - ALL GREEN, no orange
        tft.fillRect(cupX, cupY, cupWidth, cupHeight, color);
        
        // Steam lines (3 curved lines above cup)
        for (int i = 0; i < 3; i++) {
            int steamX = cupX + 5 + (i * 6);
            tft.drawLine(steamX, cupY - 2, steamX + 1, cupY - 4, TFT_WHITE);
            tft.drawLine(steamX + 1, cupY - 4, steamX, cupY - 6, TFT_WHITE);
        }
    } else if (color == TFT_RED) {
        // Red (off) - fill cup solid red
        tft.fillRect(cupX, cupY, cupWidth, cupHeight, color);
    } else if (color == TFT_YELLOW) {
        // Yellow (offline) - fill cup solid yellow
        tft.fillRect(cupX, cupY, cupWidth, cupHeight, color);
        // Add warning X mark
        int warnX = x + 32;
        int warnY = y + 5;
        tft.drawLine(warnX, warnY, warnX + 5, warnY + 5, TFT_BLACK);
        tft.drawLine(warnX, warnY + 5, warnX + 5, warnY, TFT_BLACK);
    } else {
        // Default - outline only
        tft.drawRect(cupX, cupY, cupWidth, cupHeight, color);
    }
    
    // Cup handle (always draw)
    int handleX = cupX + cupWidth + 2;
    int handleY = cupY + 8;
    // Draw handle as curved lines
    uint16_t handleColor = (color == TFT_GREEN || color == TFT_RED || color == TFT_YELLOW) ? color : TFT_WHITE;
    tft.drawLine(handleX, handleY, handleX + 3, handleY + 2, handleColor);
    tft.drawLine(handleX + 3, handleY + 2, handleX + 3, handleY + 8, handleColor);
    tft.drawLine(handleX + 3, handleY + 8, handleX, handleY + 10, handleColor);
}

uint16_t getTrailStatusColor(String status) {
    if (status == "open") {
        return TFT_BRIGHT_GREEN;  // Brighter green for better visibility
    } else if (status == "closed") {
        return TFT_BRIGHT_RED;    // Brighter red for better visibility
    } else if (status == "wet" || status == "caution") {
        return TFT_BRIGHT_YELLOW; // Bright yellow for better visibility
    } else if (status == "freeze") {
        return TFT_BRIGHT_BLUE;   // Brighter blue for better visibility
    }
    return TFT_WHITE; // Default color
}

// Get printer status color based on state
uint16_t getPrinterStatusColor(String status) {
    if (status == "printing") {
        return TFT_BRIGHT_GREEN;  // Bright green when printing
    } else if (status == "ready" || status == "standby") {
        return TFT_BRIGHT_CYAN;   // Bright cyan when ready
    } else if (status == "paused") {
        return TFT_BRIGHT_YELLOW; // Bright yellow when paused
    } else if (status == "error") {
        return TFT_BRIGHT_RED;    // Bright red when error
    } else if (status == "idle" || status == "complete" || status == "cancelled") {
        return TFT_WHITE;  // White for idle/complete/cancelled states
    }
    return TFT_DARKGREY;   // Grey when offline or unknown
}

// Draw small printer status icon (12x12 pixel circle)
void drawPrinterIcon(uint16_t color, int x, int y) {
    // Clear the area
    tft.fillRect(x, y, 14, 14, BACKGROUND);
    // Draw filled circle for status
    tft.fillCircle(x + 7, y + 7, 6, color);
    // Draw outline for visibility
    tft.drawCircle(x + 7, y + 7, 6, TFT_WHITE);
}

// Helper function to remove year from date string
String removeYearFromDate(String dateStr) {
    // Handle various date formats
    // Format: "YYYY-MM-DD" -> "MM-DD"
    if (dateStr.indexOf('-') == 4) {
        // YYYY-MM-DD format, remove first 5 characters (YYYY-)
        return dateStr.substring(5);
    }
    // Format: "Month DD, YYYY" -> "Month DD"
    int commaPos = dateStr.indexOf(',');
    if (commaPos > 0) {
        // Check if there's a 4-digit year after comma
        String afterComma = dateStr.substring(commaPos + 1);
        afterComma.trim();
        if (afterComma.length() == 4 && afterComma.toInt() > 1900) {
            return dateStr.substring(0, commaPos);
        }
    }
    // Format: "DD/MM/YYYY" or "DD-MM-YYYY" -> "DD/MM" or "DD-MM"
    int lastSlash = dateStr.lastIndexOf('/');
    int lastDash = dateStr.lastIndexOf('-');
    if (lastSlash > 0) {
        String afterSlash = dateStr.substring(lastSlash + 1);
        if (afterSlash.length() == 4 && afterSlash.toInt() > 1900) {
            return dateStr.substring(0, lastSlash);
        }
    }
    if (lastDash > 0 && dateStr.indexOf('-') != lastDash) {
        String afterDash = dateStr.substring(lastDash + 1);
        if (afterDash.length() == 4 && afterDash.toInt() > 1900) {
            return dateStr.substring(0, lastDash);
        }
    }
    // If no pattern matches, return original
    return dateStr;
}

void updateTimeDisplay() {
    // Adjust X positions for landscape orientation
    int adjustedTimeXPos = timeXPos;
    int adjustedDateXPos = dateXPos;
    
    // Use position matrix values
    if (currentRotation == 0 || currentRotation == 2) { // Portrait orientations
        adjustedTimeXPos = timeXPos + timePos.portrait.xOffset;  // Use portrait offset
    } else { // Landscape orientations
        adjustedTimeXPos = timeXPos + timePos.landscape.xOffset;  // Use landscape offset
        adjustedDateXPos = dateXPos + datePos.landscape.xOffset;
    }
    
    // Check if time changed BEFORE we update lastTime
    bool timeChanged = (currentTime.time.length() > 0 && currentTime.time != lastTime);
    bool secondsChanged = (currentTime.seconds != lastSeconds);
    
    if (currentTime.time.length() > 0) {
        // Only update time if it changed
        if (timeChanged) {
            // Clear only time area, but avoid clearing date area
            // In landscape, date is at y=70, so clear height must be limited
            // In portrait, date is at y=60, so we need to be more careful
            int timeClearHeight = (currentRotation == 1 || currentRotation == 3) ? 35 : 35;
            // In portrait, limit clear area to avoid overlapping with date at y=60
            if (currentRotation == 0 || currentRotation == 2) {
                // Portrait: time is at y=10, date at y=60, so clear only up to y=45 max (leave 15px buffer)
                timeClearHeight = 35;
            } else {
                // Landscape: time is at y=30, date at y=70, so clear only up to y=65 max (leave 5px buffer)
                timeClearHeight = 35;  // 30 + 35 = 65, which is safe
            }
            int timeY = (currentRotation == 0 || currentRotation == 2) ? timePos.portrait.y : timePos.landscape.y;
            tft.fillRect(adjustedTimeXPos, timeY, 135, timeClearHeight, BACKGROUND);
            tft.setTextSize(2);
            tft.setTextColor(TEXT_COLOR, BACKGROUND);
            tft.drawString(currentTime.time, adjustedTimeXPos, timeY, 4);
            lastTime = currentTime.time;
            
            // Redraw date after time update to ensure it's not partially cleared
            // This is especially important in landscape mode where time and date are closer
            if (currentTime.date.length() > 0) {
                lastDisplayedDate = "";  // Force date redraw
                // We'll redraw date below, but need to ensure it happens
            }
        }
        
        // Only update seconds if they changed
        if (secondsChanged) {
            int secondsY = (currentRotation == 0 || currentRotation == 2) ? 20 : timePos.landscape.y + 5;  // Adjust for landscape
            tft.fillRect(adjustedTimeXPos + 135, secondsY, 50, 30, BACKGROUND);  // Clear only seconds area
            tft.setTextSize(1);
            tft.setTextColor(TEXT_COLOR, BACKGROUND);
            tft.drawString(currentTime.seconds, adjustedTimeXPos + 135, secondsY, 4);
            lastSeconds = currentTime.seconds;
        }
    }
    
    // Only update date if it actually changed (not on every time/seconds update to prevent flickering)
    // Also track the displayed date string to redraw if needed
    // IMPORTANT: Always redraw date if lastDisplayedDate is empty (forced redraw after time update)
    if (currentTime.date.length() > 0) {
        String displayDate = removeYearFromDate(currentTime.date);
        bool dateChanged = (currentTime.date != lastDate);
        bool displayChanged = (displayDate != lastDisplayedDate);
        bool forceRedraw = (lastDisplayedDate.length() == 0);  // Force redraw if cache was cleared
        
        // Update if date changed, display string changed, forced redraw, or initial draw
        if (dateChanged || displayChanged || forceRedraw || (needRedraw && lastDate.length() == 0)) {
            tft.setTextSize(1);  // Reduced from 2 to 1 for smaller size
            tft.setTextColor(TEXT_COLOR, BACKGROUND);

            // Calculate text width and center position
            // Use more generous width calculation to account for variable character widths in font 2
            int textWidth = displayDate.length() * 8; // Increased from 6 to 8 for better coverage
            int centeredDateX = (320 - textWidth) / 2;
            int dateY = 60;

            // Adjust positioning based on orientation using position matrix
            int clearWidth = 320;
            int clearHeight = 25; // default
            // Compute stock line Y to avoid overlap when clearing date (esp. in landscape)
            int stockYForRotation = (currentRotation == 0 || currentRotation == 2) ? stockPos.portrait.y : stockPos.landscape.y;
            if (currentRotation == 0 || currentRotation == 2) { // Portrait orientations
                centeredDateX = (240 - textWidth) / 2; // Center on 240px width (portrait)
                dateY = datePos.portrait.y;
                clearWidth = 240;  // Portrait width - clear full width to prevent partial clearing
                // Portrait has ample spacing; keep default height
            } else { // Landscape orientations
                dateY = datePos.landscape.y;
                // In landscape, ensure we do not clear into the stock ticker row (typically at y ~90)
                // But we need enough height to fully clear the date text (at least 20-22px for font 2)
                // Stock is at y=90, date is at y=70, so we have 20px of space
                // Use 20px clearHeight to fully cover the date text without overlapping stock
                clearHeight = 20;  // Fixed height to fully cover date text
                // Clear full width in landscape too to prevent partial clearing
            }

            // Clear date area - use full width to ensure nothing else can partially clear it
            tft.fillRect(0, dateY, clearWidth, clearHeight, BACKGROUND);
            tft.drawString(displayDate, centeredDateX, dateY, 2);
            lastDate = currentTime.date;
            lastDisplayedDate = displayDate;
        }
    }
}

void updateStockDisplay() {
    if (spyStock.symbol.length() > 0) {
        Serial.println("Updating display with stock: " + spyStock.symbol); // Debug statement

        // Get stock display position
        int stockY = (currentRotation == 0 || currentRotation == 2) ? stockPos.portrait.y : stockPos.landscape.y;
        int stockX = stockPos.portrait.x;  // Same for both modes
        
        // Display stock information
        tft.setTextSize(1);

        // Determine color based on price change (brighter colors for better visibility)
        uint16_t stockColor = TFT_WHITE;
        if (spyStock.change > 0) {
            stockColor = TFT_BRIGHT_GREEN; // Bright green for positive change
        } else if (spyStock.change < 0) {
            stockColor = TFT_BRIGHT_RED; // Bright red for negative change
        }

        // Display stock price and change with better formatting
        String priceStr = "$" + String(spyStock.price, 2);
        String changeStr = (spyStock.change >= 0 ? "+" : "") + String(spyStock.change, 2);
        String percentStr = (spyStock.changePercent >= 0 ? "+" : "") + String(spyStock.changePercent, 2) + "%";
        String stockInfo = "$SPY: " + priceStr + " (" + changeStr + " / " + percentStr + ")";

        // ALWAYS clear and redraw to prevent disappearing
        // Calculate clear area to avoid overlapping weather (landscape) or other elements
        // In landscape: weather icon is at x=265, so clear only up to x=260
        // In portrait: full width is fine
        int clearWidth = (currentRotation == 0 || currentRotation == 2) ? 240 : 255;  // Leave room for weather icon
        // Font 2 needs full character height including ascenders/descenders
        // Clear starting 2px above draw position to capture ascenders, and 22px tall total
        // In landscape, ensure we don't clear the date area (date is at y=70, stock at y=90)
        // Date clearHeight is 20px (y=70 to y=90), so stock should start clearing from y=90
        int clearY = stockY;
        int clearHeight = 22;
        if (currentRotation == 1 || currentRotation == 3) { // Landscape
            // Don't clear above stock position in landscape to avoid clearing date
            clearY = stockY;  // Start at stockY, not stockY - 2
            clearHeight = 22;  // Keep same height
        } else {
            // Portrait: can clear 2px above as before
            clearY = stockY - 2;
        }
        tft.fillRect(stockX, clearY, clearWidth, clearHeight, BACKGROUND);  // Clear stock area
        
        // Render with background to avoid partial erasure artifacts when other areas refresh
        tft.setTextColor(stockColor, BACKGROUND);
        tft.drawString(stockInfo, stockX, stockY, 2);  // Draw at exact Y position
        lastStockDisplay = stockInfo;
    } else {
        Serial.println("No stock data to display."); // Debug statement
    }
}

void updateWeatherDisplay() {
    if (currentWeather.conditions.length() > 0) {
        Serial.println("Updating display with weather: " + currentWeather.conditions); // Debug statement

        // Clear only the weather area using position matrix
        int clearWidth = 320;
        int clearHeight = 20;  // Precise height for text
        int weatherY;
        int weatherX = weatherXPos;
        
        if (currentRotation == 0 || currentRotation == 2) { // Portrait
            weatherY = weatherTextPos.portrait.y;
            clearHeight = 65;  // 3 lines in portrait (20px per line + spacing)
            clearWidth = 185;  // Leave room for weather icon at x=190
        } else { // Landscape
            weatherY = weatherTextPos.landscape.y;
            clearWidth = 260;  // Clear left side only, leave room for icon at x=265
        }
        tft.fillRect(weatherX, weatherY, clearWidth, clearHeight, BACKGROUND);

        // Display weather information
        tft.setTextSize(1);

        // Determine color based on temperature
        uint16_t tempColor = TFT_WHITE;
        if (currentWeather.temperature <= 32) {
            tempColor = TFT_BLUE; // Cold
        } else if (currentWeather.temperature <= 60) {
            tempColor = TFT_WHITE; // Mild
        } else if (currentWeather.temperature <= 85) {
            tempColor = TFT_ORANGE; // Warm
        } else {
            tempColor = TFT_RED; // Hot
        }

        tft.setTextColor(tempColor);

        // Display weather info
        int tempC = (currentWeather.temperature - 32) * 5 / 9;
        int feelsC = (currentWeather.feels_like - 32) * 5 / 9;

        int weatherTextX = weatherXPos;  // Same for both modes
        if (currentRotation == 1 || currentRotation == 3) { // Landscape orientations
            // Single line: Temp & Feels like with Celsius
            String weatherInfo = String(currentWeather.temperature) + "°F/" + String(tempC) + "°C Feels: " + String(currentWeather.feels_like) + "°F/" + String(feelsC) + "°C H: " + String(currentWeather.humidity) + "%";
            tft.drawString(weatherInfo, weatherTextX, weatherY, 2);
        } else { // Portrait
            // Multiple lines for portrait (more readable)
            String tempInfo = "Temp: " + String(currentWeather.temperature) + "°F/" + String(tempC) + "°C";
            String feelsInfo = "Feels: " + String(currentWeather.feels_like) + "°F/" + String(feelsC) + "°C";
            String humidityInfo = "Humidity: " + String(currentWeather.humidity) + "%";
            tft.drawString(tempInfo, weatherTextX, weatherY, 2);
            tft.drawString(feelsInfo, weatherTextX, weatherY + weatherTextPos.portrait.lineSpacing, 2);
            tft.drawString(humidityInfo, weatherTextX, weatherY + weatherTextPos.portrait.lineSpacing * 2, 2);
        }

        // Draw static weather icon
        drawWeatherIconStatic();
    } else {
        Serial.println("No weather data to display."); // Debug statement
    }
}

// Function to draw forecast view (today and tomorrow) when refresh button is held
void drawForecastView() {
    // Clear entire screen for forecast view
    tft.fillScreen(BACKGROUND);

    // Title
    tft.setTextColor(TFT_CYAN);
    tft.setTextSize(1);

    int screenWidth = (currentRotation == 1 || currentRotation == 3) ? 320 : 240;
    int screenHeight = (currentRotation == 1 || currentRotation == 3) ? 240 : 320;

    // Draw title centered
    tft.drawString("Weather Forecast", screenWidth / 2 - 70, 10, 2);

    if (!weatherForecast.valid) {
        tft.setTextColor(TFT_YELLOW);
        tft.drawString("Fetching forecast...", screenWidth / 2 - 80, screenHeight / 2, 2);
        return;
    }

    tft.setTextColor(TFT_WHITE);

    // Today section
    int todayY = 45;
    tft.setTextColor(TFT_GREEN);
    tft.drawString("TODAY", 10, todayY, 2);
    tft.setTextColor(TFT_WHITE);

    // Today's high/low
    int todayHighC = (weatherForecast.today.high - 32) * 5 / 9;
    int todayLowC = (weatherForecast.today.low - 32) * 5 / 9;
    String todayTemp = "High: " + String(weatherForecast.today.high) + "F/" + String(todayHighC) + "C  Low: " + String(weatherForecast.today.low) + "F/" + String(todayLowC) + "C";
    tft.drawString(todayTemp, 10, todayY + 25, 2);

    // Today's conditions
    tft.drawString(weatherForecast.today.conditions, 10, todayY + 50, 2);

    // Draw today's weather icon
    drawWeatherIcon(weatherForecast.today.icon, screenWidth - 60, todayY + 10);

    // Tomorrow section
    int tomorrowY = todayY + 90;
    tft.setTextColor(TFT_ORANGE);
    tft.drawString("TOMORROW", 10, tomorrowY, 2);
    tft.setTextColor(TFT_WHITE);

    // Check if tomorrow's forecast is available
    if (weatherForecast.tomorrow.high != 0 || weatherForecast.tomorrow.low != 0) {
        // Tomorrow's high/low
        int tomorrowHighC = (weatherForecast.tomorrow.high - 32) * 5 / 9;
        int tomorrowLowC = (weatherForecast.tomorrow.low - 32) * 5 / 9;
        String tomorrowTemp = "High: " + String(weatherForecast.tomorrow.high) + "F/" + String(tomorrowHighC) + "C  Low: " + String(weatherForecast.tomorrow.low) + "F/" + String(tomorrowLowC) + "C";
        tft.drawString(tomorrowTemp, 10, tomorrowY + 25, 2);

        // Tomorrow's conditions
        tft.drawString(weatherForecast.tomorrow.conditions, 10, tomorrowY + 50, 2);

        // Draw tomorrow's weather icon
        if (weatherForecast.tomorrow.icon.length() > 0) {
            drawWeatherIcon(weatherForecast.tomorrow.icon, screenWidth - 60, tomorrowY + 10);
        }
    } else {
        // Forecast not available
        tft.setTextColor(TFT_GREY);
        tft.drawString(weatherForecast.tomorrow.conditions, 10, tomorrowY + 25, 2);
    }

    // Hint at bottom
    tft.setTextColor(TFT_GREY);
    tft.drawString("Release button to return", 10, screenHeight - 25, 1);
}

// Function to redraw main screen after returning from forecast view
void redrawMainScreen() {
    tft.fillScreen(BACKGROUND);
    needRedraw = true;

    // Clear cached display values to force full redraw
    lastTime = "";
    lastSeconds = "";
    lastDate = "";
    lastDisplayedDate = "";
    lastStockDisplay = "";
    lastCoffeeStatus = "";

    // Force redraw of all elements (countdown is hidden/unused)
    updateTimeDisplay();
    updateWeatherDisplay();
    updateStockDisplay();
    updateCoffeeMachineDisplay();
    updateTrailDisplay();
    updatePrinterDisplay();
    drawWeatherIconStatic();
}

void updateCoffeeMachineDisplay() {
    // Determine color based on coffee machine status (brighter colors for better visibility)
    uint16_t statusColor = TFT_WHITE;
    if (coffeeMachine.esp32Status == "offline") {
        statusColor = TFT_BRIGHT_YELLOW; // Bright yellow when ESP32 is offline
    } else if (coffeeMachine.status == "On") {
        statusColor = TFT_BRIGHT_GREEN; // Bright green when coffee machine is on
    } else if (coffeeMachine.status == "Off") {
        statusColor = TFT_BRIGHT_RED; // Bright red when coffee machine is off
    }

    // Check if status or scheduled time changed to avoid unnecessary redraws
    String currentStatusKey = coffeeMachine.status + "|" + coffeeMachine.scheduledTime + "|" + coffeeMachine.esp32Status;
    bool statusChanged = (currentStatusKey != lastCoffeeStatus);

    if (currentRotation == 1 || currentRotation == 3) {
        // Landscape orientation: use icon at top right next to time
        int iconX = coffeePos.landscape.x;
        int iconY = coffeePos.landscape.y;
        
        // Always draw the coffee icon (it clears its own 40x40 area)
        // The icon needs to be visible, so we always draw it
        drawCoffeeIcon(statusColor, iconX, iconY);
        
        // Draw scheduled time if available, or use cached value to prevent disappearing
        int timeTextX = iconX;
        int timeTextY = iconY + 42;  // Below 40px icon + 2px spacing
        String timeToDisplay = coffeeMachine.scheduledTime.length() > 0 ? coffeeMachine.scheduledTime : lastCoffeeDisplayTime;
        
        // Only clear and redraw time if it changed, status changed, or if we need initial display
        bool timeNeedsUpdate = statusChanged || 
                              (timeToDisplay.length() > 0 && timeToDisplay != lastCoffeeDisplayTime) ||
                              (lastCoffeeDisplayTime.length() == 0 && timeToDisplay.length() > 0);
        
        if (timeNeedsUpdate) {
            // Clear starting 1px above draw position to capture ascenders, and 18px tall total
            // Make sure we clear a wider area to prevent other functions from clearing it
            tft.fillRect(timeTextX - 2, timeTextY - 1, 44, 20, BACKGROUND);  // Clear time text area with extra padding
            
            if (timeToDisplay.length() > 0) {
                tft.setTextSize(1);
                tft.setTextColor(statusColor, BACKGROUND);
                tft.drawString(timeToDisplay, timeTextX, timeTextY, 1);  // Font 1 (small)
                lastCoffeeDisplayTime = timeToDisplay;  // Cache the displayed time
            } else if (lastCoffeeDisplayTime.length() > 0) {
                // Redraw cached time if scheduledTime is empty
                tft.setTextSize(1);
                tft.setTextColor(statusColor, BACKGROUND);
                tft.drawString(lastCoffeeDisplayTime, timeTextX, timeTextY, 1);
            }
        }
    } else {
        // Portrait orientation (0 or 2): use text display using position matrix
        int coffeeY = coffeePos.portrait.y;
        tft.fillRect(0, coffeeY, 320, 20, BACKGROUND);

        // Display coffee machine status with icon indicator
        tft.setTextSize(1); // Match the weather text size
        tft.setTextColor(statusColor);
        String icon = coffeeMachine.status == "On" ? "●" : "○";
        String statusInfo = "Coffee: " + icon + " " + coffeeMachine.status;
        if (coffeeMachine.status == "On" && coffeeMachine.scheduledTime.length() > 0) {
            statusInfo += " @ " + coffeeMachine.scheduledTime; // Append scheduled time if on
        }
        if (coffeeMachine.esp32Status == "offline") {
            statusInfo += " [OFFLINE]";
        }

        tft.drawString(statusInfo, coffeePos.portrait.x, coffeeY, 2); // Position below weather
    }
    
    // Update last status key to track changes
    lastCoffeeStatus = currentStatusKey;
}

// Helper function to get trail status icon
String getTrailStatusIcon(String status) {
    if (status == "open") return "●";
    else if (status == "closed") return "●";
    else if (status == "wet" || status == "caution") return "◐";
    else if (status == "freeze") return "○";
    return "?";
}

void updateTrailDisplay() {
    // Use position matrix for trail Y positions
    int mombaY, johnBryanY, caesarCreekY, clearY;
    int clearHeight = 50;
    
    if (currentRotation == 1 || currentRotation == 3) { // Landscape orientations
        mombaY = trailPos.landscape.mombaY;
        johnBryanY = trailPos.landscape.johnBryanY;
        caesarCreekY = trailPos.landscape.caesarCreekY;
        clearY = mombaY;
    } else { // Portrait orientations
        mombaY = trailPos.portrait.mombaY;
        johnBryanY = trailPos.portrait.johnBryanY;
        caesarCreekY = trailPos.portrait.caesarCreekY;
        clearY = mombaY;
    }
    
    // Clear the full trail status area (adjusted for rotation)
    // Use full screen width to ensure all text is cleared, including long status strings
    int clearWidth = (currentRotation == 0 || currentRotation == 2) ? 240 : 320;  // Full width
    tft.fillRect(0, clearY, clearWidth, clearHeight, BACKGROUND);

    // Display trail statuses with icons
    tft.setTextSize(1);

    // Momba Trail - use position matrix X position
    // Clear individual line area before drawing to prevent text remnants
    tft.fillRect(0, mombaY - 2, clearWidth, 22, BACKGROUND);  // Clear individual line with padding
    uint16_t mombaColor = getTrailStatusColor(mombaTrail.status);
    tft.setTextColor(mombaColor, BACKGROUND);  // Use background color to prevent artifacts
    String mombaIcon = getTrailStatusIcon(mombaTrail.status);
    tft.drawString(mombaIcon + " Momba " + mombaTrail.lastUpdate, trailStatusXPos, mombaY, 2);

    // John Bryan Trail
    // Clear individual line area before drawing
    tft.fillRect(0, johnBryanY - 2, clearWidth, 22, BACKGROUND);  // Clear individual line with padding
    uint16_t johnBryanColor = getTrailStatusColor(johnBryanTrail.status);
    tft.setTextColor(johnBryanColor, BACKGROUND);  // Use background color to prevent artifacts
    String jbIcon = getTrailStatusIcon(johnBryanTrail.status);
    tft.drawString(jbIcon + " JBryan " + johnBryanTrail.lastUpdate, trailStatusXPos, johnBryanY, 2);

    // Caesar Creek Trail
    // Clear individual line area before drawing to prevent text remnants
    tft.fillRect(0, caesarCreekY - 2, clearWidth, 22, BACKGROUND);  // Clear individual line with padding
    uint16_t caesarCreekColor = getTrailStatusColor(caesarCreekTrail.status);
    tft.setTextColor(caesarCreekColor, BACKGROUND);  // Use background color to prevent artifacts
    String ccIcon = getTrailStatusIcon(caesarCreekTrail.status);
    tft.drawString(ccIcon + " C.Creek " + caesarCreekTrail.lastUpdate, trailStatusXPos, caesarCreekY, 2);
}

// Helper function to check if a year is a leap year
bool isLeapYear(int year) {
    return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

// Helper function to get days in a year
int getDaysInYear(int year) {
    return isLeapYear(year) ? 366 : 365;
}

// Helper function to calculate day of year from month and day
int getDayOfYear(int month, int day, int year) {
    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int dayOfYear = day;
    
    for (int i = 0; i < month - 1; i++) {
        dayOfYear += daysInMonth[i];
    }
    
    // Add leap day if it's a leap year and we're past February
    if (month > 2 && isLeapYear(year)) {
        dayOfYear += 1;
    }
    
    return dayOfYear;
}

// Function to calculate days until December 11th
int calculateDaysUntil1211() {
    // Get current epoch time
    timeClient.update();
    unsigned long currentEpoch = timeClient.getEpochTime();
    
    // Calculate current date from epoch time
    // Epoch: January 1, 1970 00:00:00 UTC
    unsigned long secondsPerDay = 86400;
    unsigned long daysSinceEpoch = currentEpoch / secondsPerDay;
    
    // Approximate current year (starting from 1970)
    // Account for leap years: approximately 365.25 days per year
    int approximateYear = 1970 + (daysSinceEpoch / 365);
    
    // Refine year calculation by checking actual days
    int currentYear = approximateYear;
    unsigned long daysToCurrentYear = 0;
    for (int year = 1970; year < currentYear; year++) {
        daysToCurrentYear += getDaysInYear(year);
    }
    
    // Adjust if we overshot
    while (daysToCurrentYear > daysSinceEpoch && currentYear > 1970) {
        currentYear--;
        daysToCurrentYear -= getDaysInYear(currentYear);
    }
    
    // Calculate remaining days in current year
    unsigned long daysIntoYear = daysSinceEpoch - daysToCurrentYear;
    
    // Find current month and day
    int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (isLeapYear(currentYear)) {
        daysInMonth[1] = 29;  // February has 29 days in leap year
    }
    
    int currentMonth = 1;
    int currentDay = 1;
    unsigned long remainingDays = daysIntoYear;
    
    for (int i = 0; i < 12; i++) {
        if (remainingDays >= daysInMonth[i]) {
            remainingDays -= daysInMonth[i];
            currentMonth++;
        } else {
            currentDay = remainingDays + 1;
            break;
        }
    }
    
    // Calculate target date (December 11 of current year, or next year if already past)
    int targetYear = currentYear;
    int targetMonth = 12;
    int targetDay = 11;
    
    // If December 11 has already passed this year, target next year
    if (currentMonth > 12 || (currentMonth == 12 && currentDay > 11)) {
        targetYear = currentYear + 1;
    }
    
    // Calculate day of year for both dates
    int currentDayOfYear = getDayOfYear(currentMonth, currentDay, currentYear);
    int targetDayOfYear = getDayOfYear(targetMonth, targetDay, targetYear);
    
    // Calculate days difference
    int daysDiff;
    if (targetYear == currentYear) {
        // Same year: simple difference
        daysDiff = targetDayOfYear - currentDayOfYear;
    } else {
        // Different year: days remaining this year + days in target year
        int daysInCurrentYear = getDaysInYear(currentYear);
        daysDiff = (daysInCurrentYear - currentDayOfYear) + targetDayOfYear;
    }
    
    return daysDiff;
}

// Function to update countdown timer display
void updateCountdownDisplay() {
    int daysRemaining = calculateDaysUntil1211();
    
    // Only update if days changed or if forced redraw
    if (daysRemaining != lastCountdownDays || needRedraw) {
        // Get screen dimensions and countdown position
        int screenWidth = (currentRotation == 0 || currentRotation == 2) ? 240 : 320;
        int screenHeight = (currentRotation == 0 || currentRotation == 2) ? 320 : 240;
        int countdownY = (currentRotation == 0 || currentRotation == 2) ? countdownPos.portrait.y : countdownPos.landscape.y;
        
        // Format countdown text
        String countdownText = String(daysRemaining) + " days";
        
        // Use large text size (size 2 for font 4 = big text)
        tft.setTextSize(2);
        
        // Calculate text width (approximate: font 4 size 2 is about 14-16 pixels per character for large numbers)
        // Be generous to account for variable character widths
        int textWidth = countdownText.length() * 16;
        
        // Position at right bottom (right-aligned), moved 15 pixels to the left
        int countdownX = screenWidth - textWidth - 5 - 65;  // 5px margin from right edge + 15px left offset
        
        // Clear area (make sure we clear enough space for large text)
        int clearWidth = textWidth + 15;  // Extra padding for large text
        int clearHeight = 35;  // Height for large text (font 4 size 2 is tall)
        int clearX = countdownX - 5;  // Start clearing a bit before text
        
        // Ensure we don't go out of bounds
        if (clearX < 0) clearX = 0;
        if (clearX + clearWidth > screenWidth) clearWidth = screenWidth - clearX;
        if (countdownY + clearHeight > screenHeight) clearHeight = screenHeight - countdownY;
        
        // Clear the area
        tft.fillRect(clearX, countdownY, clearWidth, clearHeight, BACKGROUND);
        
        // Draw countdown text in big text
        tft.setTextColor(TEXT_COLOR, BACKGROUND);
        tft.drawString(countdownText, countdownX, countdownY, 4);  // Font 4 for big text
        
        lastCountdownDays = daysRemaining;
    }
}


bool fetchTrailStatus(TrailInfo &trail, const String &trailId) {
    HTTPClient http;
    // Use cached endpoint (GET) - server refreshes data every 30 minutes
    String url = "http://" + String(serverHost) + ":" + String(serverPort) + "/api/trail/trails/" + trailId;
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        // Parse JSON response
        StaticJsonDocument<400> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            trail.status = doc["status"].as<String>();
            String fullDate = doc["last_update"].as<String>();
            trail.lastUpdate = fullDate.substring(0, 10); // Extract only the date part
            Serial.println("Extracted trail status: " + trail.status + ", Date: " + trail.lastUpdate); // Debug statement
            http.end();
            return true;
        } else {
            Serial.println("Failed to parse trail JSON: " + String(error.c_str())); // Debug statement
        }
    } else {
        Serial.println("HTTP request for trail status failed with code: " + String(httpCode)); // Debug statement
    }
    http.end();
    return false;
}

// Function to force server-side refresh of all trails (POST request)
// Call this when user presses refresh button for fresh data from sources
bool refreshAllTrails() {
    HTTPClient http;
    String url = "http://" + String(serverHost) + ":" + String(serverPort) + "/api/trail/refresh";

    Serial.println("Forcing server-side trail refresh (POST)...");
    http.begin(url);
    http.setTimeout(30000);  // 30 second timeout - refresh can take time
    int httpCode = http.POST("");  // Empty POST body

    if (httpCode == HTTP_CODE_OK) {
        Serial.println("Server trail refresh completed successfully");
        http.end();
        return true;
    } else {
        Serial.println("Server trail refresh failed with code: " + String(httpCode));
    }
    http.end();
    return false;
}

// Function to fetch printer status from Moonraker API
bool fetchPrinterStatus(PrinterInfo &printer) {
    HTTPClient http;
    // Moonraker API endpoint - query both webhooks and print_stats to accurately detect printing
    String url = "http://" + printer.host + "/printer/objects/query?webhooks&print_stats";
    
    http.begin(url);
    http.setTimeout(2000); // 2 second timeout
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        // Parse JSON response - need larger buffer for print_stats
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error && doc.containsKey("result") && doc["result"].containsKey("status")) {
            JsonObject status = doc["result"]["status"];
            String state = "unknown";
            bool isPrinting = false;
            String previousStatus = printer.status; // Store previous status to detect transitions
            
            // First check print_stats to see if there's an active print
            if (status.containsKey("print_stats")) {
                JsonObject printStats = status["print_stats"];
                if (printStats.containsKey("state")) {
                    String printState = printStats["state"].as<String>();
                    // If print_stats shows printing, the printer is definitely printing
                    if (printState == "printing") {
                        isPrinting = true;
                        printer.status = "printing";
                        printer.lastStatus = printer.status;
                        Serial.println("Printer " + printer.name + " status: printing (from print_stats)");
                        http.end();
                        return true;
                    } else if (printState == "paused") {
                        printer.status = "paused";
                        printer.lastStatus = printer.status;
                        Serial.println("Printer " + printer.name + " status: paused (from print_stats)");
                        http.end();
                        return true;
                    } else if (printState == "complete") {
                        printer.status = "complete";
                        // Check if we transitioned from printing to complete
                        if (previousStatus == "printing" || printer.lastStatus == "printing") {
                            printer.isFlashing = true;
                            printer.flashStartTime = millis();
                            Serial.println("Printer " + printer.name + " print completed! Starting flash animation.");
                        }
                        printer.lastStatus = printer.status;
                        http.end();
                        return true;
                    }
                }
            }
            
            // If not printing according to print_stats, check webhooks state
            if (status.containsKey("webhooks")) {
                JsonObject webhooks = status["webhooks"];
                if (webhooks.containsKey("state")) {
                    state = webhooks["state"].as<String>();
                    // Map Moonraker states to our status
                    if (state == "printing") {
                        printer.status = "printing";
                    } else if (state == "ready" || state == "standby") {
                        printer.status = "ready";
                    } else if (state == "paused") {
                        printer.status = "paused";
                    } else if (state == "error") {
                        printer.status = "error";
                    } else if (state == "idle" || state == "complete" || state == "cancelled") {
                        printer.status = state; // Keep as-is for white color
                        // Check if we transitioned from printing to complete/idle
                        if ((state == "complete" || state == "idle") && (previousStatus == "printing" || printer.lastStatus == "printing")) {
                            printer.isFlashing = true;
                            printer.flashStartTime = millis();
                            Serial.println("Printer " + printer.name + " print completed! Starting flash animation.");
                        }
                    } else {
                        printer.status = state; // Use state as-is for unknown states
                    }
                    Serial.println("Printer " + printer.name + " status: " + printer.status + " (raw state: " + state + ")");
                    printer.lastStatus = printer.status;
                    http.end();
                    return true;
                }
            }
        } else {
            Serial.println("Failed to parse printer JSON for " + printer.name + ": " + String(error.c_str()));
            if (error) {
                Serial.println("Error details: " + String(error.c_str()));
            }
        }
    } else {
        Serial.println("HTTP request for printer " + printer.name + " failed with code: " + String(httpCode));
        printer.status = "offline";
        printer.lastStatus = printer.status;
    }
    http.end();
    return (httpCode == HTTP_CODE_OK);
}

// Function to update printer status display
void updatePrinterDisplay() {
    unsigned long currentMillis = millis();
    
    // Get printer icon positions
    int x1, x2, y;
    if (currentRotation == 1 || currentRotation == 3) { // Landscape
        x1 = printerPos.landscape.x1;
        x2 = printerPos.landscape.x2;
        y = printerPos.landscape.y;
    } else { // Portrait
        x1 = printerPos.portrait.x1;
        x2 = printerPos.portrait.x2;
        y = printerPos.portrait.y;
    }
    
    // Handle flashing for Sovol printer
    uint16_t sovolColor;
    if (sovolPrinter.isFlashing) {
        // Check if flash duration has expired
        if (currentMillis - sovolPrinter.flashStartTime >= PRINTER_FLASH_DURATION) {
            sovolPrinter.isFlashing = false;
            sovolColor = getPrinterStatusColor(sovolPrinter.status);
        } else {
            // Flash between white and status color
            bool flashOn = ((currentMillis - sovolPrinter.flashStartTime) / PRINTER_FLASH_INTERVAL) % 2 == 0;
            sovolColor = flashOn ? TFT_WHITE : getPrinterStatusColor(sovolPrinter.status);
        }
    } else {
        sovolColor = getPrinterStatusColor(sovolPrinter.status);
    }
    drawPrinterIcon(sovolColor, x1, y);
    
    // Handle flashing for Mandrain printer
    uint16_t mandrainColor;
    if (mandrainPrinter.isFlashing) {
        // Check if flash duration has expired
        if (currentMillis - mandrainPrinter.flashStartTime >= PRINTER_FLASH_DURATION) {
            mandrainPrinter.isFlashing = false;
            mandrainColor = getPrinterStatusColor(mandrainPrinter.status);
        } else {
            // Flash between white and status color
            bool flashOn = ((currentMillis - mandrainPrinter.flashStartTime) / PRINTER_FLASH_INTERVAL) % 2 == 0;
            mandrainColor = flashOn ? TFT_WHITE : getPrinterStatusColor(mandrainPrinter.status);
        }
    } else {
        mandrainColor = getPrinterStatusColor(mandrainPrinter.status);
    }
    drawPrinterIcon(mandrainColor, x2, y);
}

void processSerialInput() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command.startsWith("timeX ")) {
            timeXPos = command.substring(6).toInt();
            updateTimeDisplay();
        } else if (command.startsWith("dateX ")) {
            dateXPos = command.substring(6).toInt();
            updateTimeDisplay();
        } else if (command.startsWith("weatherX ")) {
            weatherXPos = command.substring(9).toInt();
            updateWeatherDisplay();
        } else if (command.startsWith("iconX ")) {
            // Icon position now in weatherIconPos matrix - update manually if needed
            Serial.println("iconX command - use position matrix to modify weatherIconPos");
            updateWeatherDisplay();
        } else if (command.startsWith("coffeeStatusX ")) {
            // Coffee position now in coffeePos matrix - update manually if needed
            Serial.println("coffeeStatusX command - use position matrix to modify coffeePos");
            updateCoffeeMachineDisplay();
        } else if (command.startsWith("coffeeTimeX ")) {
            // Coffee time position now in coffeePos matrix - update manually if needed
            Serial.println("coffeeTimeX command - use position matrix to modify coffeePos");
            updateCoffeeMachineDisplay();
        }
    }
}

// Function to calculate DST offset based on current date
// US DST: Second Sunday in March to First Sunday in November
// Simplified: March-October = EDT (UTC-4), November-February = EST (UTC-5)
// Note: Uses epoch time to determine month since NTPClient doesn't have date methods
int getDSTOffset() {
    // Save current offset to restore later (use cached value since NTPClient doesn't have getTimeOffset)
    int savedOffset = cachedDSTOffset;
    
    // Get UTC time first to determine month (need UTC to accurately determine DST period)
    timeClient.setTimeOffset(0);
    timeClient.update();
    
    // Get epoch time and calculate month
    unsigned long epochTime = timeClient.getEpochTime();
    
    // Restore original offset
    timeClient.setTimeOffset(savedOffset);
    
    // Calculate month from epoch time
    // January 1, 1970 (Unix epoch) was a Thursday
    // Calculate days since epoch, then approximate month
    // This is approximate but sufficient for DST detection
    int daysSinceEpoch = epochTime / 86400;
    
    // Approximate month calculation (accounting for leap years roughly)
    // Average days per month ≈ 30.44
    // Calculate which "30-day month" we're in, accounting for years
    int yearsSinceEpoch = daysSinceEpoch / 365; // Rough years
    int leapDays = yearsSinceEpoch / 4; // Approximate leap days
    int adjustedDays = daysSinceEpoch - (yearsSinceEpoch * 365 + leapDays);
    int month = (adjustedDays / 30) + 1;
    
    // Normalize month (1-12)
    if (month > 12) month = ((month - 1) % 12) + 1;
    if (month < 1) month = 12;
    
    // More accurate: use day of year to better approximate month
    // Days 0-59 ≈ Jan-Feb, 60-89 ≈ Mar, 90-119 ≈ Apr, etc.
    int dayOfYear = adjustedDays % 365;
    if (dayOfYear < 60) month = (dayOfYear < 31) ? 1 : 2;
    else if (dayOfYear < 90) month = 3;
    else if (dayOfYear < 120) month = 4;
    else if (dayOfYear < 151) month = 5;
    else if (dayOfYear < 181) month = 6;
    else if (dayOfYear < 212) month = 7;
    else if (dayOfYear < 243) month = 8;
    else if (dayOfYear < 273) month = 9;
    else if (dayOfYear < 304) month = 10;
    else if (dayOfYear < 334) month = 11;
    else month = 12;
    
    // DST period: March (3) to October (10) is EDT (UTC-4)
    // Outside this period is EST (UTC-5)
    if (month >= 3 && month <= 10) {
        return -4 * 3600; // EDT (UTC-4)
    } else {
        return -5 * 3600; // EST (UTC-5)
    }
}

// Function to fetch time from local server (timezone-aware)
bool fetchTimeFromLocalServer() {
    HTTPClient http;
    String url = "http://" + String(serverHost) + ":" + String(serverPort) + "/api/time";
    
    http.begin(url);
    http.setTimeout(2000); // Reduced to 2 second timeout to minimize blocking
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        // Parse JSON response
        StaticJsonDocument<300> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            // Try different possible JSON formats
            if (doc.containsKey("time")) {
                String timeStr = doc["time"].as<String>();
                // Parse time string (format: "HH:MM:SS" or "HH:MM")
                int colonPos = timeStr.indexOf(':');
                if (colonPos > 0) {
                    int hours = timeStr.substring(0, colonPos).toInt();
                    int nextColon = timeStr.indexOf(':', colonPos + 1);
                    int minutes = timeStr.substring(colonPos + 1, nextColon > 0 ? nextColon : timeStr.length()).toInt();
                    int seconds = 0;
                    if (nextColon > 0 && nextColon < timeStr.length() - 1) {
                        seconds = timeStr.substring(nextColon + 1).toInt();
                    }
                    
                    currentTime.time = (hours < 10 ? "0" : "") + String(hours) + ":" +
                                      (minutes < 10 ? "0" : "") + String(minutes);
                    currentTime.seconds = (seconds < 10 ? "0" : "") + String(seconds);
                    http.end();
                    Serial.println("Time fetched from local server: " + currentTime.time + ":" + currentTime.seconds);
                    return true;
                }
            } else if (doc.containsKey("hours") && doc.containsKey("minutes") && doc.containsKey("seconds")) {
                int hours = doc["hours"].as<int>();
                int minutes = doc["minutes"].as<int>();
                int seconds = doc["seconds"].as<int>();
                
                currentTime.time = (hours < 10 ? "0" : "") + String(hours) + ":" +
                                  (minutes < 10 ? "0" : "") + String(minutes);
                currentTime.seconds = (seconds < 10 ? "0" : "") + String(seconds);
                http.end();
                Serial.println("Time fetched from local server: " + currentTime.time + ":" + currentTime.seconds);
                return true;
            }
        } else {
            Serial.println("Failed to parse time JSON from local server: " + String(error.c_str()));
        }
    } else {
        Serial.println("Local server time endpoint not available (code: " + String(httpCode) + "), falling back to NTP");
    }
    http.end();
    return false;
}

// Function to fetch time from NTP with DST handling
bool fetchTimeFromNTP() {
    unsigned long currentMillis = millis();
    
    // Update DST offset periodically (not every call, to reduce overhead)
    if (currentMillis - lastDSTCheck >= DST_CHECK_INTERVAL || lastDSTCheck == 0) {
        cachedDSTOffset = getDSTOffset();
        timeClient.setTimeOffset(cachedDSTOffset);
        lastDSTCheck = currentMillis;
        Serial.println("DST offset updated to: " + String(cachedDSTOffset/3600) + " hours");
    }
    
    timeClient.update();
    
    // Get time components
    int hours = timeClient.getHours();
    int minutes = timeClient.getMinutes();
    int seconds = timeClient.getSeconds();
    
    // Format time string with leading zeros (24-hour format)
    currentTime.time = (hours < 10 ? "0" : "") + String(hours) + ":" +
                      (minutes < 10 ? "0" : "") + String(minutes);
    currentTime.seconds = (seconds < 10 ? "0" : "") + String(seconds);
    
    return true;
}

// Function to fetch time from API (tries local server first, falls back to NTP)
bool fetchTime() {
    // Try local server first (timezone-aware)
    if (useLocalServerTime && WiFi.status() == WL_CONNECTED) {
        if (fetchTimeFromLocalServer()) {
            return true;
        }
    }
    
    // Fall back to NTP with DST handling
    return fetchTimeFromNTP();
}

// Function to fetch date from API
bool fetchDate() {
    HTTPClient http;
    String url = "http://" + String(serverHost) + ":" + String(serverPort) + "/api/date";
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        // Parse JSON response
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            const char* date = doc["date"];
            currentTime.date = String(date); // Store date separately (with year, will be removed during display)
            Serial.println("Extracted date: " + currentTime.date); // Debug statement
            http.end();
            return true;
        } else {
            Serial.println("Failed to parse date JSON: " + String(error.c_str())); // Debug statement
        }
    } else {
        Serial.println("HTTP request for date failed with code: " + String(httpCode)); // Debug statement
    }
    http.end();
    return false;
}

// Function to fetch weather from API
bool fetchWeather() {
    HTTPClient http;
    String url = "http://" + String(serverHost) + ":" + String(serverPort) + "/api/weather/current";
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        // Parse JSON response
        StaticJsonDocument<400> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            currentWeather.conditions = doc["conditions"].as<String>();
            currentWeather.temperature = doc["temperature"].as<int>();
            currentWeather.feels_like = doc["feels_like"].as<int>();
            currentWeather.humidity = doc["humidity"].as<int>();
            currentWeather.icon = doc["icon"].as<String>();
            Serial.println("Extracted weather: " + currentWeather.conditions); // Debug statement
            http.end();
            return true;
        } else {
            Serial.println("Failed to parse weather JSON: " + String(error.c_str())); // Debug statement
        }
    } else {
        Serial.println("HTTP request for weather failed with code: " + String(httpCode)); // Debug statement
    }
    http.end();
    return false;
}

// Function to fetch weather forecast (today and tomorrow) from API
bool fetchForecast() {
    HTTPClient http;
    String url = "http://" + String(serverHost) + ":" + String(serverPort) + "/api/weather/forecast";

    http.begin(url);
    http.setTimeout(5000);  // 5 second timeout
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();

        // Parse JSON response - expecting array of forecast days
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {
            JsonArray forecasts = doc.as<JsonArray>();
            if (forecasts.size() >= 2) {
                // Today's forecast
                weatherForecast.today.date = forecasts[0]["date"].as<String>();
                weatherForecast.today.high = forecasts[0]["high"].as<int>();
                weatherForecast.today.low = forecasts[0]["low"].as<int>();
                weatherForecast.today.conditions = forecasts[0]["conditions"].as<String>();
                weatherForecast.today.icon = forecasts[0]["icon"].as<String>();

                // Tomorrow's forecast
                weatherForecast.tomorrow.date = forecasts[1]["date"].as<String>();
                weatherForecast.tomorrow.high = forecasts[1]["high"].as<int>();
                weatherForecast.tomorrow.low = forecasts[1]["low"].as<int>();
                weatherForecast.tomorrow.conditions = forecasts[1]["conditions"].as<String>();
                weatherForecast.tomorrow.icon = forecasts[1]["icon"].as<String>();

                weatherForecast.valid = true;
                Serial.println("Forecast fetched successfully");
                http.end();
                return true;
            }
        } else {
            Serial.println("Failed to parse forecast JSON: " + String(error.c_str()));
        }
    } else {
        Serial.println("HTTP request for forecast failed with code: " + String(httpCode));
    }
    http.end();

    // Fallback: Use current weather data for "today" if forecast endpoint unavailable
    if (currentWeather.conditions.length() > 0) {
        Serial.println("Using current weather as fallback for forecast");
        weatherForecast.today.date = "Today";
        weatherForecast.today.high = currentWeather.temperature;
        weatherForecast.today.low = currentWeather.feels_like;  // Use feels_like as low estimate
        weatherForecast.today.conditions = currentWeather.conditions;
        weatherForecast.today.icon = currentWeather.icon;

        // Tomorrow is unknown, use placeholder
        weatherForecast.tomorrow.date = "Tomorrow";
        weatherForecast.tomorrow.high = 0;
        weatherForecast.tomorrow.low = 0;
        weatherForecast.tomorrow.conditions = "Forecast unavailable";
        weatherForecast.tomorrow.icon = "";

        weatherForecast.valid = true;
        return true;
    }

    weatherForecast.valid = false;
    return false;
}

// Function to fetch coffee machine status from API
bool fetchCoffeeMachineStatus() {
    HTTPClient http;
    String url = "http://" + String(serverHost) + ":" + String(serverPort) + "/api/coffee/status";
    
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        
        // Parse JSON response
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (!error) {
            coffeeMachine.status = doc["status"].as<String>();
            coffeeMachine.scheduledTime = doc["time"].as<String>();
            coffeeMachine.esp32Status = doc["esp32_status"].as<String>();
            
            // Track the scheduled time when coffee is on, for auto-schedule feature
            if (coffeeMachine.status == "On" && coffeeMachine.scheduledTime.length() > 0) {
                lastCoffeeScheduledTime = coffeeMachine.scheduledTime;
            }
            
            Serial.println("Extracted coffee machine status: " + coffeeMachine.status); // Debug statement
            http.end();
            return true;
        } else {
            Serial.println("Failed to parse coffee machine JSON: " + String(error.c_str())); // Debug statement
        }
    } else {
        Serial.println("HTTP request for coffee machine status failed with code: " + String(httpCode)); // Debug statement
    }
    http.end();
    return false;
}

// Function to set coffee machine schedule via API
bool setCoffeeSchedule(String time) {
    HTTPClient http;
    String url = "http://" + String(serverHost) + ":" + String(serverPort) + "/api/coffee/on?time=" + time;
    
    http.begin(url);
    http.setTimeout(3000);  // 3 second timeout
    
    int httpCode = http.POST("");  // Empty body, time is in URL parameter
    
    if (httpCode == HTTP_CODE_OK || httpCode == 201) {
        Serial.println("Coffee schedule set to: " + time);
        http.end();
        return true;
    } else {
        Serial.println("Failed to set coffee schedule. HTTP code: " + String(httpCode));
    }
    http.end();
    return false;
}

// Function to toggle coffee machine on/off
bool toggleCoffeeMachine() {
    HTTPClient http;
    String url;
    
    // If coffee is currently on, turn it off
    if (coffeeMachine.status == "On") {
        url = "http://" + String(serverHost) + ":" + String(serverPort) + "/api/coffee/off";
        Serial.println("Turning coffee machine OFF");
    } else {
        // If off, turn it on with the last scheduled time (or activate immediately if no time)
        if (lastCoffeeScheduledTime.length() > 0) {
            url = "http://" + String(serverHost) + ":" + String(serverPort) + "/api/coffee/on?time=" + lastCoffeeScheduledTime;
            Serial.println("Turning coffee machine ON with time: " + lastCoffeeScheduledTime);
        } else {
            url = "http://" + String(serverHost) + ":" + String(serverPort) + "/api/coffee/on";
            Serial.println("Activating coffee machine immediately");
        }
    }
    
    http.begin(url);
    http.setTimeout(3000);  // 3 second timeout
    
    int httpCode = http.POST("");
    
    if (httpCode == HTTP_CODE_OK || httpCode == 201) {
        Serial.println("Coffee machine toggle successful");
        // Update display immediately
        if (fetchCoffeeMachineStatus()) {
            updateCoffeeMachineDisplay();
        }
        http.end();
        return true;
    } else {
        Serial.println("Failed to toggle coffee machine. HTTP code: " + String(httpCode));
    }
    http.end();
    return false;
}

void setDummyStockData() {
    spyStock.symbol = "SPY";
    spyStock.price = 495.28;
    spyStock.change = 2.34;
    spyStock.changePercent = 0.47;
    Serial.println("Using dummy SPY data");
}

// Function to fetch stock price from Alpha Vantage API
bool fetchStockPrice() {
    WiFiClientSecure *client = new WiFiClientSecure;
    if(client) {
        client->setInsecure();
        
        HTTPClient http;
        // Yahoo Finance API endpoint - no key required
        String url = "https://query1.finance.yahoo.com/v8/finance/chart/SPY";
        
        Serial.println("Fetching SPY price from Yahoo Finance...");
        http.begin(*client, url);
        http.setTimeout(3000);  // Reduced from 10000 to 3000ms to minimize blocking
        
        int httpCode = http.GET();
        
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            
            // Parse JSON response - Yahoo Finance format
            StaticJsonDocument<2048> doc; // Larger buffer for Yahoo response
            DeserializationError error = deserializeJson(doc, payload);
            
            if (!error && doc.containsKey("chart") && doc["chart"]["result"][0]["meta"].containsKey("regularMarketPrice")) {
                JsonObject result = doc["chart"]["result"][0];
                JsonObject meta = result["meta"];
                
                spyStock.symbol = "SPY";
                spyStock.price = meta["regularMarketPrice"].as<float>();
                
                // Calculate change
                float previousClose = meta["previousClose"].as<float>();
                spyStock.change = spyStock.price - previousClose;
                spyStock.changePercent = (spyStock.change / previousClose) * 100;
                
                http.end();
                delete client;
                return true;
            } else {
                Serial.println("Failed to parse Yahoo Finance data");
                setDummyStockData();
                http.end();
                delete client;
                return true;
            }
        } else {
            setDummyStockData();
            http.end();
            delete client;
            return true;
        }
    }
    
    setDummyStockData();
    return true;
}
// Add this function before setup()
void handleRotation() {
    static bool lastButtonState = HIGH;
    unsigned long currentTime = millis();
    
    bool currentButtonState = digitalRead(BUTTON_PIN);
    
    if (currentButtonState == LOW && lastButtonState == HIGH) {
        if (currentTime - lastButtonPress >= DEBOUNCE_DELAY) {
            lastButtonPress = currentTime;
            currentRotation = (currentRotation + 1) % 4;
            tft.setRotation(currentRotation);
            
            // Force complete refresh of display
            tft.fillScreen(BACKGROUND);
            lastTime = "";  // Reset cached values to force redraw
            lastSeconds = "";
            lastDate = "";
            lastStockDisplay = "";  // Reset stock cache
            updateTimeDisplay();
            updateStockDisplay();
            updateWeatherDisplay();
            updateCoffeeMachineDisplay();
            updateTrailDisplay();
            // updateCountdownDisplay();  // Hidden for now
            updatePrinterDisplay();
            drawWeatherIconStatic();  // Draw static weather icon
        }
    }
    
    lastButtonState = currentButtonState;
}

void handleScreenToggle() {
    static bool lastToggleState = HIGH;
    static unsigned long lastPressTime = 0;
    static bool waitingForDoublePress = false;
    const unsigned long DOUBLE_PRESS_WINDOW = 400;  // 400ms window for double press
    
    unsigned long currentTime = millis();
    bool currentToggleState = digitalRead(SCREEN_TOGGLE_PIN);
    
    // Detect button press (LOW = pressed)
    if (currentToggleState == LOW && lastToggleState == HIGH) {
        if (currentTime - lastScreenToggle >= DEBOUNCE_DELAY) {
            lastScreenToggle = currentTime;
            
            // Check if this is within the double-press window
            if (waitingForDoublePress && (currentTime - lastPressTime) < DOUBLE_PRESS_WINDOW) {
                // Double press detected - Toggle coffee machine
                waitingForDoublePress = false;
                Serial.println("Double press detected - Toggling coffee machine");
                toggleCoffeeMachine();
            } else {
                // First press - start waiting for potential double press
                waitingForDoublePress = true;
                lastPressTime = currentTime;
            }
        }
    }
    
    // Check if double-press window expired (single press confirmed)
    if (waitingForDoublePress && (currentTime - lastPressTime) >= DOUBLE_PRESS_WINDOW) {
        waitingForDoublePress = false;
        
        // Single press - Toggle screen ON/OFF
        isScreenOn = !isScreenOn;
        
        if (isScreenOn) {
            // Turn screen ON
            digitalWrite(TFT_BL, HIGH);  // Turn on backlight
            
            // Refresh everything - reset cache to force redraw
            tft.fillScreen(BACKGROUND);
            lastTime = "";  // Reset cached values to force redraw
            lastSeconds = "";
            lastDate = "";
            lastStockDisplay = "";  // Reset stock cache
            updateTimeDisplay();
            updateStockDisplay();
            updateWeatherDisplay();
            updateCoffeeMachineDisplay();
            updateTrailDisplay();
            // updateCountdownDisplay();  // Hidden for now
            updatePrinterDisplay();
            drawWeatherIconStatic();  // Draw static weather icon
            
            Serial.println("Single press - Screen ON");
        } else {
            // Turn screen OFF
            digitalWrite(TFT_BL, LOW);   // Turn off backlight
            
            // AUTO-SCHEDULE FEATURE: When screen turns off, set coffee to last scheduled time
            if (lastCoffeeScheduledTime.length() > 0) {
                Serial.println("Auto-scheduling coffee to: " + lastCoffeeScheduledTime);
                if (setCoffeeSchedule(lastCoffeeScheduledTime)) {
                    Serial.println("Coffee auto-schedule successful");
                } else {
                    Serial.println("Coffee auto-schedule failed");
                }
            } else {
                Serial.println("No previous coffee schedule time available for auto-schedule");
            }
            
            Serial.println("Single press - Screen OFF");
        }
    }
    
    lastToggleState = currentToggleState;
}

void handleRefresh() {
    static bool lastRefreshState = HIGH;
    unsigned long currentTime = millis();
    bool currentRefreshState = digitalRead(REFRESH_PIN);

    // Button just pressed (transition from HIGH to LOW)
    if (currentRefreshState == LOW && lastRefreshState == HIGH) {
        if (currentTime - lastRefreshPress >= DEBOUNCE_DELAY) {
            refreshHoldStart = currentTime;  // Start tracking hold time
            Serial.println("Refresh button pressed - hold for forecast view");
        }
    }

    // Button is being held down
    if (currentRefreshState == LOW && refreshHoldStart > 0) {
        unsigned long holdDuration = currentTime - refreshHoldStart;

        // If held long enough and not already showing forecast, show it
        if (holdDuration >= HOLD_THRESHOLD && !isShowingForecast) {
            isShowingForecast = true;
            Serial.println("Hold detected - showing forecast view");

            // Fetch forecast data and display
            fetchForecast();
            drawForecastView();
        }
    }

    // Button released (transition from LOW to HIGH)
    if (currentRefreshState == HIGH && lastRefreshState == LOW) {
        unsigned long holdDuration = currentTime - refreshHoldStart;

        // If we were showing forecast, return to main screen
        if (isShowingForecast) {
            isShowingForecast = false;
            Serial.println("Button released - returning to main screen");
            redrawMainScreen();
        }
        // Short press (not held long enough for forecast) - do normal refresh
        else if (holdDuration < HOLD_THRESHOLD && holdDuration > 0) {
            lastRefreshPress = currentTime;
            Serial.println("Manual refresh triggered");

            // Force immediate refresh of all data
            if (fetchTime() && fetchDate()) {
                updateTimeDisplay();
            }
            if (fetchStockPrice()) {
                updateStockDisplay();
            }
            if (fetchWeather()) {
                updateWeatherDisplay();
            }
            if (fetchCoffeeMachineStatus()) {
                updateCoffeeMachineDisplay();
            }
            // Force server to refresh trail data from sources, then fetch updated cache
            refreshAllTrails();
            if (fetchTrailStatus(mombaTrail, "momba") &&
                fetchTrailStatus(johnBryanTrail, "JohnBryan") &&
                fetchTrailStatus(caesarCreekTrail, "caesar_creek")) {
                updateTrailDisplay();
            }
            if (WiFi.status() == WL_CONNECTED) {
                fetchPrinterStatus(sovolPrinter);
                fetchPrinterStatus(mandrainPrinter);
            }
            updatePrinterDisplay();
            drawWeatherIconStatic();  // Draw static weather icon after refresh
        }

        refreshHoldStart = 0;  // Reset hold tracking
    }

    lastRefreshState = currentRefreshState;
}



void setup() {
    Serial.begin(115200);
    delay(1000); // Give time for serial to initialize
    
    Serial.println("ESP32 Status Screen Starting...");
    Serial.println("Initializing TFT display...");
    
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(BACKGROUND);
    tft.setTextColor(TEXT_COLOR, BACKGROUND);
    
    Serial.println("TFT initialized successfully");
    
    // Test display with a simple pattern
    tft.fillScreen(TFT_RED);
    delay(500);
    tft.fillScreen(TFT_GREEN);
    delay(500);
    tft.fillScreen(TFT_BLUE);
    delay(500);
    tft.fillScreen(BACKGROUND);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("ESP32 Ready", 50, 100);
    Serial.println("Display test pattern completed");
    
    Serial.println("Connecting to WiFi...");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Password length: ");
    Serial.println(strlen(password));
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int wifiAttempts = 0;
    while (WiFi.status() != WL_CONNECTED && wifiAttempts < 30) {
        delay(500);
        wifiAttempts++;
        Serial.print(".");
        
        // Print WiFi status for debugging
        if (wifiAttempts % 10 == 0) {
            Serial.println();
            Serial.print("WiFi Status: ");
            switch (WiFi.status()) {
                case WL_NO_SSID_AVAIL:
                    Serial.println("No SSID Available");
                    break;
                case WL_CONNECT_FAILED:
                    Serial.println("Connection Failed");
                    break;
                case WL_CONNECTION_LOST:
                    Serial.println("Connection Lost");
                    break;
                case WL_DISCONNECTED:
                    Serial.println("Disconnected");
                    break;
                default:
                    Serial.println("Other");
                    break;
            }
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected successfully!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Signal strength (RSSI): ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
    } else {
        Serial.println("\nWiFi connection failed!");
        Serial.print("Final WiFi Status: ");
        Serial.println(WiFi.status());
        
        // Try to scan for available networks
        Serial.println("Scanning for available networks...");
        int n = WiFi.scanNetworks();
        if (n == 0) {
            Serial.println("No networks found");
        } else {
            Serial.print(n);
            Serial.println(" networks found:");
            for (int i = 0; i < n; ++i) {
                Serial.print(i + 1);
                Serial.print(": ");
                Serial.print(WiFi.SSID(i));
                Serial.print(" (");
                Serial.print(WiFi.RSSI(i));
                Serial.print(")");
                Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
                delay(10);
            }
        }
    }
    
    // Continue with display setup even if WiFi fails
    Serial.println("Setting up display and buttons...");
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Enable internal pullup resistor
    pinMode(SCREEN_TOGGLE_PIN, INPUT_PULLUP);
    pinMode(REFRESH_PIN, INPUT_PULLUP);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);  // Turn on backlight

    setupNTP();
    
    // Initialize printer structures
    sovolPrinter.name = "Sovol";
    sovolPrinter.host = "sovol.lan";
    sovolPrinter.status = "offline";
    sovolPrinter.lastStatus = "offline";
    sovolPrinter.isFlashing = false;
    sovolPrinter.flashStartTime = 0;
    mandrainPrinter.name = "Mandrain";
    mandrainPrinter.host = "mandrainpi.lan";
    mandrainPrinter.status = "offline";
    mandrainPrinter.lastStatus = "offline";
    mandrainPrinter.isFlashing = false;
    mandrainPrinter.flashStartTime = 0;
    
    // Show initial display with WiFi status
    tft.fillScreen(BACKGROUND);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("ESP32 Status Screen", 20, 50);
    
    bool timeSuccess = false;
    bool stockSuccess = false;
    bool weatherSuccess = false;
    bool coffeeSuccess = false;
    bool trailSuccess = false;
    
    if (WiFi.status() == WL_CONNECTED) {
        tft.setTextColor(TFT_GREEN);
        tft.drawString("WiFi: Connected", 20, 100);
        tft.setTextColor(TFT_WHITE);
        tft.drawString("IP: " + WiFi.localIP().toString(), 20, 130);
        
        Serial.println("Starting data fetch...");
        // More robust initial data fetch
        timeSuccess = fetchTime() && fetchDate();
        stockSuccess = fetchStockPrice();
        weatherSuccess = fetchWeather();
        coffeeSuccess = fetchCoffeeMachineStatus();
        trailSuccess = fetchTrailStatus(mombaTrail, "momba") && 
                       fetchTrailStatus(johnBryanTrail, "JohnBryan") &&
                       fetchTrailStatus(caesarCreekTrail, "caesar_creek");
    } else {
        tft.setTextColor(TFT_RED);
        tft.drawString("WiFi: Failed", 20, 100);
        tft.setTextColor(TFT_YELLOW);
        tft.drawString("Demo Mode", 20, 130);
        
        // Set demo data for testing display
        currentTime.time = "12:34";
        currentTime.seconds = "56";
        currentTime.date = "Demo Mode Active";
        spyStock.symbol = "SPY";
        spyStock.price = 450.25;
        spyStock.change = 2.15;
        spyStock.changePercent = 0.48;
        currentWeather.conditions = "Demo Weather";
        currentWeather.temperature = 72;
        currentWeather.humidity = 50;
        currentWeather.icon = "01d";
        coffeeMachine.status = "Demo";
        coffeeMachine.esp32Status = "online";
        mombaTrail.status = "open";
        mombaTrail.lastUpdate = "Demo";
        johnBryanTrail.status = "closed";
        johnBryanTrail.lastUpdate = "Demo";
        caesarCreekTrail.status = "wet";
        caesarCreekTrail.lastUpdate = "Demo";
        
        Serial.println("Running in demo mode - no WiFi required");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        if (!timeSuccess || !stockSuccess || !weatherSuccess || !coffeeSuccess || !trailSuccess) {
            Serial.println("Initial data fetch failed:");
            Serial.println("Time: " + String(timeSuccess));
            Serial.println("Stock: " + String(stockSuccess));
            Serial.println("Weather: " + String(weatherSuccess));
            Serial.println("Coffee: " + String(coffeeSuccess));
            Serial.println("Trails: " + String(trailSuccess));
        }
    }
    
    // Update display regardless of fetch success
    updateTimeDisplay();
    updateStockDisplay();
    updateWeatherDisplay();
    updateCoffeeMachineDisplay();
    updateTrailDisplay();
    // updateCountdownDisplay();  // Hidden for now
    
    // Draw static weather icon
    drawWeatherIconStatic();
    
    // Fetch and display printer status
    if (WiFi.status() == WL_CONNECTED) {
        fetchPrinterStatus(sovolPrinter);
        fetchPrinterStatus(mandrainPrinter);
    }
    updatePrinterDisplay();
    
    lastTrailUpdate = millis();  // Initialize update timer
    lastStockUpdate = millis();  // Initialize stock update timer
    lastPrinterUpdate = millis();  // Initialize printer update timer
}

void setupNTP() {
    timeClient.begin();
    // Set initial offset based on DST detection
    // Will automatically adjust when DST changes (checked hourly)
    cachedDSTOffset = getDSTOffset();
    timeClient.setTimeOffset(cachedDSTOffset);
    timeClient.update();
    Serial.println("NTP initialized with offset: " + String(cachedDSTOffset/3600) + " hours (DST: " + 
                   (cachedDSTOffset == -4 * 3600 ? "EDT" : "EST") + ")");
}

void loop() {
    unsigned long currentMillis = millis();

    // PRIORITY 1: Always handle button inputs first (non-blocking)
    handleRotation();
    handleScreenToggle();
    handleRefresh();

    // Skip all display updates while showing forecast view
    if (isShowingForecast) {
        return;
    }

    // PRIORITY 2: Update time display every second (critical for keeping time current)
    if (currentMillis - lastSecondUpdate >= SECOND_UPDATE_INTERVAL) {
        if (fetchTime()) {
            lastSecondUpdate = currentMillis;
            updateTimeDisplay();
        }
    }
    
    // PRIORITY 3: Update date every 30 seconds
    if (currentMillis - lastTimeUpdate >= TIME_UPDATE_INTERVAL) {
        if (fetchDate()) {
            lastTimeUpdate = currentMillis;
            updateTimeDisplay();
            // updateCountdownDisplay();  // Hidden for now - Update countdown when date updates
        }
    }
    
    // Periodically redraw date to prevent it from being partially cleared by other elements
    // Redraw every 2 minutes to ensure date stays fully visible
    static unsigned long lastDateRedraw = 0;
    if (currentMillis - lastDateRedraw >= 120000) { // 2 minutes
        lastDateRedraw = currentMillis;
        // Force date redraw by clearing the displayed date cache
        lastDisplayedDate = "";  // Force redraw
        updateTimeDisplay();
    }
    
    // Animation disabled - weather icon is drawn statically
    
    // PRIORITY 5: Periodic updates (these can block, but time updates will catch up)
    // Only do ONE blocking operation per loop iteration to minimize time freeze
    static int updateRotation = 0;  // Track which update to perform
    updateRotation = (updateRotation + 1) % 4;  // Cycle through 4 different updates
    
    switch(updateRotation) {
        case 0:
            // Stock updates (every 5 minutes)
            if (currentMillis - lastStockUpdate >= STOCK_UPDATE_INTERVAL) {
                Serial.println("Attempting stock update..."); // Debug print
                if (fetchStockPrice()) {
                    lastStockUpdate = currentMillis;
                    updateStockDisplay();
                    // Redraw date after stock update to ensure it's not partially cleared
                    if (currentRotation == 1 || currentRotation == 3) { // Landscape only
                        lastDisplayedDate = "";  // Force date redraw
                        updateTimeDisplay();
                    }
                    Serial.println("Stock update successful");
                } else {
                    Serial.println("Stock update failed");
                }
            }
            break;
            
        case 1:
            // Weather updates (every 30 minutes)
            if (currentMillis - lastWeatherUpdate >= WEATHER_UPDATE_INTERVAL) {
                Serial.println("Attempting weather update..."); // Debug print
                if (fetchWeather()) {
                    lastWeatherUpdate = currentMillis;
                    updateWeatherDisplay();
                    Serial.println("Weather update successful");
                } else {
                    Serial.println("Weather update failed");
                }
            }
            break;
            
        case 2:
            // Coffee machine updates (every 5 minutes)
            if (currentMillis - lastCoffeeUpdate >= COFFEE_UPDATE_INTERVAL) {
                Serial.println("Attempting coffee machine update..."); // Debug print
                if (fetchCoffeeMachineStatus()) {
                    lastCoffeeUpdate = currentMillis;
                    updateCoffeeMachineDisplay();
                    Serial.println("Coffee machine update successful");
                } else {
                    Serial.println("Coffee machine update failed");
                }
            }
            break;
            
        case 3:
            // Trail status updates (every 5 minutes)
            if (currentMillis - lastTrailUpdate >= TRAIL_UPDATE_INTERVAL) {
                Serial.println("Attempting trail status update..."); // Debug print
                bool updateSuccess = fetchTrailStatus(mombaTrail, "momba") && 
                                   fetchTrailStatus(johnBryanTrail, "JohnBryan") &&
                                   fetchTrailStatus(caesarCreekTrail, "caesar_creek");
                                   
                if (updateSuccess) {
                    lastTrailUpdate = currentMillis;
                    updateTrailDisplay();
                    Serial.println("Trail status update successful");
                } else {
                    Serial.println("Trail status update failed");
                }
            }
            break;
    }
    
    // PRIORITY 6: Printer status updates (every 30 seconds)
    if (currentMillis - lastPrinterUpdate >= PRINTER_UPDATE_INTERVAL) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Attempting printer status update...");
            fetchPrinterStatus(sovolPrinter);
            fetchPrinterStatus(mandrainPrinter);
            lastPrinterUpdate = currentMillis;
            Serial.println("Printer status update completed");
        }
    }
    
    // PRIORITY 7: Update printer display (more frequently if flashing)
    // Update display more often when flashing to create smooth animation
    static unsigned long lastPrinterDisplayUpdate = 0;
    unsigned long displayUpdateInterval = (sovolPrinter.isFlashing || mandrainPrinter.isFlashing) ? PRINTER_FLASH_INTERVAL : 1000;
    if (currentMillis - lastPrinterDisplayUpdate >= displayUpdateInterval) {
        updatePrinterDisplay();
        lastPrinterDisplayUpdate = currentMillis;
    }
    
    processSerialInput();
    delay(10);  // Small delay for stability
}