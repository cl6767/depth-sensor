#include <SD.h>
#include <SPI.h>
#include <SoftwareSerial.h>

const int CS_PIN = 10; // Chip select pin for SD card module

// Create a SoftwareSerial object
SoftwareSerial pingSerial(9, 10); // RX pin, TX pin

File dataFile;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for Serial Monitor to open

  // Initialize SD card
  if (!SD.begin(CS_PIN)) {
    Serial.println("SD card initialization failed!");
    while (1); // Stop if SD card initialization fails
  }
  Serial.println("SD card initialized.");

  // Initialize communication with Teensy over SoftwareSerial
  pingSerial.begin(115200);

  // Open a file to write
  dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) {
    Serial.println("File opened successfully.");
  } else {
    Serial.println("Error opening file.");
  }
}

void loop() {
  // Read data from Teensy over SoftwareSerial
  while (pingSerial.available() > 0) {
    String data = pingSerial.readStringUntil('\n');
    // Log data to SD card
    dataFile.println(data);
    Serial.print("Data logged: ");
    Serial.println(data);
  }

  delay(100); // Adjust delay as needed
}
