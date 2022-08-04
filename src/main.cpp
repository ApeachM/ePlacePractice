#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "Circuit.h"
#include "Visualizer.h"

using namespace std;
using namespace ePlace;
using namespace cimg_library;

int main() {
  Circuit circuit;
  string lefName = "../Data/bench/ispd18/ispd18_test1/ispd18_test1.input.lef";
  string defName = "../Data/bench/ispd18/ispd18_test1/ispd18_test1.input.def";

  circuit.parsing(lefName, defName);
  circuit.initialization();
  for (int i = 0; i < 30; ++i) {
    circuit.doIteration(i);
  }

  cout << "test" << endl;


}
