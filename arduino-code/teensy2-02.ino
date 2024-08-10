#include "ping1d.h"
#include <SD.h> // Include the SD library
#include <TimeLib.h> // Include the Time library
#include <IntervalTimer.h> // Include the IntervalTimer library for Watchdog

// This serial port is used to communicate with the Ping device
// We use Serial1 and Serial2 on the Teensy 4.1
static Ping1D ping1 { Serial1 };
static Ping1D ping2 { Serial2 };

static const uint8_t ledPin = 13;
static const uint8_t chipSelect = BUILTIN_SDCARD; // Use built-in SD card slot on Teensy 4.1

unsigned long delayInterval = 10000; // Variable for the delay interval in milliseconds

String getNextFilename() {
    char filename[15]; // Adjusted size for more files
    for (int i = 0; i < 5000; i++) {
        snprintf(filename, sizeof(filename), "output%04d.csv", i);
        if (!SD.exists(filename)) {
            return String(filename);
        }
    }
    return String("");
}

void setup() {
    Serial1.begin(9600);  // Start the serial communication with the first Ping device
    Serial2.begin(9600);  // Start the serial communication with the second Ping device
    Serial.begin(115200); // Start the serial communication for debugging
    pinMode(ledPin, OUTPUT);

    initializeComponents();

    // Initialize the Teensy RTC
    setSyncProvider(getTeensy3Time);
    if (timeStatus() != timeSet) {
        Serial.println("Unable to sync with the RTC");
    } else {
        Serial.println("RTC has set the system time");
    }

    // Optional: Set the time manually if needed (for the first run)
    // setTime(hr, min, sec, day, month, yr);
    // Teensy 4.1 doesn't have a direct method to set time, usually done on first upload
    // setTime(14, 0, 0, 23, 7, 2024); // Example: set to 14:00:00 on July 23, 2024
}

void initializeComponents() {
    while (!ping1.initialize()) {
        Serial.println("\nFirst Ping device failed to initialize!");
        Serial.println("Are the Ping rx/tx wired correctly?");
        Serial.print("Ping rx (green) should be connected to Teensy pin 1 (TX)");
        Serial.print("Ping tx (white) should be connected to Teensy pin 0 (RX)");
        delay(2000);
    }
    
    while (!ping2.initialize()) {
        Serial.println("\nSecond Ping device failed to initialize!");
        Serial.println("Are the Ping rx/tx wired correctly?");
        Serial.print("Ping rx (green) should be connected to Teensy pin 9 (TX)");
        Serial.print("Ping tx (white) should be connected to Teensy pin 10 (RX)");
        delay(2000);
    }

    // Initialize the SD card
    if (!SD.begin(chipSelect)) {
        Serial.println("SD card initialization failed!");
    } else {
        Serial.println("SD card initialized.");
    }
}

time_t getTeensy3Time() {
    return Teensy3Clock.get();
}

void loop() {
    static String filename = getNextFilename(); // Get the next available filename
    static bool headerWritten = false; // Flag to check if headers are written

    for (int i = 0; i < 3; i++) { // Take three readings
        if (ping1.update() && ping2.update()) {
            // Open the file in append mode
            File dataFile = SD.open(filename.c_str(), FILE_WRITE);

            // If the file is available, write to it
            if (dataFile) {
                if (!headerWritten) {
                    // Write the headers if they haven't been written yet
                    dataFile.println("Date,Distance1,Confidence1,Distance2,Confidence2");
                    headerWritten = true;
                    Serial.println("Ping initialized, writing to SD card");
                }
                // Write the data
                dataFile.print(year());
                dataFile.print('/');
                dataFile.print(month());
                dataFile.print('/');
                dataFile.print(day());
                dataFile.print(' ');
                dataFile.print(hour());
                dataFile.print(':');
                dataFile.print(minute());
                dataFile.print(':');
                dataFile.print(second());
                dataFile.print(", ");
                dataFile.print(ping1.distance());
                dataFile.print(", ");
                dataFile.print(ping1.confidence());
                dataFile.print(", ");
                dataFile.print(ping2.distance());
                dataFile.print(", ");
                dataFile.println(ping2.confidence());
                dataFile.close(); // Close the file
            } else {
                // If the file isn't open, pop up an error
                Serial.println("Error opening " + filename);
            }
        } else {
            Serial.println("No update received!");
        }

        // Toggle the LED to show that the program is running
        digitalWrite(ledPin, !digitalRead(ledPin));

        delay(delayInterval); // Add a delay to avoid flooding the SD card with writes
    }

    delay(15*60*1000); // Wait 15 minutes before taking the next set of readings
}
