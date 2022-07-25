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


#ifndef EPLACEPRACTICE_CIRCUIT_H
#define EPLACEPRACTICE_CIRCUIT_H

#define CIRCUIT_FPRINTF(fmt, ...)  \
  {                                \
    if(fmt) {                      \
      fprintf(fmt, ##__VA_ARGS__); \
    }                              \
  }


#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstring>
#include <climits>
#include <cfloat>
#include <limits>
#include <queue>
#include <map>
#include <unordered_map>
#include <lefrReader.hpp>
#include <lefwWriter.hpp>
#include <lefiDebug.hpp>
#include <lefiEncryptInt.hpp>
#include <lefiUtil.hpp>

#include <defrReader.hpp>
#include <defiAlias.hpp>


#define HASH_MAP std::unordered_map

namespace ePlace {
using namespace std;

class Parser {
 public:
  Parser();
  /////////////////////////////////////////////////////
  // LEF parsing
  //
  double lefVersion;
  string lefDivider;
  string lefBusBitChar;

  lefiUnits lefUnit;
  double lefManufacturingGrid;
  vector<lefiMacro> lefMacroStor;
  vector<lefiLayer> lefLayerStor;
  vector<lefiVia> lefViaStor;
  vector<lefiSite> lefSiteStor;

  // Macro, via, Layer's unique name -> index of lefXXXStor.
  HASH_MAP<string, int> lefMacroMap;
  HASH_MAP<string, int> lefViaMap;
  HASH_MAP<string, int> lefLayerMap;
  HASH_MAP<string, int> lefSiteMap;

  // this will maps
  // current lefMacroStor's index
  // -> lefiPin, lefiObstruction
  //
  // below index is same with lefMacroStor
  vector<vector<lefiPin>> lefPinStor;  // macroIdx -> pinIdx -> pinObj
  vector<HASH_MAP<string, int>> lefPinMapStor;  // macroIdx -> pinName -> pinIdx
  vector<vector<lefiObstruction>> lefObsStor;  // macroIdx -> obsIdx -> obsObj

  /////////////////////////////////////////////////////
  // DEF parsing
  //
  string defVersion;
  string defDividerChar;
  string defBusBitChar;
  string defDesignName;

  vector<defiProp> defPropStor;

  double defUnit;
  defiBox defDieArea;
  vector<defiRow> defRowStor;
  vector<defiTrack> defTrackStor;
  vector<defiGcellGrid> defGcellGridStor;
  vector<defiVia> defViaStor;

  defiComponentMaskShiftLayer defComponentMaskShiftLayer;
  vector<defiComponent> defComponentStor;
  vector<defiPin> defPinStor;
  vector<defiBlockage> defBlockageStor;
  vector<defiNet> defNetStor;
  vector<defiNet> defSpecialNetStor;

  // Component's unique name -> index of defComponentStor.
  HASH_MAP<string, int> defComponentMap;
  HASH_MAP<string, int> defPinMap;

  // ROW's Y coordinate --> Orient info
  HASH_MAP<int, int> defRowY2OrientMap;

  // this will maps
  // current defComponentStor's index + string pin Name
  // -> defNetStor indexes.
  //
  // below index is same with defComponentStor
  vector<HASH_MAP<string, int>> defComponentPinToNet;
  /////////////////////////////////////////////////////


  /////////////////////////////////////////////////////
  // LEF Writing

  // for Dump Lef
  void DumpLefVersion() const;
  void DumpLefDivider();
  void DumpLefBusBitChar();

  void DumpLefUnit();
  void DumpLefManufacturingGrid();
  void DumpLefLayer();
  void DumpLefSite();
  void DumpLefMacro();

  void DumpLefPin(lefiPin* pin);
  void DumpLefObs(lefiObstruction* obs);

  void DumpLefVia();
  void DumpLefDone();

  /////////////////////////////////////////////////////
  // DEF Writing

  void DumpDefVersion();
  void DumpDefDividerChar();
  void DumpDefBusBitChar();
  void DumpDefDesignName();

  void DumpDefProp();
  void DumpDefUnit() const;

  void DumpDefDieArea();
  void DumpDefRow();
  void DumpDefTrack();
  void DumpDefGcellGrid();
  void DumpDefVia();

  void DumpDefComponentMaskShiftLayer();
  void DumpDefComponent();
  void DumpDefPin();
  void DumpDefSpecialNet();
  void DumpDefNet();
  void DumpDefDone();
  void DumpDefBlockage();

  void DumpDefComponentPinToNet();
  /////////////////////////////////////////////////////

 public:
  // Parsing Function
  int ParseLef(vector< string >& lefStor, bool isVerbose);
  int ParseDef(string filename, bool isVerbose);


};

}

#endif //EPLACEPRACTICE_CIRCUIT_H
