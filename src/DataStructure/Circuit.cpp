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
#include <iostream>

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
void Circuit::addCellList() {
  //cout<<defComponentStor.size()<<endl;
  //cout<<lefMacroStor.size()<<endl;
//    cout<<this->defComponentStor[0]. <<endl;

  //<LefDefParser::defiComponent>::iterator iter;
  this->cell_list.reserve(this->defComponentStor.size());
  for (int i = 0; i < this->defComponentStor.size(); i++) {

    Cell theCell;
    theCell.x = 0;
    theCell.y = 0;
    theCell.connected_net = 0;

    theCell.x = this->defComponentStor[i].x_;
    theCell.y = this->defComponentStor[i].y_;
    theCell.libName = this->defComponentStor[i].name_;  // library(Macro) name of the cell (ex. "NOR4X4")
    theCell.instName = this->defComponentStor[i].id_;  // instance name (ex. "inst8879")
    //macroname=theCell.name;

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

    theCell.connected_net = this->defComponentStor[i].netsAllocated_;

    this->cell_list.push_back(theCell);
    this->cellDictionary[theCell.instName] = &this->cell_list.back();

  }
}
void Circuit::addNetList() {

  int netNumber = this->defNetStor.size();
  string netName, theCellName;
  Cell *theCell = nullptr;
  for (int i = 0; i < netNumber; ++i) {
    NET theNet;  // constructor should be called every for loop
    theNet.name = this->defNetStor[i].name();  // name_ variable return
    for (int j = 0; j < this->defNetStor[i].numConnections(); ++j) {
      theCellName = this->defNetStor[i].instance(j);
//        theCell = this->cellDictionary.at(theCellName);
      theCell = this->cellDictionary[theCellName];
      theNet.connectedCells.push_back(theCell);
    }
    this->net_list.push_back(theNet);
    this->netDictionary[theNet.name] = &this->net_list.back();

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
  this->addCellList();
  this->addNetList();
  this->fftInitialization();
  // this->initialPlacement();
  // this->addFillers()
  this->cellClassificationIntoBin();

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
    binIdx_x = floor(cellCoordinate_x / binSize_x);
    binIdx_y = floor(cellCoordinate_y / binSize_y);
    this->bins[binIdx_x][binIdx_y]->correspondCells.push_back(&cell);
  }
}
void Circuit::doIteration() {
  //모든 bin에 접근

  for (int i = 0; i < this->bins.size(); i++) {
    for (int j = 0; j < this->bins[i].size(); j++) {
      //bin 안의 cell에 접근
      for (int k = 0; k < this->bins[i][j]->correspondCells.size(); k++) {
        Cell *theCell = this->bins[i][j]->correspondCells[k];

        //mass
        //this->cell_list[cell_num].mass=1;

        //force=bin electricDensity*cell area
        float e_Density = this->bins[i][j]->electricDensity;
        float cell_area = (this->bins[i][j]->correspondCells[k]->size_x) * (this->bins[i][j]->correspondCells[k]->size_y);

        float force_x = (this->bins[i][j]->electricField_x) * cell_area;
        float force_y = (this->bins[i][j]->electricField_y) * cell_area;
        theCell->force_x=force_x;
        theCell->force_y=force_y;

        //velocity
        float time_step = 0.01;
        float acceleration_x = force_x / 1;
        float acceleration_y = force_y / 1;
        theCell->velocity_x = theCell->velocity_x + acceleration_x * time_step;
        theCell->velocity_y = theCell->velocity_y + acceleration_y * time_step;
      }

    }
  }


}

void Circuit::moveCellCoordinate()
{
  // TODO: you should determine the cell coordinate by using velocity of cell
  float cellCoordinate_x, cellCoordinate_y;
  float velocity_x,velocity_y;
  float acceleration_x, acceleration_y;
  float time_step=0.01;

  for (int i = 0; i < this->cell_list.size(); i++)
  {

    cellCoordinate_x=this->cell_list[i].x;
    cellCoordinate_y=this->cell_list[i].y;
    velocity_x=this->cell_list[i].velocity_x;
    velocity_y=this->cell_list[i].velocity_y;
    acceleration_x=this->cell_list[i].force_x;
    acceleration_y=this->cell_list[i].force_y;

    //x+v0*t+1/2*a*t^2
    cellCoordinate_x = cellCoordinate_x + (velocity_x*time_step) + (0.5*acceleration_x*time_step*time_step);
    cellCoordinate_y = cellCoordinate_y + (velocity_y*time_step) + (0.5*acceleration_y*time_step*time_step);

    this->cell_list[i].x=cellCoordinate_x;
    this->cell_list[i].y=cellCoordinate_y;

  }

// cell-bin linking update
  this->cellClassificationIntoBin();
}

}

