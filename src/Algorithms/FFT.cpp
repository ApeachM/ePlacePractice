#include <cmath>
#include "FFT.h"
#define PI 3.141592653589793238462L

using namespace std;
namespace ePlace {
void FFT::init(int dieSize_x, int dieSize_y, int binCnt_x = 8, int binCnt_y = 8) {

  // set the bin numbers and the bin size
  this->binCnt_x = binCnt_x;
  this->binCnt_y = binCnt_y;
  this->binSize_x = static_cast<float>(dieSize_x) / static_cast<float>(binCnt_x);
  this->binSize_y = static_cast<float>(dieSize_y) / static_cast<float>(binCnt_y);

  // make the bin instances
  this->bins.reserve(this->binCnt_x);
  for (int i = 0; i < this->binCnt_x; ++i) {
    vector<Bin> tmp_bins;
    tmp_bins.reserve(this->binCnt_y);
    for (int j = 0; j < this->binCnt_y; ++j) {
      Bin bin{};
      tmp_bins.push_back(bin);
    }
    this->bins.push_back(tmp_bins);
  }

  // initialize variables in bins
  pair<float, float> curser = make_pair(0, 0);
  for (int i = 0; i < this->binCnt_x; ++i) {
    for (int j = 0; j < this->binCnt_y; ++j) {
      Bin *bin = &(this->bins.at(i).at(j));
      bin->size_x = this->binSize_x;
      bin->size_y = this->binSize_y;
      bin->binPlace(curser);
      curser.second += this->binSize_y;

      bin->electricDensity = 0;
      bin->electricPotential = 0;
      bin->electricField_x = 0;
      bin->electricField_y = 0;
    }
    curser.second = 0;
    curser.first += this->binSize_x;
  }

  // set wx, wx_sq, wy, wy_sq values in fft instances
  // see the explanation above eq (21) in the paper
  this->wx.resize(binCnt_x, 0);
  this->wx_sq.resize(binCnt_x, 0);
  this->wy.resize(binCnt_y, 0);
  this->wy_sq.resize(binCnt_y, 0);

  for (int i = 0; i < binCnt_x; ++i) {
    this->wx[i] = PI * static_cast<float>(i) / static_cast<float>(this->binCnt_x);
    this->wx_sq[i] = this->wx[i] * this->wx[i];
  }
  for (int i = 0; i < binCnt_y; ++i) {
    this->wy[i] = PI * static_cast<float>(i) / static_cast<float>(this->binCnt_y);
    this->wy_sq[i] = this->wy[i] * this->wy[i];
    // Question: Why this part is not same with paper and the OpenRoad code?
    // https://github.com/The-OpenROAD-Project/OpenROAD/blob/5ca26d885644fc725f9884cb5f73aa3aeca7c1ee/src/gpl/src/fft.cpp#L122-L125
  }

  this->cosTable.resize(max(this->binCnt_x, this->binCnt_y) * 3 / 2, 0); // ?
  workArea_.resize(round(sqrt(max(this->binCnt_x, this->binCnt_y))) + 2, 0);

}
void FFT::doFFT() {
  // refers link:
  // https://github.com/The-OpenROAD-Project/OpenROAD/blob/6152e58f84f491089daa6361239468c001e24e34/src/gpl/src/fft.cpp#L150-L214

  // vector to 2d array
  auto binDensity = new float *[this->binCnt_x];
  auto electricPotential = new float *[this->binCnt_x];
  auto electricForceX = new float *[this->binCnt_x];
  auto electricForceY = new float *[this->binCnt_x];

  for (int i = 0; i < this->binCnt_x; i++) {
    binDensity[i] = new float[this->binCnt_y];
    electricPotential[i] = new float[this->binCnt_y];
    electricForceX[i] = new float[this->binCnt_y];
    electricForceY[i] = new float[this->binCnt_y];

    for (int j = 0; j < this->binCnt_y; j++) {
      binDensity[i][j] = this->bins[i][j].electricDensity;
      electricPotential[i][j] = this->bins[i][j].electricPotential;
      electricForceX[i][j] = this->bins[i][j].electricField_x;
      electricForceY[i][j] = this->bins[i][j].electricField_y;
    }
  }

  // get equation (22) using external library
  this->ddct2d(this->binCnt_x,
               this->binCnt_y,
               -1,
               binDensity,
               NULL,
               this->workArea_.data(),
               this->cosTable.data());

  // Question: why we do these things?
  // https://github.com/The-OpenROAD-Project/OpenROAD/blob/6152e58f84f491089daa6361239468c001e24e34/src/gpl/src/fft.cpp#L154-L166
  for (int i = 0; i < this->binCnt_x; ++i) {
    binDensity[i][0] *= 0.5;
  }
  for (int i = 0; i < this->binCnt_y; ++i) {
    binDensity[0][i] *= 0.5;
  }
  for (int i = 0; i < this->binCnt_x; ++i) {
    for (int j = 0; j < this->binCnt_y; ++j) {
      binDensity[i][j] *= 4.0 / this->binCnt_x / this->binCnt_y;
    }
  }

  for (int i = 0; i < this->binCnt_x; i++) {
    float wx = this->wx[i];
    float wx2 = this->wx_sq[i];

    for (int j = 0; j < this->binCnt_y; j++) {
      float wy = this->wy[j];
      float wy2 = this->wy_sq[j];

      float density = binDensity[i][j];
      float phi = 0;
      float electroX = 0, electroY = 0;

      if (i == 0 && j == 0) {
        phi = electroX = electroY = 0.0f;
      } else {
        //////////// lutong
        //  denom =
        //  wx2 / 4.0 +
        //  wy2 / 4.0 ;
        // a_phi = a_den / denom ;
        ////b_phi = 0 ; // -1.0 * b / denom ;
        ////a_ex = 0 ; // b_phi * wx ;
        // a_ex = a_phi * wx / 2.0 ;
        ////a_ey = 0 ; // b_phi * wy ;
        // a_ey = a_phi * wy / 2.0 ;
        ///////////
        phi = density / (wx2 + wy2);
        electroX = phi * wx;
        electroY = phi * wy;
      }
      electricPotential[i][j] = phi;
      electricForceX[i][j] = electroX;
      electricForceY[i][j] = electroY;
    }
  }

  // get equation (23) using external library
  ddct2d(this->binCnt_x,
         this->binCnt_y,
         1,
         electricPotential,
         NULL,
         this->workArea_.data(),
         this->cosTable.data());
  // get equation (24) using external library
  ddsct2d(this->binCnt_x,
          this->binCnt_y,
          1,
          electricForceX,
          NULL,
          this->workArea_.data(),
          this->cosTable.data());
  ddcst2d(this->binCnt_x,
          this->binCnt_y,
          1,
          electricForceY,
          NULL,
          this->workArea_.data(),
          this->cosTable.data());

  // 2d array to vector(class variables)
  for (int i = 0; i < this->binCnt_x; i++) {
    for (int j = 0; j < this->binCnt_y; j++) {
      this->bins[i][j].electricDensity = binDensity[i][j];
      this->bins[i][j].electricPotential = electricPotential[i][j];
      this->bins[i][j].electricField_x = electricForceX[i][j];
      this->bins[i][j].electricField_y = electricForceY[i][j];
    }
    delete binDensity[i];
    delete electricPotential[i];
    delete electricForceX[i];
    delete electricForceY[i];
  }
  delete binDensity;
  delete electricPotential;
  delete electricForceX;
  delete electricForceY;
}

float FFT::getPotential(int x, int y) {
  return this->bins.at(x).at(y).electricPotential;
}

void FFT::updateDensity(int x, int y, float density) {
  this->bins.at(x).at(y).electricDensity = density;
}

pair<float, float> FFT::getElectricForce(int x, int y) {
  return make_pair(
      this->bins.at(x).at(y).electricField_x,
      this->bins.at(x).at(y).electricField_y);
}
int FFT::getBinCnt_x() const {
  return this->binCnt_x;
}
int FFT::getBinCnt_y() const {
  return this->binCnt_y;
}
}


