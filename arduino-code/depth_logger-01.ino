// Include libraries
#include "ping1d.h"
#include "SoftwareSerial.h"
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

// Pin definitions and initialization
// Ping2 CHANGED WIRING
static const uint8_t arduinoRxPin = 8;
static const uint8_t arduinoTxPin = 9;
SoftwareSerial pingSerial = SoftwareSerial(arduinoRxPin, arduinoTxPin);
static Ping1D ping { pingSerial };

// Data logger
const int chipSelect = 10; // Use digital pin 10 for the SD cs line

// The digital pins that connect to the LEDs
#define redLEDpin 2
#define greenLEDpin 3

// How many milliseconds between grabbing data and logging it. 1000 ms is once a second
#define LOG_INTERVAL  1000 // Mills between entries

// How many milliseconds before writing the logged data permanently to disk
// Set it to the LOG_INTERVAL to write each time (safest)
// Set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to 
// the last 10 reads if power is lost but it uses less power and is much faster!
#define SYNC_INTERVAL 1000 // Mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // Time of last sync()

// Debugging flag
#define ECHO_TO_SERIAL 1

RTC_DS1307 RTC; // Define the Real Time Clock object
File logfile; // The logging file

// Error handling function
void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  
  // Red LED indicates error
  digitalWrite(redLEDpin, HIGH);
  
  while(1);
}

// Setup
void setup(void)
{
  Serial.begin(9600);
  Serial.println();
  
#if WAIT_TO_START
  Serial.println("Type any character to start");
  while (!Serial.available());
#endif // WAIT_TO_START

  // Initialize the SD card
  Serial.print("Initializing SD card...");
  // Make sure that the default chip select pin is set to output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // See if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // Don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  
  // Create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // Only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // Leave the loop!
    }
  }
  
  if (! logfile) {
    error("couldn't create file");
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

  // Initialize Ping2 device
  Serial.println("Initializing Ping2 device...");
  while (!ping.initialize()) {
    Serial.println("Ping device failed to initialize!");
    delay(2000);
  }
  Serial.println("Ping2 initialized successfully.");
}

// Loop function
void loop(void)
{
  DateTime now;

  // Delay for the amount of time we want between readings
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
  
  digitalWrite(greenLEDpin, HIGH);

  // Log milliseconds since starting
  uint32_t m = millis();
  logfile.print(m);           // Milliseconds since start
  logfile.print(", ");
  #if ECHO_TO_SERIAL
  Serial.print(m);         // Milliseconds since start
  Serial.print(", ");  
  #endif

  // Fetch the time
  now = RTC.now();
  // Log time
  logfile.print(now.unixtime()); // Seconds since 2000
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
  Serial.print(now.unixtime()); // Seconds since 2000
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
  Serial.print('"');
  #endif // ECHO_TO_SERIAL

  // read Ping2 data

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
    Serial.println("No update received from Ping2!");
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
