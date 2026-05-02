#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_BMP280.h>
#include <MPU6050.h>

#define SD_CS_PIN 10

Adafruit_BMP280 bmp;
MPU6050 mpu;
File dataFile;

float groundPressure;
float groundAltitude;

enum FlightState { IDLE, LAUNCHED, APOGEE, LANDED };
FlightState state = IDLE;

#define LAUNCH_ACCEL_THRESHOLD  1.0
#define APOGEE_ALT_DROP         0.25
#define LANDING_ALT_THRESHOLD   0.5

float maxAltitude = 0;
unsigned long startTime;

// -----------------------------------------------
void setup() {
  Serial.begin(9600);
  delay(2000);

  Serial.println(F("Initializing..."));

  if (!bmp.begin(0x76)) {
    Serial.println(F("BMP280 not found!"));
    while (1);
  }
  Serial.println(F("BMP280 OK"));

  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X16,
    Adafruit_BMP280::SAMPLING_X2,
    Adafruit_BMP280::FILTER_X16,
    Adafruit_BMP280::STANDBY_MS_1
  );

  Wire.begin();
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println(F("MPU6050 not found!"));
    while (1);
  }
  Serial.println(F("MPU6050 OK"));

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("SD card failed!"));
    while (1);
  }
  Serial.println(F("SD card OK"));

  dataFile = SD.open("flight.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println(F("time_ms,altitude_m,ax,ay,az,total_g,state"));
    dataFile.flush();
    Serial.println(F("Log file created"));
  } else {
    Serial.println(F("Log file failed!"));
    while (1);
  }

  // Calibrate ground level
  Serial.println(F("Calibrating..."));
  float pressureSum = 0;
  for (int i = 0; i < 100; i++) {
    pressureSum += bmp.readPressure();
    delay(10);
  }
  groundPressure = pressureSum / 100.0;
  groundAltitude = bmp.readAltitude(groundPressure / 100.0);

  startTime = millis();
  Serial.println(F("Ready. Waiting for launch..."));
}

// -----------------------------------------------
void loop() {
  unsigned long currentTime = millis() - startTime;

  float altitude = bmp.readAltitude(groundPressure / 100.0) - groundAltitude;

  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  float gx = ax / 16384.0;
  float gy = ay / 16384.0;
  float gz = az / 16384.0;
  float totalG = sqrt(gx*gx + gy*gy + gz*gz);

  if (altitude > maxAltitude) maxAltitude = altitude;

  // State machine
  switch (state) {
    case IDLE:
      if (totalG > LAUNCH_ACCEL_THRESHOLD) {
        state = LAUNCHED;
        Serial.println(F("*** LAUNCH DETECTED ***"));
      }
      break;

    case LAUNCHED:
      if (maxAltitude - altitude > APOGEE_ALT_DROP && maxAltitude > 0.5) {
        state = APOGEE;
        Serial.print(F("*** APOGEE at "));
        Serial.print(maxAltitude);
        Serial.println(F(" m ***"));
      }
      break;

    case APOGEE:
      if (altitude < LANDING_ALT_THRESHOLD) {
        state = LANDED;
        Serial.println(F("*** LANDED ***"));
      }
      break;

    case LANDED:
      if (dataFile) {
        dataFile.close();
        Serial.println(F("Data saved. Done."));
      }
      while (1);
  }

  // State label
  const char* stateStr;
  switch (state) {
    case IDLE:     stateStr = "IDLE";     break;
    case LAUNCHED: stateStr = "LAUNCHED"; break;
    case APOGEE:   stateStr = "APOGEE";   break;
    default:       stateStr = "LANDED";   break;
  }
// Build CSV line manually
  char altStr[10], gxStr[8], gyStr[8], gzStr[8], tgStr[8];
  dtostrf(altitude, 6, 2, altStr);
  dtostrf(gx, 5, 3, gxStr);
  dtostrf(gy, 5, 3, gyStr);
  dtostrf(gz, 5, 3, gzStr);
  dtostrf(totalG, 5, 3, tgStr);

  char buf[72];
  snprintf(buf, sizeof(buf), "%lu,%s,%s,%s,%s,%s,%s",
           currentTime, altStr, gxStr, gyStr, gzStr, tgStr, stateStr);

  if (dataFile) {
    dataFile.println(buf);
    dataFile.flush();
  }

  Serial.println(buf);

  delay(20);
}
