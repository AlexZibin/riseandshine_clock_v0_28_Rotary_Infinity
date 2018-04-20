void f1 (void) {
  Serial.println ("Mode: f1");
}

void f2 (void) {
  Serial.println ("Mode: f2");
}

typedef void (*fPtr[])(void);

class modeChanger {
  public:
    modeChanger (fPtr funcArray);
  private:
    *fPtr _funcArray;
};

modeChanger::modeChanger (fPtr funcArray) {
  _funcArray = funcArray;
}

static fPtr modeFuncArray[] {f1, f2};
const unsigned int numModes = sizeof(modeFuncArray)/sizeof(modeFuncArray[0]);

modeChanger mode = new modeChanger (modeFuncArray);

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}