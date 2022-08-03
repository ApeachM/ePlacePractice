#include <iostream>
#include "Circuit.h"
#include <fstream>
#include <vector>
#include <sstream>
#include <string>

using namespace std;
using namespace ePlace;
int main() {
  Circuit circuit;
  string lefName = "../bench/ispd18/ispd18_test1/ispd18_test1.input.lef";
  string defName = "../bench/ispd18/ispd18_test1/ispd18_test1.input.def";

  circuit.parsing(lefName, defName);
  circuit.initialization();
  cout <<"test" << endl;


}
