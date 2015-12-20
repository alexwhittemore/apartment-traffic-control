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

/*
 * Define the voltage readings corresponding to various alcohol states. These are all determined through
 * trial and error (delicious trial and error).
 * 
 * From experience, the sensor should settle to sub .5. One drink IMMEDIATELY will blow high 2s
 * After 15m, I blew 1.83.
 * RIGHT after a second drink, 3.25. Quickly dropping to 2.8.
 * 
 * Note that in practice, after the first party deployment of the light, these values ALL had to be adjusted.
 * Maybe it was the humidity? +10-15 degree shift in temperature? Alcohol in the air?
 */

#define SETTLEDVALUE 1.9
#define LOWVALUE 1.8
#define MEDVALUE 3.3

// Sensor is attached to Analog 0
#define SENSOR A0

// Define the states of the state machine below.
#define STATEWAITING 2
#define STATEREADING 3
#define STATERECOVERING 4

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
BlinkTask yellowBlink(YELLOWLIGHT, 500);
BlinkTask redBlink(REDLIGHT, 500);

/*
 * taskMain does the heavy lifting once per second. PrintAlcoholValue makes sure the consol gets a constant
 * stream of updated values for diagnostics and interactive tuning.
 */
Task taskMain(1000, mainCode);
Task taskPrintAlcoholValue(500, printAlcoholValue);

// Start off in the "recovering" state until the sensor settles
int state = STATERECOVERING;
// waitCount is used in the reading state to peak-detect across a few cycles. The variables below are memory for this functionality
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
 * Enable this in order to block operation until you connect to the serial terminal.
  while (!Console){
    ; // wait for Console port to connect.
  }
  Console.println("You're connected to the Console!!!!");
*/

  //Start the timers
  SoftTimer.add(&taskMain);
  SoftTimer.add(&taskPrintAlcoholValue);
}

void mainCode(Task* me) {
  // Run the peak detect code. This gets reset in the state machine if it's not pertinent.
  waitCount = waitCount + 1;
  lastReading = currentReading;
  currentReading = getAlcoholVoltage();
  // Detect a new peak.
  if (currentReading > maxReading) {
    maxReading = currentReading;
  }

  // State machine.
  switch (state) {
    case STATERECOVERING:
    {
      /*
       * In the "recovering" state, we're waiting for the sensor resistance to drop to some known low value, indicating a 
       * settled reading. Low resistance corresponds to low voltage on the analog reading. 
       */
       
      /*
       * I'm not quite sure why this manual pin write is necessary - the blinker should run if we're to remain in this state,
       * and shut off if we're to transition out. But for some reason, it gets hung up. This fixes it, so I never debugged
       * exactly why. It's tough to do without a proper debugger.
       */
      digitalWrite(REDLIGHT, LOW);
      // Wait count is not pertinent here, so make sure it's reset.
      waitCount = 0;
      Console.println("Recovering");
      if (currentReading > SETTLEDVALUE) {
        // we should stay in this state because the value is not settled to below our threshold yet.
        redBlink.start();
      } else {
        // Stop the red blinking light and transition to the "waiting" state.
        redBlink.stop();
        state = STATEWAITING;
      }
    }
    break;
    case STATEWAITING:
    {
      /*
       * In the waiting state, we're waiting to detect a movement on the sensor value, indicating someone breathing booze-breaht.
       * Once we see this shift in value, we know to flash the yellow indicator until we've detected a good peak.
       */

      // Reset the wait count and peak detection in prep for the reading state.
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
      /*
       * In the reading state, we're trying to peak-detect. Flash a yellow indicator to the user to keep breathing, until we do.
       */
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
  }
}

void printAlcoholValue(Task* me) {
  Console.println(getAlcoholVoltage());
}

float getAlcoholVoltage() {
  // Calculate voltage from binary word.
  return analogRead(SENSOR) * (5.0 / 1023.0);
}

void evalLights(float reading) {
  Console.println("Evaluating:");
  Console.println(reading);
  Console.println("reading above");

  // Depending on the read value, compare it to our thresholds and indicate for 5s the proper drunkenness level.
  if (reading <= LOWVALUE) {
    digitalWrite(GREENLIGHT, HIGH);
    delay(5000);
    digitalWrite(GREENLIGHT, LOW);
  } else if (reading <= MEDVALUE) {
    digitalWrite(YELLOWLIGHT, HIGH);
    delay(5000);
    digitalWrite(YELLOWLIGHT, LOW);
  } else if (reading > MEDVALUE) {
    digitalWrite(REDLIGHT, HIGH);
    delay(5000);
    digitalWrite(REDLIGHT, LOW);
  }
}

