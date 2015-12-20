 /*
 * Apartment Traffic Control
 * Copyright: Alex Whittemore 2015
 * alexwhittemore@gmail.com
 * Please drink responsibly!
    Apartment Traffic Control is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <BlinkTask.h>
#include <SoftTimer.h>
#include <Task.h>
#include <Console.h>

// Define the pins that the lights are controlled by via the relay board.
#define GREENLIGHT 5
#define YELLOWLIGHT 6
#define REDLIGHT 7

// Define the voltage readings corresponding to various alcohol states. These are all determined through
// trial and error (delicious trial and error).

// From experience, the sensor should settle to sub .5. One drink IMMEDIATELY will blow high 2s
// After 15m, I blew 1.83.
// RIGHT after a second drink, 3.25. Quickly dropping to 2.8. 

// Note that in practice, after the first party deployment of the light, these values ALL had to be adjusted.
// Maybe it was the humidity? +10-15 degree shift in temperature? Alcohol in the air?

#define SETTLEDVALUE 1.9
#define LOWVALUE 1.8
#define MEDVALUE 3.3

// Sensor is attached to Analog 0
#define SENSOR A0

// Define the states of the state machine below.
#define STATEWAITING 2
#define STATEREADING 3
#define STATERECOVERING 4
#define STATEINDICATING 5

// Declare functions, because arduino chokes and dies when these come post-call in the file, for some reason.
void mainCode(Task* me);
void printAlcoholValue(Task* me);
float getAlcoholVoltage();
void evalLights(float reading);

// Set up blinkers:
// -- On for 200ms off for 100ms, repeat it 2 times, sleep for 2000 ms and than start again.
//BlinkTask hartbeat(LED_PIN, 200, 100, 2, 2000);
// -- On for 300ms off for 200ms, repeat it 3 times, than stop.
//BlinkTask hartbeat(LED_PIN, 300, 200, 3);

BlinkTask greenBlink(GREENLIGHT, 1000);
BlinkTask greenIndicate(GREENLIGHT, 5000, 100, 1);
BlinkTask yellowBlink(YELLOWLIGHT, 500);
BlinkTask yellowIndicate(YELLOWLIGHT, 5000, 100, 1);
BlinkTask redBlink(REDLIGHT, 500);
BlinkTask redIndicate(REDLIGHT, 5000, 100, 1);

Task taskMain(1000, mainCode);
Task taskPrintAlcoholValue(500, printAlcoholValue);

int runningTracker = LOW;

int state = STATERECOVERING;
int waitCount = 0;
float currentReading;
float lastReading;
float maxReading;

void setup() {
  // Define the relay outputs and set them OFF:
  pinMode(GREENLIGHT, OUTPUT);
  pinMode(YELLOWLIGHT, OUTPUT);
  pinMode(REDLIGHT, OUTPUT);
  digitalWrite(GREENLIGHT, LOW);
  digitalWrite(YELLOWLIGHT, LOW);
  digitalWrite(REDLIGHT, LOW);

  // Define the alcohol sensor input:
  pinMode(SENSOR, INPUT);

  // initialize serial communication:
  Bridge.begin();
  Console.begin(); 

/*
  while (!Console){
    ; // wait for Console port to connect.
  } */
  Console.println("You're connected to the Console!!!!");
  //Start the timers
  SoftTimer.add(&taskMain);
  SoftTimer.add(&taskPrintAlcoholValue);
}

void mainCode(Task* me) {
  waitCount = waitCount + 1;
  lastReading = currentReading;
  currentReading = getAlcoholVoltage();
  if (currentReading > maxReading) {
    maxReading = currentReading;
  }
  switch (state) {
    case STATERECOVERING:
    {
      digitalWrite(REDLIGHT, LOW);
      waitCount = 0;
      Console.println("Recovering");
      if (currentReading > SETTLEDVALUE) {
        redBlink.start();
      } else {
        redBlink.stop();
        state = STATEWAITING;
      }
    }
    break;
    case STATEWAITING:
    {
      waitCount = 0;
      maxReading = 0;
      Console.println("Waiting");
      // If the reading moves up by .1 volts, assume we're taking a reading).
      if (currentReading > (lastReading + .1)) {
        state = STATEREADING;
      }
    }
    break;
    case STATEREADING:
    {
      yellowBlink.start();
      Console.println("Reading");
      // Reset the waitcount if we're still moving.
      if ( !((currentReading - lastReading) < .05)){
        waitCount = 0;
      }
      // Hold for 3 counts of no movement.
      if ( (currentReading - lastReading) < .05 && waitCount > 3) {
        // Reading has settled.
        //state = STATEINDICATING;
        state = STATERECOVERING;
        yellowBlink.stop();
        evalLights(maxReading);
      }
    }
    break;
    /*
    case STATEINDICATING:
    {
      if (waitCount > 5) {
        greenIndicate.stop();
        yellowIndicate.stop();
        redIndicate.stop();
        state = STATERECOVERING;
      }
    }
    break;
    */
  }
}

void printAlcoholValue(Task* me) {
  Console.println(getAlcoholVoltage());
}

float getAlcoholVoltage() {
  return analogRead(SENSOR) * (5.0 / 1023.0);
}

void evalLights(float reading) {
  Console.println("Evaluating:");
  Console.println(reading);
  Console.println("reading above");
  
  if (reading <= LOWVALUE) {
    //greenIndicate.start();
    digitalWrite(GREENLIGHT, HIGH);
    delay(5000);
    digitalWrite(GREENLIGHT, LOW);
  } else if (reading <= MEDVALUE) {
    //yellowIndicate.start();
    digitalWrite(YELLOWLIGHT, HIGH);
    delay(5000);
    digitalWrite(YELLOWLIGHT, LOW);
  } else if (reading > MEDVALUE) {
    //redIndicate.start();
    digitalWrite(REDLIGHT, HIGH);
    delay(5000);
    digitalWrite(REDLIGHT, LOW);
  }
}

