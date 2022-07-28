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
#include "Parser.h"
#include "Cell.h"

namespace ePlace {
class Circuit : public Parser {
 public:
  vector<Cell> cell_list;
  vector<NET> net_list;

  void parsing(string lefName, string defName) {
    // parse lef
    vector<string> lefStor;
    lefStor.push_back(lefName);
    this->ParseLef(lefStor);

    // parse def
    this->ParseDef(defName);
  }

  void addCellList() {
    //cout<<defComponentStor.size()<<endl;
    //cout<<lefMacroStor.size()<<endl;
//    cout<<this->defComponentStor[0]. <<endl;

    //<LefDefParser::defiComponent>::iterator iter;
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

    }
  }

  void addNetList() {

    NET theNet;
    int netnum;

    /*
    for(int i=0;i<this->defComponentStor.size())/2;i++)
    {
      this->net_list.push_back(theNet);

    }

    for(int i=0;i<this->defComponentStor.size();i++)
    {
      netnum=this->defComponentStor[i].

    }*/

  }
};

}

#endif //EPLACEPRACTICE_SRC_DATASTRUCTURE_PARSER_CIRCUIT_H_
