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
#include "Visualizer.h"
#include <iostream>
#include <random>
#include <algorithm>
#include <cmath>
#include <cassert>

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

  // reset the density values
  for (auto &bins_col : this->bins) {
    for (auto theBin : bins_col) {
      theBin->stdArea = 0;
      theBin->fillerArea = 0;
    }
  }
  float fillerArea = 0, stdArea = 0;
  for (auto &cell : this->cell_list) {
    if (!cell.isFiller) {  // standard cell case
      for (auto &bins_col : this->bins) {
        for (auto theBin : bins_col) {
          theBin->stdArea += theBin->getOverlapWithCell(cell) / this->densityScale;
        }
      }
    } else if (cell.isFiller) {  // filler case
      for (auto &bins_col : this->bins) {
        for (auto theBin : bins_col) {
          theBin->fillerArea += theBin->getOverlapWithCell(cell) / this->densityScale;
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

void Circuit::parsing(string lefName, string defName) {
  // parse lef
  vector<string> lefStor;
  lefStor.push_back(lefName);
  this->ParseLef(lefStor);

  // parse def
  this->ParseDef(defName);

  // set the die size
  this->dieSize_x = this->defDieArea.xh();
  this->dieSize_y = this->defDieArea.yh();

}

void Circuit::makeCellList() {
  int stdCellNum = this->defComponentStor.size();
  int fillerCellNum = stdCellNum / 2;
  this->cell_list.reserve(stdCellNum + fillerCellNum);
  this->addStdCells();
  this->addFillerCells();

  // Making Cell Dictionary
  for (auto & theCell : this->cell_list) {
    Cell *theCellPtr = &theCell;
    this->cellDictionary[theCellPtr->instName] = theCellPtr;
  }
}

void Circuit::addStdCells() {
  for (int i = 0; i < this->defComponentStor.size(); i++) {

    Cell theCell;
    theCell.x = this->defComponentStor[i].x_;
    theCell.y = this->defComponentStor[i].y_;
    theCell.libName = this->defComponentStor[i].name_;  // library(Macro) name of the cell (ex. "NOR4X4")
    theCell.instName = this->defComponentStor[i].id_;  // instance name (ex. "inst8879")

    //component 종류 파악, size 대입
    for (int j = 0; j < this->lefMacroStor.size(); j++) {
      //cout<<lefMacroStor[i].name_<<endl;
      string libName = lefMacroStor[j].name_;
      if (theCell.libName == libName) {
        theCell.size_x = this->lefMacroStor[j].sizeX_;
        theCell.size_y = this->lefMacroStor[j].sizeY_;
        break;
      }
    }
    this->cell_list.push_back(theCell);
  }
}

void Circuit::addFillerCells() {
  cout << "test add filler" << endl;
  // get the average cell area and average length of width or height
  float averageArea = 0;
  for (const auto &theCell : this->cell_list) {
    averageArea += theCell.size_x * theCell.size_y;
  }
  averageArea /= static_cast<float>(this->cell_list.size());
  float averageWidth = sqrt(averageArea);

/*
 * This below number can be so huge.
  // get the proper number of filler cells
  long long int dieArea = this->dieSize_x;
  dieArea *= this->dieSize_y;  // this form is due to overflow
  long long int totalCellArea = averageArea;
  totalCellArea *= this->cell_list.size();  // this form is due to overflow
  long long int emptyArea = dieArea - totalCellArea;
  if (emptyArea < 0)
    assert(0);
  long long int fillerNum = emptyArea / averageArea;
*/
  int fillerNum = floor(this->cell_list.size() / 2);
  // place filler cells
  // mt19937 gen(1234);  // fix the seed
  mt19937 genX(random_device{}());
  mt19937 genY(random_device{}());
  uniform_real_distribution<float> disX(0, this->dieSize_x);
  uniform_real_distribution<float> disY(0, this->dieSize_y);

  for (int i = 0; i < fillerNum; ++i) {
    Cell theFiller;
    theFiller.x = floor(disX(genX));
    theFiller.y = floor(disY(genY));
    theFiller.libName = "Filler";
    theFiller.instName = "Filler" + to_string(i);

    theFiller.size_x = averageWidth;
    theFiller.size_y = averageWidth;
    theFiller.isFiller = true;

    this->cell_list.push_back(theFiller);
  }
}

void Circuit::makeNetList() {

  int netNumber = this->defNetStor.size();
  string netName, theCellName;
  Cell *theCell = nullptr;

  this->net_list.reserve(netNumber);
  for (int i = 0; i < netNumber; ++i) {
    NET theNet;  // constructor should be called every for loop
    theNet.name = this->defNetStor[i].name();  // name_ variable return
    for (int j = 0; j < this->defNetStor[i].numConnections(); ++j) {
      theCellName = this->defNetStor[i].instance(j);
      theCell = this->cellDictionary[theCellName];
      theNet.connectedCells.push_back(theCell);
    }
    this->net_list.push_back(theNet);
    this->netDictionary[theNet.name] = &this->net_list.back();

  }
  // link cell->net
  for (auto &theNet : this->net_list) {
    for (auto connectedCell : theNet.connectedCells) {
      connectedCell->connected_nets.push_back(&theNet);
    }
  }
}

float Circuit::getHPWL() {
  float hpwl = 0;
  float hpwl_edge = 0;
  float x_point, y_point;
  float max_x, min_x, max_y, min_y;


  //모든 net에 대해 delta_hpwl 계산
  for (int i = 0; i < this->net_list.size(); i++) {
    //initialize
    min_x = this->net_list[i].connectedCells[0]->x;
    max_x = this->net_list[i].connectedCells[0]->x;
    min_y = this->net_list[i].connectedCells[0]->y;
    max_y = this->net_list[i].connectedCells[0]->y;

    for (int j = 0; j < this->net_list[i].connectedCells.size(); j++) {
      x_point = this->net_list[i].connectedCells[j]->x;
      y_point = this->net_list[i].connectedCells[j]->y;

      if (max_x < x_point) {
        max_x = x_point;
      }
      if (min_x > x_point) {
        min_x = x_point;
      }
      if (max_y < y_point) {
        max_y = y_point;
      }
      if (min_y > y_point) {
        min_y = y_point;
      }

      //calculate hpwl_edge&add to hpwl
      hpwl_edge = (max_x - min_x) + (max_y - min_y);
      hpwl = hpwl + hpwl_edge;

    }

  }

  return hpwl;

}

void Circuit::initialization() {
  this->makeCellList();
  this->makeNetList();
  this->initialPlacement(this->initialIteration);
  this->fftInitialization();
  this->cellClassificationIntoBin();
  this->updateDensityInBin();
  this->fft.doFFT();
}

void Circuit::cellClassificationIntoBin() {
  int binIdx_x, binIdx_y;
  float cellCoordinate_x, cellCoordinate_y;
  float binSize_x = this->bins[0][0]->size_x;
  float binSize_y = this->bins[0][0]->size_y;

  // clear the cell pointer bowl in bin
  for (int i = 0; i < this->bins.size(); ++i) {
    for (int j = 0; j < this->bins[0].size(); ++j) {
      this->bins[i][j]->correspondCells.clear();
    }
  }
  for (auto &cell : this->cell_list) {
    cellCoordinate_x = static_cast<float>(cell.x);
    cellCoordinate_y = static_cast<float>(cell.y);
    binIdx_x = min(static_cast<int>(floor(cellCoordinate_x / binSize_x)), static_cast<int>(this->bins.size() - 1));
    binIdx_x = max(binIdx_x, 0);
    binIdx_y = min(static_cast<int>(floor(cellCoordinate_y / binSize_y)), static_cast<int>(this->bins[0].size() - 1));
    binIdx_y = max(binIdx_y, 0);

    this->bins.at(binIdx_x).at(binIdx_y)->correspondCells.push_back(&cell);
  }
}

void Circuit::doIteration(int iterationNum) {
  //모든 bin에 접근
  cout << "Electric Potential Iteration: " << iterationNum << endl;

  for (int i = 0; i < this->bins.size(); i++) {
    for (int j = 0; j < this->bins[i].size(); j++) {
      auto *theBin = this->bins[i][j];
      //bin 안의 cell에 접근
      for (auto *theCell : theBin->correspondCells) {
        // force = bin_electricDensity * cell_area
        float e_Density = theBin->electricDensity;
        float cell_area = (theCell->size_x) * (theCell->size_y);

        // Apply electric force
        theCell->force_x = (theBin->electricField_x) * cell_area;
        theCell->force_y = (theBin->electricField_y) * cell_area;

        // Apply Wire Length Force
        pair<float, float> wireLengthForce = this->getWireLengthForce(*theCell);
        theCell->force_x += wireLengthForce.first;
        theCell->force_y += wireLengthForce.second;

        // apply non-conservative force (friction) for convergence to solution
        // theCell->force_x -= this->frictionCoefficient * theCell->velocity_x;
        // theCell->force_y -= this->frictionCoefficient * theCell->velocity_y;
      }

    }
  }
  this->moveCellCoordinates();
  // cell-bin linking update
  this->cellClassificationIntoBin();
  // update the electric variables in Bins
  this->updateDensityInBin();
  this->fft.doFFT();

  // check the coordinate of cell is inside of Die
  this->checkCellPlace();

  // visualizing
  string filename = "ePlace/img" + to_string(iterationNum) + ".png";
  Visualizer::draw(*this, filename, true);
}

void Circuit::moveCellCoordinates() {
  // TODO: you should determine the cell coordinate by using velocity of cell
  float cellCoordinate_x, cellCoordinate_y;
  float velocity_x, velocity_y;
  float acceleration_x, acceleration_y;
  int margin = 10;

  for (auto &theCell : this->cell_list) {
    const float &s0_x = theCell.x;
    const float &s0_y = theCell.y;
    const float &v0_x = theCell.velocity_x;
    const float &v0_y = theCell.velocity_y;
    const float &a_x = theCell.force_x / theCell.mass;
    const float &a_y = theCell.force_y / theCell.mass;
  
    // v = v0 + at
    theCell.velocity_x = v0_x + a_x * time_step;
    theCell.velocity_y = v0_y + a_y * time_step;

    // s = s0 + (v + v0)/2 * t
    theCell.x = round(s0_x + 0.5 * (theCell.velocity_x + v0_x) * time_step);
    theCell.y = round(s0_y + 0.5 * (theCell.velocity_y + v0_y) * time_step);

    // boundary exceptions
    if (theCell.x > this->dieSize_x) {
      theCell.x = (int)this->dieSize_x;
      theCell.velocity_x = 0;
    } else if (theCell.x < 0) {
      theCell.x = 0;
      theCell.velocity_x = 0;
    }
    if (theCell.y > this->dieSize_y) {
      theCell.y = (int)this->dieSize_y;
      theCell.velocity_y = 0;
    } else if (theCell.y < 0) {
      theCell.y = 0;
      theCell.velocity_y = 0;
    }
  }
}

pair<float, float> Circuit::getWireLengthForce(const Cell &theCell) {
  float forceX{0};
  float forceY{0};
  for (auto *theNet : theCell.connected_nets) {
    for (auto *neighborCell : theNet->connectedCells) {
      forceX += neighborCell->x - theCell.x;
      forceY += neighborCell->y - theCell.y;
    }
  }
  forceX *= this->wireLengthCoefficient;
  forceY *= this->wireLengthCoefficient;
  return make_pair(forceX, forceY);
}

void Circuit::initialPlacement(int InitIterationNum = 20) {
  // place considering only wire length force
  for (int iterationNum = 0; iterationNum < InitIterationNum; ++iterationNum) {
    cout << "Initial placement Iter: " << iterationNum << endl;
    for (auto &theCell : this->cell_list) {
      // Apply Wire Length Force
      pair<float, float> wireLengthForce = this->getWireLengthForce(theCell);
      theCell.force_x = wireLengthForce.first;
      theCell.force_y = wireLengthForce.second;

      // apply non-conservative force (friction) for convergence to solution
      // theCell.force_x -= this->frictionCoefficient * theCell.velocity_x;
      // theCell.force_y -= this->frictionCoefficient * theCell.velocity_y;
    }
    this->moveCellCoordinates();

    // check the coordinate of cell is inside of Die
    this->checkCellPlace();

    // visualizing
    string filename = "initPlace/init_img" + to_string(iterationNum) + ".png";
    cout << "HPWL: " << this->getHPWL() << endl << endl;
    Visualizer::draw(*this, filename, false);
  }
  cout << endl << endl;
}

void Circuit::checkCellPlace() {
  for (const auto &cell : this->cell_list) {
    assert(cell.x >= 0);
    assert(cell.y >= 0);
    assert(cell.x <= this->dieSize_x);
    assert(cell.y <= this->dieSize_y);
  }
}

} // namespace ePlace

