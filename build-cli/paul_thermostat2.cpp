#include <Arduino.h>
// Pauls Thermostat version 2
#include <LiquidCrystal.h>
#include <Arduino.h>

#define beta 4090 // from thermistor datasheet
#define resistance 33

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
int count3 = 0;
int i = 0;
char inData[50]; // Size as appropriate
char inChar;

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
void readSerial();

void setup()
{
  lcd.begin(2, 20);
  //pinMode(ledPin, OUTPUT);
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

void loop()
{


  readSerial();
  measuredTemp = readTemp();
  if (digitalRead(buttonPin))
  {
    // something happened so turn the backlight on
    backlight(HIGH);
    is_off = ! is_off;
    updateDisplay();
    delay(500); // debounce
  }
  int change = getEncoderTurn();
  setTemp = setTemp + change * 0.5;
  if (count == 1000)
  {
    // turn the backlight off after a few seconds
    if (count2 == 100)
    {
      backlight(LOW);
      // send data roughly every 14 mins
      if (count3 == 20)
      {
        count3 = 0;
	// send data via xrf
      	Serial.print("to:001");
      	Serial.print(",from:002");
      	Serial.print(",temp:");
      	Serial.print(measuredTemp);
      	if (!is_off)
      	{
	  Serial.print(",set:");
          Serial.print(setTemp);
          Serial.print(",heat:");
          Serial.println(heatingOn, DEC);
        } 
        else 
        {
          Serial.println(",stat:0");
        }
	Serial.print("\n");
      }
      count3++;
    }
    count2++;
    updateDisplay();
    updateOutputs();
    count = 0;
  }
  count ++;
}

void backlight(boolean state)
{
  // set the backlight and reset count2
  digitalWrite(lightPin, state);
  count2 = 0;
}

int getEncoderTurn()
{
  // if off then don't change
  if (is_off)
  {
    return 0; 
  }
  // return -1, 0, or +1
  static int oldA = LOW;
  static int oldB = LOW;
  int result = 0;
  int newA = digitalRead(aPin);
  int newB = digitalRead(bPin);
  if (newA != oldA || newB != oldB)
  {
    // something has changed
    if (oldA == LOW && newA == HIGH)
    {
      result = -(oldB * 2 - 1);
      // turn the back light on
      backlight(HIGH);
    }
  }
  oldA = newA;
  oldB = newB;
  return result;
} 

float readTemp()
{
  long a = analogRead(analogPin);
  float temp = beta / (log(((1025.0 * resistance / a) - 33.0) / 33.0) + (beta / 298.0)) - 273.0;
  return temp;
}

void updateOutputs()
{
  if (!is_off &&  measuredTemp < setTemp - hysteresis)
  {
//    digitalWrite(ledPin, HIGH);
    digitalWrite(relayPin, HIGH);
    heatingOn = true;
  } 
  else if (is_off || measuredTemp > setTemp + hysteresis)
  {
//    digitalWrite(ledPin, LOW);
    digitalWrite(relayPin, LOW);
    heatingOn = false;
  }
}

void updateDisplay()
{
  lcd.setCursor(0,0);
  lcd.print("Actual: ");
  lcd.print(adjustUnits(measuredTemp));
  lcd.print(" o");
  lcd.print(mode);
  lcd.print(" ");
  
  lcd.setCursor(0,1);
  if (is_off)
  {
    lcd.print("****HEAT OFF****");
  }
  else
  {
    lcd.print("Set:    ");
    lcd.print(adjustUnits(setTemp));
    lcd.print(" o");
    lcd.print(mode);
    lcd.print(" ");
  }
}

float adjustUnits(float temp)
{
  if (mode == 'C')
  {
    return temp;
  }
  else
  {
    return (temp * 9) / 5 + 32;
  }
}

void readSerial()
{
  //int i = 0;
  //inData[0] = '\0';
  ended = false;
  while(Serial.available() > 0)
  {
    inChar = Serial.read();
    //delay(400);
    if (inChar == 13)
    {
      i = 0;
      Serial.println(inData);
      //Serial.println(i);
      ended=true;
      break;
    }
    inData[i] = inChar;
    i++;
    inData[i] = '\0';
  }
  // todo: split string around comma
  if (ended)
  {
    //String strData = inData;
    if (strcmp(inData, "heat") == 0) 
    {
      is_off = false;
      Serial.println("ON");
      //ended=false;
      //memset(inData,0,sizeof(inData));
    }
    if (strcmp(inData, "cold") == 0)
    {
      is_off = true;
      Serial.println("OFF");
      //ended=false;
      //memset(inData,0,sizeof(inData));      
    }
    //if (strData.startsWith("set"))
    //{
      //setTemp = float(strData.substring(4));
    //}
  } 
}
