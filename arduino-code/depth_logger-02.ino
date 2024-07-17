// Include libraries
#include "ping1d.h"
#include "SoftwareSerial.h"
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

// Pin definitions and initialization
static const uint8_t arduinoRxPin = 8; // Changed wiring
static const uint8_t arduinoTxPin = 9; // Changed wiring
SoftwareSerial pingSerial(arduinoRxPin, arduinoTxPin);
static Ping1D ping { pingSerial };

// Data logger
const int chipSelect = 10; // Use digital pin 10 for the SD CS line

// The digital pins that connect to the LEDs
#define redLEDpin 2
#define greenLEDpin 3

// Logging intervals
#define LOG_INTERVAL  1000 // Milliseconds between data logs
#define SYNC_INTERVAL 1000 // Milliseconds between writing data to disk
uint32_t syncTime = 0; // Time of last sync()

// Debugging flag
#define ECHO_TO_SERIAL 1

RTC_DS1307 RTC; // Define the Real Time Clock object (use DS1307 or appropriate RTC)
File logfile; // The logging file

// Error handling function
void error(const char *str) {
  Serial.print("error: ");
  Serial.println(str);
  digitalWrite(redLEDpin, HIGH); // Red LED indicates error
  while (1); // Halt further execution
}

// Setup
void setup(void) {
  Serial.begin(9600); // Start Serial communication at 9600 baud for debugging
  Serial.println("Setup Started");

#if WAIT_TO_START
  Serial.println("Type any character to start");
  while (!Serial.available());
#endif // WAIT_TO_START

  // Use debugging LEDs
  pinMode(redLEDpin, OUTPUT);
  pinMode(greenLEDpin, OUTPUT);

  // Initialize the SD card
  Serial.print("Initializing SD card...");
  pinMode(chipSelect, OUTPUT);

  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // Don't do anything more:
    return;
  }
  Serial.println("Card initialized.");

  // Create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i / 10 + '0';
    filename[7] = i % 10 + '0';
    if (!SD.exists(filename)) {
      logfile = SD.open(filename, FILE_WRITE);
      break;
    }
  }

  if (!logfile) {
    error("Couldn't create file");
  }

  Serial.print("Logging to: ");
  Serial.println(filename);

  // Connect to RTC
  Wire.begin();
  if (!RTC.begin()) {
    logfile.println("RTC failed");
    Serial.println("RTC failed");
  }

  logfile.println("millis,stamp,datetime,distance,confidence");
#if ECHO_TO_SERIAL
  Serial.println("millis,stamp,datetime,distance,confidence");
#endif

  // Initialize Ping2 device with the correct baud rate
  pingSerial.begin(115200); // Set SoftwareSerial to 115200 baud for Ping2
  Serial.println("Initializing Ping2 device...");
  while (!ping.initialize()) {
    Serial.println("Ping device failed to initialize!");
    delay(2000);
  }
  Serial.println("Ping2 initialized successfully.");
}

// Loop
void loop(void) {
  DateTime now;

  // Delay for the amount of time we want between readings
  delay((LOG_INTERVAL - 1) - (millis() % LOG_INTERVAL));

  digitalWrite(greenLEDpin, HIGH);

  // Log milliseconds since starting
  uint32_t m = millis();
  logfile.print(m); // Milliseconds since start
  logfile.print(", ");
#if ECHO_TO_SERIAL
  Serial.print(m); // Milliseconds since start
  Serial.print(", ");
#endif

  // Fetch the time
  now = RTC.now();
  logfile.print(now.unixtime()); // Seconds since 1970
  logfile.print(", ");
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
#if ECHO_TO_SERIAL
  Serial.print(now.unixtime()); // Seconds since 1970
  Serial.print(", ");
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
#endif

  // Read Ping2 data
  if (ping.update()) {
    int distance = ping.distance();
    int confidence = ping.confidence();

    logfile.print(", ");
    logfile.print(distance);
    logfile.print(", ");
    logfile.print(confidence);
#if ECHO_TO_SERIAL
    Serial.print(", ");
    Serial.print(distance);
    Serial.print(", ");
    Serial.print(confidence);
#endif
  } else {
#if ECHO_TO_SERIAL
    Serial.print(", ");
    Serial.print("No update received from Ping2!");
#endif
  }

  logfile.println();
#if ECHO_TO_SERIAL
  Serial.println();
#endif

  digitalWrite(greenLEDpin, LOW); // Turn off green LED

  // Write data to disk if the sync interval has passed
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();

  // Indicate data syncing with red LED
  digitalWrite(redLEDpin, HIGH);
  logfile.flush();
  digitalWrite(redLEDpin, LOW);
}
