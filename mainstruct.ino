#include "Timer.h"
#define numLEDs 60

int f1 (void) {
  Serial.println ("\nMode: f1");
  return 0;
}

int f2 (void) {
  Serial.println ("Mode: f2");
  return 0;
}

int f3 (void) {
  Serial.println ("Mode: f3");
  return 0;
}

/* */
// begin modeChanger.h
enum class LoopDir {FORWARD, BACK, FORWARD_AND_BACK, BACK_AND_FORWARD};
typedef int (*fPtr)(void); 

class modeChanger {
  public:
    modeChanger (fPtr *funcArray, int numModes);
    void testOp ();
    int getCurrModeNumber (void);
    int nextMode (void);
    int prevMode (void);
    int applyMode (int newMode);
    int applyMode (fPtr newModeFunc);
    int callCurrModeFunc (void);
    bool modeJustChanged (void);
    bool loopThruModeFunc (int nSec=10, int numCycles=1, LoopDir direction = LoopDir::FORWARD);
    
    // moves to next function only when current function returns non-zero:
    bool loopThruModeFunc (int numCycles=1, LoopDir direction = LoopDir::FORWARD); 
  private:
    int _currMode = -1; // -1 is an indication of an error (index out of range; -1 = array not initialized; -2 = function not found; etc);
    int _prevMode = -100;
    int _numModes = -1;
    fPtr *_funcArray = NULL;
    Timer *timer;
};

modeChanger::modeChanger (fPtr *funcArray, int numModes) {
  _funcArray = funcArray;
  _numModes = numModes;
  _currMode = 0;
  timer = new Timer ();
}

bool modeChanger::loopThruModeFunc (int nSec, int numCycles, LoopDir direction) {
    // Each function in _funcArray is called for nSec seconds
    static int _numCycles;
  
    if (!timer->isOn ()) {
        _numCycles = numCycles;
        timer->setInterval ("ms", nSec*1000);
        timer->switchOn ();
        applyMode (0);
        switch (direction) {
          case LoopDir::BACK:
          case LoopDir::BACK_AND_FORWARD:
              prevMode (); // Starting from last function and looping to the first one
        };
    } else if (timer->needToTrigger ()) {
        switch (direction) {
          case LoopDir::FORWARD:
          case LoopDir::FORWARD_AND_BACK: // FORWARD_AND_BACK now is a stub; will be developed later
              nextMode ();
              if (getCurrModeNumber () == 0) { // We've travelled the whole loop of functions!
                  if (--_numCycles == 0) {
                      timer->switchOff ();
                      return true; 
                  }
              }
              break;
          case LoopDir::BACK:
          case LoopDir::BACK_AND_FORWARD: // BACK_AND_FORWARD now is a stub; will be developed later
              prevMode ();
              if (getCurrModeNumber () == _numModes-1) { // We've travelled the whole loop of functions!
                  if (--_numCycles == 0) {
                      timer->switchOff ();
                      return true; 
                  }
              }
              break;
        }
    } 
    callCurrModeFunc ();
    return false;
}

// moves to next function only when current function returns non-zero:
bool modeChanger::loopThruModeFunc (int numCycles, LoopDir direction) {return false;}

void modeChanger::testOp () {
  (*_funcArray[0])();
  (*_funcArray[1])(); 
  (*_funcArray[2])(); 
}

bool modeChanger::modeJustChanged (void) {
    if (_prevMode != _currMode) {
        _prevMode = _currMode;
        return true;
    }
    return false;
}

int modeChanger::getCurrModeNumber (void) { return _currMode; }

int modeChanger::nextMode (void) {
    if (_currMode > -1) { // Negative stands for some error
        if (++_currMode == _numModes) { // Close the circle
            _currMode = 0;
        }
    }    
    Serial.print ("\nTrying to switch to mode ");
    Serial.println (_currMode);

    return _currMode; 
}

int modeChanger::prevMode (void) {
    if (_currMode > -1) { // Negative stands for some error
        if (--_currMode == -1) { // Close the circle backwards
            _currMode = _numModes-1;
        }
    }    
    return _currMode; 
}

int modeChanger::applyMode (int newMode) {
  
    if ((newMode >=0) && (newMode < _numModes)) {
        _currMode = newMode;
    } else {
        _currMode = -10; // out of range error;
    }
    
    return _currMode; 
}

int modeChanger::applyMode (fPtr newModeFunc) {
    _currMode = -2; // negative value is an indication of an error

    for (int i = 0; i < _numModes; i++) {
       // compare two pointers
       //Serial.print("_funcArray[i]: ");
       //Serial.println((unsigned long)_funcArray[i]);
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

int modeChanger::callCurrModeFunc (void) {
    if (_currMode > -1) { // Negative stands for some error
        return (*_funcArray[_currMode]) ();
    }
    return -1; // error
}

// end modeChanger.h

int fColorDemo10sec (void);
int (*modeFuncArray[])(void) = {f1, f2, f3, fColorDemo10sec};
modeChanger *mode = new modeChanger (modeFuncArray, sizeof(modeFuncArray)/sizeof(modeFuncArray[0]));

void setup () {
  initDevices ();
  readEEPROM ();

  Serial.println ("\n\nStarting...");
}

void loop () {
  //mode->applyMode (f2);
  
  mode->callCurrModeFunc();

  mode->nextMode();
  delay (1000);
}

void initDevices (void) {
   Serial.begin (9600);
}

void readEEPROM (void) {}

int fColorDemo10sec (void) {
  static unsigned long millisAtStart;
  
  Serial.println ("Mode: fDemo1");
  
  /* */
  if (mode->modeJustChanged()) {
      millisAtStart = millis ();
  }
  unsigned long deltaT = millis () - millisAtStart;
  int timeStep = 300;
  int direction = 1;
  int wavelen = 6;

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
  return 0;
}
