#include "ping1d.h"
#include <SD.h>
#include <TimeLib.h>
#include <IntervalTimer.h>

// First Ping2 sensor
static Ping1D ping1 { Serial1 };

// Second Ping2 sensor
static Ping1D ping2 { Serial2 };

static const uint8_t ledPin = 13;
static const uint8_t chipSelect = BUILTIN_SDCARD;

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
    Serial1.begin(9600);  // Start the serial communication with the first Ping2 device
    Serial2.begin(9600);  // Start the serial communication with the second Ping2 device
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
}

void initializeComponents() {
    while (!ping1.initialize()) {
        Serial.println("\nFirst Ping2 device failed to initialize!");
        Serial.println("Are the Ping1 rx/tx wired correctly?");
        delay(2000);
    }

    while (!ping2.initialize()) {
        Serial.println("\nSecond Ping2 device failed to initialize!");
        Serial.println("Are the Ping2 rx/tx wired correctly?");
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
    static String filename = getNextFilename();
    static bool headerWritten = false;

    if (ping1.update() && ping2.update()) {
        // Open the file in append mode
        File dataFile = SD.open(filename.c_str(), FILE_WRITE);

        // If the file is available, write to it
        if (dataFile) {
            if (!headerWritten) {
                // Write the headers if they haven't been written yet
                dataFile.println("Date,Distance1,Confidence1,Distance2,Confidence2");
                headerWritten = true;
                Serial.println("Ping devices initialized, writing to SD card");
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
