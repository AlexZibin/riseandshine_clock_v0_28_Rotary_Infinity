
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
// typedef modeFuncArrayPtr ........

class modeChanger {
  public:
    void modeChanger (modeFuncArrayPtr funcArray, unsigned int numModes);
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
    modeFuncArrayPtr _funcArray = NULL;
}

bool modeChanger::modeJustChanged (void) {
    if (_prevMode != _currMode) {
        _prevMode = _currMode;
        return true;
    }
    return false;
}

void modeChanger::modeChanger (modeFuncArrayPtr funcArray, unsigned int numModes) {
  _currMode (0);
  _funcArray = funcArray;
  _numModes = numModes;
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
        Serial.println ("\n\nERROR! MODE NOT FOUND!\n\n);
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

void (*modeFuncArray[])(void) = {&f1, &f2, &f3);
const int numModes = sizeof(modeFunc)/sizeof(modeFunc)/sizeof(modeFunc[0]);

int currMode (void) {
  static int i = 0;
  
  /*if (++i == _numModes) {
    i = 0;
  }*/
  return i;
}

void setup () {
  initDevices ();
  readEEPROM ();
}

void loop () {
  (*modeFunc)[currMode ()] ();
  delay (1000);
}

void initDevices (void) {
   Serial.begin (9600);
}

void readEEPROM (void) {}

void fColorDemo1 (void) {
  static unsigned long millisAtStart;
  
  Serial.println ("Mode: fDemo1");
  
  /* */
  if (modeJustChanged()) {
      millisAtStart = millis ();
  }
  /* */
  
  if (secondsPassed (10)) {
    Serial.println ("Timeout: 10 secondsPassed; Applying mode f1");
    applyMode (&f1);
  }
  else {
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
                                 
