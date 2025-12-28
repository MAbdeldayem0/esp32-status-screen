# ESP32 Status Screen

A real-time status display built with an ESP32 microcontroller and TFT screen. Shows time, weather, stock prices, mountain bike trail conditions, and IoT device statuses.

## Features

- **Real-time clock** with NTP synchronization
- **Weather display** with current conditions and forecasts
- **Stock price tracking** via Alpha Vantage API
- **MTB Trail Status Monitor** - Live status for local mountain bike trails:
  - Momba Trail
  - John Bryan Trail
  - Caesar Creek Trail
  - Color-coded status indicators (green=open, red=closed, yellow=wet/caution)
  - Last update timestamps
- **Coffee machine status** monitoring
- **3D printer status** (Sovol)
- Colorblind-friendly UI with high contrast colors
- Portrait and landscape mode support
- Demo mode for offline testing

## Hardware

- **Board**: uPesy ESP32 WROOM
- **Display**: TFT screen (compatible with TFT_eSPI library)

## Setup

### 1. Install PlatformIO

Install [PlatformIO IDE](https://platformio.org/install/ide) for VS Code or use the CLI.

### 2. Configure Credentials

Copy the example credentials file and fill in your values:

```bash
cp include/credentials.example.h include/credentials.h
```

Edit `include/credentials.h` with your:
- WiFi SSID and password
- Alpha Vantage API key (get free key at https://www.alphavantage.co/support/#api-key)

### 3. Build and Upload

```bash
# Build
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

## Dependencies

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) - TFT display driver
- [ArduinoJson](https://arduinojson.org/) - JSON parsing
- [Time](https://github.com/PaulStoffregen/Time) - Time library

## Project Structure

```
Status Screen - Code/
├── src/
│   └── main.cpp          # Main application code
├── include/
│   ├── credentials.h     # Your credentials (gitignored)
│   └── credentials.example.h  # Template for credentials
├── platformio.ini        # PlatformIO configuration
└── README.md
```

## Backend Server

This display connects to a backend server (default: `mainPI.local:5000`) that provides:
- Weather data
- Coffee machine status
- Trail status
- 3D printer status

## License

MIT
