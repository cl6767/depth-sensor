#include "ping1d.h"
#include <SD.h> // Include the SD library

// This serial port is used to communicate with the Ping device
// We use Serial1 on the Teensy 4.1, which corresponds to pins 0 (RX) and 1 (TX)
static Ping1D ping { Serial1 };

static const uint8_t ledPin = 13;
static const uint8_t chipSelect = BUILTIN_SDCARD; // Use built-in SD card slot on Teensy 4.1

String getNextFilename() {
    char filename[13]; // "outputXX.csv" + null terminator = 13
    for (int i = 0; i < 100; i++) {
        snprintf(filename, sizeof(filename), "output%02d.csv", i);
        if (!SD.exists(filename)) {
            return String(filename);
        }
    }
    return String("");
}

void setup()
{
    Serial1.begin(9600);  // Start the serial communication with the Ping device
    Serial.begin(115200); // Start the serial communication for debugging
    pinMode(ledPin, OUTPUT);
    Serial.println("Blue Robotics ping1d-simple.ino");

    while (!ping.initialize()) {
        Serial.println("\nPing device failed to initialize!");
        Serial.println("Are the Ping rx/tx wired correctly?");
        Serial.print("Ping rx (green) should be connected to Teensy pin 1 (TX)");
        Serial.print("Ping tx (white) should be connected to Teensy pin 0 (RX)");
        delay(2000);
    }

    // Initialize the SD card
    if (!SD.begin(chipSelect)) {
        Serial.println("SD card initialization failed!");
        return;
    }
}

void loop()
{
    static String filename = getNextFilename(); // Get the next available filename

    if (ping.update()) {
        // Open the file in write mode
        File dataFile = SD.open(filename.c_str(), FILE_WRITE);

        // If the file is available, write to it
        if (dataFile) {
            dataFile.print("Distance: ");
            dataFile.print(ping.distance());
            dataFile.print(", Confidence: ");
            dataFile.println(ping.confidence());
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

    delay(1000); // Add a delay to avoid flooding the SD card with writes
}
