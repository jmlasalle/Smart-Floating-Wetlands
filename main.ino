#include <Servo.h>

/*
   Water Conductivity Sensor

   This sketch controlls the brightness of an LED with a water conductivity sensor

   This sketch draws from the
   Water Conductivity Sketch by Emily Gertz at https://github.com/ejgertz/EMWA/tree/master/chapter-5
   &
   the Example Sketch 06: Photo Resistor by Sparkfun Electronics

   Written by Sagari Datta, John Michael LaSalle, 2018
   Licensed under WTFPL http://www.wtfpl.net/about/

*/


//conductivity sensor variables
const float ArduinoVoltage = 5.00; // CHANGE THIS FOR 3.3v Arduinos
const float ArduinoResolution = ArduinoVoltage / 1024; // the Arduino will read the current, which ranges from 0-5 Volts on a scale of 0-1024
const float resistorValue = 10000.0; // the resistance of the 10K Resistor in the voltage divider
float returnVoltage = 0.0; // variable for the voltage of the reading from the conductivity sensor
float resistance = 0.0; // variable for the resistance of the water
double Siemens = 0.0; // variable
float TDS = 0.0;

//PH sensor variables
float Offset = 0.00; //Setting an offset for deviation compensate
float voltage; //var used in ph reading
float pHValue; //var used in ph reading

//Turbidity sensor variables
float volt;
float ntu;

//Input Pins
int conductPin = A0; // pin from voltage divider
int tempPin = A1; // reading from tempurature sensor
int turbidityPin = A2; // reading from tubridity sensor
float phPin = A3; // reading from PH sensor

//Output Pins
int ledPins[] = {3, 5, 6}; // pin for LED
int servoPin = 11;
int dcPin = 9;

// Input reading variables
int conductReading; // variable for conductivity reading on a scale from 0-1023
int tempReading; // variable for conductivity reading on a scale from 0-1023
int turbidityReading; // variable for conductivity reading on a scale from 0-1023
float phReading; // variable for PH reading on scale from -7 - 7

//Intermediate variables
int conductance = 1; // 1 if water is in good conductivity range and 0 if it is in bad ranges
int ph = 1; // 1 if water is in good conductivity range and 0 if it is in bad range
int turbidity = 1; //1 if water is in good conductivity range and 0 if it is in bad ranges

// Output variables
int lightLevel = 0; // variable for LED brightness on a scale from 0-255
int servoPosition = 0; // variable for the servo position on a scale from 0-180
int combinedQuality = 0; //

Servo servo1;  // servo control object

void setup()
{
  Serial.begin(9600); //set baud rate for the serial monitor for debugging

  // Set the input pin modes
  pinMode(conductPin, INPUT); // Set the conductivity sensor pin mode
  pinMode(tempPin, INPUT); // Set the tempurature sensor pin mode
  pinMode(turbidityPin, INPUT); // Set the turbity sensor pin mode
  pinMode(phPin, INPUT); // Set the PH sensor pin mode

  // Set output pin modes
  pinMode(ledPins[0], OUTPUT); // set the red LED pin mode
  pinMode(ledPins[1], OUTPUT); // set the Green LED pin mode
  pinMode(ledPins[2], OUTPUT); // set the Blue LED pin mode
  servo1.attach(11); // set the servo control pin mode
  pinMode(dcPin, OUTPUT); // set the DC motor pin mode

}

void loop()
{
  //Creating local variables for time, pHValue and voltage
  static unsigned long samplingTime = millis();
  static unsigned long printTime = millis();
  static float pHValue, voltage;

  // take sensor readings
  tempReading = analogRead(tempPin); // tempurature reading

  //conductance reading
  float Siemens = conductivitySiemens(analogRead(conductPin));
  Serial.print("Conductance: "); Serial.print(Siemens); Serial.println(" microSeimens");
  //testing if water is the the healthy fisheries conductivity range or not.
  if (Siemens > 150 || Siemens < 500) {
    conductance = 1; // Specific conductance in bad range
  } else {
    conductance = 0;
  }

  // PH Reading
  phReading = phValue(analogRead(phPin));
  Serial.print("PH: "); Serial.println(phReading);
  if (phReading < 8) {
    ph = 1; // ph in bad range
  } else {
    ph = 0;
  }

  //Turbidity reading
  turbidityReading = turbidityValue(turbidityPin); // turbidity reading
  Serial.print("Turbididty: "); Serial.print(turbidityReading); Serial.println(" NTU");
  if (turbidityReading > 150) {
    ph = 1; // turbidity in bad range
  } else {
    ph = 0;
  }


  if ((conductance + ph + turbidity) >= 2) { // check if at least 2 indicators are in the bad range
    combinedQuality = 1; // water quality is bad
  } else {
    combinedQuality = 0; // water quality is good
  }

  motorResponse(combinedQuality);

  delay(3000); // wait 1 second before taking a reading again
}

void motorResponse(int Quality) {
  if (Quality == 1) {
    servoPosition = 0;
    servo1.write(servoPosition);
    digitalWrite(ledPins[0], HIGH);
    digitalWrite(ledPins[1], HIGH);
    digitalWrite(ledPins[2], HIGH);
    //digitalWrite(dcPin, HIGH);
    Serial.println("water bad");
  }
  else {
    servoPosition = 90;
    servo1.write(servoPosition);
    digitalWrite(ledPins[0], LOW);
    digitalWrite(ledPins[1], LOW);
    digitalWrite(ledPins[2], LOW);
    //digitalWrite(dcPin, LOW);
    Serial.println("water good");
  }
}

// function for conductivity reading
float conductivitySiemens(int conductReading) {
  returnVoltage = conductReading * ArduinoResolution; // convert the raw conductivity reading to volts
  resistance = ((5.00 * resistorValue) / returnVoltage) - resistorValue; // convert voltage to ohms
  Siemens = 1.0 / (resistance / 1000000); // convert ohms to microSiemens

  return Siemens;
}

// PH reading function
float phValue(int phReading) {
  //Calculating pHValue
  voltage = phReading * 5.0 / 1024;
  pHValue = 3.5 * voltage + Offset;

  return pHValue;
}

// Turbidity reading function
float turbidityValue(int turbidityReading) {
  for (int i = 0; i < 800; i++)    //The voltage from the sensor is quite noisy
  { //so I take 800 samples and then average those samples
    int sensorvalue = analogRead(turbidityReading);
    volt += sensorvalue * (5.0 / 1024.0);  //Convert the analog reading （0~1023） to a voltage (0 ~ 5V)
  }
  volt = volt / 800 + 0.8; // average the reading?
  volt = round_to_dp(volt, 1);     //Round the volt number to 1 decimal place

  if (volt < 2.5) {                //The equation only works between 2.5V ~ 4.2V,
    ntu = 3000;                    //so set any readings below 2.5V with 3000NTU
  } else {
    ntu = -1120.4 * square(volt) + 5742.3 * volt - 4353.8; // Calculate the current NTU
  }

  return ntu;
}

// funciton used in turbidity function
float round_to_dp( float in_value, int decimal_place )
{
  float multiplier = powf( 10.0f, decimal_place );
  in_value = roundf( in_value * multiplier ) / multiplier;
  return in_value;
}
