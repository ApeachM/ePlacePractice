#ifndef EPLACEPRACTICE_SRC_UTILITY_VISUALIZER_H_
#define EPLACEPRACTICE_SRC_UTILITY_VISUALIZER_H_
#include "Circuit.h"
#include "CImg.h"

namespace ePlace {
namespace Visualizer {
using namespace cimg_library;
using Image = cimg_library::CImg<unsigned char>;

constexpr int scaleFactor{100};

namespace Color {
// Cell     - LIGHT_YELLOW
// Net      - RED
const unsigned char BLACK[] = {0, 0, 0};
const unsigned char DIM_GRAY[] = {105, 105, 105};
const unsigned char WHITE[] = {255, 255, 255};
const unsigned char BLUE[] = {3, 252, 219};
const unsigned char RED[] = {255, 0, 0};
const unsigned char PINK[] = {255, 51, 255};
const unsigned char LIGHT_YELLOW[] = {255, 236, 196};
const unsigned char LIGHT_GREEN[] = {73, 235, 52};
}  // namespace Color

void draw(const Circuit &circuit, const string &filename, bool fillerOrNot);
void plotCells(const Circuit &circuit, Image &image, bool fillerOrNot);
void plotNets(const Circuit &circuit, Image &image);

}  // namespace Visualizer
}  // namespace ePlace

#endif //EPLACEPRACTICE_SRC_UTILITY_VISUALIZER_H_
