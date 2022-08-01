#ifndef EPLACEPRACTICE_SRC_ALGORITHMS_FFT_H_
#define EPLACEPRACTICE_SRC_ALGORITHMS_FFT_H_
#include "FFT_calculator.h"
#include "Bin.h"
#include <vector>
namespace ePlace {
using namespace std;

class FFT: private FFT_calculator{
 private:
  // refer the paper:
  // ePlace: Electrostatics based Placement using Fast Fourier Transform and Nesterov’s Method
  // 4.2 chapter

  // Total default bin number: 256
  int binCnt_x;  // horizontally bin number. default: 16
  int binCnt_y;  // vertically bin number. default: 16
  int binSize_x, binSize_y;  // the bean size. default: (Total die width or height) / 16

  vector<vector<Bin>> bins;

  vector<float> wx, wy; // means w_u, w_v each in eq (22)
  vector<float> wx_sq, wy_sq; // means w_u^2, w_v^2 each in eq (23)

  vector<float> cosTable;

 public:
  void init(int binCnt_x, int binCnt_y);
  void doFFT();
  void updateDensity(int x, int y, float density);
  pair<float, float> getElectricForce(int x, int y);
  float getPotential(int x, int y);

};

}

#endif //EPLACEPRACTICE_SRC_ALGORITHMS_FFT_H_
