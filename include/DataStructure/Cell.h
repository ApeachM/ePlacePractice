#ifndef EPLACEPRACTICE_SRC_DATASTRUCTURE_CELL_H_
#define EPLACEPRACTICE_SRC_DATASTRUCTURE_CELL_H_
#include <vector>
#include <string>

using namespace std;
class Cell {
 public:
  int x, y;
  float size_x, size_y;
  char* name;
  int connected_net;

};

class NET{
 public:
  int connected_cell1;
  int connected_cell2;

};

#endif //EPLACEPRACTICE_SRC_DATASTRUCTURE_CELL_H_
