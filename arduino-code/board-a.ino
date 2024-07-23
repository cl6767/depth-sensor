#include "ping1d.h"
#include <SPI.h>
#include "RTClib.h"
#include "SoftwareSerial.h"

// Pins for SoftwareSerial
static const uint8_t arduinoRxPin = 8;
static const uint8_t arduinoTxPin = 9;
SoftwareSerial pingSerial(arduinoRxPin, arduinoTxPin);
static Ping1D ping { pingSerial };

static const uint8_t ledPin = 13;

void setup() {
    pingSerial.begin(9600);
    Serial.begin(115200);
    pinMode(ledPin, OUTPUT);
    Serial.println("Sensor Board Initialized");

    while (!ping.initialize()) {
        Serial.println("\nPing device failed to initialize!");
        delay(2000);
    }
}

void loop() {
    if (ping.request(PingMessageId::PING1D_DISTANCE)) {
        int distance = ping.distance();
        int confidence = ping.confidence();
        String dataString = "DISTANCE:" + String(distance) + "," + String(confidence);
        Serial.println(dataString); // Send data to Data Logger Board
    } else {
        Serial.println("Ping request failed");
    }

    digitalWrite(ledPin, !digitalRead(ledPin));
    delay(2000); // Measure every 2 seconds
}
