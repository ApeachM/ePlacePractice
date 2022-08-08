#include "Visualizer.h"
#include "CImg.h"

namespace ePlace {
namespace Visualizer {

void draw(const Circuit &circuit, const string& filename, bool fillerOrNot) {
  int size_x = circuit.dieSize_x / scaleFactor;
  int size_y = circuit.dieSize_y / scaleFactor;
  int size_z = 1;
  int numOfColorChannels = 3;  // R G B
  unsigned char initialValue = 255;

  CImg<unsigned char> image(size_x, size_y, size_z, numOfColorChannels, initialValue);
  plotCells(circuit, image, fillerOrNot);

  // this->plotNets();
  string filePath = "../Data/outputs/images/" + filename;
  const char *filePathArray = filePath.c_str();
  image.save(filePathArray);

}

void plotCells(const Circuit &circuit, cimg_library::CImg<unsigned char> &image, bool fillerOrNot) {
  for (const auto &cell : circuit.cell_list) {
    int LL_x = cell.x / scaleFactor;
    int LL_y = cell.y / scaleFactor;

    int UR_x = LL_x + 5;
    int UR_y = LL_y + 5;

    if (!cell.isFiller) {
      image.draw_rectangle(LL_x, LL_y, UR_x, UR_y, Color::DIM_GRAY);
    } else if (fillerOrNot) {
      image.draw_rectangle(LL_x, LL_y, UR_x, UR_y, Color::RED);
    }
  }
}

void plotNets(const Circuit &circuit, cimg_library::CImg<unsigned char> &image) {

}
}  // namespace Visualizer
}  // namespace ePlace

