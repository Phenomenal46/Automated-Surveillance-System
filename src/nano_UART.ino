/**
 * Nano Board Code to Trigger ESP32‑CAM Capture via UART Command
 *
 * When an object is detected within a threshold distance, the servo motion stops 
 * (preventing motion blur) and the Nano sends a "CAPTURE" command via Serial.
 * The ESP32‑CAM board (with its own code) will listen for this command to capture
 * a photo and send it via email.
 */

#include <Arduino.h>
#include <Servo.h>

// ----- Pin Definitions -----
const int trigPin = 2;         // Ultrasonic sensor TRIG pin
const int echoPin = 3;         // Ultrasonic sensor ECHO pin
const int servoPin = 9;        // Servo control pin (SG90)
const int buzzerPin = 10;      // Buzzer pin (via transistor)
const int laserPin = 11;       // Laser LED pin (with resistor)
// Note: No dedicated ESP32‑CAM trigger pin is used here.


// ----- Other Parameters -----
const int thresholdDistance = 20; // Distance in cm below which an object is considered "detected"

// ----- Servo and state variables -----
Servo servoMotor;
int servoAngle = 0;
int direction = 1;           // 1: increasing angle, -1: decreasing angle
bool objectDetected = false;

// Function to read distance from an HC-SR04 ultrasonic sensor.
long readUltrasonicDistance() {
  // Ensure TRIG is LOW initially.
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Send a 10-microsecond pulse.
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Read the pulse duration from echoPin; timeout after 30000µs (30ms).
  long duration = pulseIn(echoPin, HIGH, 30000);
  long distance = duration / 58;  // Convert duration to centimeters.
  return distance;
}

void setup() {
  Serial.begin(115200);
  
  // Set sensor pins.
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // Configure output pins.
  pinMode(buzzerPin, OUTPUT);
  pinMode(laserPin, OUTPUT);
  
  // Default states: buzzer off, laser off.
  digitalWrite(buzzerPin, LOW);
  digitalWrite(laserPin, LOW);
  
  // Setup and start the servo.
  servoMotor.attach(servoPin);
  servoMotor.write(servoAngle);
}

void loop() {
  long distance = readUltrasonicDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  
  // If an object is detected within the threshold distance.
  if (distance > 0 && distance < thresholdDistance) {
    if (!objectDetected) {
      objectDetected = true;
      Serial.println("Object detected. Stopping servo, activating alerts, and sending CAPTURE command.");
      
      // Stop the servo motion by not updating its position.
      // Activate buzzer and laser LED.
      digitalWrite(buzzerPin, HIGH);
      digitalWrite(laserPin, HIGH);
      
      // Send the capture command over Serial.
      // (Ensure that the command is sent on its own line for the ESP32‑CAM to recognize it.)
      Serial.println("CAPTURE");
      
      // Optionally, additional debug lines may appear before or after,
      // but the ESP32‑CAM code checks each complete line so only an exact "CAPTURE"
      // command will trigger a capture.
      
      // Delay to avoid immediate re-triggering.
      delay(500);
    }
  } else {
    if (objectDetected) {
      // Object is now removed. Reset state and deactivate alerts.
      objectDetected = false;
      digitalWrite(buzzerPin, LOW);
      digitalWrite(laserPin, LOW);
      Serial.println("Object removed. Resuming servo motion.");
    }
    
    // Continuous servo sweeping when no object is present.
    servoAngle += direction;
    if (servoAngle <= 0 || servoAngle >= 180) {
      direction = -direction;
    }
    servoMotor.write(servoAngle);
    delay(15);  // Adjust for smooth servo motion.
  }
  
  delay(50);  // Main loop delay.
}