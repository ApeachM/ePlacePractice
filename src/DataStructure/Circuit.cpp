///////////////////////////////////////////////////////////////////////////////
// Authors: Minjae Kim (kmj0824@postech.ac.kr) on 2022/07/25.
//          based on Dr. Jingwei Lu with ePlace and ePlace-MS
//
// BSD 3-Clause License
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "Circuit.h"
namespace ePlace {
void Circuit::fftInitialization() {
  fft.init(this->dieSize_x, this->dieSize_y, 64, 64);

  // set this->bins variable
  this->bins.reserve(fft.getBinCnt_x());
  for (int i = 0; i < fft.getBinCnt_x(); ++i) {
    vector<Bin *> bins_col;
    bins_col.reserve(fft.getBinCnt_y());
    for (int j = 0; j < fft.getBinCnt_y(); ++j) {
      bins_col.push_back(&fft.bins.at(i).at(j));
    }
    this->bins.push_back(bins_col);
  }
}
void Circuit::updateDensityInBin() {
  // this part will be runtime hotspot

  float fillerArea = 0, stdArea = 0;
  for (auto &cell : this->cell_list) {
    if (!cell.isFiller) {  // standard cell case
      for (auto &bins_col : this->bins) {
        for (auto theBin : bins_col) {
          theBin->stdArea += theBin->getOverlapWithCell(cell);
        }
      }
    } else if (cell.isFiller) {  // filler case
      for (auto &bins_col : this->bins) {
        for (auto theBin : bins_col) {
          theBin->fillerArea += theBin->getOverlapWithCell(cell);
        }
      }
    }
  }

  for (auto &bins_col : this->bins) {
    for (auto theBin : bins_col) {
      theBin->electricDensity = theBin->fillerArea + theBin->stdArea;
    }
  }
}
}

