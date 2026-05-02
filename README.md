# Flight Computer Altimeter v1.0
> A full-stack embedded flight computer built from scratch  altitude sensing, accelerometry, and SD card data logging. Built as the first step in an aerospace/rocketry self-learning journey.

---

## What It Does

- Reads altitude at 50Hz using a barometric pressure sensor
- Tracks 3-axis acceleration continuously
- Automatically detects **launch**, **apogee**, and **landing** using a state machine
- Logs all flight data to a CSV file on an SD card
- Prints live telemetry to Serial Monitor during flight

---

## Hardware

| Component | Model | Purpose |
|---|---|---|
| Microcontroller | Arduino Nano ATmega328P | Brain of the system |
| Barometric sensor | BMP280 | Altitude + pressure + temperature |
| IMU | MPU6050 | 3-axis accelerometer + gyroscope |
| Storage | Micro SD card module | Flight data logging |

---

## Circuit

### I2C Bus (BMP280 + MPU6050)
```
Arduino A4 (SDA) → BMP280 SDA → MPU6050 SDA
Arduino A5 (SCL) → BMP280 SCL → MPU6050 SCL
Arduino 3.3V     → BMP280 VCC → MPU6050 VCC
Arduino GND      → BMP280 GND → MPU6050 GND
BMP280 SDO       → GND  (I2C address 0x76)
MPU6050 AD0      → GND  (I2C address 0x68)
```

### SPI Bus (SD Card)
```
Arduino D10 → CS
Arduino D11 → MOSI
Arduino D12 → MISO
Arduino D13 → SCK
Arduino 5V  → VCC
Arduino GND → GND
```


## Software

### Dependencies
Install via Arduino IDE Library Manager:
- `Adafruit BMP280` (install all dependencies when prompted)
- `MPU6050 by Electronic Cats`

### Key Implementation Details
- Uses `dtostrf()` instead of `%f` in `snprintf()` — AVR doesn't support float formatting natively
- All string literals use `F()` macro to store in flash, not RAM — reduces RAM usage from 81% to 63%
- `dataFile.flush()` called every loop — prevents data loss on unexpected power cut
- Ground level calibrated over 100 readings at startup for accurate relative altitude

---

## Plotting Flight Data

After retrieving `flight.csv` from the SD card, run this Python script:

```python
import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('flight.csv')

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
fig.suptitle('Flight Computer Data', fontsize=14)

ax1.plot(df['time_ms'] / 1000, df['altitude_m'], color='royalblue', linewidth=2)
ax1.set_ylabel('Altitude (m)')
ax1.set_xlabel('Time (s)')
ax1.set_title('Altitude Profile')
ax1.grid(True, alpha=0.3)

max_idx = df['altitude_m'].idxmax()
ax1.annotate(f"Apogee: {df['altitude_m'][max_idx]:.1f}m",
             xy=(df['time_ms'][max_idx]/1000, df['altitude_m'][max_idx]),
             xytext=(df['time_ms'][max_idx]/1000 + 1, df['altitude_m'][max_idx] - 5),
             arrowprops=dict(arrowstyle='->', color='red'), color='red')

ax2.plot(df['time_ms'] / 1000, df['total_g'], color='tomato', linewidth=2)
ax2.axhline(y=1, color='gray', linestyle='--', alpha=0.5, label='1g (rest)')
ax2.set_ylabel('Total G-force')
ax2.set_xlabel('Time (s)')
ax2.set_title('Acceleration Profile')
ax2.legend()
ax2.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('flight_profile.png', dpi=150)
plt.show()
```

---

*This is v1 of what will eventually become a full flight computer with dual-deploy parachute ejection. Every great rocket started on a breadboard.*
