/* GasLeakDetector.ino
   Advanced LPG Gas Leakage Detection & Alert System
   - MQ-series gas sensor (analog)
   - 16x2 LCD for local display
   - SG90 servo for valve shutoff
   - Buzzer for local alarm
   - Relay for exhaust fan
   - SIM800L for SMS and call alerts
   NOTE: Replace phone number placeholders and calibrate gasThreshold.
*/

#include <LiquidCrystal.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// Pin definitions (edit to match your wiring)
const int gasAnalogPin = A0;    // MQ sensor analog output
const int servoPin      = 9;    // SG90 servo signal
const int buzzerPin     = 10;   // Active buzzer
const int relayPin      = 11;   // Relay to control fan

// LCD pins (RS, E, D4, D5, D6, D7) - example mapping, change if needed
LiquidCrystal lcd(12, 5, 4, 3, 2, 6);

Servo valveServo;
bool valveClosed = false;

// GSM (SIM800L) on software serial (RX, TX)
SoftwareSerial simSerial(7, 8); // RX, TX

// Phone numbers to notify - replace with actual numbers beginning with country code
const char* phoneNumbers[] = { "+91XXXXXXXXXX" };
const int totalNumbers = sizeof(phoneNumbers) / sizeof(phoneNumbers[0]);

// Threshold - must be calibrated for your sensor/environment
int gasThreshold = 300; // adjust after calibration

void setup() {
  Serial.begin(9600);
  simSerial.begin(9600);

  pinMode(buzzerPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(relayPin, LOW);

  lcd.begin(16, 2);
  lcd.print("System Initializing");
  delay(2000);
  lcd.clear();
  lcd.print("SAFE LEVEL");
  valveServo.attach(servoPin);
  valveServo.write(0); // open position
  valveClosed = false;
  delay(1000);
}

void loop() {
  int sensorVal = analogRead(gasAnalogPin);
  Serial.print("Gas analog: "); Serial.println(sensorVal);

  lcd.setCursor(0, 0);
  lcd.print("GAS: ");
  lcd.print(sensorVal);
  lcd.print("    ");

  if (sensorVal >= gasThreshold && !valveClosed) {
    Serial.println("ALERT: Gas detected!");
    lcd.setCursor(0, 1);
    lcd.print("GAS LEAK!       ");
    digitalWrite(buzzerPin, HIGH);
    digitalWrite(relayPin, HIGH); // turn on fan
    closeValve();
    send_sms();
    send_call();
  } else if (sensorVal < gasThreshold - 50) {
    digitalWrite(relayPin, LOW);
    digitalWrite(buzzerPin, LOW);
    lcd.setCursor(0, 1);
    lcd.print("SAFE LEVEL      ");
    // Leave valve closed until manual reset if desired
  }

  delay(1000);
}

void closeValve() {
  for (int i = 0; i <= 90; i += 5) {
    valveServo.write(i);
    delay(50);
  }
  valveClosed = true;
}

void send_sms() {
  for (int i = 0; i < totalNumbers; i++) {
    simSerial.println("AT");
    delay(500);
    simSerial.println("ATE0");
    delay(300);
    simSerial.println("AT+CMGF=1");
    delay(500);
    simSerial.print("AT+CMGS=\"");
    simSerial.print(phoneNumbers[i]);
    simSerial.println("\"");
    delay(500);
    simSerial.println("⚠ Alert: Gas leakage detected! Valve shut off ⚠");
    delay(500);
    simSerial.write(26); // Ctrl+Z
    delay(3000);
  }
}

void send_call() {
  for (int i = 0; i < totalNumbers; i++) {
    simSerial.print("ATD");
    simSerial.print(phoneNumbers[i]);
    simSerial.println(";");
    delay(20000); // ring for 20 seconds
    simSerial.println("ATH"); // hang up
    delay(1000);
  }
}
