// original code: https://sourceforge.net/projects/riseandshineledclock/files/

#include <FastLED.h>  
#include <Wire.h>  // I2C: On Arduino Uno & Nano & ProMini use pins A4 for SDA (yellow/orange) and A5 for SCL (green). For other boards ee http://arduino.cc/en/Reference/Wire
#include <RTClib.h>           // Include the RTClib library to enable communication with the real time clock.
#include <EEPROM.h>           // Include the EEPROM library to enable the storing and retrieval of settings.
#include <Bounce.h>           // Include the Bounce library for de-bouncing issues with push buttons.
#include <Encoder.h>          // Include the Encoder library to read the outputs of the rotary encoders

RTC_DS1307 RTC; // Establishes the chipset of the Real Time Clock

// Pin definitions:
#define rotaryLeft 4
#define rotaryRight 5
#define LEDStripPin 9 // Pin used for the data to the LED strip
#define menuPin 2 // Arduino Pro Mini supports external interrupts only on pins 2 and 3
#define reedSwitchPin 3 // Switches off the display to reduce power consumption (before re-flashing)

#define startingLEDs 4 // Number of backlight LEDs BEFORE the strip
#define numLEDs 60 // Number of LEDs in strip
#define LEDOffset  30 // First LED in strip corresponds to 30-th second

// Setting up the LED strip
struct CRGB _leds[startingLEDs+numLEDs];
Encoder rotary1(rotaryLeft, rotaryRight); // Setting up the Rotary Encoder
#define ROTARY_TICKS 4

DateTime old; // Variable to compare new and old time, to see if it has moved on.
int rotary1Pos  = 0;
int subSeconds; // 60th's of a second
int secondBrightness;
int secondBrightness2;
int breathBrightness;
long newSecTime; // Variable to record when a new second starts, allowing to create milli seconds
long oldSecTime;
long flashTime; //
long breathCycleTime;
#define holdTime 1500
int cyclesPerSec;
float cyclesPerSecFloat; // So can be used as a float in calcs
float fracOfSec;
float breathFracOfSec;
boolean demo;
#define demoTime 15 // seconds
long previousDemoTime;
long currentDemoTime;
boolean swingBack = false;

int timeHour;
int timeMin;
int timeSec;

//////////////
// EEPROM section:
    int mode = 0; // Variable of the display mode of the clock
    #define modeMax 6 // Change this when new modes are added. This is so selecting modes can go back beyond.

    uint8_t alarmMin = 0; // The minute of the alarm  
    uint8_t alarmHour = 0; // The hour of the alarm 0-23
    uint8_t alarmDay = 0; // The day of the alarm
    boolean alarmSet = false; // Whether the alarm is set or not

    int alarmMode = 0; // Variable of the alarm display mode
    int alarmModeMax = 3;

    #define modeAddress 0 // Address of where mode is stored in the EEPROM
    #define alarmMinAddress 1 // Address of where alarm minute is stored in the EEPROM
    #define alarmHourAddress 2 // Address of where alarm hour is stored in the EEPROM
    #define alarmSetAddress 3 // Address of where alarm state is stored in the EEPROM
    #define alarmModeAddress 4 // Address of where the alarm mode is stored in the EEPROM
    #define magicValAddress 5
    #define magicVal 0x3B
// End EEPROM section :)
///////////////////////

boolean alarmTrig = false; // Whether the alarm has been triggered or not
long alarmTrigTime; // Milli seconds since the alarm was triggered
boolean countDown = false;
long countDownTime = 0;
long currentCountDown = 0;
long startCountDown;
int countDownMin;
int countDownSec;
int countDownFlash;
int demoIntro = 0;
int j = 0;
long timeInterval = 15;
long currentMillis;
long previousMillis = 0;
float LEDBrightness = 0;
float fadeTime;
float brightFadeRad;

int state = 0; // Variable of the state of the clock, with the following defined states 
#define clockState 0
#define alarmState 1
#define setAlarmHourState 2
#define setAlarmMinState 3
#define setClockHourState 4
#define setClockMinState 5
#define setClockSecState 6
#define countDownState 7
#define demoState 8

Bounce menuBouncer = Bounce(menuPin,20); // Instantiate a Bounce object with a 50 millisecond debounce time for the menu button
boolean menuButton = false; 
boolean menuPressed = false;
boolean menuReleased = false;
int advanceMove = 0;
boolean countTime = false;
long menuTimePressed;
long lastRotary;
int rotaryTime = 1000;

int LEDPosition;
int reverseLEDPosition;
int pendulumPos;
int fiveMins;
int odd;

// CONVERSIONS
        // Which LED (in [0..59] range) corresponds to given hh:mm time 
        uint8_t _hourPos (uint8_t hour, uint8_t minute) { 
          uint8_t _hp = (hour%12)*5 + (minute+6)/12;
          return (_hp == 60) ? 0 : _hp;
        }

        // Which offset (in [4..63] range) corresponds to given [0..59] mm time 
        // First [0..3] offsets are reserved for backlight LEDs
        struct CRGB* findLED (uint8_t minute) {
          return &_leds[(LEDOffset+minute)%numLEDs+startingLEDs];
        }
// END CONVERSIONS

void setup() {
  // Set up all pins
  pinMode(menuPin, INPUT_PULLUP); 
    
  // Start LEDs
  LEDS.addLeds<WS2811, LEDStripPin, GRB>(_leds, startingLEDs+numLEDs); 
  
  // Start RTC
  Wire.begin(); // Arduino Ptro Mini i2c: SDA = A4, SCL = A5.
  RTC.begin(); // Starts communications to the RTC
  
  Serial.begin(9600); // Starts the serial communications

  // Load any saved setting since power off, such as mode & alarm time  
  if (EEPROM.read(magicValAddress) == magicVal) {
    mode = EEPROM.read(modeAddress); // The mode will be stored in the address "0" of the EEPROM
    alarmMin = EEPROM.read(alarmMinAddress); 
    alarmHour = EEPROM.read(alarmHourAddress); 
    alarmSet = EEPROM.read(alarmSetAddress); 
    alarmMode = EEPROM.read(alarmModeAddress);
    // Prints all the saved EEPROM data to Serial
    Serial.print("Mode is ");Serial.println(mode);
    Serial.print("Alarm Hour is ");Serial.println(alarmHour);
    Serial.print("Alarm Min is ");Serial.println(alarmMin);
    Serial.print("Alarm is set ");Serial.println(alarmSet);
    Serial.print("Alarm Mode is ");Serial.println(alarmMode);
  } else
    Serial.print("EEPROM was empty. Writing magicVal to it.");
    EEPROM.write(magicValAddress, magicVal);
  
    EEPROM.write(modeAddress, mode); // The mode will be stored in the address "0" of the EEPROM
    EEPROM.write(alarmMinAddress, alarmMin); 
    EEPROM.write(alarmHourAddress, alarmHour); 
    EEPROM.write(alarmSetAddress, alarmSet); 
    EEPROM.write(alarmModeAddress, alarmMode);
  }

  // create a loop that calcuated the number of counted milliseconds between each second.
  DateTime now = RTC.now();
      
  Serial.print("Hour time is... ");
  Serial.println(now.hour());
  Serial.print("Min time is... ");
  Serial.println(now.minute());
  Serial.print("Sec time is... ");
  Serial.println(now.second());

  Serial.print("Year is... ");
  Serial.println(now.year());
  Serial.print("Month is... ");
  Serial.println(now.month());
  Serial.print("Day is... ");
  Serial.println(now.day());
}

void loop() {
  DateTime now = RTC.now(); // Fetches the time from RTC
  
  // Check for any button presses and action accordingley
  menuButton = menuBouncer.update();  // Update the debouncer for the menu button and saves state to menuButton
  rotary1Pos = rotary1.read(); // Checks the rotary position
  if (rotary1Pos <= -ROTARY_TICKS && lastRotary - millis() >= rotaryTime)
    {
      advanceMove = -1;
      rotary1.write(0);
      lastRotary = millis();
    } 
  if (rotary1Pos >= ROTARY_TICKS && lastRotary - millis() >= rotaryTime)
    {
      advanceMove = 1;
      rotary1.write(0);
      lastRotary = millis();
    }
  if (menuButton == true || advanceMove != 0 || countTime == true) {buttonCheck(menuBouncer,now);}
  
  // clear LED array
  clearLEDs();
  
  // Check alarm and trigger if the time matches
  if (alarmSet == true && alarmDay != now.day()) // The alarmDay statement ensures it is a newly set alarm or repeat from previous day, not within the minute of an alarm cancel.
    {
      if (alarmTrig == false) {alarm(now);}
      else {alarmDisplay();}
    }
 // Check the Countdown Timer
  if (countDown == true)
    {
      currentCountDown = countDownTime + startCountDown - now.unixtime();
      if ( currentCountDown <= 0)
        {
          state =countDownState;
        }
    } 
  // Set the time LED's
  if (state == setClockHourState || state == setClockMinState || state == setClockSecState) {setClockDisplay(now);}
  else if (state == alarmState || state == setAlarmHourState || state == setAlarmMinState) {setAlarmDisplay();}
  else if (state == countDownState) {countDownDisplay(now);}
  else if (state == demoState) {runDemo(now);}
  else {timeDisplay(now);}

  // Update LEDs
  LEDS.show();
}

void buttonCheck(Bounce menuBouncer, DateTime now)
{
  if (menuBouncer.fallingEdge()) // Checks if a button is pressed, if so sets countTime to true
    {
      countTime = true;
      Serial.println("rising edge");
    }
  if (menuBouncer.risingEdge()) // Checks if a button is released,
    {
      countTime = false;
      Serial.println("rising edge");
    } // if so sets countTime to false. Now the ...TimePressed will not be updated when enters the buttonCheck,
  if (countTime) // otherwise will menuBouncer.duration will 
    {
      menuTimePressed = menuBouncer.duration();
      if (menuTimePressed >= (holdTime - 100) && menuTimePressed <= holdTime)
        {
          clearLEDs();
          LEDS.show();
          delay(100);
        }
    }
  menuReleased = menuBouncer.risingEdge();
  if (menuPressed == true) {Serial.println("Menu Button Pressed");}
  if (menuReleased == true) {Serial.println("Menu Button Released");}
  Serial.print("Menu Bounce Duration ");
  Serial.println(menuTimePressed);
  if (alarmTrig == true)
    {
      alarmTrig = false;
      alarmDay = now.day(); // When the alarm is cancelled it will not display until next day. As without it, it would start again if within a minute, or completely turn off the alarm.
      delay(300); // I added this 300ms delay, so there is time for the button to be released
      return; // This return exits the buttonCheck function, so no actions are performs
    }  
  switch (state)
    {
      case clockState: // State 0
        if (advanceMove == -1 && mode == 0)
          {
            mode = modeMax;
            advanceMove = 0;
          }
        else if(advanceMove != 0) //if displaying the clock, advance button is pressed & released, then mode will change
          {
            mode = mode + advanceMove;
            EEPROM.write(modeAddress,mode);
            advanceMove = 0;
          }
        else if(menuReleased == true) 
          {
            if (menuTimePressed <= holdTime) {state = alarmState; newSecTime = millis();}// if displaying the clock, menu button is pressed & released, then Alarm is displayed 
            else {state = setClockHourState;} // if displaying the clock, menu button is held & released, then clock hour can be set
          }
        break;
      case alarmState: // State 1
        if (advanceMove == -1 && alarmMode <= 0)
          {
            alarmMode = alarmModeMax;
            alarmSet = 1;
          }
        else if (advanceMove == 1 && alarmMode >= alarmModeMax)
          {
            alarmMode = 0;
            alarmSet = 0;
          }
        else if (advanceMove != 0)
          {
            alarmMode = alarmMode + advanceMove;
            if (alarmMode == 0) {alarmSet = 0;}
            else {alarmSet = 1;}
          }          
        Serial.print("alarmState is ");
        Serial.println(alarmState);            
        Serial.print("alarmMode is ");
        Serial.println(alarmMode);
        EEPROM.write(alarmSetAddress,alarmSet);
        EEPROM.write(alarmModeAddress,alarmMode);
        advanceMove = 0;
        alarmTrig = false;
        if (menuReleased == true) 
          {
            if (menuTimePressed <= holdTime) {state = countDownState; j = 0;}// if displaying the alarm time, menu button is pressed & released, then clock is displayed
            else {state = setAlarmHourState;} // if displaying the alarm time, menu button is held & released, then alarm hour can be set
          }
        break;
      case setAlarmHourState: // State 2
        if (menuReleased == true) {state = setAlarmMinState;}
        else if (advanceMove == 1 && alarmHour >= 23) {alarmHour = 0;}
        else if (advanceMove == -1 && alarmHour <= 0) {alarmHour = 23;}
        else if (advanceMove != 0) {alarmHour = alarmHour + advanceMove;}
        EEPROM.write(alarmHourAddress,alarmHour);
        advanceMove = 0;
        break;
      case setAlarmMinState: // State 3
        if (menuReleased == true)
          {
            state = alarmState;
            alarmDay = 0;
            newSecTime = millis();
          }
        else if (advanceMove == 1 && alarmMin >= 59) {alarmMin = 0;}
        else if (advanceMove == -1 && alarmMin <= 0) {alarmMin = 59;}
        else if (advanceMove != 0) {alarmMin = alarmMin + advanceMove;}
        EEPROM.write(alarmMinAddress,alarmMin);
        advanceMove = 0;
        break;
      case setClockHourState: // State 4
        if (menuReleased == true) {state = setClockMinState;}
        else if (advanceMove == 1 && now.hour() == 23)
          {
            RTC.adjust(DateTime(now.year(), now.month(), now.day(), 0, now.minute(), now.second()));
            advanceMove = 0;
          }
        else if (advanceMove == -1 && now.hour() == 0)
          {
            RTC.adjust(DateTime(now.year(), now.month(), now.day(), 23, now.minute(), now.second()));
            advanceMove = 0;
          }
        else if (advanceMove != 0)
          {
            RTC.adjust(DateTime(now.year(), now.month(), now.day(), (now.hour() + advanceMove), now.minute(), now.second()));
            advanceMove = 0;
          }
        break;
      case setClockMinState: // State 5
        if (menuReleased == true) {state = setClockSecState;}
        else if (advanceMove == 1 && now.minute() == 59)
          {
            RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), 0, now.second()));
            advanceMove = 0;
          }
        else if (advanceMove == -1 && now.minute() == 0)
          {
            RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), 59, now.second()));
            advanceMove = 0;
          }
        else if (advanceMove != 0)
          {
            RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), (now.minute() + advanceMove), now.second()));
            advanceMove = 0;
          }
        break;
      case setClockSecState: // State 6
        if (menuReleased == true) {state = clockState;}
        else if (advanceMove == 1 && now.second() == 59)
          {
            RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), 0));
            advanceMove = 0;
          }
        else if (advanceMove == -1 && now.second() == 0)
          {
            RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), 59));
            advanceMove = 0;
          }
        else if (advanceMove != 0)
          {
            RTC.adjust(DateTime(now.year(), now.month(), now.day(), now.hour(), now.minute(), (now.second() + advanceMove)));
            advanceMove = 0;
          }
        break;
      case countDownState: // State 7
        if(menuReleased == true) 
          {
            if (menuTimePressed <= holdTime)
              {
                if (countDown == true && countDownTime <= 0) {countDown = false; countDownTime = 0; currentCountDown = 0;}
                else if (countDown == false && countDownTime > 0) {countDown = true; startCountDown = now.unixtime();}
                else {state = demoState; demoIntro = 1; j = 0;}// if displaying the count down, menu button is pressed & released, then demo State is displayed 
              } 
            else {countDown = false; countDownTime = 0; currentCountDown = 0; j = 0;} // if displaying the clock, menu button is held & released, then the count down is reset
          }
        else if (advanceMove == -1 && currentCountDown <= 0)
          {
            countDown = false;
            countDownTime = 0;
            currentCountDown = 0;
            demoIntro = 0;          
          }
        else if (advanceMove == 1 && currentCountDown >= 3600)
          {
            countDown = false;
            countDownTime = 3600;           
          }
        else if (advanceMove != 0) //if displaying the count down, rotary encoder is turned then will change accordingley
          {
            countDown = false;
            countDownTime = currentCountDown - currentCountDown%60 + advanceMove*60; // This rounds the count down minute up to the next minute
          }
        advanceMove = 0;
        break;
      case demoState: // State 8
        if(menuReleased == true) {state = clockState; mode = EEPROM.read(modeAddress);} // if displaying the demo, menu button pressed then the clock will display and restore to the mode before demo started
        break;
    }
  if (menuReleased || advanceMove !=0) {countTime = false;}
  Serial.print("Mode is ");
  Serial.println(mode);
  Serial.print("State is ");
  Serial.println(state);
}

void setAlarmDisplay() {

  for (int i = 0; i < numLEDs; i+=5) {
      findLED(i)->r = 100;
      findLED(i)->g = 100;
      findLED(i)->b = 100;
  }

  if (alarmSet == 0) {
      for (int i = 0; i < numLEDs; i+=5) { 
            // Sets background to red, to state that alarm IS NOT set
            findLED(i)->r = 20;
            findLED(i)->g = 0;
            findLED(i)->b = 0;
      }     
  } else {
      for (int i = 0; i < numLEDs; i+=5) {
          // Sets background to green, to state that alarm IS set
          findLED(i)->r = 0;
          findLED(i)->g = 20;
          findLED(i)->b = 0;
      }     
  }
  
  if (alarmHour <= 11) {
      findLED(_hourPos(alarmHour, alarmMin))->r = 255;
  } else {
      findLED(_hourPos(alarmHour, alarmMin))->r = 190;
      findLED(_hourPos(alarmHour, alarmMin)-1)->r = 30;
      findLED(_hourPos(alarmHour, alarmMin)+1)->r = 30;
  }
  
  findLED(alarmMin)->g = 100;
  flashTime = millis();
  if (state == setAlarmHourState && flashTime%300 >= 150)
    {
      findLED(_hourPos(alarmHour, alarmMin))->r = 0;
      findLED(_hourPos(alarmHour, alarmMin)-1)->r = 0;
      findLED(_hourPos(alarmHour, alarmMin)+1)->r = 0;
    }
  if (state == setAlarmMinState && flashTime%300 >= 150) {
      findLED(alarmMin)->g = 0;
  }
  findLED(alarmMode)->b = 255;
}

void setClockDisplay (DateTime now) {

  for (int i = 0; i < numLEDs; i+=numLEDs/12) {
      findLED(i)->r = 100;
      findLED(i)->g = 100;
      findLED(i)->b = 100;
  }

  if (now.hour() <= 11) {findLED(_hourPos(now.hour(), now.minute()))->r = 255;}
  else {
      findLED(_hourPos(now.hour(), now.minute()))->r = 190;
      findLED(_hourPos(now.hour(), now.minute())-1)->r = 30;
      findLED(_hourPos(now.hour(), now.minute())+1)->r = 30;
  }
  flashTime = millis();
  if (state == setClockHourState && flashTime%300 >= 150) {
      findLED(_hourPos(now.hour(), now.minute()))->r = 0;
      findLED(_hourPos(now.hour(), now.minute())-1)->r = 0;
      findLED(_hourPos(now.hour(), now.minute())+1)->r = 0;
  }
    
  if (state == setClockMinState && flashTime%300 >= 150) {
      findLED(now.minute())->g = 0;
  } else {
      findLED(now.minute())->g = 255;
  }
  if (state == setClockSecState && flashTime%300 >= 150) {
      findLED(now.second())->b = 0;
  } else {
      findLED(now.second())->b = 255;
  }
}

// Check if alarm is active and if is it time for the alarm to trigger
void alarm(DateTime now)
{
  if ((alarmMin == now.minute()%60) && (alarmHour == now.hour()%24)) //check if the time is the same to trigger alarm
    {
      alarmTrig = true;
      alarmTrigTime = millis();
    }
}

void alarmDisplay() // Displays the alarm
{
  switch (alarmMode)
    {
      case 1:
        // set all LEDs to a dim white
        for (int i = 0; i < numLEDs; i++)
          {
            findLED(i)->r = 100;
            findLED(i)->g = 100;
            findLED(i)->b = 100;
          }
        break;
      case 2:
        LEDPosition = ((millis() - alarmTrigTime)/300);
        reverseLEDPosition = 60 - LEDPosition;
        if (LEDPosition >= 0 && LEDPosition <= 29)
          {
            for (int i = 0; i < LEDPosition; i++)
              {
                findLED(i)->r = 5;
                findLED(i)->g = 5;
                findLED(i)->.b = 5;
              }
          }
        if (reverseLEDPosition <= 59 && reverseLEDPosition >= 31)
          {
            for (int i = 59; i > reverseLEDPosition; i--)
              {
                findLED(i)->r = 5;
                findLED(i)->g = 5;
                findLED(i)->b = 5;
              }              
          }
        if (LEDPosition >= 30)
          {
            for (int i = 0; i < numLEDs; i++)
              {
                findLED(i)->r = 5;
                findLED(i)->g = 5;
                findLED(i)->b = 5;
              }           
          }            
        break;
      case 3:
        fadeTime = 60000;
        brightFadeRad = (millis() - alarmTrigTime)/fadeTime; // Divided by the time period of the fade up.
        if (millis() > alarmTrigTime + fadeTime) LEDBrightness = 255;
        else LEDBrightness = 255.0*(1.0+sin((1.57*brightFadeRad)-1.57));
        Serial.println(brightFadeRad);
        Serial.println(LEDBrightness);
        for (int i = 0; i < numLEDs; i++)
          {
            findLED(i)->r = LEDBrightness;
            findLED(i)->g = LEDBrightness;
            findLED(i)->b = LEDBrightness;
          }
        break;
    }
}

//  
void countDownDisplay(DateTime now)
{
  flashTime = millis();
  if (countDown == true)
    {
      currentCountDown = countDownTime + startCountDown - now.unixtime();
      if (currentCountDown > 0)
        {
          countDownMin = currentCountDown / 60;
          countDownSec = currentCountDown%60 * 4; // have multiplied by 4 to create brightness
          for (int i = 0; i < countDownMin; i++) {
              // Set a blue LED for each complete minute that is remaining 
              findLED(i+1)->b = 240;
          } 
          findLED(i+1)->b = countDownSec; // Display the remaining secconds of the current minute as its brightness      
        }
      else
        {
          countDownFlash = now.unixtime()%2;
          if (countDownFlash == 0)
            {
              for (int i = 0; i < numLEDs; i++) // Set the background as all off
                {
                  findLED(i)->r = 0;
                  findLED(i)->g = 0;
                  findLED(i)->b = 0;
                }
            }
          else
            {
              for (int i = 0; i < numLEDs; i++) // Set the background as all blue
                {
                  findLED(i)->r = 0;
                  findLED(i)->g = 0;
                  findLED(i)->b = 255;
                }
            }
        }
    }
  else
    {
      currentCountDown = countDownTime;
      if (countDownTime == 0)
        {
          currentMillis = millis();
          clearLEDs();
          switch (demoIntro)
            {
              case 0:
                for (int i = 0; i < j; i++) {
                    findLED(i+1)->b = 20;
                }
                if (currentMillis - previousMillis > timeInterval) {
                    j++; 
                    previousMillis = currentMillis;
                }
                if (j == numLEDs) {demoIntro = 1;}
                break;
              case 1:
                for (int i = 0; i < j; i++) {
                    findLED(i+1)->b = 20;
                }
                if (currentMillis - previousMillis > timeInterval) {j--; previousMillis = currentMillis;}
                if (j < 0) {demoIntro = 0;}
                break;
            }
        }
      else if (countDownTime > 0 && flashTime%300 >= 150)
        {
          countDownMin = currentCountDown / 60; //
          for (int i = 0; i < countDownMin; i++) {
              // Set a blue LED for each complete minute that is remaining
              findLED(i+1)->b = 255;
          } 
        }
    }
}

void runDemo(DateTime now)
{
  currentDemoTime = now.unixtime();
  currentMillis = millis();
  clearLEDs();
  switch (demoIntro)
    {
      case 0:
        timeDisplay(now);
        if (currentDemoTime - previousDemoTime > demoTime) {previousDemoTime = currentDemoTime; mode++;}
        break;
      case 1:
        for (int i = 0; i < j; i++) {
            findLED(i)->r = 255;
        }
        if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
        if (j == numLEDs) {j = 0; demoIntro++;}
        break;
      case 2:
        for (int i = j; i < numLEDs; i++) {findLED(i)->r = 255;}
        if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
        if (j == numLEDs) {j = 0; demoIntro++;}
        break;
      case 3:
        for (int i = 0; i < j; i++) {findLED(i)->g = 255;}
        if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
        if (j == numLEDs) {j = 0; demoIntro++;}
        break;
      case 4:
        for (int i = j; i < numLEDs; i++) {findLED(i)->g = 255;}
        if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
        if (j == numLEDs) {j = 0; demoIntro++;}
        break;
      case 5:
        for (int i = 0; i < j; i++) {findLED(i)->b = 255;}
        if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
        if (j == numLEDs) {j = 0; demoIntro++;}
        break;
      case 6:
        for (int i = j; i < numLEDs; i++) {findLED(i)->b = 255;}
        if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
        if (j == numLEDs) {j = 0; demoIntro++;}
        break;
      case 7:
        for (int i = 0; i < j; i++) {findLED(i)->r = 255; findLED(i)->g = 255; findLED(i)->b = 255;}
        if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
        if (j == numLEDs) {j = 0; demoIntro++;}
        break;
      case 8:
        for (int i = j; i < numLEDs; i++) {findLED(i)->r = 255; findLED(i)->g = 255; findLED(i)->b = 255;}
        if (currentMillis - previousMillis > timeInterval) {j++; previousMillis = currentMillis;}
        if (j == numLEDs)
          {
            demoIntro = 0;
            mode = 0;
            Serial.print("Mode is ");
            Serial.println(mode);
            Serial.print("State is ");
            Serial.println(state);
          }
        break;
    }
}

void clearLEDs() {      
  for (int i = 0; i < numLEDs; i++) {
      findLED(i)->r = 0;
      findLED(i)->g = 0;
      findLED(i)->b = 0;
  }
}

void timeDisplay(DateTime now)
{ 
  switch (mode)
    {
      case 0:
        minimalClock(now);
        break;
      case 1:
        basicClock(now);
        break;
      case 2:
        smoothSecond(now);
        break;
      case 3:
        outlineClock(now);
        break;
      case 4:
        minimalMilliSec(now);
        break;
      case 5:
        simplePendulum(now);
        break;
      case 6:
        breathingClock(now);
        break;
      default: // Keep this here and add more timeDisplay modes as defined cases.
        {
          mode = 0;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
//   CLOCK DISPLAY MODES
// Add any new display mode functions here. Then add to the "void timeDisplay(DateTime now)" function.
// Add each of the new display mode functions as a new "case", leaving default last.
////////////////////////////////////////////////////////////////////////////////////////////

//
void minimalClock(DateTime now) {
  uint8_t hourPos = _hourPos (now.hour, now.minute);
  
  findLED(hourPos)->r = 255;
  findLED(now.minute)->g = 255;
  findLED(now.second)->b = 255;
}

//
void basicClock(DateTime now) {
  uint8_t hourPos = _hourPos (now.hour, now.minute);

  // Hour (9 lines of code)
          findLED(hourPos-1)->r = 30;
          findLED(hourPos-1)->g =  0;
          findLED(hourPos-1)->b =  0;

          findLED(hourPos+1)->r = 30;
          findLED(hourPos+1)->g =  0;
          findLED(hourPos+1)->b =  0;

          findLED(hourPos)->r  = 190;
          findLED(hourPos)->g  =   0;
          findLED(hourPos)->b  =   0;
  
  // Minute  
          findLED(now.minute)->r =   0;
          findLED(now.minute)->g = 255;
          findLED(now.minute)->b =   0;
    
  // Second  
          findLED(now.second)->r =   0;
          findLED(now.second)->g =   0;
          findLED(now.second)->b = 255;
}

// 
void smoothSecond(DateTime now)
{
  if (now.second()!=old.second())
    {
      old = now;
      cyclesPerSec = millis() - newSecTime;
      cyclesPerSecFloat = (float) cyclesPerSec;
      newSecTime = millis();      
    } 
  // set hour, min & sec LEDs
  fracOfSec = (millis() - newSecTime)/cyclesPerSecFloat;  // This divides by 733, but should be 1000 and not sure why???
  if (subSeconds < cyclesPerSec) {secondBrightness = 50.0*(1.0+sin((3.14*fracOfSec)-1.57));}
  if (subSeconds < cyclesPerSec) {secondBrightness2 = 50.0*(1.0+sin((3.14*fracOfSec)+1.57));}

  uint8_t hourPos = _hourPos (now.hour, now.minute);
  // The colours are set last, so if on same LED mixed colours are created
  // Hour (3 lines of code)
          findLED(hourPos-1)->r = 30;
          findLED(hourPos+1)->r = 30;
          findLED(hourPos)->r  = 190;
  
  // Minute  
          findLED(now.minute)->g = 255;
    
  // Second  
          findLED(now.second())->b = secondBrightness;
          findLED(now.second()+59)->b = secondBrightness2;
}

//
void outlineClock(DateTime now)
{
  for (int i = 0; i < numLEDs; i+= numLEDs/12) {
      findLED(i)->r = 100;
      findLED(i)->g = 100;
      findLED(i)->b = 100;
  }
  uint8_t hourPos = _hourPos (now.hour, now.minute);

  // Hour (3 lines of code)
          findLED(hourPos-1)->r = 30;
          findLED(hourPos+1)->r = 30;
          findLED(hourPos)->r  = 190;
  
  // Minute  
          findLED(now.minute)->g = 255;
    
  // Second  
          findLED(now.second)->b = 255;
}

//
void minimalMilliSec(DateTime now)
{
  if (now.second()!=old.second())
    {
      old = now;
      cyclesPerSec = (millis() - newSecTime);
      newSecTime = millis();
    } 
  // set hour, min & sec LEDs
  uint8_t hourPos = _hourPos (now);
  subSeconds = (((millis() - newSecTime)*60)/cyclesPerSec)%60;  // This divides by 733, but should be 1000 and not sure why???

  // Millisec lights are set first, so hour/min/sec lights override and don't flicker as millisec passes
          findLED(subSeconds)->r = 50;
          findLED(subSeconds)->g = 50;
          findLED(subSeconds)->b = 50;

  // The colours are set last, so if on same LED mixed colours are created
  // Hour (3 lines of code)
          findLED(hourPos-1)->r = 30;
          findLED(hourPos+1)->r = 30;
          findLED(hourPos)->r  = 190;
  
  // Minute  
          findLED(now.minute)->g = 255;
}

// Pendulum will be at the bottom and left for one second and right for one second
void simplePendulum(DateTime now)
{
  if (now.second()!=old.second())
    {
      old = now;
      cyclesPerSec = millis() - newSecTime;
      cyclesPerSecFloat = (float) cyclesPerSec;
      newSecTime = millis();
      if (swingBack == true) {swingBack = false;}
      else {swingBack = true;}
    } 
    
  // set hour, min & sec LEDs
  fracOfSec = (millis() - newSecTime)/cyclesPerSecFloat;  // This divides by 733, but should be 1000 and not sure why???
  if (subSeconds < cyclesPerSec && swingBack == true) {pendulumPos = 27.0 + 3.4*(1.0+sin((3.14*fracOfSec)-1.57));}
  if (subSeconds < cyclesPerSec && swingBack == false) {pendulumPos = 27.0 + 3.4*(1.0+sin((3.14*fracOfSec)+1.57));}

  uint8_t hourPos = _hourPos (now.hour, now.minute);
    
  // Pendulum lights are set first, so hour/min/sec lights override and don't flicker as millisec passes
  findLED(pendulumPos)->r = 100;
  findLED(pendulumPos)->g = 100;
  findLED(pendulumPos)->b = 100;
    
  // The colours are set last, so if on same LED mixed colours are created
  // Hour (3 lines of code)
          findLED(hourPos-1)->r = 30;
          findLED(hourPos+1)->r = 30;
          findLED(hourPos)->r  = 190;
  
  // Minute  
          findLED(now.minute)->g = 255;
    
  // Second  
          findLED(now.second)->b = 255;
}

void breathingClock(DateTime now)
{
  if (alarmTrig == false) {
      breathBrightness = 15.0*(1.0+sin((3.14*millis()/2000.0)-1.57));
      for (int i = 0; i < numLEDs; i++) {
          fiveMins = i%5;
          if (fiveMins == 0) {
              findLED(i)->r = breathBrightness + 5;
              findLED(i)->g = breathBrightness + 5;
              findLED(i)->b = breathBrightness + 5;
          } else {
              findLED(i)->r = 0;
              findLED(i)->g = 0;
              findLED(i)->b = 0;
          }
       }
  }
  
  uint8_t hourPos = _hourPos (now.hour, now.minute);

  // Hour (3 lines of code)
          findLED(hourPos-1)->r = 30;
          findLED(hourPos+1)->r = 30;
          findLED(hourPos)->r  = 190;
  
  // Minute  
          findLED(now.minute)->g = 255;
    
  // Second  
          findLED(now.second)->b = 255;
}

