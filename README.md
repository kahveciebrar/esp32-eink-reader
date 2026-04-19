# ESP32 E-Ink Reader (Portable Embedded System)

A portable e-ink reader built on an ESP32 microcontroller with a local web interface, file upload support, persistent reading state, and power-aware operation.

The device allows users to upload text files wirelessly and read them on an e-ink display, while reducing heat and power usage by automatically disabling Wi-Fi after inactivity.

---

## Overview

This project is a standalone embedded reading device using an ESP32 and a 4.2" e-ink display. It combines networking, file storage, and display control into one system.

A built-in web server allows uploading `.txt` files directly to the device. The content is processed and displayed page-by-page with automatic word wrapping.

The system remembers the last reading position and restores it after restart, so reading can continue without losing progress.

Wi-Fi is automatically turned off after 5 minutes to reduce power consumption and prevent overheating.

---

## Features

- 4.2" e-ink display (low power, easy on the eyes)
- Local Wi-Fi access point with browser interface
- File upload via web page
- Supports `.txt` files
- Automatic word wrapping and pagination
- Remembers last reading position
- Page navigation with physical buttons
- Wi-Fi auto shutdown after 5 minutes
- Display power toggle
- Portable battery-powered system

---

## Hardware Components

### Microcontroller
- ESP32

### Display
- 4.2" E-Ink Display (GDEY042Z98, 3-color)
- SPI interface

### Power System
- Lithium battery
- TP4056 charging module
- Voltage regulation (if required)

### Input
- Next Page button
- Previous Page button
- Power toggle button

---

## Pin Connections

### E-Ink Display (SPI)

| Display Pin | ESP32 Pin |
|------------|----------|
| CS         | GPIO 5   |
| DC         | GPIO 17  |
| RST        | GPIO 16  |
| BUSY       | GPIO 4   |
| MOSI       | SPI MOSI |
| SCK        | SPI SCK  |
| VCC        | 3.3V     |
| GND        | GND      |

---

### Buttons

| Function      | ESP32 Pin |
|--------------|----------|
| Next Page     | GPIO 25 |
| Previous Page | GPIO 33 |
| Power Toggle  | GPIO 26 |

(All buttons use internal pull-up resistors)

---

## System Architecture

- Files are stored using LittleFS (internal flash)
- ESP32 runs a local web server
- Device acts as a Wi-Fi access point
- Files are uploaded via browser
- Text is parsed and displayed page-by-page
- Reading state is saved in flash memory

---

## How It Works

1. Device powers on and initializes the display
2. ESP32 creates a Wi-Fi access point:
   - SSID: ESP32-BOOK
3. User connects via phone or computer
4. Opens browser and accesses the web interface
5. Uploads a `.txt` file
6. File is stored in internal memory
7. Text is processed:
   - words are grouped into lines
   - lines are fitted to screen width
   - pages are generated dynamically
8. Page is displayed on the e-ink screen
9. Reading position is saved continuously
10. On restart:
    - last file and page are restored

---

## Wi-Fi & Power Management

- Wi-Fi automatically turns off after 5 minutes
- Reduces:
  - heat generation
  - power consumption
- Device continues reading offline
- Wi-Fi turns back on when:
  - device is restarted
  - screen is powered back on

---

## File Upload

- Files are uploaded through a browser
- Supported format: `.txt`
- Upload process:
  1. Connect to ESP32 Wi-Fi
  2. Open browser
  3. Upload file
  4. File is saved and opened automatically

---

## Navigation

- Buttons allow moving between pages
- Pages are generated dynamically
- Text layout adapts to display width

---

## Persistent State

The system stores:
- Current file path
- Current page position
- Next page position

Saved in: /state.dat
This allows:
- Resume reading after restart
- No loss of progress

---

## Power Control

- Power button toggles display on/off
- When off:
  - display is powered down
- When on:
  - display is reinitialized
  - Wi-Fi is enabled
  - last page is restored

---

## Setup

1. Connect all components
2. Upload code to ESP32
3. Power the device
4. Connect to Wi-Fi:
   - SSID: ESP32-BOOK
   - Password: 12345678
5. Open browser and upload a `.txt` file

---

## Code Structure

- Web server handles file upload and selection
- LittleFS manages storage
- Page builder handles text layout and pagination
- Display driver renders content

---

## What I Learned

- Building embedded web interfaces on ESP32
- Using LittleFS for file storage
- Rendering text on e-ink displays
- Managing power and heat in embedded systems
- Combining hardware and software into a full system

---

## Images

I also 3D printed a case for this device. It can be seen below:<img width="1309" height="803" alt="Deviceparts" src="https://github.com/user-attachments/assets/1416dced-2b93-4c70-ade9-92bc4f42c349" />
<img width="1309" height="803" alt="WhatsApp Image 2026-04-19 at 6 20 27 PM" src="https://github.com/user-attachments/assets/2eec6113-e5ce-432e-8f29-232acc4c2a76" />


---

## Author

Biomedical Engineering student working on embedded systems, sensors, and hardware prototyping.
