#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include "CImg.h"
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

  Visualizer::draw(circuit);
  cout << "test" << endl;


}
