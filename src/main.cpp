#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "Circuit.h"
#include "Visualizer.h"

using namespace std;
using namespace ePlace;
using namespace cimg_library;

int main(int argc, char* argv[]) {
  Circuit circuit;
  int benchNum = 1;
  string benchNum_str = to_string(benchNum);
//  string lefName = "../Data/bench/simple/nangate45.lef";
//  string defName = "../Data/bench/simple/simple01.def";

  string lefName = "../Data/bench/ispd18/ispd18_test" + benchNum_str + "/ispd18_test" + benchNum_str + ".input.lef";
  string defName = "../Data/bench/ispd18/ispd18_test" + benchNum_str + "/ispd18_test" + benchNum_str + ".input.def";


  circuit.parsing(lefName, defName);
  circuit.initialization();

  cout << "Electric potential apply." << endl;
  for (int i = 0; i < 300; ++i) {
    circuit.doIteration(i);
    cout << "HPWL:" << circuit.getHPWL() << endl;
  }

  cout << "Progress End." << endl;


}
