#include "Visualizer.h"
#include "CImg.h"

namespace ePlace {
namespace Visualizer {
using namespace cimg_library;
using Image = cimg_library::CImg<unsigned char>;
int scaleFactor = 100;
void draw(const Circuit &circuit) {
  int size_x, size_y, size_z;
  size_x = circuit.dieSize_x/scaleFactor;
  size_y = circuit.dieSize_y/scaleFactor;
  size_z = 1;
  int numOfColorChannels = 3;  // R G B
  unsigned char initialValue = 0;

  CImg<unsigned char> image(size_x, size_y, size_z, numOfColorChannels, initialValue);
  plotCells(circuit, image);

  // this->plotNets();
  image.save("../Data/outputs/images/save.bmp");

}
void plotCells(const Circuit &circuit, cimg_library::CImg<unsigned char> &image) {
  for (auto &cell : circuit.cell_list) {
    int LL_x = cell.x / scaleFactor;
    int LL_y = cell.y / scaleFactor;
    int UR_x, UR_y;
    if (floor(cell.size_x/scaleFactor) == 0) {
      UR_x = cell.x / scaleFactor + 5;
    } else {
      UR_x = (cell.x + floor(cell.size_x)) / scaleFactor;
    }
    if (floor(cell.size_y/scaleFactor) == 0) {
      UR_y = cell.y / scaleFactor + 5;
    } else {
      UR_y = (cell.y + floor(cell.size_y)) / scaleFactor;
    }

    if (!cell.isFiller) {
      image.draw_rectangle(LL_x, LL_y, UR_x, UR_y, Color::LIGHT_YELLOW);
    } else {
      image.draw_rectangle(LL_x, LL_y, UR_x, UR_y, Color::LIGHT_GREEN);
    }
  }
}
void plotNets(const Circuit &circuit, cimg_library::CImg<unsigned char> &image) {

}
}
}

