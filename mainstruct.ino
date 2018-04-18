
void f1 (void) {
  Serial.println ("f1");
}

void f2 (void) {
  Serial.println ("f2");
}

void f3 (void) {
  Serial.println ("f3");
}

unsigned int currMode (void) {
  return 0;
}

void (*modeFunc[])(void) = {&f1, &f2, &f3);
const int numModes = sizeof(modeFunc)/sizeof(modeFunc)/sizeof(modeFunc[0]);

void setup () {
  initDevices ();
  readEEPROM ();
}

void loop () {
  modeFunc[currMode ()] ();
}
