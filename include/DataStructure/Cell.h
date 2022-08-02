#ifndef EPLACEPRACTICE_SRC_DATASTRUCTURE_CELL_H_
#define EPLACEPRACTICE_SRC_DATASTRUCTURE_CELL_H_
#include <vector>
#include <string>

using namespace std;
class Cell {
 public:
  int x, y;
  float size_x, size_y;
  string libName;
  string instName;
  int connected_net;

  bool isFiller = false;
};

class NET {
 public:
  string name;
  vector<Cell*> connectedCells;
};

#endif //EPLACEPRACTICE_SRC_DATASTRUCTURE_CELL_H_
