
void f1 (void) {
  Serial.println ("f1");
  
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
  Serial.println ("f2");
}

void f3 (void) {
  Serial.println ("f3");
}

void (*modeFunc[])(void) = {&f1, &f2, &f3);
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
