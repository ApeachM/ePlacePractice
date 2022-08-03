#ifndef EPLACEPRACTICE_INCLUDE_DATASTRUCTURE_BIN_H_
#define EPLACEPRACTICE_INCLUDE_DATASTRUCTURE_BIN_H_
#include "Cell.h"

namespace ePlace {
class Bin {
 public:
  float size_x, size_y;
  pair<float, float> LL, UR;  // the bin coordinate. Each means lower left, upper right

  float electricDensity;  // means ρ_DCT in eq (22)
  float electricPotential;  // means phi_DCT in eq (23)
  float electricForce_x;  // means ξ_x_DSCT in eq (24)
  float electricForce_y;  // means ξ_y_DSCT in eq (24)

  float stdArea = 0, fillerArea = 0;

  vector<Cell *> correspondCells;

  void binPlace(pair<float, float> ll);
  float getOverlapWithCell(const Cell &cell) const;
};

}

#endif //EPLACEPRACTICE_INCLUDE_DATASTRUCTURE_BIN_H_
