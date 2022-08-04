#include <iostream>
#include "Circuit.h"
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include "CImg.h"

using namespace std;
using namespace ePlace;
using namespace cimg_library;

int main() {
  Circuit circuit;
  string lefName = "../Data/bench/ispd18/ispd18_test1/ispd18_test1.input.lef";
  string defName = "../Data/bench/ispd18/ispd18_test1/ispd18_test1.input.def";

  circuit.parsing(lefName, defName);
  circuit.initialization();
  cout << "test" << endl;


  int size_x = 640;
  int size_y = 480;
  int size_z = 1;
  int numberOfColorChannels = 3; // R G B
  unsigned char initialValue = 0;

  CImg<unsigned char> image(size_x, size_y, size_z, numberOfColorChannels, initialValue);

  image.save("../Data/outputs/images/save.bmp");



}
