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

#ifndef EPLACEPRACTICE_SRC_DATASTRUCTURE_PARSER_CIRCUIT_H_
#define EPLACEPRACTICE_SRC_DATASTRUCTURE_PARSER_CIRCUIT_H_

#include <unordered_map>
#include <Eigen/Core>
#include "Parser.h"
#include "Cell.h"
#include "FFT.h"

namespace ePlace {
class Circuit : public Parser {
 public:
  int dieSize_x = 0, dieSize_y = 0;
  vector<Cell> cell_list;
  vector<NET> net_list;

  unordered_map<string, Cell *> cellDictionary;  // this data type is similar with dictionary in python
  unordered_map<string, NET *> netDictionary;

  FFT fft;
  vector<vector<Bin *>> bins;

  void parsing(string lefName, string defName);

  void addCellList();

  void addNetList();

  float getHPWL();

  void initialPlacement(){

  }
  void initialization();
  void fftInitialization();

  void updateDensityInBin();
  void cellClassificationIntoBin();
};

}

#endif //EPLACEPRACTICE_SRC_DATASTRUCTURE_PARSER_CIRCUIT_H_
