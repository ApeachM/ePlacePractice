#include "FFT.h"
#define PI 3.141592653589793238462L

using namespace std;
void ePlace::FFT::init(int binCnt_x = 8, int binCnt_y = 8) {
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
      bin->electricForce_x = 0;
      bin->electricForce_y = 0;
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

}
void ePlace::FFT::doFFT() {

}

float ePlace::FFT::getPotential(int x, int y) {
  return this->bins.at(x).at(y).electricPotential;
}

void ePlace::FFT::updateDensity(int x, int y, float density) {
  this->bins.at(x).at(y).electricDensity = density;
}

pair<float, float> ePlace::FFT::getElectricForce(int x, int y) {
  return make_pair(
      this->bins.at(x).at(y).electricForce_x,
      this->bins.at(x).at(y).electricForce_y);
}

