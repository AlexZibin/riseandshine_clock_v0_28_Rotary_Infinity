
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

/*
// массив указателей на функции:
// typedef ........

class modeChanger {
  public:
    void modeChanger ();
    int currMode (void);
    int nextMode (void);
    int prevMode (void);
    int applyMode (void *newModeFunc(void));
  private:
}*/

void (*modeFuncArray[])(void) = {&f1, &f2, &f3);
const int numModes = sizeof(modeFunc)/sizeof(modeFunc)/sizeof(modeFunc[0]);

unsigned int currMode (void) {
  static int i = 0;
  
  if (++i == numModes) {
    i = 0;
  }
  return i;
}

void setup () {
  initDevices ();
  readEEPROM ();
}

void loop () {
  modeFunc[currMode ()] ();
  delay (1000);
}

void initDevices (void) {
   Serial.begin (9600);
}

void readEEPROM (void) {}

void fColorDemo1 (void) {
  Serial.println ("Mode: fDemo1");
  
  if (secondsPassed (10)) {
    Serial.println ("Applying mode f1");
    applyMode (&f1);
  }
  else {
  }
}

