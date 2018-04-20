void f1 (void) {
  Serial.println ("Mode: f1");
}

void f2 (void) {
  Serial.println ("Mode: f2");
}

typedef void (*fPtr[])(void);

class modeChanger {
  public:
    modeChanger (fPtr *funcArray);
  private:
    fPtr *_funcArray;
};

modeChanger::modeChanger (fPtr *funcArray) {
  _funcArray = funcArray;
}

static fPtr modeFuncArray = {f1, f2};
const unsigned int numModes = sizeof(modeFuncArray)/sizeof(modeFuncArray[0]);

modeChanger mode = new modeChanger (modeFuncArray);

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}

/* https://ideone.com
Works like a charm:

#include <iostream>

void f1(void) {
   std::cout << "f1" << "\n";
}

void f2(void) {
   std::cout << "f2" << "\n";
}

int main() {
	typedef void (*fPtr[])(void);
	static fPtr pt = {f1, f2, f2};
	
	(*pt[2])();
	(*pt[1])();
	(*pt[0])();

	return 0;
}
*/

/* Really works thanos to stackowerflow:
#include <iostream>

void f1 (void) {
  std::cout << "f1" << "\n";
}

void f2 (void) {
  std::cout << "f2" << "\n";
}

typedef void (*fPtr)(void); // simple "[]" gets the compile error

class modeChanger {
  public:
    modeChanger (fPtr *funcArray);
    void op ();
  private:
    fPtr *_funcArray;
};

modeChanger::modeChanger (fPtr *funcArray) {
  _funcArray = funcArray;
}

void modeChanger::op () {
  (*_funcArray[0])();
  (*_funcArray[1])(); // Line 27: this line generates a runtime error! Just comment it to get all work
}

void (*modeFuncArray[])(void) = {f1, f2, f2};

modeChanger *mode = new modeChanger (modeFuncArray);

int main() {
    (*modeFuncArray[1])(); // Works fine

    mode->op(); // generates a runtime error
    return 0;
}

*/
