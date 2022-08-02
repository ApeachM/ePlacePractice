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

  void parsing(string lefName, string defName) {
    // parse lef
    vector<string> lefStor;
    lefStor.push_back(lefName);
    this->ParseLef(lefStor);

    // parse def
    this->ParseDef(defName);

    // set the die size
    this->dieSize_x = this->defDieArea.xh();
    this->dieSize_y = this->defDieArea.yh();

    // this->addCellList()
    // this->addNetList()
  }

  void addCellList() {
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

  void addNetList() {

    int netNumber = this->defNetStor.size();
    string netName, theCellName;
    Cell* theCell = nullptr;
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

  float getHPWL()
  {
    float hpwl=0;
    float hpwl_edge=0;
    float x_point, y_point;
    float max_x,min_x,max_y,min_y;


    //모든 net에 대해 delta_hpwl 계산
    for(int i=0;i<this->net_list.size();i++)
    {
      //initialize
      min_x=this->net_list[i].connectedCells[0]->x;
      max_x=this->net_list[i].connectedCells[0]->x;
      min_y=this->net_list[i].connectedCells[0]->y;
      max_y=this->net_list[i].connectedCells[0]->y;

      for(int j=0;j<this->net_list[i].connectedCells.size();j++)
      {
        x_point=this->net_list[i].connectedCells[j]->x;
        y_point=this->net_list[i].connectedCells[j]->y;

        if(max_x<x_point)
        {
          max_x=x_point;
        }
        if(min_x>x_point)
        {
          min_x=x_point;
        }
        if(max_y<y_point)
        {
          max_y=y_point;
        }
        if(min_y>y_point)
        {
          min_y=y_point;
        }

        //calculate hpwl_edge&add to hpwl
        hpwl_edge=(max_x-min_x)+(max_y-min_y);
        hpwl=hpwl+hpwl_edge;


      }

    }

    return hpwl;

  }

  void initialPlacement(){

  }

  void fftInitialization();

  void updateDensityInBin();
};

}

#endif //EPLACEPRACTICE_SRC_DATASTRUCTURE_PARSER_CIRCUIT_H_
/* for(int i=0;i<this->cell_list.size();i++)
     {
       //모든 net에 대해 max와 min 값 구한
       max_x=this->cell_list[net_list[i].connectedCells[0]].x;
       max_y=this->cell_list[net_list[i].connectedCells[0]].y;
       min_x=this->cell_list[net_list[i].connectedCells[0]].x;
       min_y=this->cell_list[net_list[i].connectedCells[0]].y;

       for(int j=0;j<this->net_list[i].connectedCells.size();j++)
       {
         cell_for_look=this->net_list[i].connectedCells[j];
         x_point=this->cell_list[cell_for_look].x;
         y_point=this->cell_list[cell_for_look].y;

         //update max and min
         if(max_x<x_point)
         {
           max_x=x_point;
         }
         if(min_x>x_point)
         {
           min_x=x_point;
         }
         if(max_y<y_point)
         {
           max_y=y_point;
         }
         if(min_y>y_point)
         {
           min_y=y_point;
         }

         delta_hpwl=(max_x-min_x)+(max_y-min_y);
         hpwl=hpwl+delta_hpwl;
       }
     }
     return hpwl;*/