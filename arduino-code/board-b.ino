// This code is for board b (data logger) for the two Arduino board setup

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <SoftwareSerial.h>

const int chipSelect = 10;
const uint8_t statusLedPin = 13; // Status LED

const uint8_t dataLoggerRxPin = 11; // RX from Sensor Board
const uint8_t dataLoggerTxPin = 10; // TX to Sensor Board (not used in this example)
SoftwareSerial dataLoggerSerial(dataLoggerRxPin, dataLoggerTxPin);

RTC_PCF8523 RTC;
File logfile;

void error(const char* str) {
    Serial.print("Error: ");
    Serial.println(str);
    digitalWrite(statusLedPin, HIGH);
    while (1);
}

void setup() {
    Serial.begin(115200); // Serial monitor
    pinMode(statusLedPin, OUTPUT);
    dataLoggerSerial.begin(9600); // Serial communication with Sensor Board

    Serial.println("Initializing SD card...");
    if (!SD.begin(chipSelect)) {
        error("Card failed, or not present");
    }
    Serial.println("SD card initialized.");

    char filename[] = "LOGGER00.CSV";
    for (uint8_t i = 0; i < 100; i++) {
        filename[6] = i / 10 + '0';
        filename[7] = i % 10 + '0';
        if (!SD.exists(filename)) {
            logfile = SD.open(filename, FILE_WRITE);
            Serial.print("Created file: ");
            Serial.println(filename);
            break;
        }
    }

    if (!logfile) {
        error("Couldn't create file");
    }

    Wire.begin();
    if (!RTC.begin()) {
        Serial.println("RTC failed");
    }

    logfile.println("millis,datetime,distance,confidence");
    logfile.flush();
    Serial.println("Logging headers written to file.");
}

void loop() {
    DateTime now = RTC.now();
    String dataString = String(millis()) + ", \"" +
                        String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + " " +
                        String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + "\"";

    // Read data from Sensor Board
    if (dataLoggerSerial.available()) {
        String input = dataLoggerSerial.readStringUntil('\n');
        
        // Debugging: Print raw data from Sensor Board to Serial Monitor
        Serial.print("Received: ");
        Serial.println(input);

        if (input.startsWith("DISTANCE:")) {
            input.remove(0, 9); // Remove "DISTANCE:" prefix
            int commaIndex = input.indexOf(',');
            if (commaIndex != -1) {
                int distance = input.substring(0, commaIndex).toInt();
                int confidence = input.substring(commaIndex + 1).toInt();
                dataString += ", " + String(distance) + ", " + String(confidence);
            } else {
                Serial.println("Error: Malformed input");
                dataString += ", , "; // Handle missing data
            }
        } else {
            Serial.println("Error: Input does not start with DISTANCE:");
            dataString += ", , "; // Handle missing data
        }
    } else {
        Serial.println("No data available from Sensor Board.");
    }

    logfile.println(dataString);
    logfile.flush();
    Serial.println("Data logged: " + dataString); // Debugging: Print final data string to Serial Monitor

    digitalWrite(statusLedPin, !digitalRead(statusLedPin));
    delay(2000); // Adjust delay as needed
}
