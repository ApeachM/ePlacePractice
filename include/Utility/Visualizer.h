#ifndef EPLACEPRACTICE_SRC_UTILITY_VISUALIZER_H_
#define EPLACEPRACTICE_SRC_UTILITY_VISUALIZER_H_
#include "Circuit.h"
#include "CImg.h"

namespace ePlace {
namespace Visualizer {
using namespace cimg_library;
using Image = cimg_library::CImg<unsigned char>;
void draw(const Circuit &circuit, const string &filename);
void plotCells(const Circuit &circuit, cimg_library::CImg<unsigned char> &image);
void plotNets(const Circuit &circuit, cimg_library::CImg<unsigned char> &image);

}  // namespace Visualizer
}  // namespace ePlace


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

#endif //EPLACEPRACTICE_SRC_UTILITY_VISUALIZER_H_
