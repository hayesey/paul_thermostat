// Pauls Thermostat version 3
// changed to use llap
#include <LiquidCrystal.h>
#include <Arduino.h>

const int beta = 4090; // from thermistor datasheet
const int resistance = 33;
const char llapId[] = "TS";

// LiquidCrystal display with:
// rs on pin 12
// rw on pin 11
// enable on pin 10
// d4-7 on pins 5-2
LiquidCrystal lcd(12, 11, 10, 5, 4, 3, 2);

//int ledPin = 15;
int relayPin = 16;
int aPin = 6;
int xrfPin = 8;
int bPin = 17;
int buttonPin = 7;
int analogPin = 0;
int lightPin = 13;
int count2 = 0;
int count = 0;
int i = 0;
//char inData[50]; // Size as appropriate
//char inChar;

float setTemp = 20.0;
float measuredTemp;
char mode = 'C';        // can be changed to F 
boolean is_off = false;
boolean heatingOn = false;
boolean ended = false;
float hysteresis = 0.25;

// function prototypes
void backlight(boolean state);
int getEncoderTurn();
float readTemp();
void updateOutputs();
void updateDisplay();
float adjustUnits(float temp);
void readLlap();
void readSerial();

void setup() {
  lcd.begin(2, 20);
  //  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(aPin, INPUT);  
  pinMode(xrfPin, OUTPUT);
  digitalWrite(xrfPin, HIGH);
  pinMode(bPin, INPUT);
  pinMode(buttonPin, INPUT);
  pinMode(lightPin, OUTPUT);
  // back light on at first boot
  digitalWrite(lightPin, HIGH);
  lcd.clear();
  Serial.begin(9600);
  //inData[0] = '\0';
}

void loop() {
  measuredTemp = readTemp();
  if (digitalRead(buttonPin)) {
    // something happened so turn the backlight on
    backlight(HIGH);
    is_off = ! is_off;
    updateDisplay();
    delay(500); // debounce
  }
  int change = getEncoderTurn();
  setTemp = setTemp + change * 0.5;
  if (count == 1000) {
    // turn the backlight off after a few seconds
    if (count2 == 100) {
      backlight(LOW);
    }
    count2++;
    updateDisplay();
    updateOutputs();
    count = 0;
  }
  count ++;
}

void backlight(boolean state) {
  // set the backlight and reset count2
  digitalWrite(lightPin, state);
  count2 = 0;
}

int getEncoderTurn() {
  // if off then don't change
  if (is_off) {
    return 0; 
  }
  // return -1, 0, or +1
  static int oldA = LOW;
  static int oldB = LOW;
  int result = 0;
  int newA = digitalRead(aPin);
  int newB = digitalRead(bPin);
  if (newA != oldA || newB != oldB) {
    // something has changed
    if (oldA == LOW && newA == HIGH) {
      result = -(oldB * 2 - 1);
      // turn the back light on
      backlight(HIGH);
    }
  }
  oldA = newA;
  oldB = newB;
  return result;
} 

float readTemp() {
  long a = analogRead(analogPin);
  float temp = beta / (log(((1025.0 * resistance / a) - 33.0) / 33.0) + (beta / 298.0)) - 273.0;
  return temp;
}

void updateOutputs() {
  if (!is_off &&  measuredTemp < setTemp - hysteresis) {
    //digitalWrite(ledPin, HIGH);
    digitalWrite(relayPin, HIGH);
    heatingOn = true;
  } 
  else if (is_off || measuredTemp > setTemp + hysteresis) {
    //digitalWrite(ledPin, LOW);
    digitalWrite(relayPin, LOW);
    heatingOn = false;
  }
}

void updateDisplay() {
  lcd.setCursor(0,0);
  lcd.print("Actual: ");
  lcd.print(adjustUnits(measuredTemp));
  lcd.print(" o");
  lcd.print(mode);
  lcd.print(" ");
  
  lcd.setCursor(0,1);
  if (is_off) {
    lcd.print("****HEAT OFF****");
  }
  else {
    lcd.print("Set:    ");
    lcd.print(adjustUnits(setTemp));
    lcd.print(" o");
    lcd.print(mode);
    lcd.print(" ");
  }
}

float adjustUnits(float temp) {
  if (mode == 'C') {
    return temp;
  }
  else {
    return (temp * 9) / 5 + 32;
  }
}


/*
  serialEvent runs after each loop if data is waiting
  replace readSerial with this once done
  if there is anything to read at the serial port
  read in 12 bytes (with timeout) which is an llap
  message.  Make sure it's for us and then act accordingly
*/
void serialEvent() {
  char llapMsg[13];
  // this is default but just to make sure
  Serial.setTimeout(1000);
  // try and read 12 bytes, give up after one second
  // presumably dont need serial.available in this function
  int byteCount = Serial.readBytes(llapMsg, 12);
  if (byteCount < 12) {
    // then not enough data was received
    return;      	
  }
  // make sure it's null terminated
  llapMsg[12] = '\0';

  if (llapMsg[0] == 'a') {
    // not a valid llap message
    return;
  }
  // now decode the message
  char msgId[] = {llapMsg[1], llapMsg[2], '\0'};
  if (strcmp(msgId, llapId) != 0) {
    // message isn't for us
    return;
  }
  char msg[9];
  msg[9] = '\0';
  for (int i=0; i<9; i++) {
    msg[i] = llapMsg[i+3];
  }
  // whenever sending an llap message we much ensure it is always exactly 12 bytes
  if (strstr(msg, "RTMP-----")) {
    // reply with the temp
    //char msgSend[] = "aTSRTMP" + measuredTemp;
    Serial.print("aTSRTMP");
    Serial.print(measuredTemp);
  }
  else if (strstr(msg, "TTMP")) {
    // set or reply with target temp
    char msgTail[6] = {msg[4], msg[5], msg[6], msg[7], msg[8], '\0'};
    if (strcmp(msgTail, "-----")) {
      // return the target temp
      Serial.print("aTSTTMP");
      Serial.print(setTemp);
    }
    else {
      // set the target temp
      sscanf(msgTail, "%f", &setTemp);
      Serial.print("aTSTTMP");
      Serial.print(setTemp);
    }
  }
  else if (strstr(msg, "STAT")) {
    // set state or reply with state
    char msgTail[6] = {msg[4], msg[5], msg[6], msg[7], msg[8], '\0'};
    if (strcmp(msgTail, "-----")) {
      // return the state of the thermostat
      Serial.print("aTSSTAT");
      if (is_off) {
	Serial.print(0);
      } 
      else {
	Serial.print(1);
      }
      Serial.print("----");
    }
    else if (strstr(msgTail, "1----")) {
      // turn stat on
      is_off = false;
      Serial.print("aTSSTAT1----");
    }
    else if (strstr(msgTail, "0----")) {
      // turn stat off
      is_off = true;
      Serial.print("aTSSTAT0----");
    }
  }
  else if (strstr(msg, "RELA-----")) {
    // reply with relay state
    Serial.print("aTSRELA");
    if (heatingOn = true) {
      Serial.print("1----");
    }
    else {
      Serial.print("0----");
    }
  }
}
