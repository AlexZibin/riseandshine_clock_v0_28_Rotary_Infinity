
void f1 (void) {
  Serial.println ("Mode: f1");
  
  /*
  if (turnLeft ()) {
  } 
  elseif (turnRight ()) {
  }
  if (shortPress ()) {
  } 
  elseif (longPress ()) {
  }
  */
}

void f2 (void) {
  Serial.println ("Mode: f2");
}

void f3 (void) {
  Serial.println ("Mode: f3");
}

/* */
// begin modeChanger.h
// массив указателей на функции:
typedef void (*fPtr)(void);

//void (*modeFuncArray[])(void) = {f1, f2, f3);
void fColorDemo10sec (void);

class modeChanger {
  public:
    modeChanger (fPtr funcArray, unsigned int numModes);
    int currMode (void);
    int nextMode (void);
    int prevMode (void);
    int applyMode (void *newModeFunc(void));
    void callCurrModeFunc (void);
    bool modeJustChanged (void);
  private:
    int _currMode = -1; // -1 is an indication of an error (index out of range; -1 = array not initialized; -2 = function not found; etc);
    int _prevMode = -100;
    unsigned int _numModes = -1;
    fPtr _funcArray = NULL;
};

modeChanger::modeChanger (fPtr funcArray, const unsigned int numModes) {
  _currMode = 0;
  _funcArray = funcArray;
  _numModes = numModes;
}

bool modeChanger::modeJustChanged (void) {
    if (_prevMode != _currMode) {
        _prevMode = _currMode;
        return true;
    }
    return false;
}

int modeChanger::currMode (void) { return _currMode; }

int modeChanger::nextMode (void) {
    if (_currMode > -1) { // Negative stands for some error
        if (++_currMode == _numModes) { // Close the circle
            _currMode = 0;
        }
    }    
    return _currMode; 
}

int modeChanger::prevMode (void) {
    if (_currMode > -1) { // Negative stands for some error
        if (--_currMode == -1) { // Close the circle backwards
            _currMode = _numModes;
        }
    }    
    return _currMode; 
}

int modeChanger::applyMode (void *newModeFunc(void)) {
    _currMode = -2; // negative value is an indication of an error
    for (int i = 0; i++; i < _numModes) {
        // compare two pointers
        if (newModeFunc == _funcArray[i]) {
            _currMode = i;
        }
    }
    if (_currMode < 0) { // newModeFunc not found in the list of our registered functions! This is not allowed!
        Serial.println ("\n\nERROR! MODE NOT FOUND!\n\n");
        delay (1000);
    }
    return _currMode; 
}

void modeChanger::callCurrModeFunc (void) {
    if (_currMode > -1) { // Negative stands for some error
        (*_funcArray)[_currMode] ();
    }
}

// end modeChanger.h
/**/

int currMode (void) {
  static int i = 0;
  
  /*if (++i == _numModes) {
    i = 0;
  }*/
  return i;
}

static fPtr modeFuncArray[] {f1, f2, f3, fColorDemo10sec};
const unsigned int numModes = sizeof(modeFuncArray)/sizeof(modeFuncArray[0]);
modeChanger mode = new modeChanger (modeFuncArray, numModes);

void setup () {
  initDevices ();
  readEEPROM ();
}

void loop () {
  //(*modeFuncArray)[currMode ()] ();
  mode.callCurrModeFunc ();
  delay (1000);
}

void initDevices (void) {
   Serial.begin (9600);
}

void readEEPROM (void) {}

void fColorDemo10sec (void) {
  static unsigned long millisAtStart;
  
  Serial.println ("Mode: fDemo1");
  
  /* */
  if (mode.modeJustChanged()) {
      millisAtStart = millis ();
  }
  /* */
  
  if (secondsPassed (10)) {
    Serial.println ("Timeout: 10 secondsPassed; Applying mode f1");
    applyMode (f1);
  }
  else {
      unsigned long deltaT = millis () - millisAtStart;
      int timeStep = 300;
      int direction = 1;
      double wavelen = 6;
    
      for (int led = 0; led < numLEDs; led++) {
          // =ОСТАТ(время/timeStep1-направление*диод; waveLen)<1
          if ((deltaT/timeStep-direction*led)%wavelen < 1) {
              // switch this led on
              // Хорошо бы и предыдущий led несильно зажигать, чтобы выглядело как затухающий след 
              // И встречную волну пустить другого цвета
          } else {
              // switch this led off
          }
      }
  }
}

bool secondsPassed (unsigned int seconds) {
  static bool countdownIsRunning = false;
  static unsigned long savedMillisec;
  
  if (!countdownIsRunning) { 
      countdownIsRunning = true; 
      savedMillisec = millis ();
  }
  if ((millis () - savedMillisec)/1000UL >= seconds) {
      countdownIsRunning = false;
      return true;
  }
  return false;
}
                                 
