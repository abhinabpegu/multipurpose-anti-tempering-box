# Multipurpose Anti-Tamper Box

A low-cost Arduino UNO R3 prototype that combines **RFID-based access control**, **lid tamper detection**, and **temperature and humidity monitoring** in a single storage box.

Developed for **Smart-Craft, Tecnoesis 2026, NIT Silchar** by Team **Ghee Refilled**.

---

## Project overview

The box is designed to protect stored items from unauthorised access while monitoring the environmental conditions inside it.

- An **MFRC522 RFID reader** checks the UID of a scanned card or tag.
- An **SG90 micro servo** operates the lid's locking latch.
- A **Hall effect sensor and magnet** detect whether the lid is open or closed.
- A **DHT11 sensor** measures the temperature and relative humidity inside the box.
- **Eight LEDs** communicate access, tamper, temperature, humidity and normal operating states.

The prototype uses an Arduino UNO R3 and a cardboard enclosure. It is intended as a low-cost demonstration platform, not as a certified security locker or medical cold-chain device.

## Features

- RFID-based authorisation using registered card UIDs.
- Servo-operated locking and unlocking.
- Lid-state monitoring using a Hall effect sensor.
- Tamper indication when the lid is opened while the system is locked.
- Automatic re-locking after the lid is closed.
- Temperature and humidity monitoring.
- LED-based status indication without requiring a display or network connection.

## Hardware

| Component | Quantity | Purpose |
|---|---:|---|
| Arduino UNO R3 | 1 | Main controller |
| MFRC522 RFID reader with card/tag | 1 | Reads RFID card UID |
| Hall effect sensor | 1 | Detects lid position |
| Small magnet | 1 | Mounted on the lid for Hall sensor detection |
| DHT11 sensor | 1 | Measures temperature and humidity |
| SG90 micro servo | 1 | Operates the locking latch |
| LEDs | 8 | Visual status indicators |
| 330 Ω resistors | 8 | Limits LED current |
| Breadboard and jumper wires | As required | Circuit connections |
| Cardboard box | 1 | Project enclosure |

## System operation

### 1. RFID access control

When a card is presented to the reader, the Arduino reads its UID and compares it with the authorised UID list stored in the program.

- **Authorised card:** the servo moves to the unlocked position.
- **Unauthorised card:** the box remains locked and the red Tamper LED flashes three times.

### 2. Lid sensing and automatic re-locking

The magnet is attached to the lid and the Hall sensor is mounted on the box body. When the magnet is near the sensor, the system interprets the lid as closed.

After an authorised scan, the box re-locks **2 seconds after the lid has been opened and closed again**. If the lid is not opened, the system re-locks after **10 seconds**.

### 3. Tamper detection

While the box is commanded to remain locked, the Arduino checks the Hall sensor's reading. If the sensor reports that the lid has opened, the system treats this mismatch as a possible forced entry. The Tamper LED remains on until an authorised card is scanned.

### 4. Environment monitoring

The DHT11 is read every 2 seconds. Four temperature LEDs provide a visual temperature indication, while a separate humidity LED indicates high humidity.

### 5. LED status indicators

| Indicator | Colour | Arduino pin | Function |
|---|---|---:|---|
| Temperature 1 | Blue | A1 | Lowest temperature band |
| Temperature 2 | Green | 5 | Temperature band |
| Temperature 3 | Yellow | 7 | Temperature band |
| Temperature 4 | Red | 8 | Highest temperature band |
| High humidity | Yellow | A4 | High-humidity warning |
| Access granted | Green | 4 | Access/unlocked status |
| Tamper | Red | A2 | Tamper or unauthorised-access indication |
| All OK / locked | Blue | A3 | Normal locked state |

The analogue pins A1–A4 are used as digital outputs.

## Circuit connections

### MFRC522 RFID reader

The MFRC522 operates at **3.3 V**. Connect it to the UNO as follows.

| MFRC522 pin | Arduino UNO pin |
|---|---|
| 3.3V | 3.3V |
| GND | GND |
| RST | 9 |
| SDA (SS) | 10 |
| MOSI | 11 |
| MISO | 12 |
| SCK | 13 |

### Other modules

| Module | Module pins | Arduino UNO pins |
|---|---|---|
| Hall effect sensor | VCC / GND / OUT | 5V / GND / 3 |
| DHT11 | VCC / GND / DATA | 5V / GND / 2 |
| SG90 servo | Red / Brown / Orange | 5V / GND / 6 |

### LED connections

Use one **330 Ω resistor in series with each LED**.

| LED | Arduino UNO pin |
|---|---:|
| Temperature 1 | A1 |
| Temperature 2 | 5 |
| Temperature 3 | 7 |
| Temperature 4 | 8 |
| High humidity | A4 |
| Access granted | 4 |
| Tamper | A2 |
| All OK / locked | A3 |

**Power note:** The MFRC522 is a 3.3 V module; do not power it from the UNO's 5 V pin. The project report specifies 5 V for the Hall sensor, DHT11 and servo. Ensure all modules share a common ground. A servo can draw current spikes; a separate suitable 5 V supply may be needed if the Arduino resets or the servo behaves erratically. Connect the grounds together.

## Software

The firmware is written in the Arduino C++ environment.

### Required libraries

Install these libraries through the Arduino IDE Library Manager:

- **MFRC522** — RFID reader support (Miguel Balboa).
- **DHT sensor library** — DHT11 sensor support (Adafruit).
- **Adafruit Unified Sensor** — dependency for the Adafruit DHT library.
- **Servo** — servo motor control (Arduino library).

### Uploading the firmware

1. Install the Arduino IDE.
2. Install the libraries listed above.
3. Open the project's Arduino sketch (`.ino` file).
4. Select **Arduino UNO** as the board.
5. Select the serial port corresponding to the UNO.
6. Check the pin assignments against the wiring tables above.
7. Upload the sketch.
8. Open Serial Monitor if you need to inspect RFID UIDs or debugging output.

> **Firmware file:** Add the Arduino sketch to this repository and update this section with its filename or link. The project report does not contain the complete source code.

### Configuration

The project firmware stores authorised RFID UIDs as byte arrays. The report also describes configurable constants for temperature thresholds, servo angles and Hall sensor polarity. Update these values in the sketch to match your hardware and requirements.

## Timing and threshold settings

The following values are specified in the project report.

| Setting | Value |
|---|---:|
| Temperature LED thresholds | 26, 28, 30 and 32 °C |
| Blue temperature LED blinking | Below 26 °C |
| Over-temperature alarm | 35 °C and above |
| High-humidity indication | 75% RH and above |
| Re-lock after lid is closed | 2 seconds |
| Re-lock if lid is never opened | 10 seconds |
| Lid-left-open warning threshold | 30 seconds |
| Servo-to-LED status delay | 1 second |
| DHT11 reading interval | 2 seconds |
| Hall sensor debounce | 50 ms |

The four temperature LEDs are ordered blue, green, yellow and red. At the over-temperature alarm threshold, all four blink together.

## Testing

The project was tested in stages before the complete program was assembled:

- **RFID and Hall sensor:** A test sketch printed card UIDs and magnet state to the Serial Monitor.
- **LEDs:** Each LED was tested individually, followed by a test with all eight LEDs illuminated.
- **Servo:** The servo was tested by moving it between two angles; the locked and unlocked angles were adjusted to match its direction.
- **Full system:** Authorised and unauthorised cards, lid opening and closing, forced opening, and temperature and humidity thresholds were checked.

## Known limitations

- The DHT11 has an approximate accuracy of ±2 °C for temperature and ±5% for relative humidity. The prototype is **not a certified medical cold-chain device**.
- The SG90 is a light-duty servo intended for a small latch.
- Standard RFID tags may be cloned; this is not high-security access control.
- The Arduino UNO has no built-in Wi-Fi or Bluetooth. Alerts are local through LEDs and the Serial Monitor.
- The cardboard enclosure is not designed to withstand determined physical attacks.

## Future improvements

- Add a buzzer for audible alerts.
- Add an LCD or OLED to display measured values.
- Use an ESP32 for wireless alerts and remote data logging.
- Upgrade to a more accurate temperature/humidity sensor, such as a DHT22 or DS18B20 (for temperature).
- Add a real-time clock for timestamped environmental records.
- Use a stronger enclosure and a more robust locking mechanism.
- Add battery backup for operation during power cuts.

## Team

**Team name:** Ghee Refilled

| Role | Name | Department and year | Institution |
|---|---|---|---|
| Team Leader | Abhinab Pegu | Electronics and Communication Engineering, 1st Year | National Institute of Technology Silchar |
| Team Member | Satyam Kesari | Electronics and Communication Engineering, 1st Year | National Institute of Technology Silchar |
| Team Member | Syed Mohammad Taha | Electronics and Communication Engineering, 1st Year | National Institute of Technology Silchar |

## Project report

The detailed project report is included in the repository as:

`Multipurpose_Anti_Tempering_box.pdf`

## References

- Arduino Servo library.
- MFRC522 library by Miguel Balboa.
- Adafruit DHT sensor library.
- Adafruit Unified Sensor library.
- MFRC522 (NXP), DHT11, SG90 micro servo and Hall effect sensor datasheets.

---

**Developed for Smart-Craft, Tecnoesis 2026, NIT Silchar.**
