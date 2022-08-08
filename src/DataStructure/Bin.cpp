#include "Bin.h"
namespace ePlace {

void Bin::binPlace(pair<float, float> ll) {
  this->LL = ll;
  float ur_x = ll.first + this->size_x;
  float ur_y = ll.second + this->size_y;
  pair<float, float> ur = make_pair(ur_x, ur_y);
  this->UR = ur;
}
float Bin::getOverlapWithCell(const Cell &cell) const {
  // See eq(3) in the paper
  pair<float, float> cell_LL, cell_UR;
  cell_LL = make_pair(cell.x, cell.y);
  cell_UR = make_pair(cell_LL.first + cell.size_x, cell_LL.second + cell.size_y);
  float rectLx = max(this->LL.first, cell_LL.first);
  float rectLy = max(this->LL.second, cell_LL.second);
  float rectUx = min(this->UR.first, cell_UR.first);
  float rectUy = min(this->UR.second, cell_UR.second);

  float overlapWidth = rectUx - rectLx;
  float overlapHeight = rectUy - rectLy;

  if (overlapWidth < 0 || overlapHeight < 0) {
    return 0;
  } else {
    return (rectUx - rectLx) * (rectUy - rectLy);
  }
}
}
