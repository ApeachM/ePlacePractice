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

// *****************************************************************************
// Copyright 2014 - 2017, Cadence Design Systems
//
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8.
//
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
//
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
//
//  $Author$
//  $Revision$
//  $Date$
//  $State:  $
// *****************************************************************************


#include "Parser.h"

using namespace std;


static FILE* fout;
////////////////////////////////////////////////////////////////////////
///////////////////////////// Lef Parser ///////////////////////////////
////////////////////////////////////////////////////////////////////////

static int parse65nm = 0;
static int parseLef58Type = 0;

static double* lefVersionPtr = nullptr;
static string* lefDividerPtr = nullptr;
static string* lefBusBitCharPtr = nullptr;

static lefiUnits* lefUnitPtr = nullptr;
static double* lefManufacturingGridPtr = nullptr;

static vector< lefiLayer >* lefLayerStorPtr = nullptr;
static vector< lefiSite >* lefSiteStorPtr = nullptr;
static vector< lefiMacro >* lefMacroStorPtr = nullptr;
static vector< lefiVia >* lefViaStorPtr = 0;

// cursor of PIN & OBS
static vector< lefiPin >* curPinStorPtr = nullptr;
static HASH_MAP< string, int >* curPinMapPtr = nullptr;
static vector< lefiObstruction >* curObsStorPtr = nullptr;

// PIN & OBS info
static vector< vector< lefiPin > >* lefPinStorPtr = nullptr;
static vector< HASH_MAP< string, int > >* lefPinMapStorPtr = nullptr;
static vector< vector< lefiObstruction > >* lefObsStorPtr = nullptr;

static HASH_MAP< string, int >* lefMacroMapPtr = nullptr;
static HASH_MAP< string, int >* lefViaMapPtr = nullptr;
static HASH_MAP< string, int >* lefLayerMapPtr = nullptr;
static HASH_MAP< string, int >* lefSiteMapPtr = nullptr;

static void dataError() {
  CIRCUIT_FPRINTF(fout, "ERROR: returned user data is not correct!\n");
}

static void checkType(lefrCallbackType_e c) {
  if(c >= 0 && c <= lefrLibraryEndCbkType) {
    // OK
  }
  else {
    CIRCUIT_FPRINTF(fout, "ERROR: callback type is out of bounds!\n")
  }
}

static char* orientStr(int orient) {
  switch(orient) {
    case 0:
      return ((char*)"N");
    case 1:
      return ((char*)"W");
    case 2:
      return ((char*)"S");
    case 3:
      return ((char*)"E");
    case 4:
      return ((char*)"FN");
    case 5:
      return ((char*)"FW");
    case 6:
      return ((char*)"FS");
    case 7:
      return ((char*)"FE");
  }
  return ((char*)"BOGUS");
}

void lefVia(lefiVia* via) {
  int i, j;

  lefrSetCaseSensitivity(1);
  CIRCUIT_FPRINTF(fout, "VIA %s ", via->lefiVia::name());
  if(via->lefiVia::hasDefault()) {
    CIRCUIT_FPRINTF(fout, "DEFAULT")
  }
  else if(via->lefiVia::hasGenerated()) {
    CIRCUIT_FPRINTF(fout, "GENERATED");
  }
  CIRCUIT_FPRINTF(fout, "\n");
  if(via->lefiVia::hasTopOfStack())
  CIRCUIT_FPRINTF(fout, "  TOPOFSTACKONLY\n");
  if(via->lefiVia::hasForeign()) {
    CIRCUIT_FPRINTF(fout, "  FOREIGN %s ", via->lefiVia::foreign());
    if(via->lefiVia::hasForeignPnt()) {
      CIRCUIT_FPRINTF(fout, "( %g %g ) ", via->lefiVia::foreignX(),
                      via->lefiVia::foreignY());
      if(via->lefiVia::hasForeignOrient())
      CIRCUIT_FPRINTF(fout, "%s ", orientStr(via->lefiVia::foreignOrient()));
    }
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  if(via->lefiVia::hasProperties()) {
    CIRCUIT_FPRINTF(fout, "  PROPERTY ");
    for(i = 0; i < via->lefiVia::numProperties(); i++) {
      CIRCUIT_FPRINTF(fout, "%s ", via->lefiVia::propName(i));
      if(via->lefiVia::propIsNumber(i))
      CIRCUIT_FPRINTF(fout, "%g ", via->lefiVia::propNumber(i));
      if(via->lefiVia::propIsString(i))
      CIRCUIT_FPRINTF(fout, "%s ", via->lefiVia::propValue(i));
      /*
         if (i+1 == via->lefiVia::numProperties())  // end of properties
         CIRCUIT_FPRINTF(fout, ";\n");
         else      // just add new line
         CIRCUIT_FPRINTF(fout, "\n");
         */
      switch(via->lefiVia::propType(i)) {
        case 'R':
        CIRCUIT_FPRINTF(fout, "REAL ");
          break;
        case 'I':
        CIRCUIT_FPRINTF(fout, "INTEGER ");
          break;
        case 'S':
        CIRCUIT_FPRINTF(fout, "STRING ");
          break;
        case 'Q':
        CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
          break;
        case 'N':
        CIRCUIT_FPRINTF(fout, "NUMBER ");
          break;
      }
    }
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  if(via->lefiVia::hasResistance())
  CIRCUIT_FPRINTF(fout, "  RESISTANCE %g ;\n", via->lefiVia::resistance());
  if(via->lefiVia::numLayers() > 0) {
    for(i = 0; i < via->lefiVia::numLayers(); i++) {
      CIRCUIT_FPRINTF(fout, "  LAYER %s\n", via->lefiVia::layerName(i));
      for(j = 0; j < via->lefiVia::numRects(i); j++)
        if(via->lefiVia::rectColorMask(i, j)) {
          CIRCUIT_FPRINTF(fout, "    RECT MASK %d ( %f %f ) ( %f %f ) ;\n",
                          via->lefiVia::rectColorMask(i, j),
                          via->lefiVia::xl(i, j), via->lefiVia::yl(i, j),
                          via->lefiVia::xh(i, j), via->lefiVia::yh(i, j));
        }
        else {
          CIRCUIT_FPRINTF(fout, "    RECT ( %f %f ) ( %f %f ) ;\n",
                          via->lefiVia::xl(i, j), via->lefiVia::yl(i, j),
                          via->lefiVia::xh(i, j), via->lefiVia::yh(i, j));
        }
      for(j = 0; j < via->lefiVia::numPolygons(i); j++) {
        struct lefiGeomPolygon poly;
        poly = via->lefiVia::getPolygon(i, j);
        if(via->lefiVia::polyColorMask(i, j)) {
          CIRCUIT_FPRINTF(fout, "    POLYGON MASK %d",
                          via->lefiVia::polyColorMask(i, j));
        }
        else {
          CIRCUIT_FPRINTF(fout, "    POLYGON ");
        }
        for(int k = 0; k < poly.numPoints; k++)
        CIRCUIT_FPRINTF(fout, " %g %g ", poly.x[k], poly.y[k]);
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
  }
  if(via->lefiVia::hasViaRule()) {
    CIRCUIT_FPRINTF(fout, "  VIARULE %s ;\n", via->lefiVia::viaRuleName());
    CIRCUIT_FPRINTF(fout, "    CUTSIZE %g %g ;\n", via->lefiVia::xCutSize(),
                    via->lefiVia::yCutSize());
    CIRCUIT_FPRINTF(fout, "    LAYERS %s %s %s ;\n",
                    via->lefiVia::botMetalLayer(), via->lefiVia::cutLayer(),
                    via->lefiVia::topMetalLayer());
    CIRCUIT_FPRINTF(fout, "    CUTSPACING %g %g ;\n",
                    via->lefiVia::xCutSpacing(), via->lefiVia::yCutSpacing());
    CIRCUIT_FPRINTF(fout, "    ENCLOSURE %g %g %g %g ;\n",
                    via->lefiVia::xBotEnc(), via->lefiVia::yBotEnc(),
                    via->lefiVia::xTopEnc(), via->lefiVia::yTopEnc());
    if(via->lefiVia::hasRowCol())
    CIRCUIT_FPRINTF(fout, "    ROWCOL %d %d ;\n", via->lefiVia::numCutRows(),
                    via->lefiVia::numCutCols());
    if(via->lefiVia::hasOrigin())
    CIRCUIT_FPRINTF(fout, "    ORIGIN %g %g ;\n", via->lefiVia::xOffset(),
                    via->lefiVia::yOffset());
    if(via->lefiVia::hasOffset())
    CIRCUIT_FPRINTF(fout, "    OFFSET %g %g %g %g ;\n",
                    via->lefiVia::xBotOffset(), via->lefiVia::yBotOffset(),
                    via->lefiVia::xTopOffset(), via->lefiVia::yTopOffset());
    if(via->lefiVia::hasCutPattern())
    CIRCUIT_FPRINTF(fout, "    PATTERN %s ;\n", via->lefiVia::cutPattern());
  }
  CIRCUIT_FPRINTF(fout, "END %s\n\n", via->lefiVia::name());

  return;
}

void lefSpacing(lefiSpacing* spacing) {
  CIRCUIT_FPRINTF(fout, "  SAMENET %s %s %g ", spacing->lefiSpacing::name1(),
                  spacing->lefiSpacing::name2(),
                  spacing->lefiSpacing::distance());
  if(spacing->lefiSpacing::hasStack())
  CIRCUIT_FPRINTF(fout, "STACK ");
  CIRCUIT_FPRINTF(fout, ";\n");
  return;
}

void lefViaRuleLayer(lefiViaRuleLayer* vLayer) {
  CIRCUIT_FPRINTF(fout, "  LAYER %s ;\n", vLayer->lefiViaRuleLayer::name());
  if(vLayer->lefiViaRuleLayer::hasDirection()) {
    if(vLayer->lefiViaRuleLayer::isHorizontal())
    CIRCUIT_FPRINTF(fout, "    DIRECTION HORIZONTAL ;\n");
    if(vLayer->lefiViaRuleLayer::isVertical())
    CIRCUIT_FPRINTF(fout, "    DIRECTION VERTICAL ;\n");
  }
  if(vLayer->lefiViaRuleLayer::hasEnclosure()) {
    CIRCUIT_FPRINTF(fout, "    ENCLOSURE %g %g ;\n",
                    vLayer->lefiViaRuleLayer::enclosureOverhang1(),
                    vLayer->lefiViaRuleLayer::enclosureOverhang2());
  }
  if(vLayer->lefiViaRuleLayer::hasWidth())
  CIRCUIT_FPRINTF(fout, "    WIDTH %g TO %g ;\n",
                  vLayer->lefiViaRuleLayer::widthMin(),
                  vLayer->lefiViaRuleLayer::widthMax());
  if(vLayer->lefiViaRuleLayer::hasResistance())
  CIRCUIT_FPRINTF(fout, "    RESISTANCE %g ;\n",
                  vLayer->lefiViaRuleLayer::resistance());
  if(vLayer->lefiViaRuleLayer::hasOverhang())
  CIRCUIT_FPRINTF(fout, "    OVERHANG %g ;\n",
                  vLayer->lefiViaRuleLayer::overhang());
  if(vLayer->lefiViaRuleLayer::hasMetalOverhang())
  CIRCUIT_FPRINTF(fout, "    METALOVERHANG %g ;\n",
                  vLayer->lefiViaRuleLayer::metalOverhang());
  if(vLayer->lefiViaRuleLayer::hasSpacing())
  CIRCUIT_FPRINTF(fout, "    SPACING %g BY %g ;\n",
                  vLayer->lefiViaRuleLayer::spacingStepX(),
                  vLayer->lefiViaRuleLayer::spacingStepY());
  if(vLayer->lefiViaRuleLayer::hasRect())
  CIRCUIT_FPRINTF(
      fout, "    RECT ( %f %f ) ( %f %f ) ;\n",
      vLayer->lefiViaRuleLayer::xl(), vLayer->lefiViaRuleLayer::yl(),
      vLayer->lefiViaRuleLayer::xh(), vLayer->lefiViaRuleLayer::yh());
  return;
}

void prtGeometry(lefiGeometries* geometry) {
  int numItems = geometry->lefiGeometries::numItems();
  int i, j;
  lefiGeomPath* path;
  lefiGeomPathIter* pathIter;
  lefiGeomRect* rect;
  lefiGeomRectIter* rectIter;
  lefiGeomPolygon* polygon;
  lefiGeomPolygonIter* polygonIter;
  lefiGeomVia* via;
  lefiGeomViaIter* viaIter;

  for(i = 0; i < numItems; i++) {
    switch(geometry->lefiGeometries::itemType(i)) {
      case lefiGeomClassE:
      CIRCUIT_FPRINTF(fout, "CLASS %s ",
                      geometry->lefiGeometries::getClass(i));
        break;
      case lefiGeomLayerE:
      CIRCUIT_FPRINTF(fout, "      LAYER %s ;\n",
                      geometry->lefiGeometries::getLayer(i));
        break;
      case lefiGeomLayerExceptPgNetE:
      CIRCUIT_FPRINTF(fout, "      EXCEPTPGNET ;\n");
        break;
      case lefiGeomLayerMinSpacingE:
      CIRCUIT_FPRINTF(fout, "      SPACING %g ;\n",
                      geometry->lefiGeometries::getLayerMinSpacing(i));
        break;
      case lefiGeomLayerRuleWidthE:
      CIRCUIT_FPRINTF(fout, "      DESIGNRULEWIDTH %g ;\n",
                      geometry->lefiGeometries::getLayerRuleWidth(i));
        break;
      case lefiGeomWidthE:
      CIRCUIT_FPRINTF(fout, "      WIDTH %g ;\n",
                      geometry->lefiGeometries::getWidth(i));
        break;
      case lefiGeomPathE:
        path = geometry->lefiGeometries::getPath(i);
        if(path->colorMask != 0) {
          CIRCUIT_FPRINTF(fout, "      PATH MASK %d ", path->colorMask);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      PATH ");
        }
        for(j = 0; j < path->numPoints; j++) {
          if(j + 1 == path->numPoints) {  // last one on the list
            CIRCUIT_FPRINTF(fout, "      ( %g %g ) ;\n", path->x[j],
                            path->y[j]);
          }
          else {
            CIRCUIT_FPRINTF(fout, "      ( %g %g )\n", path->x[j], path->y[j]);
          }
        }
        break;
      case lefiGeomPathIterE:
        pathIter = geometry->lefiGeometries::getPathIter(i);
        if(pathIter->colorMask != 0) {
          CIRCUIT_FPRINTF(fout, "      PATH MASK %d ITERATED ",
                          pathIter->colorMask);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      PATH ITERATED ");
        }
        for(j = 0; j < pathIter->numPoints; j++)
        CIRCUIT_FPRINTF(fout, "      ( %g %g )\n", pathIter->x[j],
                        pathIter->y[j]);
        CIRCUIT_FPRINTF(fout, "      DO %g BY %g STEP %g %g ;\n",
                        pathIter->xStart, pathIter->yStart, pathIter->xStep,
                        pathIter->yStep);
        break;
      case lefiGeomRectE:
        rect = geometry->lefiGeometries::getRect(i);
        if(rect->colorMask != 0) {
          CIRCUIT_FPRINTF(fout, "      RECT MASK %d ( %f %f ) ( %f %f ) ;\n",
                          rect->colorMask, rect->xl, rect->yl, rect->xh,
                          rect->yh);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      RECT ( %f %f ) ( %f %f ) ;\n", rect->xl,
                          rect->yl, rect->xh, rect->yh);
        }
        break;
      case lefiGeomRectIterE:
        rectIter = geometry->lefiGeometries::getRectIter(i);
        if(rectIter->colorMask != 0) {
          CIRCUIT_FPRINTF(fout,
                          "      RECT MASK %d ITERATE ( %f %f ) ( %f %f )\n",
                          rectIter->colorMask, rectIter->xl, rectIter->yl,
                          rectIter->xh, rectIter->yh);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      RECT ITERATE ( %f %f ) ( %f %f )\n",
                          rectIter->xl, rectIter->yl, rectIter->xh,
                          rectIter->yh);
        }
        CIRCUIT_FPRINTF(fout, "      DO %g BY %g STEP %g %g ;\n",
                        rectIter->xStart, rectIter->yStart, rectIter->xStep,
                        rectIter->yStep);
        break;
      case lefiGeomPolygonE:
        polygon = geometry->lefiGeometries::getPolygon(i);
        if(polygon->colorMask != 0) {
          CIRCUIT_FPRINTF(fout, "      POLYGON MASK %d ", polygon->colorMask);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      POLYGON ");
        }
        for(j = 0; j < polygon->numPoints; j++) {
          if(j + 1 == polygon->numPoints) {  // last one on the list
            CIRCUIT_FPRINTF(fout, "      ( %g %g ) ;\n", polygon->x[j],
                            polygon->y[j]);
          }
          else {
            CIRCUIT_FPRINTF(fout, "      ( %g %g )\n", polygon->x[j],
                            polygon->y[j]);
          }
        }
        break;
      case lefiGeomPolygonIterE:
        polygonIter = geometry->lefiGeometries::getPolygonIter(i);
        if(polygonIter->colorMask != 0) {
          CIRCUIT_FPRINTF(fout, "       POLYGON MASK %d ITERATE ",
                          polygonIter->colorMask);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      POLYGON ITERATE");
        }
        for(j = 0; j < polygonIter->numPoints; j++)
        CIRCUIT_FPRINTF(fout, "      ( %g %g )\n", polygonIter->x[j],
                        polygonIter->y[j]);
        CIRCUIT_FPRINTF(fout, "      DO %g BY %g STEP %g %g ;\n",
                        polygonIter->xStart, polygonIter->yStart,
                        polygonIter->xStep, polygonIter->yStep);
        break;
      case lefiGeomViaE:
        via = geometry->lefiGeometries::getVia(i);
        if(via->topMaskNum != 0 || via->bottomMaskNum != 0 ||
            via->cutMaskNum != 0) {
          CIRCUIT_FPRINTF(fout, "      VIA MASK %d%d%d ( %g %g ) %s ;\n",
                          via->topMaskNum, via->cutMaskNum, via->bottomMaskNum,
                          via->x, via->y, via->name);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      VIA ( %g %g ) %s ;\n", via->x, via->y,
                          via->name);
        }
        break;
      case lefiGeomViaIterE:
        viaIter = geometry->lefiGeometries::getViaIter(i);
        if(viaIter->topMaskNum != 0 || viaIter->cutMaskNum != 0 ||
            viaIter->bottomMaskNum != 0) {
          CIRCUIT_FPRINTF(fout, "      VIA ITERATE MASK %d%d%d ( %g %g ) %s\n",
                          viaIter->topMaskNum, viaIter->cutMaskNum,
                          viaIter->bottomMaskNum, viaIter->x, viaIter->y,
                          viaIter->name);
        }
        else {
          CIRCUIT_FPRINTF(fout, "      VIA ITERATE ( %g %g ) %s\n", viaIter->x,
                          viaIter->y, viaIter->name);
        }
        CIRCUIT_FPRINTF(fout, "      DO %g BY %g STEP %g %g ;\n",
                        viaIter->xStart, viaIter->yStart, viaIter->xStep,
                        viaIter->yStep);
        break;
      default:
      CIRCUIT_FPRINTF(fout, "BOGUS geometries type.\n");
        break;
    }
  }
}

int antennaCB(lefrCallbackType_e c, double value, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  switch(c) {
    case lefrAntennaInputCbkType:
    CIRCUIT_FPRINTF(fout, "ANTENNAINPUTGATEAREA %g ;\n", value);
      break;
    case lefrAntennaInoutCbkType:
    CIRCUIT_FPRINTF(fout, "ANTENNAINOUTDIFFAREA %g ;\n", value);
      break;
    case lefrAntennaOutputCbkType:
    CIRCUIT_FPRINTF(fout, "ANTENNAOUTPUTDIFFAREA %g ;\n", value);
      break;
    case lefrInputAntennaCbkType:
    CIRCUIT_FPRINTF(fout, "INPUTPINANTENNASIZE %g ;\n", value);
      break;
    case lefrOutputAntennaCbkType:
    CIRCUIT_FPRINTF(fout, "OUTPUTPINANTENNASIZE %g ;\n", value);
      break;
    case lefrInoutAntennaCbkType:
    CIRCUIT_FPRINTF(fout, "INOUTPINANTENNASIZE %g ;\n", value);
      break;
    default:
    CIRCUIT_FPRINTF(fout, "BOGUS antenna type.\n");
      break;
  }
  return 0;
}

int arrayBeginCB(lefrCallbackType_e c, const char* name, lefiUserData) {
  int status;

  checkType(c);
  // if ((long)ud != userData) dataError();
  // use the lef writer to write the data out
  status = lefwStartArray(name);
  if(status != LEFW_OK)
    return status;
  return 0;
}

int arrayCB(lefrCallbackType_e c, lefiArray* a, lefiUserData) {
  int status, i, j, defCaps;
  lefiSitePattern* pattern;
  lefiTrackPattern* track;
  lefiGcellPattern* gcell;

  checkType(c);
  // if ((long)ud != userData) dataError();

  if(a->lefiArray::numSitePattern() > 0) {
    for(i = 0; i < a->lefiArray::numSitePattern(); i++) {
      pattern = a->lefiArray::sitePattern(i);
      status = lefwArraySite(
          pattern->lefiSitePattern::name(), pattern->lefiSitePattern::x(),
          pattern->lefiSitePattern::y(), pattern->lefiSitePattern::orient(),
          pattern->lefiSitePattern::xStart(),
          pattern->lefiSitePattern::yStart(), pattern->lefiSitePattern::xStep(),
          pattern->lefiSitePattern::yStep());
      if(status != LEFW_OK)
        dataError();
    }
  }
  if(a->lefiArray::numCanPlace() > 0) {
    for(i = 0; i < a->lefiArray::numCanPlace(); i++) {
      pattern = a->lefiArray::canPlace(i);
      status = lefwArrayCanplace(
          pattern->lefiSitePattern::name(), pattern->lefiSitePattern::x(),
          pattern->lefiSitePattern::y(), pattern->lefiSitePattern::orient(),
          pattern->lefiSitePattern::xStart(),
          pattern->lefiSitePattern::yStart(), pattern->lefiSitePattern::xStep(),
          pattern->lefiSitePattern::yStep());
      if(status != LEFW_OK)
        dataError();
    }
  }
  if(a->lefiArray::numCannotOccupy() > 0) {
    for(i = 0; i < a->lefiArray::numCannotOccupy(); i++) {
      pattern = a->lefiArray::cannotOccupy(i);
      status = lefwArrayCannotoccupy(
          pattern->lefiSitePattern::name(), pattern->lefiSitePattern::x(),
          pattern->lefiSitePattern::y(), pattern->lefiSitePattern::orient(),
          pattern->lefiSitePattern::xStart(),
          pattern->lefiSitePattern::yStart(), pattern->lefiSitePattern::xStep(),
          pattern->lefiSitePattern::yStep());
      if(status != LEFW_OK)
        dataError();
    }
  }

  if(a->lefiArray::numTrack() > 0) {
    for(i = 0; i < a->lefiArray::numTrack(); i++) {
      track = a->lefiArray::track(i);
      CIRCUIT_FPRINTF(fout, "  TRACKS %s, %g DO %d STEP %g\n",
                      track->lefiTrackPattern::name(),
                      track->lefiTrackPattern::start(),
                      track->lefiTrackPattern::numTracks(),
                      track->lefiTrackPattern::space());
      if(track->lefiTrackPattern::numLayers() > 0) {
        CIRCUIT_FPRINTF(fout, "  LAYER ");
        for(j = 0; j < track->lefiTrackPattern::numLayers(); j++)
        CIRCUIT_FPRINTF(fout, "%s ", track->lefiTrackPattern::layerName(j));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
  }

  if(a->lefiArray::numGcell() > 0) {
    for(i = 0; i < a->lefiArray::numGcell(); i++) {
      gcell = a->lefiArray::gcell(i);
      CIRCUIT_FPRINTF(
          fout, "  GCELLGRID %s, %g DO %d STEP %g\n",
          gcell->lefiGcellPattern::name(), gcell->lefiGcellPattern::start(),
          gcell->lefiGcellPattern::numCRs(), gcell->lefiGcellPattern::space());
    }
  }

  if(a->lefiArray::numFloorPlans() > 0) {
    for(i = 0; i < a->lefiArray::numFloorPlans(); i++) {
      status = lefwStartArrayFloorplan(a->lefiArray::floorPlanName(i));
      if(status != LEFW_OK)
        dataError();
      for(j = 0; j < a->lefiArray::numSites(i); j++) {
        pattern = a->lefiArray::site(i, j);
        status = lefwArrayFloorplan(
            a->lefiArray::siteType(i, j), pattern->lefiSitePattern::name(),
            pattern->lefiSitePattern::x(), pattern->lefiSitePattern::y(),
            pattern->lefiSitePattern::orient(),
            (int)pattern->lefiSitePattern::xStart(),
            (int)pattern->lefiSitePattern::yStart(),
            pattern->lefiSitePattern::xStep(),
            pattern->lefiSitePattern::yStep());
        if(status != LEFW_OK)
          dataError();
      }
      status = lefwEndArrayFloorplan(a->lefiArray::floorPlanName(i));
      if(status != LEFW_OK)
        dataError();
    }
  }

  defCaps = a->lefiArray::numDefaultCaps();
  if(defCaps > 0) {
    status = lefwStartArrayDefaultCap(defCaps);
    if(status != LEFW_OK)
      dataError();
    for(i = 0; i < defCaps; i++) {
      status = lefwArrayDefaultCap(a->lefiArray::defaultCapMinPins(i),
                                   a->lefiArray::defaultCap(i));
      if(status != LEFW_OK)
        dataError();
    }
    status = lefwEndArrayDefaultCap();
    if(status != LEFW_OK)
      dataError();
  }
  return 0;
}

int arrayEndCB(lefrCallbackType_e c, const char* name, lefiUserData) {
  int status;

  checkType(c);
  // if ((long)ud != userData) dataError();
  // use the lef writer to write the data out
  status = lefwEndArray(name);
  if(status != LEFW_OK)
    return status;
  return 0;
}

int busBitCharsCB(lefrCallbackType_e c, const char* busBit, lefiUserData) {
  checkType(c);

  // use the lef writer to write out the data
  //    int status = lefwBusBitChars(busBit);
  //    if (status != LEFW_OK)
  //        dataError();

  (*lefBusBitCharPtr) = string(busBit);
  CIRCUIT_FPRINTF(fout, "BUSBITCHARS \"%s\" ;\n", busBit);
  return 0;
}

int caseSensCB(lefrCallbackType_e c, int caseSense, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  if(caseSense == TRUE) {
    CIRCUIT_FPRINTF(fout, "NAMESCASESENSITIVE ON ;\n");
  }
  else {
    CIRCUIT_FPRINTF(fout, "NAMESCASESENSITIVE OFF ;\n");
  }
  return 0;
}

int fixedMaskCB(lefrCallbackType_e c, int fixedMask, lefiUserData) {
  checkType(c);

  if(fixedMask == 1)
  CIRCUIT_FPRINTF(fout, "FIXEDMASK ;\n");
  return 0;
}

int clearanceCB(lefrCallbackType_e c, const char* name, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  CIRCUIT_FPRINTF(fout, "CLEARANCEMEASURE %s ;\n", name);
  return 0;
}

int dividerCB(lefrCallbackType_e c, const char* name, lefiUserData) {
  checkType(c);
  (*lefDividerPtr) = string(name);

  CIRCUIT_FPRINTF(fout, "DIVIDERCHAR \"%s\" ;\n", name);
  return 0;
}

int noWireExtCB(lefrCallbackType_e c, const char* name, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  CIRCUIT_FPRINTF(fout, "NOWIREEXTENSION %s ;\n", name);
  return 0;
}

int noiseMarCB(lefrCallbackType_e c, lefiNoiseMargin*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  return 0;
}

int edge1CB(lefrCallbackType_e c, double name, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  CIRCUIT_FPRINTF(fout, "EDGERATETHRESHOLD1 %g ;\n", name);
  return 0;
}

int edge2CB(lefrCallbackType_e c, double name, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  CIRCUIT_FPRINTF(fout, "EDGERATETHRESHOLD2 %g ;\n", name);
  return 0;
}

int edgeScaleCB(lefrCallbackType_e c, double name, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  CIRCUIT_FPRINTF(fout, "EDGERATESCALEFACTORE %g ;\n", name);
  return 0;
}

int noiseTableCB(lefrCallbackType_e c, lefiNoiseTable*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  return 0;
}

int correctionCB(lefrCallbackType_e c, lefiCorrectionTable*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  return 0;
}

int dielectricCB(lefrCallbackType_e c, double dielectric, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  CIRCUIT_FPRINTF(fout, "DIELECTRIC %g ;\n", dielectric);
  return 0;
}

int irdropBeginCB(lefrCallbackType_e c, void*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "IRDROP\n");
  return 0;
}

int irdropCB(lefrCallbackType_e c, lefiIRDrop* irdrop, lefiUserData) {
  int i;
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "  TABLE %s ", irdrop->lefiIRDrop::name());
  for(i = 0; i < irdrop->lefiIRDrop::numValues(); i++)
  CIRCUIT_FPRINTF(fout, "%g %g ", irdrop->lefiIRDrop::value1(i),
                  irdrop->lefiIRDrop::value2(i));
  CIRCUIT_FPRINTF(fout, ";\n");
  return 0;
}

int irdropEndCB(lefrCallbackType_e c, void*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "END IRDROP\n");
  return 0;
}

int layerCB(lefrCallbackType_e c, lefiLayer* layer, lefiUserData) {
  //    int i, j, k;
  //    int numPoints, propNum;
  //    double *widths, *current;
  //    lefiLayerDensity* density;
  //    lefiAntennaPWL* pwl;
  //    lefiSpacingTable* spTable;
  //    lefiInfluence* influence;
  //    lefiParallel* parallel;
  //    lefiTwoWidths* twoWidths;
  //    char pType;
  //    int numMinCut, numMinenclosed;
  //    lefiAntennaModel* aModel;
  //    lefiOrthogonal*   ortho;

  checkType(c);
  // if ((long)ud != userData) dataError();

  lefrSetCaseSensitivity(0);

  // Call parse65nmRules for 5.7 syntax in 5.6
  if(parse65nm)
    layer->lefiLayer::parse65nmRules();

  // Call parseLef58Type for 5.8 syntax in 5.7
  if(parseLef58Type)
    layer->lefiLayer::parseLEF58Layer();

  // layerName -> lefLayerStor's index.
  (*lefLayerMapPtr)[string(layer->name())] = lefLayerStorPtr->size();
  lefLayerStorPtr->push_back(*layer);

  // Set it to case sensitive from here on
  lefrSetCaseSensitivity(1);

  return 0;
}

int macroBeginCB(lefrCallbackType_e c, const char* macroName, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  (*lefMacroMapPtr)[string(macroName)] = lefMacroStorPtr->size();

  curPinStorPtr = new vector< lefiPin >;
  curObsStorPtr = new vector< lefiObstruction >;
  curPinMapPtr = new HASH_MAP< string, int >;
#ifdef USE_GOOGLE_HASH
  curPinMapPtr->set_empty_key(INIT_STR);
#endif

  CIRCUIT_FPRINTF(fout, "MACRO %s\n", macroName);
  return 0;
}

int macroFixedMaskCB(lefrCallbackType_e c, int, lefiUserData) {
  checkType(c);

  return 0;
}

int macroClassTypeCB(lefrCallbackType_e c, const char* macroClassType,
                     lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "MACRO CLASS %s\n", macroClassType);
  return 0;
}

int macroOriginCB(lefrCallbackType_e c, lefiNum, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  // CIRCUIT_FPRINTF(fout, "  ORIGIN ( %g %g ) ;\n", macroNum.x, macroNum.y);
  return 0;
}

int macroSizeCB(lefrCallbackType_e c, lefiNum, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  // CIRCUIT_FPRINTF(fout, "  SIZE %g BY %g ;\n", macroNum.x, macroNum.y);
  return 0;
}

int macroCB(lefrCallbackType_e c, lefiMacro* macro, lefiUserData) {
  lefiSitePattern* pattern;
  int propNum, i, hasPrtSym = 0;

  lefMacroStorPtr->push_back(*macro);
  checkType(c);
  // if ((long)ud != userData) dataError();
  if(macro->lefiMacro::hasClass())
  CIRCUIT_FPRINTF(fout, "  CLASS %s ;\n", macro->lefiMacro::macroClass());
  if(macro->lefiMacro::isFixedMask())
  CIRCUIT_FPRINTF(fout, "  FIXEDMASK ;\n");
  if(macro->lefiMacro::hasEEQ())
  CIRCUIT_FPRINTF(fout, "  EEQ %s ;\n", macro->lefiMacro::EEQ());
  if(macro->lefiMacro::hasLEQ())
  CIRCUIT_FPRINTF(fout, "  LEQ %s ;\n", macro->lefiMacro::LEQ());
  if(macro->lefiMacro::hasSource())
  CIRCUIT_FPRINTF(fout, "  SOURCE %s ;\n", macro->lefiMacro::source());
  if(macro->lefiMacro::hasXSymmetry()) {
    CIRCUIT_FPRINTF(fout, "  SYMMETRY X ");
    hasPrtSym = 1;
  }
  if(macro->lefiMacro::hasYSymmetry()) {  // print X Y & R90 in one line
    if(!hasPrtSym) {
      CIRCUIT_FPRINTF(fout, "  SYMMETRY Y ");
      hasPrtSym = 1;
    }
    else
    CIRCUIT_FPRINTF(fout, "Y ");
  }
  if(macro->lefiMacro::has90Symmetry()) {
    if(!hasPrtSym) {
      CIRCUIT_FPRINTF(fout, "  SYMMETRY R90 ");
      hasPrtSym = 1;
    }
    else
    CIRCUIT_FPRINTF(fout, "R90 ");
  }
  if(hasPrtSym) {
    CIRCUIT_FPRINTF(fout, ";\n");
    hasPrtSym = 0;
  }
  if(macro->lefiMacro::hasSiteName())
  CIRCUIT_FPRINTF(fout, "  SITE %s ;\n", macro->lefiMacro::siteName());
  if(macro->lefiMacro::hasSitePattern()) {
    for(i = 0; i < macro->lefiMacro::numSitePattern(); i++) {
      pattern = macro->lefiMacro::sitePattern(i);
      if(pattern->lefiSitePattern::hasStepPattern()) {
        CIRCUIT_FPRINTF(fout, "  SITE %s %g %g %s DO %g BY %g STEP %g %g ;\n",
                        pattern->lefiSitePattern::name(),
                        pattern->lefiSitePattern::x(),
                        pattern->lefiSitePattern::y(),
                        orientStr(pattern->lefiSitePattern::orient()),
                        pattern->lefiSitePattern::xStart(),
                        pattern->lefiSitePattern::yStart(),
                        pattern->lefiSitePattern::xStep(),
                        pattern->lefiSitePattern::yStep());
      }
      else {
        CIRCUIT_FPRINTF(
            fout, "  SITE %s %g %g %s ;\n", pattern->lefiSitePattern::name(),
            pattern->lefiSitePattern::x(), pattern->lefiSitePattern::y(),
            orientStr(pattern->lefiSitePattern::orient()));
      }
    }
  }
  if(macro->lefiMacro::hasSize())
  CIRCUIT_FPRINTF(fout, "  SIZE %g BY %g ;\n", macro->lefiMacro::sizeX(),
                  macro->lefiMacro::sizeY());

  if(macro->lefiMacro::hasForeign()) {
    for(i = 0; i < macro->lefiMacro::numForeigns(); i++) {
      CIRCUIT_FPRINTF(fout, "  FOREIGN %s ", macro->lefiMacro::foreignName(i));
      if(macro->lefiMacro::hasForeignPoint(i)) {
        CIRCUIT_FPRINTF(fout, "( %g %g ) ", macro->lefiMacro::foreignX(i),
                        macro->lefiMacro::foreignY(i));
        if(macro->lefiMacro::hasForeignOrient(i))
        CIRCUIT_FPRINTF(fout, "%s ", macro->lefiMacro::foreignOrientStr(i));
      }
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }
  if(macro->lefiMacro::hasOrigin())
  CIRCUIT_FPRINTF(fout, "  ORIGIN ( %g %g ) ;\n", macro->lefiMacro::originX(),
                  macro->lefiMacro::originY());
  if(macro->lefiMacro::hasPower())
  CIRCUIT_FPRINTF(fout, "  POWER %g ;\n", macro->lefiMacro::power());
  propNum = macro->lefiMacro::numProperties();
  if(propNum > 0) {
    CIRCUIT_FPRINTF(fout, "  PROPERTY ");
    for(i = 0; i < propNum; i++) {
      // value can either be a string or number
      if(macro->lefiMacro::propValue(i)) {
        CIRCUIT_FPRINTF(fout, "%s %s ", macro->lefiMacro::propName(i),
                        macro->lefiMacro::propValue(i));
      }
      else
      CIRCUIT_FPRINTF(fout, "%s %g ", macro->lefiMacro::propName(i),
                      macro->lefiMacro::propNum(i));

      switch(macro->lefiMacro::propType(i)) {
        case 'R':
        CIRCUIT_FPRINTF(fout, "REAL ");
          break;
        case 'I':
        CIRCUIT_FPRINTF(fout, "INTEGER ");
          break;
        case 'S':
        CIRCUIT_FPRINTF(fout, "STRING ");
          break;
        case 'Q':
        CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
          break;
        case 'N':
        CIRCUIT_FPRINTF(fout, "NUMBER ");
          break;
      }
    }
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  // CIRCUIT_FPRINTF(fout, "END %s\n", macro->lefiMacro::name());
  return 0;
}

int macroEndCB(lefrCallbackType_e c, const char* macroName, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  lefPinStorPtr->push_back(*curPinStorPtr);
  lefObsStorPtr->push_back(*curObsStorPtr);
  lefPinMapStorPtr->push_back(*curPinMapPtr);

  delete curPinStorPtr;
  delete curObsStorPtr;
  delete curPinMapPtr;
  curPinStorPtr = NULL;
  curObsStorPtr = NULL;
  curPinMapPtr = NULL;

  CIRCUIT_FPRINTF(fout, "END %s\n", macroName);
  return 0;
}

int manufacturingCB(lefrCallbackType_e c, double num, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  CIRCUIT_FPRINTF(fout, "MANUFACTURINGGRID %g ;\n", num);
  *lefManufacturingGridPtr = num;
  return 0;
}

int maxStackViaCB(lefrCallbackType_e c, lefiMaxStackVia* maxStack,
                  lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "MAXVIASTACK %d ",
                  maxStack->lefiMaxStackVia::maxStackVia());
  if(maxStack->lefiMaxStackVia::hasMaxStackViaRange())
  CIRCUIT_FPRINTF(fout, "RANGE %s %s ",
                  maxStack->lefiMaxStackVia::maxStackViaBottomLayer(),
                  maxStack->lefiMaxStackVia::maxStackViaTopLayer());
  CIRCUIT_FPRINTF(fout, ";\n");
  return 0;
}

int minFeatureCB(lefrCallbackType_e c, lefiMinFeature* min, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "MINFEATURE %g %g ;\n", min->lefiMinFeature::one(),
                  min->lefiMinFeature::two());
  return 0;
}

int nonDefaultCB(lefrCallbackType_e c, lefiNonDefault* def, lefiUserData) {
  int i;
  lefiVia* via;
  lefiSpacing* spacing;

  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "NONDEFAULTRULE %s\n", def->lefiNonDefault::name());
  if(def->lefiNonDefault::hasHardspacing())
  CIRCUIT_FPRINTF(fout, "  HARDSPACING ;\n");
  for(i = 0; i < def->lefiNonDefault::numLayers(); i++) {
    CIRCUIT_FPRINTF(fout, "  LAYER %s\n", def->lefiNonDefault::layerName(i));
    if(def->lefiNonDefault::hasLayerWidth(i))
    CIRCUIT_FPRINTF(fout, "    WIDTH %g ;\n",
                    def->lefiNonDefault::layerWidth(i));
    if(def->lefiNonDefault::hasLayerSpacing(i))
    CIRCUIT_FPRINTF(fout, "    SPACING %g ;\n",
                    def->lefiNonDefault::layerSpacing(i));
    if(def->lefiNonDefault::hasLayerDiagWidth(i))
    CIRCUIT_FPRINTF(fout, "    DIAGWIDTH %g ;\n",
                    def->lefiNonDefault::layerDiagWidth(i));
    if(def->lefiNonDefault::hasLayerWireExtension(i))
    CIRCUIT_FPRINTF(fout, "    WIREEXTENSION %g ;\n",
                    def->lefiNonDefault::layerWireExtension(i));
    if(def->lefiNonDefault::hasLayerResistance(i))
    CIRCUIT_FPRINTF(fout, "    RESISTANCE RPERSQ %g ;\n",
                    def->lefiNonDefault::layerResistance(i));
    if(def->lefiNonDefault::hasLayerCapacitance(i))
    CIRCUIT_FPRINTF(fout, "    CAPACITANCE CPERSQDIST %g ;\n",
                    def->lefiNonDefault::layerCapacitance(i));
    if(def->lefiNonDefault::hasLayerEdgeCap(i))
    CIRCUIT_FPRINTF(fout, "    EDGECAPACITANCE %g ;\n",
                    def->lefiNonDefault::layerEdgeCap(i));
    CIRCUIT_FPRINTF(fout, "  END %s\n", def->lefiNonDefault::layerName(i));
  }

  // handle via in nondefaultrule
  for(i = 0; i < def->lefiNonDefault::numVias(); i++) {
    via = def->lefiNonDefault::viaRule(i);
    lefVia(via);
  }

  // handle spacing in nondefaultrule
  for(i = 0; i < def->lefiNonDefault::numSpacingRules(); i++) {
    spacing = def->lefiNonDefault::spacingRule(i);
    lefSpacing(spacing);
  }

  // handle usevia
  for(i = 0; i < def->lefiNonDefault::numUseVia(); i++)
  CIRCUIT_FPRINTF(fout, "    USEVIA %s ;\n", def->lefiNonDefault::viaName(i));

  // handle useviarule
  for(i = 0; i < def->lefiNonDefault::numUseViaRule(); i++)
  CIRCUIT_FPRINTF(fout, "    USEVIARULE %s ;\n",
                  def->lefiNonDefault::viaRuleName(i));

  // handle mincuts
  for(i = 0; i < def->lefiNonDefault::numMinCuts(); i++) {
    CIRCUIT_FPRINTF(fout, "   MINCUTS %s %d ;\n",
                    def->lefiNonDefault::cutLayerName(i),
                    def->lefiNonDefault::numCuts(i));
  }

  // handle property in nondefaultrule
  if(def->lefiNonDefault::numProps() > 0) {
    CIRCUIT_FPRINTF(fout, "   PROPERTY ");
    for(i = 0; i < def->lefiNonDefault::numProps(); i++) {
      CIRCUIT_FPRINTF(fout, "%s ", def->lefiNonDefault::propName(i));
      if(def->lefiNonDefault::propIsNumber(i))
      CIRCUIT_FPRINTF(fout, "%g ", def->lefiNonDefault::propNumber(i));
      if(def->lefiNonDefault::propIsString(i))
      CIRCUIT_FPRINTF(fout, "%s ", def->lefiNonDefault::propValue(i));
      switch(def->lefiNonDefault::propType(i)) {
        case 'R':
        CIRCUIT_FPRINTF(fout, "REAL ");
          break;
        case 'I':
        CIRCUIT_FPRINTF(fout, "INTEGER ");
          break;
        case 'S':
        CIRCUIT_FPRINTF(fout, "STRING ");
          break;
        case 'Q':
        CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
          break;
        case 'N':
        CIRCUIT_FPRINTF(fout, "NUMBER ");
          break;
      }
    }
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  CIRCUIT_FPRINTF(fout, "END %s ;\n", def->lefiNonDefault::name());

  return 0;
}

int obstructionCB(lefrCallbackType_e c, lefiObstruction* obs, lefiUserData) {
  lefiGeometries* geometry;

  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "  OBS\n");
  geometry = obs->lefiObstruction::geometries();
  prtGeometry(geometry);
  CIRCUIT_FPRINTF(fout, "  END\n");
  curObsStorPtr->push_back(*obs);
  return 0;
}

int pinCB(lefrCallbackType_e c, lefiPin* pin, lefiUserData) {
  int numPorts, i, j;
  lefiGeometries* geometry;
  lefiPinAntennaModel* aModel;

  (*curPinMapPtr)[string(pin->name())] = curPinStorPtr->size();
  curPinStorPtr->push_back(*pin);

  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "  PIN %s\n", pin->lefiPin::name());
  if(pin->lefiPin::hasForeign()) {
    for(i = 0; i < pin->lefiPin::numForeigns(); i++) {
      if(pin->lefiPin::hasForeignOrient(i)) {
        CIRCUIT_FPRINTF(fout, "    FOREIGN %s STRUCTURE ( %g %g ) %s ;\n",
                        pin->lefiPin::foreignName(i), pin->lefiPin::foreignX(i),
                        pin->lefiPin::foreignY(i),
                        pin->lefiPin::foreignOrientStr(i));
      }
      else if(pin->lefiPin::hasForeignPoint(i)) {
        CIRCUIT_FPRINTF(fout, "    FOREIGN %s STRUCTURE ( %g %g ) ;\n",
                        pin->lefiPin::foreignName(i), pin->lefiPin::foreignX(i),
                        pin->lefiPin::foreignY(i));
      }
      else {
        CIRCUIT_FPRINTF(fout, "    FOREIGN %s ;\n",
                        pin->lefiPin::foreignName(i));
      }
    }
  }
  if(pin->lefiPin::hasLEQ())
  CIRCUIT_FPRINTF(fout, "    LEQ %s ;\n", pin->lefiPin::LEQ());
  if(pin->lefiPin::hasDirection())
  CIRCUIT_FPRINTF(fout, "    DIRECTION %s ;\n", pin->lefiPin::direction());
  if(pin->lefiPin::hasUse())
  CIRCUIT_FPRINTF(fout, "    USE %s ;\n", pin->lefiPin::use());
  if(pin->lefiPin::hasShape())
  CIRCUIT_FPRINTF(fout, "    SHAPE %s ;\n", pin->lefiPin::shape());
  if(pin->lefiPin::hasMustjoin())
  CIRCUIT_FPRINTF(fout, "    MUSTJOIN %s ;\n", pin->lefiPin::mustjoin());
  if(pin->lefiPin::hasOutMargin())
  CIRCUIT_FPRINTF(fout, "    OUTPUTNOISEMARGIN %g %g ;\n",
                  pin->lefiPin::outMarginHigh(),
                  pin->lefiPin::outMarginLow());
  if(pin->lefiPin::hasOutResistance())
  CIRCUIT_FPRINTF(fout, "    OUTPUTRESISTANCE %g %g ;\n",
                  pin->lefiPin::outResistanceHigh(),
                  pin->lefiPin::outResistanceLow());
  if(pin->lefiPin::hasInMargin())
  CIRCUIT_FPRINTF(fout, "    INPUTNOISEMARGIN %g %g ;\n",
                  pin->lefiPin::inMarginHigh(), pin->lefiPin::inMarginLow());
  if(pin->lefiPin::hasPower())
  CIRCUIT_FPRINTF(fout, "    POWER %g ;\n", pin->lefiPin::power());
  if(pin->lefiPin::hasLeakage())
  CIRCUIT_FPRINTF(fout, "    LEAKAGE %g ;\n", pin->lefiPin::leakage());
  if(pin->lefiPin::hasMaxload())
  CIRCUIT_FPRINTF(fout, "    MAXLOAD %g ;\n", pin->lefiPin::maxload());
  if(pin->lefiPin::hasCapacitance())
  CIRCUIT_FPRINTF(fout, "    CAPACITANCE %g ;\n",
                  pin->lefiPin::capacitance());
  if(pin->lefiPin::hasResistance())
  CIRCUIT_FPRINTF(fout, "    RESISTANCE %g ;\n", pin->lefiPin::resistance());
  if(pin->lefiPin::hasPulldownres())
  CIRCUIT_FPRINTF(fout, "    PULLDOWNRES %g ;\n",
                  pin->lefiPin::pulldownres());
  if(pin->lefiPin::hasTieoffr())
  CIRCUIT_FPRINTF(fout, "    TIEOFFR %g ;\n", pin->lefiPin::tieoffr());
  if(pin->lefiPin::hasVHI())
  CIRCUIT_FPRINTF(fout, "    VHI %g ;\n", pin->lefiPin::VHI());
  if(pin->lefiPin::hasVLO())
  CIRCUIT_FPRINTF(fout, "    VLO %g ;\n", pin->lefiPin::VLO());
  if(pin->lefiPin::hasRiseVoltage())
  CIRCUIT_FPRINTF(fout, "    RISEVOLTAGETHRESHOLD %g ;\n",
                  pin->lefiPin::riseVoltage());
  if(pin->lefiPin::hasFallVoltage())
  CIRCUIT_FPRINTF(fout, "    FALLVOLTAGETHRESHOLD %g ;\n",
                  pin->lefiPin::fallVoltage());
  if(pin->lefiPin::hasRiseThresh())
  CIRCUIT_FPRINTF(fout, "    RISETHRESH %g ;\n", pin->lefiPin::riseThresh());
  if(pin->lefiPin::hasFallThresh())
  CIRCUIT_FPRINTF(fout, "    FALLTHRESH %g ;\n", pin->lefiPin::fallThresh());
  if(pin->lefiPin::hasRiseSatcur())
  CIRCUIT_FPRINTF(fout, "    RISESATCUR %g ;\n", pin->lefiPin::riseSatcur());
  if(pin->lefiPin::hasFallSatcur())
  CIRCUIT_FPRINTF(fout, "    FALLSATCUR %g ;\n", pin->lefiPin::fallSatcur());
  if(pin->lefiPin::hasRiseSlewLimit())
  CIRCUIT_FPRINTF(fout, "    RISESLEWLIMIT %g ;\n",
                  pin->lefiPin::riseSlewLimit());
  if(pin->lefiPin::hasFallSlewLimit())
  CIRCUIT_FPRINTF(fout, "    FALLSLEWLIMIT %g ;\n",
                  pin->lefiPin::fallSlewLimit());
  if(pin->lefiPin::hasCurrentSource())
  CIRCUIT_FPRINTF(fout, "    CURRENTSOURCE %s ;\n",
                  pin->lefiPin::currentSource());
  if(pin->lefiPin::hasTables())
  CIRCUIT_FPRINTF(fout, "    IV_TABLES %s %s ;\n",
                  pin->lefiPin::tableHighName(),
                  pin->lefiPin::tableLowName());
  if(pin->lefiPin::hasTaperRule())
  CIRCUIT_FPRINTF(fout, "    TAPERRULE %s ;\n", pin->lefiPin::taperRule());
  if(pin->lefiPin::hasNetExpr())
  CIRCUIT_FPRINTF(fout, "    NETEXPR \"%s\" ;\n", pin->lefiPin::netExpr());
  if(pin->lefiPin::hasSupplySensitivity())
  CIRCUIT_FPRINTF(fout, "    SUPPLYSENSITIVITY %s ;\n",
                  pin->lefiPin::supplySensitivity());
  if(pin->lefiPin::hasGroundSensitivity())
  CIRCUIT_FPRINTF(fout, "    GROUNDSENSITIVITY %s ;\n",
                  pin->lefiPin::groundSensitivity());
  if(pin->lefiPin::hasAntennaSize()) {
    for(i = 0; i < pin->lefiPin::numAntennaSize(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNASIZE %g ",
                      pin->lefiPin::antennaSize(i));
      if(pin->lefiPin::antennaSizeLayer(i))
      CIRCUIT_FPRINTF(fout, "LAYER %s ", pin->lefiPin::antennaSizeLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }
  if(pin->lefiPin::hasAntennaMetalArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaMetalArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAMETALAREA %g ",
                      pin->lefiPin::antennaMetalArea(i));
      if(pin->lefiPin::antennaMetalAreaLayer(i))
      CIRCUIT_FPRINTF(fout, "LAYER %s ",
                      pin->lefiPin::antennaMetalAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }
  if(pin->lefiPin::hasAntennaMetalLength()) {
    for(i = 0; i < pin->lefiPin::numAntennaMetalLength(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAMETALLENGTH %g ",
                      pin->lefiPin::antennaMetalLength(i));
      if(pin->lefiPin::antennaMetalLengthLayer(i))
      CIRCUIT_FPRINTF(fout, "LAYER %s ",
                      pin->lefiPin::antennaMetalLengthLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaPartialMetalArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaPartialMetalArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAPARTIALMETALAREA %g ",
                      pin->lefiPin::antennaPartialMetalArea(i));
      if(pin->lefiPin::antennaPartialMetalAreaLayer(i))
      CIRCUIT_FPRINTF(fout, "LAYER %s ",
                      pin->lefiPin::antennaPartialMetalAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaPartialMetalSideArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaPartialMetalSideArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAPARTIALMETALSIDEAREA %g ",
                      pin->lefiPin::antennaPartialMetalSideArea(i));
      if(pin->lefiPin::antennaPartialMetalSideAreaLayer(i))
      CIRCUIT_FPRINTF(fout, "LAYER %s ",
                      pin->lefiPin::antennaPartialMetalSideAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaPartialCutArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaPartialCutArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAPARTIALCUTAREA %g ",
                      pin->lefiPin::antennaPartialCutArea(i));
      if(pin->lefiPin::antennaPartialCutAreaLayer(i))
      CIRCUIT_FPRINTF(fout, "LAYER %s ",
                      pin->lefiPin::antennaPartialCutAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaDiffArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaDiffArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNADIFFAREA %g ",
                      pin->lefiPin::antennaDiffArea(i));
      if(pin->lefiPin::antennaDiffAreaLayer(i))
      CIRCUIT_FPRINTF(fout, "LAYER %s ",
                      pin->lefiPin::antennaDiffAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  for(j = 0; j < pin->lefiPin::numAntennaModel(); j++) {
    aModel = pin->lefiPin::antennaModel(j);

    CIRCUIT_FPRINTF(fout, "    ANTENNAMODEL %s ;\n",
                    aModel->lefiPinAntennaModel::antennaOxide());

    if(aModel->lefiPinAntennaModel::hasAntennaGateArea()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaGateArea(); i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAGATEAREA %g ",
                        aModel->lefiPinAntennaModel::antennaGateArea(i));
        if(aModel->lefiPinAntennaModel::antennaGateAreaLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ",
                        aModel->lefiPinAntennaModel::antennaGateAreaLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }

    if(aModel->lefiPinAntennaModel::hasAntennaMaxAreaCar()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaMaxAreaCar(); i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAMAXAREACAR %g ",
                        aModel->lefiPinAntennaModel::antennaMaxAreaCar(i));
        if(aModel->lefiPinAntennaModel::antennaMaxAreaCarLayer(i))
        CIRCUIT_FPRINTF(
            fout, "LAYER %s ",
            aModel->lefiPinAntennaModel::antennaMaxAreaCarLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }

    if(aModel->lefiPinAntennaModel::hasAntennaMaxSideAreaCar()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaMaxSideAreaCar();
          i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAMAXSIDEAREACAR %g ",
                        aModel->lefiPinAntennaModel::antennaMaxSideAreaCar(i));
        if(aModel->lefiPinAntennaModel::antennaMaxSideAreaCarLayer(i))
        CIRCUIT_FPRINTF(
            fout, "LAYER %s ",
            aModel->lefiPinAntennaModel::antennaMaxSideAreaCarLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }

    if(aModel->lefiPinAntennaModel::hasAntennaMaxCutCar()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaMaxCutCar(); i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAMAXCUTCAR %g ",
                        aModel->lefiPinAntennaModel::antennaMaxCutCar(i));
        if(aModel->lefiPinAntennaModel::antennaMaxCutCarLayer(i))
        CIRCUIT_FPRINTF(
            fout, "LAYER %s ",
            aModel->lefiPinAntennaModel::antennaMaxCutCarLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
  }

  if(pin->lefiPin::numProperties() > 0) {
    CIRCUIT_FPRINTF(fout, "    PROPERTY ");
    for(i = 0; i < pin->lefiPin::numProperties(); i++) {
      // value can either be a string or number
      if(pin->lefiPin::propValue(i)) {
        CIRCUIT_FPRINTF(fout, "%s %s ", pin->lefiPin::propName(i),
                        pin->lefiPin::propValue(i));
      }
      else {
        CIRCUIT_FPRINTF(fout, "%s %g ", pin->lefiPin::propName(i),
                        pin->lefiPin::propNum(i));
      }
      switch(pin->lefiPin::propType(i)) {
        case 'R':
        CIRCUIT_FPRINTF(fout, "REAL ");
          break;
        case 'I':
        CIRCUIT_FPRINTF(fout, "INTEGER ");
          break;
        case 'S':
        CIRCUIT_FPRINTF(fout, "STRING ");
          break;
        case 'Q':
        CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
          break;
        case 'N':
        CIRCUIT_FPRINTF(fout, "NUMBER ");
          break;
      }
    }
    CIRCUIT_FPRINTF(fout, ";\n");
  }

  numPorts = pin->lefiPin::numPorts();
  for(i = 0; i < numPorts; i++) {
    CIRCUIT_FPRINTF(fout, "    PORT\n");
    fflush(stdout);
    geometry = pin->lefiPin::port(i);
    prtGeometry(geometry);
    CIRCUIT_FPRINTF(fout, "    END\n");
  }
  CIRCUIT_FPRINTF(fout, "  END %s\n", pin->lefiPin::name());
  return 0;
}

int densityCB(lefrCallbackType_e c, lefiDensity* density, lefiUserData) {
  struct lefiGeomRect rect;

  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "  DENSITY\n");
  for(int i = 0; i < density->lefiDensity::numLayer(); i++) {
    CIRCUIT_FPRINTF(fout, "    LAYER %s ;\n",
                    density->lefiDensity::layerName(i));
    for(int j = 0; j < density->lefiDensity::numRects(i); j++) {
      rect = density->lefiDensity::getRect(i, j);
      CIRCUIT_FPRINTF(fout, "      RECT %g %g %g %g ", rect.xl, rect.yl,
                      rect.xh, rect.yh);
      CIRCUIT_FPRINTF(fout, "%g ;\n", density->lefiDensity::densityValue(i, j));
    }
  }
  CIRCUIT_FPRINTF(fout, "  END\n");
  return 0;
}

int propDefBeginCB(lefrCallbackType_e c, void*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "PROPERTYDEFINITIONS\n");
  return 0;
}

int propDefCB(lefrCallbackType_e c, lefiProp* prop, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, " %s %s", prop->lefiProp::propType(),
                  prop->lefiProp::propName());
  switch(prop->lefiProp::dataType()) {
    case 'I':
    CIRCUIT_FPRINTF(fout, " INTEGER");
      break;
    case 'R':
    CIRCUIT_FPRINTF(fout, " REAL");
      break;
    case 'S':
    CIRCUIT_FPRINTF(fout, " STRING");
      break;
  }
  if(prop->lefiProp::hasNumber())
  CIRCUIT_FPRINTF(fout, " %g", prop->lefiProp::number());
  if(prop->lefiProp::hasRange())
  CIRCUIT_FPRINTF(fout, " RANGE %g %g", prop->lefiProp::left(),
                  prop->lefiProp::right());
  if(prop->lefiProp::hasString())
  CIRCUIT_FPRINTF(fout, " %s", prop->lefiProp::string());
  CIRCUIT_FPRINTF(fout, "\n");
  return 0;
}

int propDefEndCB(lefrCallbackType_e c, void*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "END PROPERTYDEFINITIONS\n");
  return 0;
}

int siteCB(lefrCallbackType_e c, lefiSite* site, lefiUserData) {
  int hasPrtSym = 0;
  int i;

  checkType(c);
  (*lefSiteMapPtr)[string(site->name())] = lefSiteStorPtr->size();
  lefSiteStorPtr->push_back(*site);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "SITE %s\n", site->lefiSite::name());
  if(site->lefiSite::hasClass())
  CIRCUIT_FPRINTF(fout, "  CLASS %s ;\n", site->lefiSite::siteClass());
  if(site->lefiSite::hasXSymmetry()) {
    CIRCUIT_FPRINTF(fout, "  SYMMETRY X ");
    hasPrtSym = 1;
  }
  if(site->lefiSite::hasYSymmetry()) {
    if(hasPrtSym) {
      CIRCUIT_FPRINTF(fout, "Y ");
    }
    else {
      CIRCUIT_FPRINTF(fout, "  SYMMETRY Y ");
      hasPrtSym = 1;
    }
  }
  if(site->lefiSite::has90Symmetry()) {
    if(hasPrtSym) {
      CIRCUIT_FPRINTF(fout, "R90 ");
    }
    else {
      CIRCUIT_FPRINTF(fout, "  SYMMETRY R90 ");
      hasPrtSym = 1;
    }
  }
  if(hasPrtSym)
  CIRCUIT_FPRINTF(fout, ";\n");
  if(site->lefiSite::hasSize())
  CIRCUIT_FPRINTF(fout, "  SIZE %g BY %g ;\n", site->lefiSite::sizeX(),
                  site->lefiSite::sizeY());

  if(site->hasRowPattern()) {
    CIRCUIT_FPRINTF(fout, "  ROWPATTERN ");
    for(i = 0; i < site->lefiSite::numSites(); i++)
    CIRCUIT_FPRINTF(fout, "  %s %s ", site->lefiSite::siteName(i),
                    site->lefiSite::siteOrientStr(i));
    CIRCUIT_FPRINTF(fout, ";\n");
  }

  CIRCUIT_FPRINTF(fout, "END %s\n", site->lefiSite::name());
  return 0;
}

int spacingBeginCB(lefrCallbackType_e c, void*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "SPACING\n");
  return 0;
}

int spacingCB(lefrCallbackType_e c, lefiSpacing* spacing, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  lefSpacing(spacing);
  return 0;
}

int spacingEndCB(lefrCallbackType_e c, void*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "END SPACING\n");
  return 0;
}

int timingCB(lefrCallbackType_e c, lefiTiming* timing, lefiUserData) {
  int i;
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "TIMING\n");
  for(i = 0; i < timing->numFromPins(); i++)
  CIRCUIT_FPRINTF(fout, " FROMPIN %s ;\n", timing->fromPin(i));
  for(i = 0; i < timing->numToPins(); i++)
  CIRCUIT_FPRINTF(fout, " TOPIN %s ;\n", timing->toPin(i));
  CIRCUIT_FPRINTF(fout, " RISE SLEW1 %g %g %g %g ;\n", timing->riseSlewOne(),
                  timing->riseSlewTwo(), timing->riseSlewThree(),
                  timing->riseSlewFour());
  if(timing->hasRiseSlew2())
  CIRCUIT_FPRINTF(fout, " RISE SLEW2 %g %g %g ;\n", timing->riseSlewFive(),
                  timing->riseSlewSix(), timing->riseSlewSeven());
  if(timing->hasFallSlew())
  CIRCUIT_FPRINTF(fout, " FALL SLEW1 %g %g %g %g ;\n", timing->fallSlewOne(),
                  timing->fallSlewTwo(), timing->fallSlewThree(),
                  timing->fallSlewFour());
  if(timing->hasFallSlew2())
  CIRCUIT_FPRINTF(fout, " FALL SLEW2 %g %g %g ;\n", timing->fallSlewFive(),
                  timing->fallSlewSix(), timing->riseSlewSeven());
  if(timing->hasRiseIntrinsic()) {
    CIRCUIT_FPRINTF(fout, "TIMING RISE INTRINSIC %g %g ;\n",
                    timing->riseIntrinsicOne(), timing->riseIntrinsicTwo());
    CIRCUIT_FPRINTF(fout, "TIMING RISE VARIABLE %g %g ;\n",
                    timing->riseIntrinsicThree(), timing->riseIntrinsicFour());
  }
  if(timing->hasFallIntrinsic()) {
    CIRCUIT_FPRINTF(fout, "TIMING FALL INTRINSIC %g %g ;\n",
                    timing->fallIntrinsicOne(), timing->fallIntrinsicTwo());
    CIRCUIT_FPRINTF(fout, "TIMING RISE VARIABLE %g %g ;\n",
                    timing->fallIntrinsicThree(), timing->fallIntrinsicFour());
  }
  if(timing->hasRiseRS())
  CIRCUIT_FPRINTF(fout, "TIMING RISERS %g %g ;\n", timing->riseRSOne(),
                  timing->riseRSTwo());
  if(timing->hasRiseCS())
  CIRCUIT_FPRINTF(fout, "TIMING RISECS %g %g ;\n", timing->riseCSOne(),
                  timing->riseCSTwo());
  if(timing->hasFallRS())
  CIRCUIT_FPRINTF(fout, "TIMING FALLRS %g %g ;\n", timing->fallRSOne(),
                  timing->fallRSTwo());
  if(timing->hasFallCS())
  CIRCUIT_FPRINTF(fout, "TIMING FALLCS %g %g ;\n", timing->fallCSOne(),
                  timing->fallCSTwo());
  if(timing->hasUnateness())
  CIRCUIT_FPRINTF(fout, "TIMING UNATENESS %s ;\n", timing->unateness());
  if(timing->hasRiseAtt1())
  CIRCUIT_FPRINTF(fout, "TIMING RISESATT1 %g %g ;\n", timing->riseAtt1One(),
                  timing->riseAtt1Two());
  if(timing->hasFallAtt1())
  CIRCUIT_FPRINTF(fout, "TIMING FALLSATT1 %g %g ;\n", timing->fallAtt1One(),
                  timing->fallAtt1Two());
  if(timing->hasRiseTo())
  CIRCUIT_FPRINTF(fout, "TIMING RISET0 %g %g ;\n", timing->riseToOne(),
                  timing->riseToTwo());
  if(timing->hasFallTo())
  CIRCUIT_FPRINTF(fout, "TIMING FALLT0 %g %g ;\n", timing->fallToOne(),
                  timing->fallToTwo());
  if(timing->hasSDFonePinTrigger())
  CIRCUIT_FPRINTF(fout, " %s TABLEDIMENSION %g %g %g ;\n",
                  timing->SDFonePinTriggerType(), timing->SDFtriggerOne(),
                  timing->SDFtriggerTwo(), timing->SDFtriggerThree());
  if(timing->hasSDFtwoPinTrigger())
  CIRCUIT_FPRINTF(fout, " %s %s %s TABLEDIMENSION %g %g %g ;\n",
                  timing->SDFtwoPinTriggerType(), timing->SDFfromTrigger(),
                  timing->SDFtoTrigger(), timing->SDFtriggerOne(),
                  timing->SDFtriggerTwo(), timing->SDFtriggerThree());
  CIRCUIT_FPRINTF(fout, "END TIMING\n");
  return 0;
}

int unitsCB(lefrCallbackType_e c, lefiUnits* unit, lefiUserData) {
  checkType(c);
  *lefUnitPtr = (*unit);
  // if ((long)ud != userData) dataError();

  *lefUnitPtr = *unit;
  CIRCUIT_FPRINTF(fout, "UNITS\n");
  if(unit->lefiUnits::hasDatabase())
  CIRCUIT_FPRINTF(fout, "  DATABASE %s %g ;\n",
                  unit->lefiUnits::databaseName(),
                  unit->lefiUnits::databaseNumber());
  if(unit->lefiUnits::hasCapacitance())
  CIRCUIT_FPRINTF(fout, "  CAPACITANCE PICOFARADS %g ;\n",
                  unit->lefiUnits::capacitance());
  if(unit->lefiUnits::hasResistance())
  CIRCUIT_FPRINTF(fout, "  RESISTANCE OHMS %g ;\n",
                  unit->lefiUnits::resistance());
  if(unit->lefiUnits::hasPower())
  CIRCUIT_FPRINTF(fout, "  POWER MILLIWATTS %g ;\n",
                  unit->lefiUnits::power());
  if(unit->lefiUnits::hasCurrent())
  CIRCUIT_FPRINTF(fout, "  CURRENT MILLIAMPS %g ;\n",
                  unit->lefiUnits::current());
  if(unit->lefiUnits::hasVoltage())
  CIRCUIT_FPRINTF(fout, "  VOLTAGE VOLTS %g ;\n", unit->lefiUnits::voltage());
  if(unit->lefiUnits::hasFrequency())
  CIRCUIT_FPRINTF(fout, "  FREQUENCY MEGAHERTZ %g ;\n",
                  unit->lefiUnits::frequency());
  CIRCUIT_FPRINTF(fout, "END UNITS\n");
  return 0;
}

int useMinSpacingCB(lefrCallbackType_e c, lefiUseMinSpacing* spacing,
                    lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "USEMINSPACING %s ",
                  spacing->lefiUseMinSpacing::name());
  if(spacing->lefiUseMinSpacing::value()) {
    CIRCUIT_FPRINTF(fout, "ON ;\n");
  }
  else {
    CIRCUIT_FPRINTF(fout, "OFF ;\n");
  }
  return 0;
}

int versionCB(lefrCallbackType_e c, double num, lefiUserData) {
  checkType(c);

  *lefVersionPtr = num;
  CIRCUIT_FPRINTF(fout, "VERSION %g ;\n", num);
  return 0;
}

int versionStrCB(lefrCallbackType_e c, const char* versionName, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "VERSION %s ;\n", versionName);
  return 0;
}

int viaCB(lefrCallbackType_e c, lefiVia* via, lefiUserData) {
  checkType(c);

  (*lefViaMapPtr)[string(via->name())] = lefViaStorPtr->size();
  lefViaStorPtr->push_back(*via);
  // if ((long)ud != userData) dataError();
  lefVia(via);
  return 0;
}

int viaRuleCB(lefrCallbackType_e c, lefiViaRule* viaRule, lefiUserData) {
  int numLayers, numVias, i;
  lefiViaRuleLayer* vLayer;

  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "VIARULE %s", viaRule->lefiViaRule::name());
  if(viaRule->lefiViaRule::hasGenerate())
  CIRCUIT_FPRINTF(fout, " GENERATE");
  if(viaRule->lefiViaRule::hasDefault())
  CIRCUIT_FPRINTF(fout, " DEFAULT");
  CIRCUIT_FPRINTF(fout, "\n");

  numLayers = viaRule->lefiViaRule::numLayers();
  // if numLayers == 2, it is VIARULE without GENERATE and has via name
  // if numLayers == 3, it is VIARULE with GENERATE, and the 3rd layer is cut
  for(i = 0; i < numLayers; i++) {
    vLayer = viaRule->lefiViaRule::layer(i);
    lefViaRuleLayer(vLayer);
  }

  if(numLayers == 2 && !(viaRule->lefiViaRule::hasGenerate())) {
    // should have vianames
    numVias = viaRule->lefiViaRule::numVias();
    if(numVias == 0) {
      CIRCUIT_FPRINTF(fout, "Should have via names in VIARULE.\n");
    }
    else {
      for(i = 0; i < numVias; i++)
      CIRCUIT_FPRINTF(fout, "  VIA %s ;\n", viaRule->lefiViaRule::viaName(i));
    }
  }
  if(viaRule->lefiViaRule::numProps() > 0) {
    CIRCUIT_FPRINTF(fout, "  PROPERTY ");
    for(i = 0; i < viaRule->lefiViaRule::numProps(); i++) {
      CIRCUIT_FPRINTF(fout, "%s ", viaRule->lefiViaRule::propName(i));
      if(viaRule->lefiViaRule::propValue(i))
      CIRCUIT_FPRINTF(fout, "%s ", viaRule->lefiViaRule::propValue(i));
      switch(viaRule->lefiViaRule::propType(i)) {
        case 'R':
        CIRCUIT_FPRINTF(fout, "REAL ");
          break;
        case 'I':
        CIRCUIT_FPRINTF(fout, "INTEGER ");
          break;
        case 'S':
        CIRCUIT_FPRINTF(fout, "STRING ");
          break;
        case 'Q':
        CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
          break;
        case 'N':
        CIRCUIT_FPRINTF(fout, "NUMBER ");
          break;
      }
    }
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  CIRCUIT_FPRINTF(fout, "END %s\n", viaRule->lefiViaRule::name());
  return 0;
}

int extensionCB(lefrCallbackType_e c, const char* extsn, lefiUserData) {
  checkType(c);
  // lefrSetCaseSensitivity(0);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "BEGINEXT %s ;\n", extsn);
  // lefrSetCaseSensitivity(1);
  return 0;
}

int doneCB(lefrCallbackType_e c, void*, lefiUserData) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  CIRCUIT_FPRINTF(fout, "END LIBRARY\n");
  return 0;
}

void errorCB(const char* msg) {
  printf("%s : %s\n", lefrGetUserData(), (char*)msg);
}

void warningCB(const char* msg) {
  printf("%s : %s\n", lefrGetUserData(), (char*)msg);
}

void* mallocCB(int size) {
  return malloc(size);
}

void* reallocCB(void* name, int size) {
  return realloc(name, size);
}

void freeCB(void* name) {
  free(name);
  return;
}

void lineNumberCB(int lineNo) {
  cout << "[LEF] Parsed " << lineNo << " number of lines!!" << endl;
}

void printWarning(const char* str) {
  CIRCUIT_FPRINTF(stderr, "%s\n", str);
}


ePlace::Parser::Parser() {}
void ePlace::Parser::DumpLefVersion() const {
  CIRCUIT_FPRINTF(fout, "VERSION %g ;\n", lefVersion);
}

void ePlace::Parser::DumpLefBusBitChar() {
  CIRCUIT_FPRINTF(fout, "BUSBITCHARS \"%s\" ;\n", lefBusBitChar.c_str());
}

void ePlace::Parser::DumpLefDivider() {
  CIRCUIT_FPRINTF(fout, "DIVIDERCHAR \"%s\" ;\n\n", lefDivider.c_str());
}

void ePlace::Parser::DumpLefUnit() {
  //    lefiUnits* unit = &lefUnit;
  CIRCUIT_FPRINTF(fout, "UNITS\n");
  if(lefUnit.lefiUnits::hasDatabase())
  CIRCUIT_FPRINTF(fout, "  DATABASE %s %g ;\n",
                  lefUnit.lefiUnits::databaseName(),
                  lefUnit.lefiUnits::databaseNumber());
  if(lefUnit.lefiUnits::hasCapacitance())
  CIRCUIT_FPRINTF(fout, "  CAPACITANCE PICOFARADS %g ;\n",
                  lefUnit.lefiUnits::capacitance());
  if(lefUnit.lefiUnits::hasResistance())
  CIRCUIT_FPRINTF(fout, "  RESISTANCE OHMS %g ;\n",
                  lefUnit.lefiUnits::resistance());
  if(lefUnit.lefiUnits::hasPower())
  CIRCUIT_FPRINTF(fout, "  POWER MILLIWATTS %g ;\n",
                  lefUnit.lefiUnits::power());
  if(lefUnit.lefiUnits::hasCurrent())
  CIRCUIT_FPRINTF(fout, "  CURRENT MILLIAMPS %g ;\n",
                  lefUnit.lefiUnits::current());
  if(lefUnit.lefiUnits::hasVoltage())
  CIRCUIT_FPRINTF(fout, "  VOLTAGE VOLTS %g ;\n",
                  lefUnit.lefiUnits::voltage());
  if(lefUnit.lefiUnits::hasFrequency())
  CIRCUIT_FPRINTF(fout, "  FREQUENCY MEGAHERTZ %g ;\n",
                  lefUnit.lefiUnits::frequency());
  CIRCUIT_FPRINTF(fout, "END UNITS\n\n");
}

void ePlace::Parser::DumpLefManufacturingGrid() {
  if(lefManufacturingGrid != DBL_MIN) {
    CIRCUIT_FPRINTF(fout, "MANUFACTURINGGRID %g ;\n\n", lefManufacturingGrid);
  }
}

void ePlace::Parser::DumpLefLayer() {
  if(lefLayerStor.size() == 0) {
    return;
  }

  int i, j, k;
  int numPoints, propNum;
  double *widths, *current;
  lefiLayerDensity* density;
  lefiAntennaPWL* pwl;
  lefiSpacingTable* spTable;
  lefiInfluence* influence;
  lefiParallel* parallel;
  lefiTwoWidths* twoWidths;
  char pType;
  int numMinCut, numMinenclosed;
  lefiAntennaModel* aModel;
  lefiOrthogonal* ortho;

  lefrSetCaseSensitivity(0);
  for(auto& curLayer : lefLayerStor) {
    // Call parse65nmRules for 5.7 syntax in 5.6
    if(parse65nm)
      curLayer.lefiLayer::parse65nmRules();

    // Call parseLef58Type for 5.8 syntax in 5.7
    if(parseLef58Type)
      curLayer.lefiLayer::parseLEF58Layer();

    CIRCUIT_FPRINTF(fout, "LAYER %s\n", curLayer.lefiLayer::name());
    if(curLayer.lefiLayer::hasType())
    CIRCUIT_FPRINTF(fout, "  TYPE %s ;\n", curLayer.lefiLayer::type());
    if(curLayer.lefiLayer::hasLayerType())
    CIRCUIT_FPRINTF(fout, "  LAYER TYPE %s ;\n",
                    curLayer.lefiLayer::layerType());
    if(curLayer.lefiLayer::hasMask())
    CIRCUIT_FPRINTF(fout, "  MASK %d ;\n", curLayer.lefiLayer::mask());
    if(curLayer.lefiLayer::hasPitch()) {
      CIRCUIT_FPRINTF(fout, "  PITCH %g ;\n", curLayer.lefiLayer::pitch());
    }
    else if(curLayer.lefiLayer::hasXYPitch()) {
      CIRCUIT_FPRINTF(fout, "  PITCH %g %g ;\n", curLayer.lefiLayer::pitchX(),
                      curLayer.lefiLayer::pitchY());
    }
    if(curLayer.lefiLayer::hasOffset()) {
      CIRCUIT_FPRINTF(fout, "  OFFSET %g ;\n", curLayer.lefiLayer::offset());
    }
    else if(curLayer.lefiLayer::hasXYOffset()) {
      CIRCUIT_FPRINTF(fout, "  OFFSET %g %g ;\n", curLayer.lefiLayer::offsetX(),
                      curLayer.lefiLayer::offsetY());
    }
    if(curLayer.lefiLayer::hasDiagPitch()) {
      CIRCUIT_FPRINTF(fout, "  DIAGPITCH %g ;\n",
                      curLayer.lefiLayer::diagPitch());
    }
    else if(curLayer.lefiLayer::hasXYDiagPitch()) {
      CIRCUIT_FPRINTF(fout, "  DIAGPITCH %g %g ;\n",
                      curLayer.lefiLayer::diagPitchX(),
                      curLayer.lefiLayer::diagPitchY());
    }
    if(curLayer.lefiLayer::hasDiagWidth())
    CIRCUIT_FPRINTF(fout, "  DIAGWIDTH %g ;\n",
                    curLayer.lefiLayer::diagWidth());
    if(curLayer.lefiLayer::hasDiagSpacing())
    CIRCUIT_FPRINTF(fout, "  DIAGSPACING %g ;\n",
                    curLayer.lefiLayer::diagSpacing());
    if(curLayer.lefiLayer::hasWidth())
    CIRCUIT_FPRINTF(fout, "  WIDTH %g ;\n", curLayer.lefiLayer::width());
    if(curLayer.lefiLayer::hasArea())
    CIRCUIT_FPRINTF(fout, "  AREA %g ;\n", curLayer.lefiLayer::area());
    if(curLayer.lefiLayer::hasSlotWireWidth())
    CIRCUIT_FPRINTF(fout, "  SLOTWIREWIDTH %g ;\n",
                    curLayer.lefiLayer::slotWireWidth());
    if(curLayer.lefiLayer::hasSlotWireLength())
    CIRCUIT_FPRINTF(fout, "  SLOTWIRELENGTH %g ;\n",
                    curLayer.lefiLayer::slotWireLength());
    if(curLayer.lefiLayer::hasSlotWidth())
    CIRCUIT_FPRINTF(fout, "  SLOTWIDTH %g ;\n",
                    curLayer.lefiLayer::slotWidth());
    if(curLayer.lefiLayer::hasSlotLength())
    CIRCUIT_FPRINTF(fout, "  SLOTLENGTH %g ;\n",
                    curLayer.lefiLayer::slotLength());
    if(curLayer.lefiLayer::hasMaxAdjacentSlotSpacing())
    CIRCUIT_FPRINTF(fout, "  MAXADJACENTSLOTSPACING %g ;\n",
                    curLayer.lefiLayer::maxAdjacentSlotSpacing());
    if(curLayer.lefiLayer::hasMaxCoaxialSlotSpacing())
    CIRCUIT_FPRINTF(fout, "  MAXCOAXIALSLOTSPACING %g ;\n",
                    curLayer.lefiLayer::maxCoaxialSlotSpacing());
    if(curLayer.lefiLayer::hasMaxEdgeSlotSpacing())
    CIRCUIT_FPRINTF(fout, "  MAXEDGESLOTSPACING %g ;\n",
                    curLayer.lefiLayer::maxEdgeSlotSpacing());
    if(curLayer.lefiLayer::hasMaxFloatingArea())  // 5.7
    CIRCUIT_FPRINTF(fout, "  MAXFLOATINGAREA %g ;\n",
                    curLayer.lefiLayer::maxFloatingArea());
    if(curLayer.lefiLayer::hasArraySpacing()) {  // 5.7
      CIRCUIT_FPRINTF(fout, "  ARRAYSPACING ");
      if(curLayer.lefiLayer::hasLongArray())
      CIRCUIT_FPRINTF(fout, "LONGARRAY ");
      if(curLayer.lefiLayer::hasViaWidth())
      CIRCUIT_FPRINTF(fout, "WIDTH %g ", curLayer.lefiLayer::viaWidth());
      CIRCUIT_FPRINTF(fout, "CUTSPACING %g", curLayer.lefiLayer::cutSpacing());
      for(i = 0; i < curLayer.lefiLayer::numArrayCuts(); i++)
      CIRCUIT_FPRINTF(fout, "\n\tARRAYCUTS %d SPACING %g",
                      curLayer.lefiLayer::arrayCuts(i),
                      curLayer.lefiLayer::arraySpacing(i));
      CIRCUIT_FPRINTF(fout, " ;\n");
    }
    if(curLayer.lefiLayer::hasSplitWireWidth())
    CIRCUIT_FPRINTF(fout, "  SPLITWIREWIDTH %g ;\n",
                    curLayer.lefiLayer::splitWireWidth());
    if(curLayer.lefiLayer::hasMinimumDensity())
    CIRCUIT_FPRINTF(fout, "  MINIMUMDENSITY %g ;\n",
                    curLayer.lefiLayer::minimumDensity());
    if(curLayer.lefiLayer::hasMaximumDensity())
    CIRCUIT_FPRINTF(fout, "  MAXIMUMDENSITY %g ;\n",
                    curLayer.lefiLayer::maximumDensity());
    if(curLayer.lefiLayer::hasDensityCheckWindow())
    CIRCUIT_FPRINTF(fout, "  DENSITYCHECKWINDOW %g %g ;\n",
                    curLayer.lefiLayer::densityCheckWindowLength(),
                    curLayer.lefiLayer::densityCheckWindowWidth());
    if(curLayer.lefiLayer::hasDensityCheckStep())
    CIRCUIT_FPRINTF(fout, "  DENSITYCHECKSTEP %g ;\n",
                    curLayer.lefiLayer::densityCheckStep());
    if(curLayer.lefiLayer::hasFillActiveSpacing())
    CIRCUIT_FPRINTF(fout, "  FILLACTIVESPACING %g ;\n",
                    curLayer.lefiLayer::fillActiveSpacing());
    // 5.4.1
    numMinCut = curLayer.lefiLayer::numMinimumcut();
    if(numMinCut > 0) {
      for(i = 0; i < numMinCut; i++) {
        CIRCUIT_FPRINTF(fout, "  MINIMUMCUT %d WIDTH %g ",
                        curLayer.lefiLayer::minimumcut(i),
                        curLayer.lefiLayer::minimumcutWidth(i));
        if(curLayer.lefiLayer::hasMinimumcutWithin(i))
        CIRCUIT_FPRINTF(fout, "WITHIN %g ",
                        curLayer.lefiLayer::minimumcutWithin(i));
        if(curLayer.lefiLayer::hasMinimumcutConnection(i))
        CIRCUIT_FPRINTF(fout, "%s ",
                        curLayer.lefiLayer::minimumcutConnection(i));
        if(curLayer.lefiLayer::hasMinimumcutNumCuts(i))
        CIRCUIT_FPRINTF(fout, "LENGTH %g WITHIN %g ",
                        curLayer.lefiLayer::minimumcutLength(i),
                        curLayer.lefiLayer::minimumcutDistance(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
    // 5.4.1
    if(curLayer.lefiLayer::hasMaxwidth()) {
      CIRCUIT_FPRINTF(fout, "  MAXWIDTH %g ;\n",
                      curLayer.lefiLayer::maxwidth());
    }
    // 5.5
    if(curLayer.lefiLayer::hasMinwidth()) {
      CIRCUIT_FPRINTF(fout, "  MINWIDTH %g ;\n",
                      curLayer.lefiLayer::minwidth());
    }
    // 5.5
    numMinenclosed = curLayer.lefiLayer::numMinenclosedarea();
    if(numMinenclosed > 0) {
      for(i = 0; i < numMinenclosed; i++) {
        CIRCUIT_FPRINTF(fout, "  MINENCLOSEDAREA %g ",
                        curLayer.lefiLayer::minenclosedarea(i));
        if(curLayer.lefiLayer::hasMinenclosedareaWidth(i))
        CIRCUIT_FPRINTF(fout, "MINENCLOSEDAREAWIDTH %g ",
                        curLayer.lefiLayer::minenclosedareaWidth(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
    // 5.4.1 & 5.6
    if(curLayer.lefiLayer::hasMinstep()) {
      for(i = 0; i < curLayer.lefiLayer::numMinstep(); i++) {
        CIRCUIT_FPRINTF(fout, "  MINSTEP %g ", curLayer.lefiLayer::minstep(i));
        if(curLayer.lefiLayer::hasMinstepType(i))
        CIRCUIT_FPRINTF(fout, "%s ", curLayer.lefiLayer::minstepType(i));
        if(curLayer.lefiLayer::hasMinstepLengthsum(i))
        CIRCUIT_FPRINTF(fout, "LENGTHSUM %g ",
                        curLayer.lefiLayer::minstepLengthsum(i));
        if(curLayer.lefiLayer::hasMinstepMaxedges(i))
        CIRCUIT_FPRINTF(fout, "MAXEDGES %d ",
                        curLayer.lefiLayer::minstepMaxedges(i));
        if(curLayer.lefiLayer::hasMinstepMinAdjLength(i))
        CIRCUIT_FPRINTF(fout, "MINADJLENGTH %g ",
                        curLayer.lefiLayer::minstepMinAdjLength(i));
        if(curLayer.lefiLayer::hasMinstepMinBetLength(i))
        CIRCUIT_FPRINTF(fout, "MINBETLENGTH %g ",
                        curLayer.lefiLayer::minstepMinBetLength(i));
        if(curLayer.lefiLayer::hasMinstepXSameCorners(i))
        CIRCUIT_FPRINTF(fout, "XSAMECORNERS");
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
    // 5.4.1
    if(curLayer.lefiLayer::hasProtrusion()) {
      CIRCUIT_FPRINTF(fout, "  PROTRUSIONWIDTH %g LENGTH %g WIDTH %g ;\n",
                      curLayer.lefiLayer::protrusionWidth1(),
                      curLayer.lefiLayer::protrusionLength(),
                      curLayer.lefiLayer::protrusionWidth2());
    }
    if(curLayer.lefiLayer::hasSpacingNumber()) {
      for(i = 0; i < curLayer.lefiLayer::numSpacing(); i++) {
        CIRCUIT_FPRINTF(fout, "  SPACING %g ", curLayer.lefiLayer::spacing(i));
        if(curLayer.lefiLayer::hasSpacingName(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ",
                        curLayer.lefiLayer::spacingName(i));
        if(curLayer.lefiLayer::hasSpacingLayerStack(i))
        CIRCUIT_FPRINTF(fout, "STACK ");  // 5.7
        if(curLayer.lefiLayer::hasSpacingAdjacent(i))
        CIRCUIT_FPRINTF(fout, "ADJACENTCUTS %d WITHIN %g ",
                        curLayer.lefiLayer::spacingAdjacentCuts(i),
                        curLayer.lefiLayer::spacingAdjacentWithin(i));
        if(curLayer.lefiLayer::hasSpacingAdjacentExcept(i))  // 5.7
        CIRCUIT_FPRINTF(fout, "EXCEPTSAMEPGNET ");
        if(curLayer.lefiLayer::hasSpacingCenterToCenter(i))
        CIRCUIT_FPRINTF(fout, "CENTERTOCENTER ");
        if(curLayer.lefiLayer::hasSpacingSamenet(i))  // 5.7
        CIRCUIT_FPRINTF(fout, "SAMENET ");
        if(curLayer.lefiLayer::hasSpacingSamenetPGonly(i))  // 5.7
        CIRCUIT_FPRINTF(fout, "PGONLY ");
        if(curLayer.lefiLayer::hasSpacingArea(i))  // 5.7
        CIRCUIT_FPRINTF(fout, "AREA %g ", curLayer.lefiLayer::spacingArea(i));
        if(curLayer.lefiLayer::hasSpacingRange(i)) {
          CIRCUIT_FPRINTF(fout, "RANGE %g %g ",
                          curLayer.lefiLayer::spacingRangeMin(i),
                          curLayer.lefiLayer::spacingRangeMax(i));
          if(curLayer.lefiLayer::hasSpacingRangeUseLengthThreshold(i)) {
            CIRCUIT_FPRINTF(fout, "USELENGTHTHRESHOLD ");
          }
          else if(curLayer.lefiLayer::hasSpacingRangeInfluence(i)) {
            CIRCUIT_FPRINTF(fout, "INFLUENCE %g ",
                            curLayer.lefiLayer::spacingRangeInfluence(i));
            if(curLayer.lefiLayer::hasSpacingRangeInfluenceRange(i)) {
              CIRCUIT_FPRINTF(fout, "RANGE %g %g ",
                              curLayer.lefiLayer::spacingRangeInfluenceMin(i),
                              curLayer.lefiLayer::spacingRangeInfluenceMax(i));
            }
          }
          else if(curLayer.lefiLayer::hasSpacingRangeRange(i)) {
            CIRCUIT_FPRINTF(fout, "RANGE %g %g ",
                            curLayer.lefiLayer::spacingRangeRangeMin(i),
                            curLayer.lefiLayer::spacingRangeRangeMax(i));
          }
        }
        else if(curLayer.lefiLayer::hasSpacingLengthThreshold(i)) {
          CIRCUIT_FPRINTF(fout, "LENGTHTHRESHOLD %g ",
                          curLayer.lefiLayer::spacingLengthThreshold(i));
          if(curLayer.lefiLayer::hasSpacingLengthThresholdRange(i))
          CIRCUIT_FPRINTF(
              fout, "RANGE %g %g",
              curLayer.lefiLayer::spacingLengthThresholdRangeMin(i),
              curLayer.lefiLayer::spacingLengthThresholdRangeMax(i));
        }
        else if(curLayer.lefiLayer::hasSpacingNotchLength(i)) {  // 5.7
          CIRCUIT_FPRINTF(fout, "NOTCHLENGTH %g",
                          curLayer.lefiLayer::spacingNotchLength(i));
        }
        else if(curLayer.lefiLayer::hasSpacingEndOfNotchWidth(i))  // 5.7
        CIRCUIT_FPRINTF(fout,
                        "ENDOFNOTCHWIDTH %g NOTCHSPACING %g, NOTCHLENGTH %g",
                        curLayer.lefiLayer::spacingEndOfNotchWidth(i),
                        curLayer.lefiLayer::spacingEndOfNotchSpacing(i),
                        curLayer.lefiLayer::spacingEndOfNotchLength(i));

        if(curLayer.lefiLayer::hasSpacingParallelOverlap(i))  // 5.7
        CIRCUIT_FPRINTF(fout, "PARALLELOVERLAP ");
        if(curLayer.lefiLayer::hasSpacingEndOfLine(i)) {  // 5.7
          CIRCUIT_FPRINTF(fout, "ENDOFLINE %g WITHIN %g ",
                          curLayer.lefiLayer::spacingEolWidth(i),
                          curLayer.lefiLayer::spacingEolWithin(i));
          if(curLayer.lefiLayer::hasSpacingParellelEdge(i)) {
            CIRCUIT_FPRINTF(fout, "PARALLELEDGE %g WITHIN %g ",
                            curLayer.lefiLayer::spacingParSpace(i),
                            curLayer.lefiLayer::spacingParWithin(i));
            if(curLayer.lefiLayer::hasSpacingTwoEdges(i)) {
              CIRCUIT_FPRINTF(fout, "TWOEDGES ");
            }
          }
        }
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
    if(curLayer.lefiLayer::hasSpacingTableOrtho()) {  // 5.7
      CIRCUIT_FPRINTF(fout, "SPACINGTABLE ORTHOGONAL");
      ortho = curLayer.lefiLayer::orthogonal();
      for(i = 0; i < ortho->lefiOrthogonal::numOrthogonal(); i++) {
        CIRCUIT_FPRINTF(fout, "\n   WITHIN %g SPACING %g",
                        ortho->lefiOrthogonal::cutWithin(i),
                        ortho->lefiOrthogonal::orthoSpacing(i));
      }
      CIRCUIT_FPRINTF(fout, ";\n");
    }
    for(i = 0; i < curLayer.lefiLayer::numEnclosure(); i++) {
      CIRCUIT_FPRINTF(fout, "ENCLOSURE ");
      if(curLayer.lefiLayer::hasEnclosureRule(i))
      CIRCUIT_FPRINTF(fout, "%s ", curLayer.lefiLayer::enclosureRule(i));
      CIRCUIT_FPRINTF(fout, "%g %g ", curLayer.lefiLayer::enclosureOverhang1(i),
                      curLayer.lefiLayer::enclosureOverhang2(i));
      if(curLayer.lefiLayer::hasEnclosureWidth(i))
      CIRCUIT_FPRINTF(fout, "WIDTH %g ",
                      curLayer.lefiLayer::enclosureMinWidth(i));
      if(curLayer.lefiLayer::hasEnclosureExceptExtraCut(i))
      CIRCUIT_FPRINTF(fout, "EXCEPTEXTRACUT %g ",
                      curLayer.lefiLayer::enclosureExceptExtraCut(i));
      if(curLayer.lefiLayer::hasEnclosureMinLength(i))
      CIRCUIT_FPRINTF(fout, "LENGTH %g ",
                      curLayer.lefiLayer::enclosureMinLength(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
    for(i = 0; i < curLayer.lefiLayer::numPreferEnclosure(); i++) {
      CIRCUIT_FPRINTF(fout, "PREFERENCLOSURE ");
      if(curLayer.lefiLayer::hasPreferEnclosureRule(i))
      CIRCUIT_FPRINTF(fout, "%s ",
                      curLayer.lefiLayer::preferEnclosureRule(i));
      CIRCUIT_FPRINTF(fout, "%g %g ",
                      curLayer.lefiLayer::preferEnclosureOverhang1(i),
                      curLayer.lefiLayer::preferEnclosureOverhang2(i));
      if(curLayer.lefiLayer::hasPreferEnclosureWidth(i))
      CIRCUIT_FPRINTF(fout, "WIDTH %g ",
                      curLayer.lefiLayer::preferEnclosureMinWidth(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
    if(curLayer.lefiLayer::hasResistancePerCut())
    CIRCUIT_FPRINTF(fout, "  RESISTANCE %g ;\n",
                    curLayer.lefiLayer::resistancePerCut());
    if(curLayer.lefiLayer::hasCurrentDensityPoint())
    CIRCUIT_FPRINTF(fout, "  CURRENTDEN %g ;\n",
                    curLayer.lefiLayer::currentDensityPoint());
    if(curLayer.lefiLayer::hasCurrentDensityArray()) {
      curLayer.lefiLayer::currentDensityArray(&numPoints, &widths, &current);
      for(i = 0; i < numPoints; i++)
      CIRCUIT_FPRINTF(fout, "  CURRENTDEN ( %g %g ) ;\n", widths[i],
                      current[i]);
    }
    if(curLayer.lefiLayer::hasDirection())
    CIRCUIT_FPRINTF(fout, "  DIRECTION %s ;\n",
                    curLayer.lefiLayer::direction());
    if(curLayer.lefiLayer::hasResistance())
    CIRCUIT_FPRINTF(fout, "  RESISTANCE RPERSQ %g ;\n",
                    curLayer.lefiLayer::resistance());
    if(curLayer.lefiLayer::hasCapacitance())
    CIRCUIT_FPRINTF(fout, "  CAPACITANCE CPERSQDIST %g ;\n",
                    curLayer.lefiLayer::capacitance());
    if(curLayer.lefiLayer::hasEdgeCap())
    CIRCUIT_FPRINTF(fout, "  EDGECAPACITANCE %g ;\n",
                    curLayer.lefiLayer::edgeCap());
    if(curLayer.lefiLayer::hasHeight())
    CIRCUIT_FPRINTF(fout, "  TYPE %g ;\n", curLayer.lefiLayer::height());
    if(curLayer.lefiLayer::hasThickness())
    CIRCUIT_FPRINTF(fout, "  THICKNESS %g ;\n",
                    curLayer.lefiLayer::thickness());
    if(curLayer.lefiLayer::hasWireExtension())
    CIRCUIT_FPRINTF(fout, "  WIREEXTENSION %g ;\n",
                    curLayer.lefiLayer::wireExtension());
    if(curLayer.lefiLayer::hasShrinkage())
    CIRCUIT_FPRINTF(fout, "  SHRINKAGE %g ;\n",
                    curLayer.lefiLayer::shrinkage());
    if(curLayer.lefiLayer::hasCapMultiplier())
    CIRCUIT_FPRINTF(fout, "  CAPMULTIPLIER %g ;\n",
                    curLayer.lefiLayer::capMultiplier());
    if(curLayer.lefiLayer::hasAntennaArea())
    CIRCUIT_FPRINTF(fout, "  ANTENNAAREAFACTOR %g ;\n",
                    curLayer.lefiLayer::antennaArea());
    if(curLayer.lefiLayer::hasAntennaLength())
    CIRCUIT_FPRINTF(fout, "  ANTENNALENGTHFACTOR %g ;\n",
                    curLayer.lefiLayer::antennaLength());

    // 5.5 AntennaModel
    for(i = 0; i < curLayer.lefiLayer::numAntennaModel(); i++) {
      aModel = curLayer.lefiLayer::antennaModel(i);

      CIRCUIT_FPRINTF(fout, "  ANTENNAMODEL %s ;\n",
                      aModel->lefiAntennaModel::antennaOxide());

      if(aModel->lefiAntennaModel::hasAntennaAreaRatio())
      CIRCUIT_FPRINTF(fout, "  ANTENNAAREARATIO %g ;\n",
                      aModel->lefiAntennaModel::antennaAreaRatio());
      if(aModel->lefiAntennaModel::hasAntennaDiffAreaRatio()) {
        CIRCUIT_FPRINTF(fout, "  ANTENNADIFFAREARATIO %g ;\n",
                        aModel->lefiAntennaModel::antennaDiffAreaRatio());
      }
      else if(aModel->lefiAntennaModel::hasAntennaDiffAreaRatioPWL()) {
        pwl = aModel->lefiAntennaModel::antennaDiffAreaRatioPWL();
        CIRCUIT_FPRINTF(fout, "  ANTENNADIFFAREARATIO PWL ( ");
        for(j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++) {
          CIRCUIT_FPRINTF(fout, "( %g %g ) ",
                          pwl->lefiAntennaPWL::PWLdiffusion(j),
                          pwl->lefiAntennaPWL::PWLratio(j));
        }
        CIRCUIT_FPRINTF(fout, ") ;\n");
      }
      if(aModel->lefiAntennaModel::hasAntennaCumAreaRatio())
      CIRCUIT_FPRINTF(fout, "  ANTENNACUMAREARATIO %g ;\n",
                      aModel->lefiAntennaModel::antennaCumAreaRatio());
      if(aModel->lefiAntennaModel::hasAntennaCumDiffAreaRatio())
      CIRCUIT_FPRINTF(fout, "  ANTENNACUMDIFFAREARATIO %g\n",
                      aModel->lefiAntennaModel::antennaCumDiffAreaRatio());
      if(aModel->lefiAntennaModel::hasAntennaCumDiffAreaRatioPWL()) {
        pwl = aModel->lefiAntennaModel::antennaCumDiffAreaRatioPWL();
        CIRCUIT_FPRINTF(fout, "  ANTENNACUMDIFFAREARATIO PWL ( ");
        for(j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
        CIRCUIT_FPRINTF(fout, "( %g %g ) ",
                        pwl->lefiAntennaPWL::PWLdiffusion(j),
                        pwl->lefiAntennaPWL::PWLratio(j));
        CIRCUIT_FPRINTF(fout, ") ;\n");
      }
      if(aModel->lefiAntennaModel::hasAntennaAreaFactor()) {
        CIRCUIT_FPRINTF(fout, "  ANTENNAAREAFACTOR %g ",
                        aModel->lefiAntennaModel::antennaAreaFactor());
        if(aModel->lefiAntennaModel::hasAntennaAreaFactorDUO())
        CIRCUIT_FPRINTF(fout, "  DIFFUSEONLY ");
        CIRCUIT_FPRINTF(fout, ";\n");
      }
      if(aModel->lefiAntennaModel::hasAntennaSideAreaRatio())
      CIRCUIT_FPRINTF(fout, "  ANTENNASIDEAREARATIO %g ;\n",
                      aModel->lefiAntennaModel::antennaSideAreaRatio());
      if(aModel->lefiAntennaModel::hasAntennaDiffSideAreaRatio()) {
        CIRCUIT_FPRINTF(fout, "  ANTENNADIFFSIDEAREARATIO %g\n",
                        aModel->lefiAntennaModel::antennaDiffSideAreaRatio());
      }
      else if(aModel->lefiAntennaModel::hasAntennaDiffSideAreaRatioPWL()) {
        pwl = aModel->lefiAntennaModel::antennaDiffSideAreaRatioPWL();
        CIRCUIT_FPRINTF(fout, "  ANTENNADIFFSIDEAREARATIO PWL ( ");
        for(j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
        CIRCUIT_FPRINTF(fout, "( %g %g ) ",
                        pwl->lefiAntennaPWL::PWLdiffusion(j),
                        pwl->lefiAntennaPWL::PWLratio(j));
        CIRCUIT_FPRINTF(fout, ") ;\n");
      }
      if(aModel->lefiAntennaModel::hasAntennaCumSideAreaRatio())
      CIRCUIT_FPRINTF(fout, "  ANTENNACUMSIDEAREARATIO %g ;\n",
                      aModel->lefiAntennaModel::antennaCumSideAreaRatio());
      if(aModel->lefiAntennaModel::hasAntennaCumDiffSideAreaRatio()) {
        CIRCUIT_FPRINTF(
            fout, "  ANTENNACUMDIFFSIDEAREARATIO %g\n",
            aModel->lefiAntennaModel::antennaCumDiffSideAreaRatio());
      }
      else if(aModel->lefiAntennaModel::hasAntennaCumDiffSideAreaRatioPWL()) {
        pwl = aModel->lefiAntennaModel::antennaCumDiffSideAreaRatioPWL();
        CIRCUIT_FPRINTF(fout, "  ANTENNACUMDIFFSIDEAREARATIO PWL ( ");
        for(j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
        CIRCUIT_FPRINTF(fout, "( %g %g ) ",
                        pwl->lefiAntennaPWL::PWLdiffusion(j),
                        pwl->lefiAntennaPWL::PWLratio(j));
        CIRCUIT_FPRINTF(fout, ") ;\n");
      }
      if(aModel->lefiAntennaModel::hasAntennaSideAreaFactor()) {
        CIRCUIT_FPRINTF(fout, "  ANTENNASIDEAREAFACTOR %g ",
                        aModel->lefiAntennaModel::antennaSideAreaFactor());
        if(aModel->lefiAntennaModel::hasAntennaSideAreaFactorDUO())
        CIRCUIT_FPRINTF(fout, "  DIFFUSEONLY ");
        CIRCUIT_FPRINTF(fout, ";\n");
      }
      if(aModel->lefiAntennaModel::hasAntennaCumRoutingPlusCut())
      CIRCUIT_FPRINTF(fout, "  ANTENNACUMROUTINGPLUSCUT ;\n");
      if(aModel->lefiAntennaModel::hasAntennaGatePlusDiff())
      CIRCUIT_FPRINTF(fout, "  ANTENNAGATEPLUSDIFF %g ;\n",
                      aModel->lefiAntennaModel::antennaGatePlusDiff());
      if(aModel->lefiAntennaModel::hasAntennaAreaMinusDiff())
      CIRCUIT_FPRINTF(fout, "  ANTENNAAREAMINUSDIFF %g ;\n",
                      aModel->lefiAntennaModel::antennaAreaMinusDiff());
      if(aModel->lefiAntennaModel::hasAntennaAreaDiffReducePWL()) {
        pwl = aModel->lefiAntennaModel::antennaAreaDiffReducePWL();
        CIRCUIT_FPRINTF(fout, "  ANTENNAAREADIFFREDUCEPWL ( ");
        for(j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
        CIRCUIT_FPRINTF(fout, "( %g %g ) ",
                        pwl->lefiAntennaPWL::PWLdiffusion(j),
                        pwl->lefiAntennaPWL::PWLratio(j));
        CIRCUIT_FPRINTF(fout, ") ;\n");
      }
    }

    if(curLayer.lefiLayer::numAccurrentDensity()) {
      for(i = 0; i < curLayer.lefiLayer::numAccurrentDensity(); i++) {
        density = curLayer.lefiLayer::accurrent(i);
        CIRCUIT_FPRINTF(fout, "  ACCURRENTDENSITY %s", density->type());
        if(density->hasOneEntry()) {
          CIRCUIT_FPRINTF(fout, " %g ;\n", density->oneEntry());
        }
        else {
          CIRCUIT_FPRINTF(fout, "\n");
          if(density->numFrequency()) {
            CIRCUIT_FPRINTF(fout, "    FREQUENCY");
            for(j = 0; j < density->numFrequency(); j++)
            CIRCUIT_FPRINTF(fout, " %g", density->frequency(j));
            CIRCUIT_FPRINTF(fout, " ;\n");
          }
          if(density->numCutareas()) {
            CIRCUIT_FPRINTF(fout, "    CUTAREA");
            for(j = 0; j < density->numCutareas(); j++)
            CIRCUIT_FPRINTF(fout, " %g", density->cutArea(j));
            CIRCUIT_FPRINTF(fout, " ;\n");
          }
          if(density->numWidths()) {
            CIRCUIT_FPRINTF(fout, "    WIDTH");
            for(j = 0; j < density->numWidths(); j++)
            CIRCUIT_FPRINTF(fout, " %g", density->width(j));
            CIRCUIT_FPRINTF(fout, " ;\n");
          }
          if(density->numTableEntries()) {
            k = 5;
            CIRCUIT_FPRINTF(fout, "    TABLEENTRIES");
            for(j = 0; j < density->numTableEntries(); j++)
              if(k > 4) {
                CIRCUIT_FPRINTF(fout, "\n     %g", density->tableEntry(j));
                k = 1;
              }
              else {
                CIRCUIT_FPRINTF(fout, " %g", density->tableEntry(j));
                k++;
              }
            CIRCUIT_FPRINTF(fout, " ;\n");
          }
        }
      }
    }
    if(curLayer.lefiLayer::numDccurrentDensity()) {
      for(i = 0; i < curLayer.lefiLayer::numDccurrentDensity(); i++) {
        density = curLayer.lefiLayer::dccurrent(i);
        CIRCUIT_FPRINTF(fout, "  DCCURRENTDENSITY %s", density->type());
        if(density->hasOneEntry()) {
          CIRCUIT_FPRINTF(fout, " %g ;\n", density->oneEntry());
        }
        else {
          CIRCUIT_FPRINTF(fout, "\n");
          if(density->numCutareas()) {
            CIRCUIT_FPRINTF(fout, "    CUTAREA");
            for(j = 0; j < density->numCutareas(); j++)
            CIRCUIT_FPRINTF(fout, " %g", density->cutArea(j));
            CIRCUIT_FPRINTF(fout, " ;\n");
          }
          if(density->numWidths()) {
            CIRCUIT_FPRINTF(fout, "    WIDTH");
            for(j = 0; j < density->numWidths(); j++)
            CIRCUIT_FPRINTF(fout, " %g", density->width(j));
            CIRCUIT_FPRINTF(fout, " ;\n");
          }
          if(density->numTableEntries()) {
            CIRCUIT_FPRINTF(fout, "    TABLEENTRIES");
            for(j = 0; j < density->numTableEntries(); j++)
            CIRCUIT_FPRINTF(fout, " %g", density->tableEntry(j));
            CIRCUIT_FPRINTF(fout, " ;\n");
          }
        }
      }
    }

    for(i = 0; i < curLayer.lefiLayer::numSpacingTable(); i++) {
      spTable = curLayer.lefiLayer::spacingTable(i);
      CIRCUIT_FPRINTF(fout, "   SPACINGTABLE\n");
      if(spTable->lefiSpacingTable::isInfluence()) {
        influence = spTable->lefiSpacingTable::influence();
        CIRCUIT_FPRINTF(fout, "      INFLUENCE");
        for(j = 0; j < influence->lefiInfluence::numInfluenceEntry(); j++) {
          CIRCUIT_FPRINTF(fout, "\n          WIDTH %g WITHIN %g SPACING %g",
                          influence->lefiInfluence::width(j),
                          influence->lefiInfluence::distance(j),
                          influence->lefiInfluence::spacing(j));
        }
        CIRCUIT_FPRINTF(fout, " ;\n");
      }
      else if(spTable->lefiSpacingTable::isParallel()) {
        parallel = spTable->lefiSpacingTable::parallel();
        CIRCUIT_FPRINTF(fout, "      PARALLELRUNLENGTH");
        for(j = 0; j < parallel->lefiParallel::numLength(); j++) {
          CIRCUIT_FPRINTF(fout, " %g", parallel->lefiParallel::length(j));
        }
        for(j = 0; j < parallel->lefiParallel::numWidth(); j++) {
          CIRCUIT_FPRINTF(fout, "\n          WIDTH %g",
                          parallel->lefiParallel::width(j));
          for(k = 0; k < parallel->lefiParallel::numLength(); k++) {
            CIRCUIT_FPRINTF(fout, " %g",
                            parallel->lefiParallel::widthSpacing(j, k));
          }
        }
        CIRCUIT_FPRINTF(fout, " ;\n");
      }
      else {  // 5.7 TWOWIDTHS
        twoWidths = spTable->lefiSpacingTable::twoWidths();
        CIRCUIT_FPRINTF(fout, "      TWOWIDTHS");
        for(j = 0; j < twoWidths->lefiTwoWidths::numWidth(); j++) {
          CIRCUIT_FPRINTF(fout, "\n          WIDTH %g ",
                          twoWidths->lefiTwoWidths::width(j));
          if(twoWidths->lefiTwoWidths::hasWidthPRL(j))
          CIRCUIT_FPRINTF(fout, "PRL %g ",
                          twoWidths->lefiTwoWidths::widthPRL(j));
          for(k = 0; k < twoWidths->lefiTwoWidths::numWidthSpacing(j); k++)
          CIRCUIT_FPRINTF(fout, "%g ",
                          twoWidths->lefiTwoWidths::widthSpacing(j, k));
        }
        CIRCUIT_FPRINTF(fout, " ;\n");
      }
    }

    propNum = curLayer.lefiLayer::numProps();
    if(propNum > 0) {
      CIRCUIT_FPRINTF(fout, "  PROPERTY ");
      for(i = 0; i < propNum; i++) {
        // value can either be a string or number
        CIRCUIT_FPRINTF(fout, "%s ", curLayer.lefiLayer::propName(i));
        if(curLayer.lefiLayer::propIsNumber(i))
        CIRCUIT_FPRINTF(fout, "%g ", curLayer.lefiLayer::propNumber(i));
        if(curLayer.lefiLayer::propIsString(i))
        CIRCUIT_FPRINTF(fout, "%s ", curLayer.lefiLayer::propValue(i));
        pType = curLayer.lefiLayer::propType(i);
        switch(pType) {
          case 'R':
          CIRCUIT_FPRINTF(fout, "REAL ");
            break;
          case 'I':
          CIRCUIT_FPRINTF(fout, "INTEGER ");
            break;
          case 'S':
          CIRCUIT_FPRINTF(fout, "STRING ");
            break;
          case 'Q':
          CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
            break;
          case 'N':
          CIRCUIT_FPRINTF(fout, "NUMBER ");
            break;
        }
      }
      CIRCUIT_FPRINTF(fout, ";\n");
    }
    if(curLayer.lefiLayer::hasDiagMinEdgeLength())
    CIRCUIT_FPRINTF(fout, "  DIAGMINEDGELENGTH %g ;\n",
                    curLayer.lefiLayer::diagMinEdgeLength());
    if(curLayer.lefiLayer::numMinSize()) {
      CIRCUIT_FPRINTF(fout, "  MINSIZE ");
      for(i = 0; i < curLayer.lefiLayer::numMinSize(); i++) {
        CIRCUIT_FPRINTF(fout, "%g %g ", curLayer.lefiLayer::minSizeWidth(i),
                        curLayer.lefiLayer::minSizeLength(i));
      }
      CIRCUIT_FPRINTF(fout, ";\n");
    }

    CIRCUIT_FPRINTF(fout, "END %s\n\n", curLayer.lefiLayer::name());
  }
  // Set it to case sensitive from here on
  lefrSetCaseSensitivity(1);
}

void ePlace::Parser::DumpLefSite() {
  if(lefSiteStor.size() == 0) {
    return;
  }

  int hasPrtSym = 0;
  for(auto& curSite : lefSiteStor) {
    CIRCUIT_FPRINTF(fout, "SITE %s\n", curSite.lefiSite::name());
    if(curSite.lefiSite::hasClass())
    CIRCUIT_FPRINTF(fout, "  CLASS %s ;\n", curSite.lefiSite::siteClass());
    if(curSite.lefiSite::hasXSymmetry()) {
      CIRCUIT_FPRINTF(fout, "  SYMMETRY X ");
      hasPrtSym = 1;
    }
    if(curSite.lefiSite::hasYSymmetry()) {
      if(hasPrtSym) {
        CIRCUIT_FPRINTF(fout, "Y ");
      }
      else {
        CIRCUIT_FPRINTF(fout, "  SYMMETRY Y ");
        hasPrtSym = 1;
      }
    }
    if(curSite.lefiSite::has90Symmetry()) {
      if(hasPrtSym) {
        CIRCUIT_FPRINTF(fout, "R90 ");
      }
      else {
        CIRCUIT_FPRINTF(fout, "  SYMMETRY R90 ");
        hasPrtSym = 1;
      }
    }
    if(hasPrtSym)
    CIRCUIT_FPRINTF(fout, ";\n");
    if(curSite.lefiSite::hasSize())
    CIRCUIT_FPRINTF(fout, "  SIZE %g BY %g ;\n", curSite.lefiSite::sizeX(),
                    curSite.lefiSite::sizeY());

    if(curSite.hasRowPattern()) {
      CIRCUIT_FPRINTF(fout, "  ROWPATTERN ");
      for(int i = 0; i < curSite.lefiSite::numSites(); i++)
      CIRCUIT_FPRINTF(fout, "  %s %s ", curSite.lefiSite::siteName(i),
                      curSite.lefiSite::siteOrientStr(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }

    CIRCUIT_FPRINTF(fout, "END %s\n\n", curSite.lefiSite::name());
  }
}

void ePlace::Parser::DumpLefVia() {
  if(lefViaStor.size() == 0) {
    return;
  }

  for(auto& curVia : lefViaStor) {
    lefVia(&curVia);
  }
}

void ePlace::Parser::DumpLefMacro() {
  lefiSitePattern* pattern;
  int propNum, i, hasPrtSym = 0;

  for(auto& curMacro : lefMacroStor) {
    //        cout << "MACRO " << curMacro.lefiMacro::name() << endl;
    CIRCUIT_FPRINTF(fout, "MACRO %s\n", curMacro.lefiMacro::name());
    // if ((long)ud != userData) dataError();
    if(curMacro.lefiMacro::hasClass())
    CIRCUIT_FPRINTF(fout, "  CLASS %s ;\n", curMacro.lefiMacro::macroClass());
    if(curMacro.lefiMacro::isFixedMask())
    CIRCUIT_FPRINTF(fout, "  FIXEDMASK ;\n");
    if(curMacro.lefiMacro::hasEEQ())
    CIRCUIT_FPRINTF(fout, "  EEQ %s ;\n", curMacro.lefiMacro::EEQ());
    if(curMacro.lefiMacro::hasLEQ())
    CIRCUIT_FPRINTF(fout, "  LEQ %s ;\n", curMacro.lefiMacro::LEQ());
    if(curMacro.lefiMacro::hasSource())
    CIRCUIT_FPRINTF(fout, "  SOURCE %s ;\n", curMacro.lefiMacro::source());
    if(curMacro.lefiMacro::hasXSymmetry()) {
      CIRCUIT_FPRINTF(fout, "  SYMMETRY X ");
      hasPrtSym = 1;
    }
    if(curMacro.lefiMacro::hasYSymmetry()) {  // print X Y & R90 in one line
      if(!hasPrtSym) {
        CIRCUIT_FPRINTF(fout, "  SYMMETRY Y ");
        hasPrtSym = 1;
      }
      else
      CIRCUIT_FPRINTF(fout, "Y ");
    }
    if(curMacro.lefiMacro::has90Symmetry()) {
      if(!hasPrtSym) {
        CIRCUIT_FPRINTF(fout, "  SYMMETRY R90 ");
        hasPrtSym = 1;
      }
      else
      CIRCUIT_FPRINTF(fout, "R90 ");
    }
    if(hasPrtSym) {
      CIRCUIT_FPRINTF(fout, ";\n");
      hasPrtSym = 0;
    }
    if(curMacro.lefiMacro::hasSiteName())
    CIRCUIT_FPRINTF(fout, "  SITE %s ;\n", curMacro.lefiMacro::siteName());
    if(curMacro.lefiMacro::hasSitePattern()) {
      for(i = 0; i < curMacro.lefiMacro::numSitePattern(); i++) {
        pattern = curMacro.lefiMacro::sitePattern(i);
        if(pattern->lefiSitePattern::hasStepPattern()) {
          CIRCUIT_FPRINTF(fout, "  SITE %s %g %g %s DO %g BY %g STEP %g %g ;\n",
                          pattern->lefiSitePattern::name(),
                          pattern->lefiSitePattern::x(),
                          pattern->lefiSitePattern::y(),
                          orientStr(pattern->lefiSitePattern::orient()),
                          pattern->lefiSitePattern::xStart(),
                          pattern->lefiSitePattern::yStart(),
                          pattern->lefiSitePattern::xStep(),
                          pattern->lefiSitePattern::yStep());
        }
        else {
          CIRCUIT_FPRINTF(
              fout, "  SITE %s %g %g %s ;\n", pattern->lefiSitePattern::name(),
              pattern->lefiSitePattern::x(), pattern->lefiSitePattern::y(),
              orientStr(pattern->lefiSitePattern::orient()));
        }
      }
    }
    if(curMacro.lefiMacro::hasSize())
    CIRCUIT_FPRINTF(fout, "  SIZE %g BY %g ;\n", curMacro.lefiMacro::sizeX(),
                    curMacro.lefiMacro::sizeY());

    if(curMacro.lefiMacro::hasForeign()) {
      for(i = 0; i < curMacro.lefiMacro::numForeigns(); i++) {
        CIRCUIT_FPRINTF(fout, "  FOREIGN %s ",
                        curMacro.lefiMacro::foreignName(i));
        if(curMacro.lefiMacro::hasForeignPoint(i)) {
          CIRCUIT_FPRINTF(fout, "( %g %g ) ", curMacro.lefiMacro::foreignX(i),
                          curMacro.lefiMacro::foreignY(i));
          if(curMacro.lefiMacro::hasForeignOrient(i))
          CIRCUIT_FPRINTF(fout, "%s ",
                          curMacro.lefiMacro::foreignOrientStr(i));
        }
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
    if(curMacro.lefiMacro::hasOrigin())
    CIRCUIT_FPRINTF(fout, "  ORIGIN ( %g %g ) ;\n",
                    curMacro.lefiMacro::originX(),
                    curMacro.lefiMacro::originY());
    if(curMacro.lefiMacro::hasPower())
    CIRCUIT_FPRINTF(fout, "  POWER %g ;\n", curMacro.lefiMacro::power());
    propNum = curMacro.lefiMacro::numProperties();
    if(propNum > 0) {
      CIRCUIT_FPRINTF(fout, "  PROPERTY ");
      for(i = 0; i < propNum; i++) {
        // value can either be a string or number
        if(curMacro.lefiMacro::propValue(i)) {
          CIRCUIT_FPRINTF(fout, "%s %s ", curMacro.lefiMacro::propName(i),
                          curMacro.lefiMacro::propValue(i));
        }
        else
        CIRCUIT_FPRINTF(fout, "%s %g ", curMacro.lefiMacro::propName(i),
                        curMacro.lefiMacro::propNum(i));

        switch(curMacro.lefiMacro::propType(i)) {
          case 'R':
          CIRCUIT_FPRINTF(fout, "REAL ");
            break;
          case 'I':
          CIRCUIT_FPRINTF(fout, "INTEGER ");
            break;
          case 'S':
          CIRCUIT_FPRINTF(fout, "STRING ");
            break;
          case 'Q':
          CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
            break;
          case 'N':
          CIRCUIT_FPRINTF(fout, "NUMBER ");
            break;
        }
      }
      CIRCUIT_FPRINTF(fout, ";\n");
    }
    fflush(stdout);

    // curPin.first : pinName
    // curPin.second : lefPinStor's index
    for(auto& curPin : lefPinStor[&curMacro - &lefMacroStor[0]]) {
      DumpLefPin(&curPin);
    }

    for(auto& curObs : lefObsStor[&curMacro - &lefMacroStor[0]]) {
      DumpLefObs(&curObs);
    }

    CIRCUIT_FPRINTF(fout, "END %s\n\n", curMacro.lefiMacro::name());
  }
}

void ePlace::Parser::DumpLefDone() {
  CIRCUIT_FPRINTF(fout, "END LIBRARY\n");
}

// below is helper function
void ePlace::Parser::DumpLefObs(lefiObstruction* obs) {
  CIRCUIT_FPRINTF(fout, "  OBS\n");
  lefiGeometries* geometry = obs->lefiObstruction::geometries();
  prtGeometry(geometry);
  CIRCUIT_FPRINTF(fout, "  END\n");
}

void ePlace::Parser::DumpLefPin(lefiPin* pin) {
  int numPorts, i, j;
  lefiGeometries* geometry;
  lefiPinAntennaModel* aModel;

  CIRCUIT_FPRINTF(fout, "  PIN %s\n", pin->lefiPin::name());
  if(pin->lefiPin::hasForeign()) {
    for(i = 0; i < pin->lefiPin::numForeigns(); i++) {
      if(pin->lefiPin::hasForeignOrient(i)) {
        CIRCUIT_FPRINTF(fout, "    FOREIGN %s STRUCTURE ( %g %g ) %s ;\n",
                        pin->lefiPin::foreignName(i), pin->lefiPin::foreignX(i),
                        pin->lefiPin::foreignY(i),
                        pin->lefiPin::foreignOrientStr(i));
      }
      else if(pin->lefiPin::hasForeignPoint(i)) {
        CIRCUIT_FPRINTF(fout, "    FOREIGN %s STRUCTURE ( %g %g ) ;\n",
                        pin->lefiPin::foreignName(i), pin->lefiPin::foreignX(i),
                        pin->lefiPin::foreignY(i));
      }
      else {
        CIRCUIT_FPRINTF(fout, "    FOREIGN %s ;\n",
                        pin->lefiPin::foreignName(i));
      }
    }
  }
  if(pin->lefiPin::hasLEQ())
  CIRCUIT_FPRINTF(fout, "    LEQ %s ;\n", pin->lefiPin::LEQ());
  if(pin->lefiPin::hasDirection())
  CIRCUIT_FPRINTF(fout, "    DIRECTION %s ;\n", pin->lefiPin::direction());
  if(pin->lefiPin::hasUse())
  CIRCUIT_FPRINTF(fout, "    USE %s ;\n", pin->lefiPin::use());
  if(pin->lefiPin::hasShape())
  CIRCUIT_FPRINTF(fout, "    SHAPE %s ;\n", pin->lefiPin::shape());
  if(pin->lefiPin::hasMustjoin())
  CIRCUIT_FPRINTF(fout, "    MUSTJOIN %s ;\n", pin->lefiPin::mustjoin());
  if(pin->lefiPin::hasOutMargin())
  CIRCUIT_FPRINTF(fout, "    OUTPUTNOISEMARGIN %g %g ;\n",
                  pin->lefiPin::outMarginHigh(),
                  pin->lefiPin::outMarginLow());
  if(pin->lefiPin::hasOutResistance())
  CIRCUIT_FPRINTF(fout, "    OUTPUTRESISTANCE %g %g ;\n",
                  pin->lefiPin::outResistanceHigh(),
                  pin->lefiPin::outResistanceLow());
  if(pin->lefiPin::hasInMargin())
  CIRCUIT_FPRINTF(fout, "    INPUTNOISEMARGIN %g %g ;\n",
                  pin->lefiPin::inMarginHigh(), pin->lefiPin::inMarginLow());
  if(pin->lefiPin::hasPower())
  CIRCUIT_FPRINTF(fout, "    POWER %g ;\n", pin->lefiPin::power());
  if(pin->lefiPin::hasLeakage())
  CIRCUIT_FPRINTF(fout, "    LEAKAGE %g ;\n", pin->lefiPin::leakage());
  if(pin->lefiPin::hasMaxload())
  CIRCUIT_FPRINTF(fout, "    MAXLOAD %g ;\n", pin->lefiPin::maxload());
  if(pin->lefiPin::hasCapacitance())
  CIRCUIT_FPRINTF(fout, "    CAPACITANCE %g ;\n",
                  pin->lefiPin::capacitance());
  if(pin->lefiPin::hasResistance())
  CIRCUIT_FPRINTF(fout, "    RESISTANCE %g ;\n", pin->lefiPin::resistance());
  if(pin->lefiPin::hasPulldownres())
  CIRCUIT_FPRINTF(fout, "    PULLDOWNRES %g ;\n",
                  pin->lefiPin::pulldownres());
  if(pin->lefiPin::hasTieoffr())
  CIRCUIT_FPRINTF(fout, "    TIEOFFR %g ;\n", pin->lefiPin::tieoffr());
  if(pin->lefiPin::hasVHI())
  CIRCUIT_FPRINTF(fout, "    VHI %g ;\n", pin->lefiPin::VHI());
  if(pin->lefiPin::hasVLO())
  CIRCUIT_FPRINTF(fout, "    VLO %g ;\n", pin->lefiPin::VLO());
  if(pin->lefiPin::hasRiseVoltage())
  CIRCUIT_FPRINTF(fout, "    RISEVOLTAGETHRESHOLD %g ;\n",
                  pin->lefiPin::riseVoltage());
  if(pin->lefiPin::hasFallVoltage())
  CIRCUIT_FPRINTF(fout, "    FALLVOLTAGETHRESHOLD %g ;\n",
                  pin->lefiPin::fallVoltage());
  if(pin->lefiPin::hasRiseThresh())
  CIRCUIT_FPRINTF(fout, "    RISETHRESH %g ;\n", pin->lefiPin::riseThresh());
  if(pin->lefiPin::hasFallThresh())
  CIRCUIT_FPRINTF(fout, "    FALLTHRESH %g ;\n", pin->lefiPin::fallThresh());
  if(pin->lefiPin::hasRiseSatcur())
  CIRCUIT_FPRINTF(fout, "    RISESATCUR %g ;\n", pin->lefiPin::riseSatcur());
  if(pin->lefiPin::hasFallSatcur())
  CIRCUIT_FPRINTF(fout, "    FALLSATCUR %g ;\n", pin->lefiPin::fallSatcur());
  if(pin->lefiPin::hasRiseSlewLimit())
  CIRCUIT_FPRINTF(fout, "    RISESLEWLIMIT %g ;\n",
                  pin->lefiPin::riseSlewLimit());
  if(pin->lefiPin::hasFallSlewLimit())
  CIRCUIT_FPRINTF(fout, "    FALLSLEWLIMIT %g ;\n",
                  pin->lefiPin::fallSlewLimit());
  if(pin->lefiPin::hasCurrentSource())
  CIRCUIT_FPRINTF(fout, "    CURRENTSOURCE %s ;\n",
                  pin->lefiPin::currentSource());
  if(pin->lefiPin::hasTables())
  CIRCUIT_FPRINTF(fout, "    IV_TABLES %s %s ;\n",
                  pin->lefiPin::tableHighName(),
                  pin->lefiPin::tableLowName());
  if(pin->lefiPin::hasTaperRule())
  CIRCUIT_FPRINTF(fout, "    TAPERRULE %s ;\n", pin->lefiPin::taperRule());
  if(pin->lefiPin::hasNetExpr())
  CIRCUIT_FPRINTF(fout, "    NETEXPR \"%s\" ;\n", pin->lefiPin::netExpr());
  if(pin->lefiPin::hasSupplySensitivity())
  CIRCUIT_FPRINTF(fout, "    SUPPLYSENSITIVITY %s ;\n",
                  pin->lefiPin::supplySensitivity());
  if(pin->lefiPin::hasGroundSensitivity())
  CIRCUIT_FPRINTF(fout, "    GROUNDSENSITIVITY %s ;\n",
                  pin->lefiPin::groundSensitivity());
  if(pin->lefiPin::hasAntennaSize()) {
    for(i = 0; i < pin->lefiPin::numAntennaSize(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNASIZE %g ",
                      pin->lefiPin::antennaSize(i));
      if(pin->lefiPin::antennaSizeLayer(i))
      CIRCUIT_FPRINTF(fout, "LAYER %s ", pin->lefiPin::antennaSizeLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }
  if(pin->lefiPin::hasAntennaMetalArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaMetalArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAMETALAREA %g ",
                      pin->lefiPin::antennaMetalArea(i));
      if(pin->lefiPin::antennaMetalAreaLayer(i))
      CIRCUIT_FPRINTF(fout, "LAYER %s ",
                      pin->lefiPin::antennaMetalAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }
  if(pin->lefiPin::hasAntennaMetalLength()) {
    for(i = 0; i < pin->lefiPin::numAntennaMetalLength(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAMETALLENGTH %g ",
                      pin->lefiPin::antennaMetalLength(i));
      if(pin->lefiPin::antennaMetalLengthLayer(i))
      CIRCUIT_FPRINTF(fout, "LAYER %s ",
                      pin->lefiPin::antennaMetalLengthLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaPartialMetalArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaPartialMetalArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAPARTIALMETALAREA %g ",
                      pin->lefiPin::antennaPartialMetalArea(i));
      if(pin->lefiPin::antennaPartialMetalAreaLayer(i))
      CIRCUIT_FPRINTF(fout, "LAYER %s ",
                      pin->lefiPin::antennaPartialMetalAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaPartialMetalSideArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaPartialMetalSideArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAPARTIALMETALSIDEAREA %g ",
                      pin->lefiPin::antennaPartialMetalSideArea(i));
      if(pin->lefiPin::antennaPartialMetalSideAreaLayer(i))
      CIRCUIT_FPRINTF(fout, "LAYER %s ",
                      pin->lefiPin::antennaPartialMetalSideAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaPartialCutArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaPartialCutArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNAPARTIALCUTAREA %g ",
                      pin->lefiPin::antennaPartialCutArea(i));
      if(pin->lefiPin::antennaPartialCutAreaLayer(i))
      CIRCUIT_FPRINTF(fout, "LAYER %s ",
                      pin->lefiPin::antennaPartialCutAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(pin->lefiPin::hasAntennaDiffArea()) {
    for(i = 0; i < pin->lefiPin::numAntennaDiffArea(); i++) {
      CIRCUIT_FPRINTF(fout, "    ANTENNADIFFAREA %g ",
                      pin->lefiPin::antennaDiffArea(i));
      if(pin->lefiPin::antennaDiffAreaLayer(i))
      CIRCUIT_FPRINTF(fout, "LAYER %s ",
                      pin->lefiPin::antennaDiffAreaLayer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  for(j = 0; j < pin->lefiPin::numAntennaModel(); j++) {
    aModel = pin->lefiPin::antennaModel(j);

    CIRCUIT_FPRINTF(fout, "    ANTENNAMODEL %s ;\n",
                    aModel->lefiPinAntennaModel::antennaOxide());

    if(aModel->lefiPinAntennaModel::hasAntennaGateArea()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaGateArea(); i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAGATEAREA %g ",
                        aModel->lefiPinAntennaModel::antennaGateArea(i));
        if(aModel->lefiPinAntennaModel::antennaGateAreaLayer(i))
        CIRCUIT_FPRINTF(fout, "LAYER %s ",
                        aModel->lefiPinAntennaModel::antennaGateAreaLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }

    if(aModel->lefiPinAntennaModel::hasAntennaMaxAreaCar()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaMaxAreaCar(); i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAMAXAREACAR %g ",
                        aModel->lefiPinAntennaModel::antennaMaxAreaCar(i));
        if(aModel->lefiPinAntennaModel::antennaMaxAreaCarLayer(i))
        CIRCUIT_FPRINTF(
            fout, "LAYER %s ",
            aModel->lefiPinAntennaModel::antennaMaxAreaCarLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }

    if(aModel->lefiPinAntennaModel::hasAntennaMaxSideAreaCar()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaMaxSideAreaCar();
          i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAMAXSIDEAREACAR %g ",
                        aModel->lefiPinAntennaModel::antennaMaxSideAreaCar(i));
        if(aModel->lefiPinAntennaModel::antennaMaxSideAreaCarLayer(i))
        CIRCUIT_FPRINTF(
            fout, "LAYER %s ",
            aModel->lefiPinAntennaModel::antennaMaxSideAreaCarLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }

    if(aModel->lefiPinAntennaModel::hasAntennaMaxCutCar()) {
      for(i = 0; i < aModel->lefiPinAntennaModel::numAntennaMaxCutCar(); i++) {
        CIRCUIT_FPRINTF(fout, "    ANTENNAMAXCUTCAR %g ",
                        aModel->lefiPinAntennaModel::antennaMaxCutCar(i));
        if(aModel->lefiPinAntennaModel::antennaMaxCutCarLayer(i))
        CIRCUIT_FPRINTF(
            fout, "LAYER %s ",
            aModel->lefiPinAntennaModel::antennaMaxCutCarLayer(i));
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
  }

  if(pin->lefiPin::numProperties() > 0) {
    CIRCUIT_FPRINTF(fout, "    PROPERTY ");
    for(i = 0; i < pin->lefiPin::numProperties(); i++) {
      // value can either be a string or number
      if(pin->lefiPin::propValue(i)) {
        CIRCUIT_FPRINTF(fout, "%s %s ", pin->lefiPin::propName(i),
                        pin->lefiPin::propValue(i));
      }
      else {
        CIRCUIT_FPRINTF(fout, "%s %g ", pin->lefiPin::propName(i),
                        pin->lefiPin::propNum(i));
      }
      switch(pin->lefiPin::propType(i)) {
        case 'R':
        CIRCUIT_FPRINTF(fout, "REAL ");
          break;
        case 'I':
        CIRCUIT_FPRINTF(fout, "INTEGER ");
          break;
        case 'S':
        CIRCUIT_FPRINTF(fout, "STRING ");
          break;
        case 'Q':
        CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
          break;
        case 'N':
        CIRCUIT_FPRINTF(fout, "NUMBER ");
          break;
      }
    }
    CIRCUIT_FPRINTF(fout, ";\n");
  }

  numPorts = pin->lefiPin::numPorts();
  for(i = 0; i < numPorts; i++) {
    CIRCUIT_FPRINTF(fout, "    PORT\n");
    fflush(stdout);
    geometry = pin->lefiPin::port(i);
    prtGeometry(geometry);
    CIRCUIT_FPRINTF(fout, "    END\n");
  }
  CIRCUIT_FPRINTF(fout, "  END %s\n", pin->lefiPin::name());
  fflush(stdout);
}

/*
int ePlace::Parser::ParseLef(vector< string >& lefStor,
                               bool isVerbose)  {
  //    char* outFile;

  FILE* f;
  int res;
  //    int noCalls = 0;

  //  long start_mem;
  //    int num;
  int status;
  int retStr = 0;
  //    int numInFile = 0;
  //    int fileCt = 0;
  int relax = 0;
  //    const char* version = "N/A";
  //    int setVer = 0;
  //    int msgCb = 0;

  // start_mem = (long)sbrk(0);

  char* userData = strdup("(lefrw-5100)");

  fout = stdout;

  lefVersionPtr = &(this->lefVersion);
  lefDividerPtr = &(this->lefDivider);
  lefBusBitCharPtr = &(this->lefBusBitChar);

  lefUnitPtr = &(this->lefUnit);
  lefManufacturingGridPtr = &(this->lefManufacturingGrid);

  lefLayerStorPtr = &(this->lefLayerStor);
  lefSiteStorPtr = &(this->lefSiteStor);
  lefMacroStorPtr = &(this->lefMacroStor);
  lefViaStorPtr = &(this->lefViaStor);

  // vector of vector
  lefPinStorPtr = &(this->lefPinStor);
  lefObsStorPtr = &(this->lefObsStor);

  lefPinMapStorPtr = &(this->lefPinMapStor);

  //    lefMacroToPinPtr = &(this->lefMacroToPin);

  lefMacroMapPtr = &(this->lefMacroMap);
  lefViaMapPtr = &(this->lefViaMap);
  lefLayerMapPtr = &(this->lefLayerMap);
  lefSiteMapPtr = &(this->lefSiteMap);

  // sets the parser to be case sensitive...
  // default was supposed to be the case but false...
  // lefrSetCaseSensitivity(true);

  lefrInitSession(1);

  lefrSetWarningLogFunction(printWarning);
  lefrSetAntennaInputCbk(antennaCB);
  lefrSetAntennaInoutCbk(antennaCB);
  lefrSetAntennaOutputCbk(antennaCB);
  lefrSetArrayBeginCbk(arrayBeginCB);
  lefrSetArrayCbk(arrayCB);
  lefrSetArrayEndCbk(arrayEndCB);
  lefrSetBusBitCharsCbk(busBitCharsCB);
  lefrSetCaseSensitiveCbk(caseSensCB);
  lefrSetFixedMaskCbk(fixedMaskCB);
  lefrSetClearanceMeasureCbk(clearanceCB);
  lefrSetDensityCbk(densityCB);
  lefrSetDividerCharCbk(dividerCB);
  lefrSetNoWireExtensionCbk(noWireExtCB);
  lefrSetNoiseMarginCbk(noiseMarCB);
  lefrSetEdgeRateThreshold1Cbk(edge1CB);
  lefrSetEdgeRateThreshold2Cbk(edge2CB);
  lefrSetEdgeRateScaleFactorCbk(edgeScaleCB);
  lefrSetExtensionCbk(extensionCB);
  lefrSetNoiseTableCbk(noiseTableCB);
  lefrSetCorrectionTableCbk(correctionCB);
  lefrSetDielectricCbk(dielectricCB);
  lefrSetIRDropBeginCbk(irdropBeginCB);
  lefrSetIRDropCbk(irdropCB);
  lefrSetIRDropEndCbk(irdropEndCB);
  lefrSetLayerCbk(layerCB);
  lefrSetLibraryEndCbk(doneCB);
  lefrSetMacroBeginCbk(macroBeginCB);
  lefrSetMacroCbk(macroCB);
  lefrSetMacroClassTypeCbk(macroClassTypeCB);
  lefrSetMacroOriginCbk(macroOriginCB);
  lefrSetMacroSizeCbk(macroSizeCB);
  lefrSetMacroFixedMaskCbk(macroFixedMaskCB);
  lefrSetMacroEndCbk(macroEndCB);
  lefrSetManufacturingCbk(manufacturingCB);
  lefrSetMaxStackViaCbk(maxStackViaCB);
  lefrSetMinFeatureCbk(minFeatureCB);
  lefrSetNonDefaultCbk(nonDefaultCB);
  lefrSetObstructionCbk(obstructionCB);
  lefrSetPinCbk(pinCB);
  lefrSetPropBeginCbk(propDefBeginCB);
  lefrSetPropCbk(propDefCB);
  lefrSetPropEndCbk(propDefEndCB);
  lefrSetSiteCbk(siteCB);
  lefrSetSpacingBeginCbk(spacingBeginCB);
  lefrSetSpacingCbk(spacingCB);
  lefrSetSpacingEndCbk(spacingEndCB);
  lefrSetTimingCbk(timingCB);
  lefrSetUnitsCbk(unitsCB);
  lefrSetUseMinSpacingCbk(useMinSpacingCB);
  lefrSetUserData((void*)3);
  if(!retStr)
    lefrSetVersionCbk(versionCB);
  else
    lefrSetVersionStrCbk(versionStrCB);
  lefrSetViaCbk(viaCB);
  lefrSetViaRuleCbk(viaRuleCB);
  lefrSetInputAntennaCbk(antennaCB);
  lefrSetOutputAntennaCbk(antennaCB);
  lefrSetInoutAntennaCbk(antennaCB);

  //    if (msgCb) {
  //        lefrSetLogFunction(errorCB);
  //        lefrSetWarningLogFunction(warningCB);
  //    }

  lefrSetMallocFunction(mallocCB);
  lefrSetReallocFunction(reallocCB);
  lefrSetFreeFunction(freeCB);

  if(isVerbose) {
    lefrSetLineNumberFunction(lineNumberCB);
    lefrSetDeltaNumberLines(500);
  }

  lefrSetRegisterUnusedCallbacks();

  if(relax)
    lefrSetRelaxMode();

  //    if (setVer)
  //        (void)lefrSetVersionValue(version);

  lefrSetAntennaInoutWarnings(30);
  lefrSetAntennaInputWarnings(30);
  lefrSetAntennaOutputWarnings(30);
  lefrSetArrayWarnings(30);
  lefrSetCaseSensitiveWarnings(30);
  lefrSetCorrectionTableWarnings(30);
  lefrSetDielectricWarnings(30);
  lefrSetEdgeRateThreshold1Warnings(30);
  lefrSetEdgeRateThreshold2Warnings(30);
  lefrSetEdgeRateScaleFactorWarnings(30);
  lefrSetInoutAntennaWarnings(30);
  lefrSetInputAntennaWarnings(30);
  lefrSetIRDropWarnings(30);
  lefrSetLayerWarnings(30);
  lefrSetMacroWarnings(30);
  lefrSetMaxStackViaWarnings(30);
  lefrSetMinFeatureWarnings(30);
  lefrSetNoiseMarginWarnings(30);
  lefrSetNoiseTableWarnings(30);
  lefrSetNonDefaultWarnings(30);
  lefrSetNoWireExtensionWarnings(30);
  lefrSetOutputAntennaWarnings(30);
  lefrSetPinWarnings(30);
  lefrSetSiteWarnings(30);
  lefrSetSpacingWarnings(30);
  lefrSetTimingWarnings(30);
  lefrSetUnitsWarnings(30);
  lefrSetUseMinSpacingWarnings(30);
  lefrSetViaRuleWarnings(30);
  lefrSetViaWarnings(30);

  (void)lefrSetShiftCase();  // will shift name to uppercase if caseinsensitive

  // is set to off or not set
  lefrSetOpenLogFileAppend();

  for(auto& lefName : lefStor) {
    lefrReset();

    if((f = fopen(lefName.c_str(), "r")) == 0) {
      fprintf(stderr, "\n**ERROR: Couldn't open input file '%s'\n",
              lefName.c_str());
      exit(1);
    }

    (void)lefrEnableReadEncrypted();

    status = lefwInit(fout);  // initialize the lef writer,
    // need to be called 1st
    if(status != LEFW_OK)
      return 1;

    fout = NULL;
    res = lefrRead(f, lefName.c_str(), (void*)userData);

    if(res) {
      CIRCUIT_FPRINTF(stderr, "Reader returns bad status.\n", lefName.c_str());
      return res;
    }

    (void)lefrPrintUnusedCallbacks(fout);
    (void)lefrReleaseNResetMemory();
  }

  (void)lefrUnsetCallbacks();

  // Unset all the callbacks
  void lefrUnsetAntennaInputCbk();
  void lefrUnsetAntennaInoutCbk();
  void lefrUnsetAntennaOutputCbk();
  void lefrUnsetArrayBeginCbk();
  void lefrUnsetArrayCbk();
  void lefrUnsetArrayEndCbk();
  void lefrUnsetBusBitCharsCbk();
  void lefrUnsetCaseSensitiveCbk();
  void lefrUnsetFixedMaskCbk();
  void lefrUnsetClearanceMeasureCbk();
  void lefrUnsetCorrectionTableCbk();
  void lefrUnsetDensityCbk();
  void lefrUnsetDielectricCbk();
  void lefrUnsetDividerCharCbk();
  void lefrUnsetEdgeRateScaleFactorCbk();
  void lefrUnsetEdgeRateThreshold1Cbk();
  void lefrUnsetEdgeRateThreshold2Cbk();
  void lefrUnsetExtensionCbk();
  void lefrUnsetInoutAntennaCbk();
  void lefrUnsetInputAntennaCbk();
  void lefrUnsetIRDropBeginCbk();
  void lefrUnsetIRDropCbk();
  void lefrUnsetIRDropEndCbk();
  void lefrUnsetLayerCbk();
  void lefrUnsetLibraryEndCbk();
  void lefrUnsetMacroBeginCbk();
  void lefrUnsetMacroCbk();
  void lefrUnsetMacroClassTypeCbk();
  void lefrUnsetMacroEndCbk();
  void lefrUnsetMacroOriginCbk();
  void lefrUnsetMacroSizeCbk();
  void lefrUnsetManufacturingCbk();
  void lefrUnsetMaxStackViaCbk();
  void lefrUnsetMinFeatureCbk();
  void lefrUnsetNoiseMarginCbk();
  void lefrUnsetNoiseTableCbk();
  void lefrUnsetNonDefaultCbk();
  void lefrUnsetNoWireExtensionCbk();
  void lefrUnsetObstructionCbk();
  void lefrUnsetOutputAntennaCbk();
  void lefrUnsetPinCbk();
  void lefrUnsetPropBeginCbk();
  void lefrUnsetPropCbk();
  void lefrUnsetPropEndCbk();
  void lefrUnsetSiteCbk();
  void lefrUnsetSpacingBeginCbk();
  void lefrUnsetSpacingCbk();
  void lefrUnsetSpacingEndCbk();
  void lefrUnsetTimingCbk();
  void lefrUnsetUseMinSpacingCbk();
  void lefrUnsetUnitsCbk();
  void lefrUnsetVersionCbk();
  void lefrUnsetVersionStrCbk();
  void lefrUnsetViaCbk();
  void lefrUnsetViaRuleCbk();

  //    fclose(fout);

  // Release allocated singleton data.
  //    lefrClear();

  return res;
}
void ePlace::Parser::WriteLef(FILE* _fout) {
  fout = _fout;
  //    fout = stdout;

  DumpLefVersion();
  DumpLefBusBitChar();
  DumpLefDivider();

  DumpLefUnit();
  DumpLefManufacturingGrid();
  DumpLefLayer();
  DumpLefSite();
  DumpLefVia();

  //    DumpLefSite();

  DumpLefMacro();
  DumpLefDone();
  //    DumpLefPin();

  //    fflush(stdout);
}
*/


////////////////////////////////////////////////////////////////////////
///////////////////////////// Def Parser ///////////////////////////////
////////////////////////////////////////////////////////////////////////

static void* userData;
static int numObjs;
static int isSumSet;    // to keep track if within SUM
static int isProp = 0;  // for PROPERTYDEFINITIONS
static int
    begOperand;  // to keep track for constraint, to print - as the 1st char
static double curVer = 0;
static int setSNetWireCbk = 0;
static int ignoreRowNames = 0;
static int ignoreViaNames = 0;
static int testDebugPrint = 0;  // test for ccr1488696

static string* defVersionPtr = 0;
static string* defDividerCharPtr = 0;
static string* defBusBitCharPtr = 0;
static string* defDesignNamePtr = 0;

static bool isVersionVisit = false;
static bool isDividerCharVisit = false;
static bool isBusBitCharVisit = false;
static bool isDesignNameVisit = false;

static vector< defiProp >* defPropStorPtr = 0;

static defiBox* defDieAreaPtr = 0;
static vector< defiRow >* defRowStorPtr = 0;
static vector< defiTrack >* defTrackStorPtr = 0;
static vector< defiGcellGrid >* defGcellGridStorPtr = 0;
static vector< defiVia >* defViaStorPtr = 0;

static defiComponentMaskShiftLayer* defComponentMaskShiftLayerPtr = 0;
static vector< defiComponent >* defComponentStorPtr = 0;
static vector< defiNet >* defNetStorPtr = 0;
static vector< defiBlockage >* defBlockageStorPtr = 0;

static vector< defiNet >* defSpecialNetStorPtr = 0;
// 0 for SpecialNet, 1 for SpecialPartialPath
#define DEF_SPECIALNET_ORIGINAL 0
#define DEF_SPECIALNET_PARTIAL_PATH 1
static vector< int > defSpecialNetType;

static vector< defiPin >* defPinStorPtr = 0;

static double* defUnitPtr = 0;

static HASH_MAP< string, int >* defComponentMapPtr = 0;
static HASH_MAP< string, int >* defPinMapPtr = 0;
static HASH_MAP< int, int >* defRowY2OrientMapPtr = 0;

static vector< HASH_MAP< string, int > >* defComponentPinToNetPtr = 0;
static HASH_MAP< string, int >* currentPinMap = 0;



static void checkType(defrCallbackType_e c) {
  if(c >= 0 && c <= defrDesignEndCbkType) {
    // OK
  }
  else {
    CIRCUIT_FPRINTF(fout, "ERROR: callback type is out of bounds!\n");
  }
}

static int done(defrCallbackType_e c, void*, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "END DESIGN\n");
  return 0;
}

static int endfunc(defrCallbackType_e c, void*, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  return 0;
}


int compMSL(defrCallbackType_e c, defiComponentMaskShiftLayer* co,
            defiUserData ud) {
  int i;

  checkType(c);
  if(ud != userData)
    dataError();

  if(co->numMaskShiftLayers()) {
    *defComponentMaskShiftLayerPtr = *co;
    CIRCUIT_FPRINTF(fout, "COMPONENTMASKSHIFT ");

    for(i = 0; i < co->numMaskShiftLayers(); i++) {
      CIRCUIT_FPRINTF(fout, "%s ", co->maskShiftLayer(i));
    }
    CIRCUIT_FPRINTF(fout, ";\n");
  }

  return 0;
}

int compf(defrCallbackType_e c, defiComponent* co, defiUserData ud) {
  currentPinMap = new HASH_MAP< string, int >;
#ifdef USE_GOOGLE_HASH
  currentPinMap->set_empty_key(INIT_STR);
#endif

  (*defComponentMapPtr)[string(co->id())] = defComponentStorPtr->size();
  defComponentStorPtr->push_back(*co);
  defComponentPinToNetPtr->push_back(*currentPinMap);

  delete currentPinMap;
  currentPinMap = NULL;

  if(testDebugPrint) {
    co->print(fout);
  }
  else {
    int i;

    checkType(c);
    if(ud != userData)
      dataError();
    //  missing GENERATE, FOREIGN
    CIRCUIT_FPRINTF(fout, "- %s %s ", co->id(), co->name());
    //    co->changeIdAndName("idName", "modelName");
    //    CIRCUIT_FPRINTF(fout, "%s %s ", co->id(),
    //            co->name());
    if(co->hasNets()) {
      for(i = 0; i < co->numNets(); i++)
      CIRCUIT_FPRINTF(fout, "%s ", co->net(i));
    }
    if(co->isFixed())
    CIRCUIT_FPRINTF(fout, "+ FIXED %d %d %s ", co->placementX(),
                    co->placementY(),
    // orientStr(co->placementOrient()));
                    co->placementOrientStr());
    if(co->isCover())
    CIRCUIT_FPRINTF(fout, "+ COVER %d %d %s ", co->placementX(),
                    co->placementY(), orientStr(co->placementOrient()));
    if(co->isPlaced())
    CIRCUIT_FPRINTF(fout, "+ PLACED %d %d %s ", co->placementX(),
                    co->placementY(), orientStr(co->placementOrient()));
    if(co->isUnplaced()) {
      CIRCUIT_FPRINTF(fout, "+ UNPLACED ");
      if((co->placementX() != -1) || (co->placementY() != -1))
      CIRCUIT_FPRINTF(fout, "%d %d %s ", co->placementX(), co->placementY(),
                      orientStr(co->placementOrient()));
    }
    if(co->hasSource())
    CIRCUIT_FPRINTF(fout, "+ SOURCE %s ", co->source());
    if(co->hasGenerate()) {
      CIRCUIT_FPRINTF(fout, "+ GENERATE %s ", co->generateName());
      if(co->macroName() && *(co->macroName()))
      CIRCUIT_FPRINTF(fout, "%s ", co->macroName());
    }
    if(co->hasWeight())
    CIRCUIT_FPRINTF(fout, "+ WEIGHT %d ", co->weight());
    if(co->hasEEQ())
    CIRCUIT_FPRINTF(fout, "+ EEQMASTER %s ", co->EEQ());
    if(co->hasRegionName())
    CIRCUIT_FPRINTF(fout, "+ REGION %s ", co->regionName());
    if(co->hasRegionBounds()) {
      int *xl, *yl, *xh, *yh;
      int size;
      co->regionBounds(&size, &xl, &yl, &xh, &yh);
      for(i = 0; i < size; i++) {
        CIRCUIT_FPRINTF(fout, "+ REGION %d %d %d %d \n", xl[i], yl[i], xh[i],
                        yh[i]);
      }
    }
    if(co->maskShiftSize()) {
      CIRCUIT_FPRINTF(fout, "+ MASKSHIFT ");

      for(int i = co->maskShiftSize() - 1; i >= 0; i--) {
        CIRCUIT_FPRINTF(fout, "%d", co->maskShift(i));
      }
      CIRCUIT_FPRINTF(fout, "\n");
    }
    if(co->hasHalo()) {
      int left, bottom, right, top;
      (void)co->haloEdges(&left, &bottom, &right, &top);
      CIRCUIT_FPRINTF(fout, "+ HALO ");
      if(co->hasHaloSoft())
      CIRCUIT_FPRINTF(fout, "SOFT ");
      CIRCUIT_FPRINTF(fout, "%d %d %d %d\n", left, bottom, right, top);
    }
    if(co->hasRouteHalo()) {
      CIRCUIT_FPRINTF(fout, "+ ROUTEHALO %d %s %s\n", co->haloDist(),
                      co->minLayer(), co->maxLayer());
    }
    if(co->hasForeignName()) {
      CIRCUIT_FPRINTF(fout, "+ FOREIGN %s %d %d %s %d ", co->foreignName(),
                      co->foreignX(), co->foreignY(), co->foreignOri(),
                      co->foreignOrient());
    }
    if(co->numProps()) {
      for(i = 0; i < co->numProps(); i++) {
        CIRCUIT_FPRINTF(fout, "+ PROPERTY %s %s ", co->propName(i),
                        co->propValue(i));
        switch(co->propType(i)) {
          case 'R':
          CIRCUIT_FPRINTF(fout, "REAL ");
            break;
          case 'I':
          CIRCUIT_FPRINTF(fout, "INTEGER ");
            break;
          case 'S':
          CIRCUIT_FPRINTF(fout, "STRING ");
            break;
          case 'Q':
          CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
            break;
          case 'N':
          CIRCUIT_FPRINTF(fout, "NUMBER ");
            break;
        }
      }
    }
    CIRCUIT_FPRINTF(fout, ";\n");
    --numObjs;
    if(numObjs <= 0)
    CIRCUIT_FPRINTF(fout, "END COMPONENTS\n");
  }

  return 0;
}

int netpath(defrCallbackType_e, defiNet*, defiUserData) {
  CIRCUIT_FPRINTF(fout, "\n");

  CIRCUIT_FPRINTF(fout, "Callback of partial path for net\n");

  return 0;
}

int netNamef(defrCallbackType_e c, const char* netName, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "- %s ", netName);
  return 0;
}

int subnetNamef(defrCallbackType_e c, const char* subnetName, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  if(curVer >= 5.6)
  CIRCUIT_FPRINTF(fout, "   + SUBNET CBK %s ", subnetName);
  return 0;
}

int nondefRulef(defrCallbackType_e c, const char* ruleName, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  if(curVer >= 5.6)
  CIRCUIT_FPRINTF(fout, "   + NONDEFAULTRULE CBK %s ", ruleName);
  return 0;
}

int netf(defrCallbackType_e c, defiNet* net, defiUserData ud) {
  // For net and special net.
  int i, j, k, w, x, y, z, count, newLayer;
  defiPath* p;
  defiSubnet* s;
  int path;
  defiVpin* vpin;
  // defiShield *noShield;
  defiWire* wire;

  checkType(c);
  if(ud != userData)
    dataError();
  if(c != defrNetCbkType)
  CIRCUIT_FPRINTF(fout, "BOGUS NET TYPE  ");

  if(net->pinIsMustJoin(0))
  CIRCUIT_FPRINTF(fout, "- MUSTJOIN ");
  // 5/6/2004 - don't need since I have a callback for the name
  //  else
  //      CIRCUIT_FPRINTF(fout, "- %s ", net->name());

  //  net->changeNetName("newNetName");
  //  CIRCUIT_FPRINTF(fout, "%s ", net->name());
  count = 0;
  // compName & pinName
  for(i = 0; i < net->numConnections(); i++) {
    // set the limit of only 5 items per line
    count++;
    if(count >= 5) {
      CIRCUIT_FPRINTF(fout, "\n");
      count = 0;
    }
    CIRCUIT_FPRINTF(fout, "( %s %s ) ", net->instance(i), net->pin(i));
    //      net->changeInstance("newInstance", i);
    //      net->changePin("newPin", i);
    //      CIRCUIT_FPRINTF(fout, "( %s %s ) ", net->instance(i),
    //              net->pin(i));

    // skip for PIN instance
    if(strcmp(net->instance(i), "PIN") != 0) {
      HASH_MAP< string, int >* currentPinMap =
          &(*defComponentPinToNetPtr)[(
              *defComponentMapPtr)[string(net->instance(i))]];

      (*currentPinMap)[string(net->pin(i))] = defNetStorPtr->size();
    }

    if(net->pinIsSynthesized(i))
    CIRCUIT_FPRINTF(fout, "+ SYNTHESIZED ");
  }

  if(net->hasNonDefaultRule())
  CIRCUIT_FPRINTF(fout, "+ NONDEFAULTRULE %s\n", net->nonDefaultRule());

  for(i = 0; i < net->numVpins(); i++) {
    vpin = net->vpin(i);
    CIRCUIT_FPRINTF(fout, "  + %s", vpin->name());
    if(vpin->layer())
    CIRCUIT_FPRINTF(fout, " %s", vpin->layer());
    CIRCUIT_FPRINTF(fout, " %d %d %d %d", vpin->xl(), vpin->yl(), vpin->xh(),
                    vpin->yh());
    if(vpin->status() != ' ') {
      CIRCUIT_FPRINTF(fout, " %c", vpin->status());
      CIRCUIT_FPRINTF(fout, " %d %d", vpin->xLoc(), vpin->yLoc());
      if(vpin->orient() != -1)
      CIRCUIT_FPRINTF(fout, " %s", orientStr(vpin->orient()));
    }
    CIRCUIT_FPRINTF(fout, "\n");
  }

  // regularWiring
  if(net->numWires()) {
    for(i = 0; i < net->numWires(); i++) {
      newLayer = 0;
      wire = net->wire(i);
      CIRCUIT_FPRINTF(fout, "\n  + %s ", wire->wireType());
      count = 0;
      for(j = 0; j < wire->numPaths(); j++) {
        p = wire->path(j);
        p->initTraverse();
        while((path = (int)p->next()) != DEFIPATH_DONE) {
          count++;
          // Don't want the line to be too long
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          switch(path) {
            case DEFIPATH_LAYER:
              if(newLayer == 0) {
                CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                newLayer = 1;
              }
              else {
                CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
              }
              break;
            case DEFIPATH_MASK:
            CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
            CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                            p->getViaCutMask(), p->getViaBottomMask());
              break;
            case DEFIPATH_VIA:
            CIRCUIT_FPRINTF(fout, "%s ",
                            ignoreViaNames ? "XXX" : p->getVia());
              break;
            case DEFIPATH_VIAROTATION:
            CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
              break;
            case DEFIPATH_RECT:
              p->getViaRect(&w, &x, &y, &z);
              CIRCUIT_FPRINTF(fout, "RECT ( %d %d %d %d ) ", w, x, y, z);
              break;
            case DEFIPATH_VIRTUALPOINT:
              p->getVirtualPoint(&x, &y);
              CIRCUIT_FPRINTF(fout, "VIRTUAL ( %d %d ) ", x, y);
              break;
            case DEFIPATH_WIDTH:
            CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
              break;
            case DEFIPATH_FLUSHPOINT:
              p->getFlushPoint(&x, &y, &z);
              CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
              break;
            case DEFIPATH_TAPER:
            CIRCUIT_FPRINTF(fout, "TAPER ");
              break;
            case DEFIPATH_TAPERRULE:
            CIRCUIT_FPRINTF(fout, "TAPERRULE %s ", p->getTaperRule());
              break;
            case DEFIPATH_STYLE:
            CIRCUIT_FPRINTF(fout, "STYLE %d ", p->getStyle());
              break;
          }
        }
      }
      CIRCUIT_FPRINTF(fout, "\n");
      count = 0;
    }
  }

  // SHIELDNET
  if(net->numShieldNets()) {
    for(i = 0; i < net->numShieldNets(); i++)
    CIRCUIT_FPRINTF(fout, "\n  + SHIELDNET %s", net->shieldNet(i));
  }

  /* obsolete in 5.4
  if (net->numNoShields()) {
      for (i = 0; i < net->numNoShields(); i++) {
          noShield = net->noShield(i);
          CIRCUIT_FPRINTF(fout, "\n  + NOSHIELD ");
          newLayer = 0;
          for (j = 0; j < noShield->numPaths(); j++) {
              p = noShield->path(j);
              p->initTraverse();
              while ((path = (int)p->next()) != DEFIPATH_DONE) {
                  count++;
                  // Don't want the line to be too long
                  if (count >= 5) {
                      CIRCUIT_FPRINTF(fout, "\n");
                      count = 0;
                  }
                  switch (path) {
                      case DEFIPATH_LAYER:
                          if (newLayer == 0) {
                              CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                              newLayer = 1;
                          } else
                              CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                          break;
                      case DEFIPATH_VIA:
                          CIRCUIT_FPRINTF(fout, "%s ", p->getVia());
                          break;
                      case DEFIPATH_VIAROTATION:
                          CIRCUIT_FPRINTF(fout, "%s ",
                                  orientStr(p->getViaRotation()));
                          break;
                      case DEFIPATH_WIDTH:
                          CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                          break;
                      case DEFIPATH_POINT:
                          p->getPoint(&x, &y);
                          CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                          break;
                      case DEFIPATH_FLUSHPOINT:
                          p->getFlushPoint(&x, &y, &z);
                          CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                          break;
                      case DEFIPATH_TAPER:
                          CIRCUIT_FPRINTF(fout, "TAPER ");
                          break;
                      case DEFIPATH_TAPERRULE:
                          CIRCUIT_FPRINTF(fout, "TAPERRULE %s ",
                                  p->getTaperRule());
                          break;
                  }
              }
          }
      }
  }
  */

  if(net->hasSubnets()) {
    for(i = 0; i < net->numSubnets(); i++) {
      s = net->subnet(i);
      CIRCUIT_FPRINTF(fout, "\n");

      if(s->numConnections()) {
        if(s->pinIsMustJoin(0)) {
          CIRCUIT_FPRINTF(fout, "- MUSTJOIN ");
        }
        else {
          CIRCUIT_FPRINTF(fout, "  + SUBNET %s ", s->name());
        }
        for(j = 0; j < s->numConnections(); j++)
        CIRCUIT_FPRINTF(fout, " ( %s %s )\n", s->instance(j), s->pin(j));

        // regularWiring
        if(s->numWires()) {
          for(k = 0; k < s->numWires(); k++) {
            newLayer = 0;
            wire = s->wire(k);
            CIRCUIT_FPRINTF(fout, "  %s ", wire->wireType());
            count = 0;
            for(j = 0; j < wire->numPaths(); j++) {
              p = wire->path(j);
              p->initTraverse();
              while((path = (int)p->next()) != DEFIPATH_DONE) {
                count++;
                // Don't want the line to be too long
                if(count >= 5) {
                  CIRCUIT_FPRINTF(fout, "\n");
                  count = 0;
                }
                switch(path) {
                  case DEFIPATH_LAYER:
                    if(newLayer == 0) {
                      CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                      newLayer = 1;
                    }
                    else
                    CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                    break;
                  case DEFIPATH_VIA:
                  CIRCUIT_FPRINTF(fout, "%s ",
                                  ignoreViaNames ? "XXX" : p->getVia());
                    break;
                  case DEFIPATH_VIAROTATION:
                  CIRCUIT_FPRINTF(fout, "%s ", p->getViaRotationStr());
                    break;
                  case DEFIPATH_WIDTH:
                  CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                    break;
                  case DEFIPATH_POINT:
                    p->getPoint(&x, &y);
                    CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                    break;
                  case DEFIPATH_FLUSHPOINT:
                    p->getFlushPoint(&x, &y, &z);
                    CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                    break;
                  case DEFIPATH_TAPER:
                  CIRCUIT_FPRINTF(fout, "TAPER ");
                    break;
                  case DEFIPATH_TAPERRULE:
                  CIRCUIT_FPRINTF(fout, "TAPERRULE  %s ", p->getTaperRule());
                    break;
                  case DEFIPATH_STYLE:
                  CIRCUIT_FPRINTF(fout, "STYLE  %d ", p->getStyle());
                    break;
                }
              }
            }
          }
        }
      }
    }
  }

  if(net->numProps()) {
    for(i = 0; i < net->numProps(); i++) {
      CIRCUIT_FPRINTF(fout, "  + PROPERTY %s ", net->propName(i));
      switch(net->propType(i)) {
        case 'R':
        CIRCUIT_FPRINTF(fout, "%g REAL ", net->propNumber(i));
          break;
        case 'I':
        CIRCUIT_FPRINTF(fout, "%g INTEGER ", net->propNumber(i));
          break;
        case 'S':
        CIRCUIT_FPRINTF(fout, "%s STRING ", net->propValue(i));
          break;
        case 'Q':
        CIRCUIT_FPRINTF(fout, "%s QUOTESTRING ", net->propValue(i));
          break;
        case 'N':
        CIRCUIT_FPRINTF(fout, "%g NUMBER ", net->propNumber(i));
          break;
      }
      CIRCUIT_FPRINTF(fout, "\n");
    }
  }

  if(net->hasWeight())
  CIRCUIT_FPRINTF(fout, "+ WEIGHT %d ", net->weight());
  if(net->hasCap())
  CIRCUIT_FPRINTF(fout, "+ ESTCAP %g ", net->cap());
  if(net->hasSource())
  CIRCUIT_FPRINTF(fout, "+ SOURCE %s ", net->source());
  if(net->hasFixedbump())
  CIRCUIT_FPRINTF(fout, "+ FIXEDBUMP ");
  if(net->hasFrequency())
  CIRCUIT_FPRINTF(fout, "+ FREQUENCY %g ", net->frequency());
  if(net->hasPattern())
  CIRCUIT_FPRINTF(fout, "+ PATTERN %s ", net->pattern());
  if(net->hasOriginal())
  CIRCUIT_FPRINTF(fout, "+ ORIGINAL %s ", net->original());
  if(net->hasUse())
  CIRCUIT_FPRINTF(fout, "+ USE %s ", net->use());

  CIRCUIT_FPRINTF(fout, ";\n");
  --numObjs;
  if(numObjs <= 0)
  CIRCUIT_FPRINTF(fout, "END NETS\n");

  defNetStorPtr->push_back(*net);
  return 0;
}

int snetpath(defrCallbackType_e c, defiNet* ppath, defiUserData ud) {
  int i, j, x, y, z, count, newLayer;
  char* layerName;
  double dist, left, right;
  defiPath* p;
  defiSubnet* s;
  int path;
  defiShield* shield;
  defiWire* wire;
  int numX, numY, stepX, stepY;

  if(c != defrSNetPartialPathCbkType)
    return 1;
  if(ud != userData)
    dataError();

  CIRCUIT_FPRINTF(fout, "SPECIALNET partial data\n");

  CIRCUIT_FPRINTF(fout, "- %s ", ppath->name());
  defSpecialNetStorPtr->push_back(*ppath);
  defSpecialNetType.push_back(DEF_SPECIALNET_PARTIAL_PATH);

  count = 0;
  // compName & pinName
  for(i = 0; i < ppath->numConnections(); i++) {
    // set the limit of only 5 items print out in one line
    count++;
    if(count >= 5) {
      CIRCUIT_FPRINTF(fout, "\n");
      count = 0;
    }
    CIRCUIT_FPRINTF(fout, "( %s %s ) ", ppath->instance(i), ppath->pin(i));
    if(ppath->pinIsSynthesized(i))
    CIRCUIT_FPRINTF(fout, "+ SYNTHESIZED ");
  }

  // specialWiring
  // POLYGON
  if(ppath->numPolygons()) {
    struct defiPoints points;
    for(i = 0; i < ppath->numPolygons(); i++) {
      CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", ppath->polygonName(i));
      points = ppath->getPolygon(i);
      for(j = 0; j < points.numPoints; j++)
      CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
    }
  }
  // RECT
  if(ppath->numRectangles()) {
    for(i = 0; i < ppath->numRectangles(); i++) {
      CIRCUIT_FPRINTF(fout, "\n  + RECT %s %d %d %d %d", ppath->rectName(i),
                      ppath->xl(i), ppath->yl(i), ppath->xh(i), ppath->yh(i));
    }
  }

  // COVER, FIXED, ROUTED or SHIELD
  if(ppath->numWires()) {
    newLayer = 0;
    for(i = 0; i < ppath->numWires(); i++) {
      newLayer = 0;
      wire = ppath->wire(i);
      CIRCUIT_FPRINTF(fout, "\n  + %s ", wire->wireType());
      if(strcmp(wire->wireType(), "SHIELD") == 0)
      CIRCUIT_FPRINTF(fout, "%s ", wire->wireShieldNetName());
      for(j = 0; j < wire->numPaths(); j++) {
        p = wire->path(j);
        p->initTraverse();
        while((path = (int)p->next()) != DEFIPATH_DONE) {
          count++;
          // Don't want the line to be too long
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          switch(path) {
            case DEFIPATH_LAYER:
              if(newLayer == 0) {
                CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                newLayer = 1;
              }
              else
              CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
              break;
            case DEFIPATH_VIA:
            CIRCUIT_FPRINTF(fout, "%s ",
                            ignoreViaNames ? "XXX" : p->getVia());
              break;
            case DEFIPATH_VIAROTATION:
            CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
              break;
            case DEFIPATH_VIADATA:
              p->getViaData(&numX, &numY, &stepX, &stepY);
              CIRCUIT_FPRINTF(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                              stepX, stepY);
              break;
            case DEFIPATH_WIDTH:
            CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
              break;
            case DEFIPATH_MASK:
            CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
            CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                            p->getViaCutMask(), p->getViaBottomMask());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
              break;
            case DEFIPATH_FLUSHPOINT:
              p->getFlushPoint(&x, &y, &z);
              CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
              break;
            case DEFIPATH_TAPER:
            CIRCUIT_FPRINTF(fout, "TAPER ");
              break;
            case DEFIPATH_SHAPE:
            CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
              break;
            case DEFIPATH_STYLE:
            CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
              break;
          }
        }
      }
      CIRCUIT_FPRINTF(fout, "\n");
      count = 0;
    }
  }

  if(ppath->hasSubnets()) {
    for(i = 0; i < ppath->numSubnets(); i++) {
      s = ppath->subnet(i);
      if(s->numConnections()) {
        if(s->pinIsMustJoin(0)) {
          CIRCUIT_FPRINTF(fout, "- MUSTJOIN ");
        }
        else {
          CIRCUIT_FPRINTF(fout, "- %s ", s->name());
        }
        for(j = 0; j < s->numConnections(); j++) {
          CIRCUIT_FPRINTF(fout, " ( %s %s )\n", s->instance(j), s->pin(j));
        }
      }

      // regularWiring
      if(s->numWires()) {
        for(i = 0; i < s->numWires(); i++) {
          wire = s->wire(i);
          CIRCUIT_FPRINTF(fout, "  + %s ", wire->wireType());
          for(j = 0; j < wire->numPaths(); j++) {
            p = wire->path(j);
            p->print(fout);
          }
        }
      }
    }
  }

  if(ppath->numProps()) {
    for(i = 0; i < ppath->numProps(); i++) {
      if(ppath->propIsString(i))
      CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %s ", ppath->propName(i),
                      ppath->propValue(i));
      if(ppath->propIsNumber(i))
      CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %g ", ppath->propName(i),
                      ppath->propNumber(i));
      switch(ppath->propType(i)) {
        case 'R':
        CIRCUIT_FPRINTF(fout, "REAL ");
          break;
        case 'I':
        CIRCUIT_FPRINTF(fout, "INTEGER ");
          break;
        case 'S':
        CIRCUIT_FPRINTF(fout, "STRING ");
          break;
        case 'Q':
        CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
          break;
        case 'N':
        CIRCUIT_FPRINTF(fout, "NUMBER ");
          break;
      }
      CIRCUIT_FPRINTF(fout, "\n");
    }
  }

  // SHIELD
  count = 0;
  // testing the SHIELD for 5.3, obsolete in 5.4
  if(ppath->numShields()) {
    for(i = 0; i < ppath->numShields(); i++) {
      shield = ppath->shield(i);
      CIRCUIT_FPRINTF(fout, "\n  + SHIELD %s ", shield->shieldName());
      newLayer = 0;
      for(j = 0; j < shield->numPaths(); j++) {
        p = shield->path(j);
        p->initTraverse();
        while((path = (int)p->next()) != DEFIPATH_DONE) {
          count++;
          // Don't want the line to be too long
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          switch(path) {
            case DEFIPATH_LAYER:
              if(newLayer == 0) {
                CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                newLayer = 1;
              }
              else
              CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
              break;
            case DEFIPATH_VIA:
            CIRCUIT_FPRINTF(fout, "%s ",
                            ignoreViaNames ? "XXX" : p->getVia());
              break;
            case DEFIPATH_VIAROTATION:
              if(newLayer) {
                CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
              }
              else {
                CIRCUIT_FPRINTF(fout, "Str %s ", p->getViaRotationStr());
              }
              break;
            case DEFIPATH_WIDTH:
            CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
              break;
            case DEFIPATH_MASK:
            CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
            CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                            p->getViaCutMask(), p->getViaBottomMask());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
              break;
            case DEFIPATH_FLUSHPOINT:
              p->getFlushPoint(&x, &y, &z);
              CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
              break;
            case DEFIPATH_TAPER:
            CIRCUIT_FPRINTF(fout, "TAPER ");
              break;
            case DEFIPATH_SHAPE:
            CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
              break;
            case DEFIPATH_STYLE:
            CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
          }
        }
      }
    }
  }

  // layerName width
  if(ppath->hasWidthRules()) {
    for(i = 0; i < ppath->numWidthRules(); i++) {
      ppath->widthRule(i, &layerName, &dist);
      CIRCUIT_FPRINTF(fout, "\n  + WIDTH %s %g ", layerName, dist);
    }
  }

  // layerName spacing
  if(ppath->hasSpacingRules()) {
    for(i = 0; i < ppath->numSpacingRules(); i++) {
      ppath->spacingRule(i, &layerName, &dist, &left, &right);
      if(left == right) {
        CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g ", layerName, dist);
      }
      else {
        CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g RANGE %g %g ", layerName,
                        dist, left, right);
      }
    }
  }

  if(ppath->hasFixedbump())
  CIRCUIT_FPRINTF(fout, "\n  + FIXEDBUMP ");
  if(ppath->hasFrequency())
  CIRCUIT_FPRINTF(fout, "\n  + FREQUENCY %g ", ppath->frequency());
  if(ppath->hasVoltage())
  CIRCUIT_FPRINTF(fout, "\n  + VOLTAGE %g ", ppath->voltage());
  if(ppath->hasWeight())
  CIRCUIT_FPRINTF(fout, "\n  + WEIGHT %d ", ppath->weight());
  if(ppath->hasCap())
  CIRCUIT_FPRINTF(fout, "\n  + ESTCAP %g ", ppath->cap());
  if(ppath->hasSource())
  CIRCUIT_FPRINTF(fout, "\n  + SOURCE %s ", ppath->source());
  if(ppath->hasPattern())
  CIRCUIT_FPRINTF(fout, "\n  + PATTERN %s ", ppath->pattern());
  if(ppath->hasOriginal())
  CIRCUIT_FPRINTF(fout, "\n  + ORIGINAL %s ", ppath->original());
  if(ppath->hasUse())
  CIRCUIT_FPRINTF(fout, "\n  + USE %s ", ppath->use());

  CIRCUIT_FPRINTF(fout, "\n");

  return 0;
}

int snetwire(defrCallbackType_e c, defiNet* ppath, defiUserData ud) {
  int i, j, x, y, z, count = 0, newLayer;
  defiPath* p;
  int path;
  defiWire* wire;
  defiShield* shield;
  int numX, numY, stepX, stepY;

  if(c != defrSNetWireCbkType)
    return 1;
  if(ud != userData)
    dataError();

  CIRCUIT_FPRINTF(fout, "SPECIALNET wire data\n");

  CIRCUIT_FPRINTF(fout, "- %s ", ppath->name());

  // POLYGON
  if(ppath->numPolygons()) {
    struct defiPoints points;
    for(i = 0; i < ppath->numPolygons(); i++) {
      CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", ppath->polygonName(i));

      points = ppath->getPolygon(i);

      for(j = 0; j < points.numPoints; j++) {
        CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
      }
    }
    // RECT
  }
  if(ppath->numRectangles()) {
    for(i = 0; i < ppath->numRectangles(); i++) {
      CIRCUIT_FPRINTF(fout, "\n  + RECT %s %d %d %d %d", ppath->rectName(i),
                      ppath->xl(i), ppath->yl(i), ppath->xh(i), ppath->yh(i));
    }
  }
  // VIA
  if(ppath->numViaSpecs()) {
    for(i = 0; i < ppath->numViaSpecs(); i++) {
      CIRCUIT_FPRINTF(fout, "\n  + VIA %s ", ppath->viaName(i));
      CIRCUIT_FPRINTF(fout, " %s", ppath->viaOrientStr(i));

      defiPoints points = ppath->getViaPts(i);

      for(int j = 0; j < points.numPoints; j++) {
        CIRCUIT_FPRINTF(fout, " %d %d", points.x[j], points.y[j]);
      }
    }
  }

  // specialWiring
  if(ppath->numWires()) {
    newLayer = 0;
    for(i = 0; i < ppath->numWires(); i++) {
      newLayer = 0;
      wire = ppath->wire(i);
      CIRCUIT_FPRINTF(fout, "\n  + %s ", wire->wireType());
      if(strcmp(wire->wireType(), "SHIELD") == 0)
      CIRCUIT_FPRINTF(fout, "%s ", wire->wireShieldNetName());
      for(j = 0; j < wire->numPaths(); j++) {
        p = wire->path(j);
        p->initTraverse();
        while((path = (int)p->next()) != DEFIPATH_DONE) {
          count++;
          // Don't want the line to be too long
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          switch(path) {
            case DEFIPATH_LAYER:
              if(newLayer == 0) {
                CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                newLayer = 1;
              }
              else
              CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
              break;
            case DEFIPATH_VIA:
            CIRCUIT_FPRINTF(fout, "%s ",
                            ignoreViaNames ? "XXX" : p->getVia());
              break;
            case DEFIPATH_VIAROTATION:
            CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
              break;
            case DEFIPATH_VIADATA:
              p->getViaData(&numX, &numY, &stepX, &stepY);
              CIRCUIT_FPRINTF(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                              stepX, stepY);
              break;
            case DEFIPATH_WIDTH:
            CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
              break;
            case DEFIPATH_MASK:
            CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
            CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                            p->getViaCutMask(), p->getViaBottomMask());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
              break;
            case DEFIPATH_FLUSHPOINT:
              p->getFlushPoint(&x, &y, &z);
              CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
              break;
            case DEFIPATH_TAPER:
            CIRCUIT_FPRINTF(fout, "TAPER ");
              break;
            case DEFIPATH_SHAPE:
            CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
              break;
            case DEFIPATH_STYLE:
            CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
              break;
          }
        }
      }
      CIRCUIT_FPRINTF(fout, "\n");
      count = 0;
    }
  }
  else if(ppath->numShields()) {
    for(i = 0; i < ppath->numShields(); i++) {
      shield = ppath->shield(i);
      CIRCUIT_FPRINTF(fout, "\n  + SHIELD %s ", shield->shieldName());
      newLayer = 0;
      for(j = 0; j < shield->numPaths(); j++) {
        p = shield->path(j);
        p->initTraverse();
        while((path = (int)p->next()) != DEFIPATH_DONE) {
          count++;
          // Don't want the line to be too long
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          switch(path) {
            case DEFIPATH_LAYER:
              if(newLayer == 0) {
                CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                newLayer = 1;
              }
              else
              CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
              break;
            case DEFIPATH_VIA:
            CIRCUIT_FPRINTF(fout, "%s ",
                            ignoreViaNames ? "XXX" : p->getVia());
              break;
            case DEFIPATH_VIAROTATION:
            CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
              break;
            case DEFIPATH_WIDTH:
            CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
              break;
            case DEFIPATH_MASK:
            CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
            CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                            p->getViaCutMask(), p->getViaBottomMask());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
              break;
            case DEFIPATH_FLUSHPOINT:
              p->getFlushPoint(&x, &y, &z);
              CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
              break;
            case DEFIPATH_TAPER:
            CIRCUIT_FPRINTF(fout, "TAPER ");
              break;
            case DEFIPATH_SHAPE:
            CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
              break;
            case DEFIPATH_STYLE:
            CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
              break;
          }
        }
      }
    }
  }

  CIRCUIT_FPRINTF(fout, "\n");

  return 0;
}

int snetf(defrCallbackType_e c, defiNet* net, defiUserData ud) {
  // For net and special net.
  int i, j, x, y, z, count, newLayer;
  char* layerName;
  double dist, left, right;
  defiPath* p;
  defiSubnet* s;
  int path;
  defiShield* shield;
  defiWire* wire;
  int numX, numY, stepX, stepY;

  checkType(c);
  if(ud != userData)
    dataError();
  if(c != defrSNetCbkType)
  CIRCUIT_FPRINTF(fout, "BOGUS NET TYPE  ");

  // 5/6/2004 - don't need since I have a callback for the name
  //  CIRCUIT_FPRINTF(fout, "- %s ", net->name());

  count = 0;
  // compName & pinName
  for(i = 0; i < net->numConnections(); i++) {
    // set the limit of only 5 items print out in one line
    count++;
    if(count >= 5) {
      CIRCUIT_FPRINTF(fout, "\n");
      count = 0;
    }
    CIRCUIT_FPRINTF(fout, "( %s %s ) ", net->instance(i), net->pin(i));
    if(net->pinIsSynthesized(i))
    CIRCUIT_FPRINTF(fout, "+ SYNTHESIZED ");
  }

  // specialWiring
  if(net->numWires()) {
    newLayer = 0;
    for(i = 0; i < net->numWires(); i++) {
      newLayer = 0;
      wire = net->wire(i);
      CIRCUIT_FPRINTF(fout, "\n  + %s ", wire->wireType());
      if(strcmp(wire->wireType(), "SHIELD") == 0)
      CIRCUIT_FPRINTF(fout, "%s ", wire->wireShieldNetName());
      for(j = 0; j < wire->numPaths(); j++) {
        p = wire->path(j);
        p->initTraverse();
        if(testDebugPrint) {
          p->print(fout);
        }
        else {
          while((path = (int)p->next()) != DEFIPATH_DONE) {
            count++;
            // Don't want the line to be too long
            if(count >= 5) {
              CIRCUIT_FPRINTF(fout, "\n");
              count = 0;
            }
            switch(path) {
              case DEFIPATH_LAYER:
                if(newLayer == 0) {
                  CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                  newLayer = 1;
                }
                else
                CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                break;
              case DEFIPATH_VIA:
              CIRCUIT_FPRINTF(fout, "%s ",
                              ignoreViaNames ? "XXX" : p->getVia());
                break;
              case DEFIPATH_VIAROTATION:
              CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
                break;
              case DEFIPATH_VIADATA:
                p->getViaData(&numX, &numY, &stepX, &stepY);
                CIRCUIT_FPRINTF(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                                stepX, stepY);
                break;
              case DEFIPATH_WIDTH:
              CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                break;
              case DEFIPATH_MASK:
              CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
                break;
              case DEFIPATH_VIAMASK:
              CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                              p->getViaCutMask(), p->getViaBottomMask());
                break;
              case DEFIPATH_POINT:
                p->getPoint(&x, &y);
                CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                break;
              case DEFIPATH_FLUSHPOINT:
                p->getFlushPoint(&x, &y, &z);
                CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                break;
              case DEFIPATH_TAPER:
              CIRCUIT_FPRINTF(fout, "TAPER ");
                break;
              case DEFIPATH_SHAPE:
              CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
                break;
              case DEFIPATH_STYLE:
              CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
                break;
            }
          }
        }
      }
      CIRCUIT_FPRINTF(fout, "\n");
      count = 0;
    }
  }

  // POLYGON
  if(net->numPolygons()) {
    struct defiPoints points;

    for(i = 0; i < net->numPolygons(); i++) {
      if(curVer >= 5.8) {
        if(strcmp(net->polyRouteStatus(i), "") != 0) {
          CIRCUIT_FPRINTF(fout, "\n  + %s ", net->polyRouteStatus(i));
          if(strcmp(net->polyRouteStatus(i), "SHIELD") == 0) {
            CIRCUIT_FPRINTF(fout, "\n  + %s ",
                            net->polyRouteStatusShieldName(i));
          }
        }
        if(strcmp(net->polyShapeType(i), "") != 0) {
          CIRCUIT_FPRINTF(fout, "\n  + SHAPE %s ", net->polyShapeType(i));
        }
      }
      if(net->polyMask(i)) {
        CIRCUIT_FPRINTF(fout, "\n  + MASK %d + POLYGON % s ", net->polyMask(i),
                        net->polygonName(i));
      }
      else {
        CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", net->polygonName(i));
      }
      points = net->getPolygon(i);
      for(j = 0; j < points.numPoints; j++)
      CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
    }
  }
  // RECT
  if(net->numRectangles()) {
    for(i = 0; i < net->numRectangles(); i++) {
      if(curVer >= 5.8) {
        if(strcmp(net->rectRouteStatus(i), "") != 0) {
          CIRCUIT_FPRINTF(fout, "\n  + %s ", net->rectRouteStatus(i));
          if(strcmp(net->rectRouteStatus(i), "SHIELD") == 0) {
            CIRCUIT_FPRINTF(fout, "\n  + %s ",
                            net->rectRouteStatusShieldName(i));
          }
        }
        if(strcmp(net->rectShapeType(i), "") != 0) {
          CIRCUIT_FPRINTF(fout, "\n  + SHAPE %s ", net->rectShapeType(i));
        }
      }
      if(net->rectMask(i)) {
        CIRCUIT_FPRINTF(fout, "\n  + MASK %d + RECT %s %d %d %d %d",
                        net->rectMask(i), net->rectName(i), net->xl(i),
                        net->yl(i), net->xh(i), net->yh(i));
      }
      else {
        CIRCUIT_FPRINTF(fout, "\n  + RECT %s %d %d %d %d", net->rectName(i),
                        net->xl(i), net->yl(i), net->xh(i), net->yh(i));
      }
    }
  }
  // VIA
  if(curVer >= 5.8 && net->numViaSpecs()) {
    for(i = 0; i < net->numViaSpecs(); i++) {
      if(strcmp(net->viaRouteStatus(i), "") != 0) {
        CIRCUIT_FPRINTF(fout, "\n  + %s ", net->viaRouteStatus(i));
        if(strcmp(net->viaRouteStatus(i), "SHIELD") == 0) {
          CIRCUIT_FPRINTF(fout, "\n  + %s ", net->viaRouteStatusShieldName(i));
        }
      }
      if(strcmp(net->viaShapeType(i), "") != 0) {
        CIRCUIT_FPRINTF(fout, "\n  + SHAPE %s ", net->viaShapeType(i));
      }
      if(net->topMaskNum(i) || net->cutMaskNum(i) || net->bottomMaskNum(i)) {
        CIRCUIT_FPRINTF(fout, "\n  + MASK %d%d%d + VIA %s ", net->topMaskNum(i),
                        net->cutMaskNum(i), net->bottomMaskNum(i),
                        net->viaName(i));
      }
      else {
        CIRCUIT_FPRINTF(fout, "\n  + VIA %s ", net->viaName(i));
      }
      CIRCUIT_FPRINTF(fout, " %s", net->viaOrientStr(i));

      defiPoints points = net->getViaPts(i);

      for(int j = 0; j < points.numPoints; j++) {
        CIRCUIT_FPRINTF(fout, " %d %d", points.x[j], points.y[j]);
      }
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }

  if(net->hasSubnets()) {
    for(i = 0; i < net->numSubnets(); i++) {
      s = net->subnet(i);
      if(s->numConnections()) {
        if(s->pinIsMustJoin(0)) {
          CIRCUIT_FPRINTF(fout, "- MUSTJOIN ");
        }
        else {
          CIRCUIT_FPRINTF(fout, "- %s ", s->name());
        }
        for(j = 0; j < s->numConnections(); j++) {
          CIRCUIT_FPRINTF(fout, " ( %s %s )\n", s->instance(j), s->pin(j));
        }
      }

      // regularWiring
      if(s->numWires()) {
        for(i = 0; i < s->numWires(); i++) {
          wire = s->wire(i);
          CIRCUIT_FPRINTF(fout, "  + %s ", wire->wireType());
          for(j = 0; j < wire->numPaths(); j++) {
            p = wire->path(j);
            p->print(fout);
          }
        }
      }
    }
  }

  if(net->numProps()) {
    for(i = 0; i < net->numProps(); i++) {
      if(net->propIsString(i))
      CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %s ", net->propName(i),
                      net->propValue(i));
      if(net->propIsNumber(i))
      CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %g ", net->propName(i),
                      net->propNumber(i));
      switch(net->propType(i)) {
        case 'R':
        CIRCUIT_FPRINTF(fout, "REAL ");
          break;
        case 'I':
        CIRCUIT_FPRINTF(fout, "INTEGER ");
          break;
        case 'S':
        CIRCUIT_FPRINTF(fout, "STRING ");
          break;
        case 'Q':
        CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
          break;
        case 'N':
        CIRCUIT_FPRINTF(fout, "NUMBER ");
          break;
      }
      CIRCUIT_FPRINTF(fout, "\n");
    }
  }

  // SHIELD
  count = 0;
  // testing the SHIELD for 5.3, obsolete in 5.4
  if(net->numShields()) {
    for(i = 0; i < net->numShields(); i++) {
      shield = net->shield(i);
      CIRCUIT_FPRINTF(fout, "\n  + SHIELD %s ", shield->shieldName());
      newLayer = 0;
      for(j = 0; j < shield->numPaths(); j++) {
        p = shield->path(j);
        p->initTraverse();
        while((path = (int)p->next()) != DEFIPATH_DONE) {
          count++;
          // Don't want the line to be too long
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          switch(path) {
            case DEFIPATH_LAYER:
              if(newLayer == 0) {
                CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                newLayer = 1;
              }
              else
              CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
              break;
            case DEFIPATH_VIA:
            CIRCUIT_FPRINTF(fout, "%s ",
                            ignoreViaNames ? "XXX" : p->getVia());
              break;
            case DEFIPATH_VIAROTATION:
            CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
              break;
            case DEFIPATH_WIDTH:
            CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
              break;
            case DEFIPATH_MASK:
            CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
              break;
            case DEFIPATH_VIAMASK:
            CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                            p->getViaCutMask(), p->getViaBottomMask());
              break;
            case DEFIPATH_POINT:
              p->getPoint(&x, &y);
              CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
              break;
            case DEFIPATH_FLUSHPOINT:
              p->getFlushPoint(&x, &y, &z);
              CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
              break;
            case DEFIPATH_TAPER:
            CIRCUIT_FPRINTF(fout, "TAPER ");
              break;
            case DEFIPATH_SHAPE:
            CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
              break;
            case DEFIPATH_STYLE:
            CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
              break;
          }
        }
      }
    }
  }

  // layerName width
  if(net->hasWidthRules()) {
    for(i = 0; i < net->numWidthRules(); i++) {
      net->widthRule(i, &layerName, &dist);
      CIRCUIT_FPRINTF(fout, "\n  + WIDTH %s %g ", layerName, dist);
    }
  }

  // layerName spacing
  if(net->hasSpacingRules()) {
    for(i = 0; i < net->numSpacingRules(); i++) {
      net->spacingRule(i, &layerName, &dist, &left, &right);
      if(left == right) {
        CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g ", layerName, dist);
      }
      else {
        CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g RANGE %g %g ", layerName,
                        dist, left, right);
      }
    }
  }

  if(net->hasFixedbump())
  CIRCUIT_FPRINTF(fout, "\n  + FIXEDBUMP ");
  if(net->hasFrequency())
  CIRCUIT_FPRINTF(fout, "\n  + FREQUENCY %g ", net->frequency());
  if(net->hasVoltage())
  CIRCUIT_FPRINTF(fout, "\n  + VOLTAGE %g ", net->voltage());
  if(net->hasWeight())
  CIRCUIT_FPRINTF(fout, "\n  + WEIGHT %d ", net->weight());
  if(net->hasCap())
  CIRCUIT_FPRINTF(fout, "\n  + ESTCAP %g ", net->cap());
  if(net->hasSource())
  CIRCUIT_FPRINTF(fout, "\n  + SOURCE %s ", net->source());
  if(net->hasPattern())
  CIRCUIT_FPRINTF(fout, "\n  + PATTERN %s ", net->pattern());
  if(net->hasOriginal())
  CIRCUIT_FPRINTF(fout, "\n  + ORIGINAL %s ", net->original());
  if(net->hasUse())
  CIRCUIT_FPRINTF(fout, "\n  + USE %s ", net->use());

  CIRCUIT_FPRINTF(fout, ";\n");
  defSpecialNetStorPtr->push_back(*net);
  defSpecialNetType.push_back(DEF_SPECIALNET_ORIGINAL);
  --numObjs;
  if(numObjs <= 0)
  CIRCUIT_FPRINTF(fout, "END SPECIALNETS\n");
  return 0;
}

int ndr(defrCallbackType_e c, defiNonDefault* nd, defiUserData ud) {
  // For nondefaultrule
  int i;

  checkType(c);
  if(ud != userData)
    dataError();
  if(c != defrNonDefaultCbkType)
  CIRCUIT_FPRINTF(fout, "BOGUS NONDEFAULTRULE TYPE  ");
  CIRCUIT_FPRINTF(fout, "- %s\n", nd->name());
  if(nd->hasHardspacing())
  CIRCUIT_FPRINTF(fout, "   + HARDSPACING\n");
  for(i = 0; i < nd->numLayers(); i++) {
    CIRCUIT_FPRINTF(fout, "   + LAYER %s", nd->layerName(i));
    CIRCUIT_FPRINTF(fout, " WIDTH %d", nd->layerWidthVal(i));
    if(nd->hasLayerDiagWidth(i))
    CIRCUIT_FPRINTF(fout, " DIAGWIDTH %d", nd->layerDiagWidthVal(i));
    if(nd->hasLayerSpacing(i))
    CIRCUIT_FPRINTF(fout, " SPACING %d", nd->layerSpacingVal(i));
    if(nd->hasLayerWireExt(i))
    CIRCUIT_FPRINTF(fout, " WIREEXT %d", nd->layerWireExtVal(i));
    CIRCUIT_FPRINTF(fout, "\n");
  }
  for(i = 0; i < nd->numVias(); i++)
  CIRCUIT_FPRINTF(fout, "   + VIA %s\n", nd->viaName(i));
  for(i = 0; i < nd->numViaRules(); i++)
  CIRCUIT_FPRINTF(fout, "   + VIARULE %s\n",
                  ignoreViaNames ? "XXX" : nd->viaRuleName(i));
  for(i = 0; i < nd->numMinCuts(); i++)
  CIRCUIT_FPRINTF(fout, "   + MINCUTS %s %d\n", nd->cutLayerName(i),
                  nd->numCuts(i));
  for(i = 0; i < nd->numProps(); i++) {
    CIRCUIT_FPRINTF(fout, "   + PROPERTY %s %s ", nd->propName(i),
                    nd->propValue(i));
    switch(nd->propType(i)) {
      case 'R':
      CIRCUIT_FPRINTF(fout, "REAL\n");
        break;
      case 'I':
      CIRCUIT_FPRINTF(fout, "INTEGER\n");
        break;
      case 'S':
      CIRCUIT_FPRINTF(fout, "STRING\n");
        break;
      case 'Q':
      CIRCUIT_FPRINTF(fout, "QUOTESTRING\n");
        break;
      case 'N':
      CIRCUIT_FPRINTF(fout, "NUMBER\n");
        break;
    }
  }
  --numObjs;
  if(numObjs <= 0)
  CIRCUIT_FPRINTF(fout, "END NONDEFAULTRULES\n");
  return 0;
}

int tname(defrCallbackType_e c, const char* string, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "TECHNOLOGY %s ;\n", string);
  return 0;
}

int dname(defrCallbackType_e c, const char* str, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();

  (*defDesignNamePtr) = string(str);
  isDesignNameVisit = true;

  CIRCUIT_FPRINTF(fout, "DESIGN %s ;\n", str);
  return 0;
}

char* address(const char* in) {
  return ((char*)in);
}

int cs(defrCallbackType_e c, int num, defiUserData ud) {
  char* name;

  checkType(c);

  if(ud != userData)
    dataError();

  switch(c) {
    case defrComponentStartCbkType:
      name = address("COMPONENTS");
      defComponentStorPtr->reserve(num);
      defComponentPinToNetPtr->reserve(num);
      break;
    case defrNetStartCbkType:
      name = address("NETS");
      defNetStorPtr->reserve(num);
      break;
    case defrStartPinsCbkType:
      name = address("PINS");
      defPinStorPtr->reserve(num);
      break;
    case defrViaStartCbkType:
      name = address("VIAS");
      break;
    case defrRegionStartCbkType:
      name = address("REGIONS");
      break;
    case defrSNetStartCbkType:
      name = address("SPECIALNETS");
      break;
    case defrGroupsStartCbkType:
      name = address("GROUPS");
      break;
    case defrScanchainsStartCbkType:
      name = address("SCANCHAINS");
      break;
    case defrIOTimingsStartCbkType:
      name = address("IOTIMINGS");
      break;
    case defrFPCStartCbkType:
      name = address("FLOORPLANCONSTRAINTS");
      break;
    case defrTimingDisablesStartCbkType:
      name = address("TIMING DISABLES");
      break;
    case defrPartitionsStartCbkType:
      name = address("PARTITIONS");
      break;
    case defrPinPropStartCbkType:
      name = address("PINPROPERTIES");
      break;
    case defrBlockageStartCbkType:
      name = address("BLOCKAGES");
      break;
    case defrSlotStartCbkType:
      name = address("SLOTS");
      break;
    case defrFillStartCbkType:
      name = address("FILLS");
      break;
    case defrNonDefaultStartCbkType:
      name = address("NONDEFAULTRULES");
      break;
    case defrStylesStartCbkType:
      name = address("STYLES");
      break;
    default:
      name = address("BOGUS");
      return 1;
  }
  CIRCUIT_FPRINTF(fout, "\n%s %d ;\n", name, num);
  numObjs = num;
  return 0;
}

int constraintst(defrCallbackType_e c, int num, defiUserData ud) {
  // Handles both constraints and assertions
  checkType(c);
  if(ud != userData)
    dataError();
  if(c == defrConstraintsStartCbkType) {
    CIRCUIT_FPRINTF(fout, "\nCONSTRAINTS %d ;\n\n", num);
  }
  else {
    CIRCUIT_FPRINTF(fout, "\nASSERTIONS %d ;\n\n", num);
  }
  numObjs = num;
  return 0;
}

void operand(defrCallbackType_e c, defiAssertion* a, int ind) {
  int i, first = 1;
  char* netName;
  char *fromInst, *fromPin, *toInst, *toPin;

  if(a->isSum()) {
    // Sum in operand, recursively call operand
    CIRCUIT_FPRINTF(fout, "- SUM ( ");
    a->unsetSum();
    isSumSet = 1;
    begOperand = 0;
    operand(c, a, ind);
    CIRCUIT_FPRINTF(fout, ") ");
  }
  else {
    // operand
    if(ind >= a->numItems()) {
      CIRCUIT_FPRINTF(fout, "ERROR: when writing out SUM in Constraints.\n");
      return;
    }
    if(begOperand) {
      CIRCUIT_FPRINTF(fout, "- ");
      begOperand = 0;
    }
    for(i = ind; i < a->numItems(); i++) {
      if(a->isNet(i)) {
        a->net(i, &netName);
        if(!first)
        CIRCUIT_FPRINTF(fout, ", ");  // print , as separator
        CIRCUIT_FPRINTF(fout, "NET %s ", netName);
      }
      else if(a->isPath(i)) {
        a->path(i, &fromInst, &fromPin, &toInst, &toPin);
        if(!first)
        CIRCUIT_FPRINTF(fout, ", ");
        CIRCUIT_FPRINTF(fout, "PATH %s %s %s %s ", fromInst, fromPin, toInst,
                        toPin);
      }
      else if(isSumSet) {
        // SUM within SUM, reset the flag
        a->setSum();
        operand(c, a, i);
      }
      first = 0;
    }
  }
}

int constraint(defrCallbackType_e c, defiAssertion* a, defiUserData ud) {
  // Handles both constraints and assertions

  checkType(c);
  if(ud != userData)
    dataError();
  if(a->isWiredlogic()) {
    // Wirelogic
    CIRCUIT_FPRINTF(fout, "- WIREDLOGIC %s + MAXDIST %g ;\n",
    // Wiredlogic dist is also store in fallMax
    //              a->netName(), a->distance());
                    a->netName(), a->fallMax());
  }
  else {
    // Call the operand function
    isSumSet = 0;  // reset the global variable
    begOperand = 1;
    operand(c, a, 0);
    // Get the Rise and Fall
    if(a->hasRiseMax())
    CIRCUIT_FPRINTF(fout, "+ RISEMAX %g ", a->riseMax());
    if(a->hasFallMax())
    CIRCUIT_FPRINTF(fout, "+ FALLMAX %g ", a->fallMax());
    if(a->hasRiseMin())
    CIRCUIT_FPRINTF(fout, "+ RISEMIN %g ", a->riseMin());
    if(a->hasFallMin())
    CIRCUIT_FPRINTF(fout, "+ FALLMIN %g ", a->fallMin());
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  --numObjs;
  if(numObjs <= 0) {
    if(c == defrConstraintCbkType) {
      CIRCUIT_FPRINTF(fout, "END CONSTRAINTS\n");
    }
    else {
      CIRCUIT_FPRINTF(fout, "END ASSERTIONS\n");
    }
  }
  return 0;
}

int propstart(defrCallbackType_e c, void*, defiUserData) {
  checkType(c);
  CIRCUIT_FPRINTF(fout, "\nPROPERTYDEFINITIONS\n");
  isProp = 1;

  return 0;
}

int prop(defrCallbackType_e c, defiProp* p, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();

  defPropStorPtr->push_back(*p);
  if(strcmp(p->propType(), "design") == 0) {
    CIRCUIT_FPRINTF(fout, "DESIGN %s ", p->propName());
  }
  else if(strcmp(p->propType(), "net") == 0) {
    CIRCUIT_FPRINTF(fout, "NET %s ", p->propName());
  }
  else if(strcmp(p->propType(), "component") == 0) {
    CIRCUIT_FPRINTF(fout, "COMPONENT %s ", p->propName());
  }
  else if(strcmp(p->propType(), "specialnet") == 0) {
    CIRCUIT_FPRINTF(fout, "SPECIALNET %s ", p->propName());
  }
  else if(strcmp(p->propType(), "group") == 0) {
    CIRCUIT_FPRINTF(fout, "GROUP %s ", p->propName());
  }
  else if(strcmp(p->propType(), "row") == 0) {
    CIRCUIT_FPRINTF(fout, "ROW %s ", p->propName());
  }
  else if(strcmp(p->propType(), "componentpin") == 0) {
    CIRCUIT_FPRINTF(fout, "COMPONENTPIN %s ", p->propName());
  }
  else if(strcmp(p->propType(), "region") == 0) {
    CIRCUIT_FPRINTF(fout, "REGION %s ", p->propName());
  }
  else if(strcmp(p->propType(), "nondefaultrule") == 0) {
    CIRCUIT_FPRINTF(fout, "NONDEFAULTRULE %s ", p->propName());
  }

  if(p->dataType() == 'I')
  CIRCUIT_FPRINTF(fout, "INTEGER ");
  if(p->dataType() == 'R')
  CIRCUIT_FPRINTF(fout, "REAL ");
  if(p->dataType() == 'S')
  CIRCUIT_FPRINTF(fout, "STRING ");
  if(p->dataType() == 'Q')
  CIRCUIT_FPRINTF(fout, "STRING ");
  if(p->hasRange()) {
    CIRCUIT_FPRINTF(fout, "RANGE %g %g ", p->left(), p->right());
  }
  if(p->hasNumber())
  CIRCUIT_FPRINTF(fout, "%g ", p->number());
  if(p->hasString())
  CIRCUIT_FPRINTF(fout, "\"%s\" ", p->string());
  CIRCUIT_FPRINTF(fout, ";\n");

  return 0;
}

int propend(defrCallbackType_e c, void*, defiUserData) {
  checkType(c);
  if(isProp) {
    CIRCUIT_FPRINTF(fout, "END PROPERTYDEFINITIONS\n\n");
    isProp = 0;
  }

  return 0;
}

int hist(defrCallbackType_e c, const char* h, defiUserData ud) {
  checkType(c);
  defrSetCaseSensitivity(0);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "HISTORY %s ;\n", h);
  defrSetCaseSensitivity(1);
  return 0;
}

int an(defrCallbackType_e c, const char* h, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "ARRAY %s ;\n", h);
  return 0;
}

int fn(defrCallbackType_e c, const char* h, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "FLOORPLAN %s ;\n", h);
  return 0;
}

int bbn(defrCallbackType_e c, const char* h, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();

  (*defBusBitCharPtr) = string(h);
  isBusBitCharVisit = true;

  CIRCUIT_FPRINTF(fout, "BUSBITCHARS \"%s\" ;\n", h);
  return 0;
}

int vers(defrCallbackType_e c, double d, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "VERSION %g ;\n", d);
  curVer = d;

  CIRCUIT_FPRINTF(fout, "ALIAS alias1 aliasValue1 1 ;\n");
  CIRCUIT_FPRINTF(fout, "ALIAS alias2 aliasValue2 0 ;\n");

  return 0;
}

int versStr(defrCallbackType_e c, const char* versionName, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();

  (*defVersionPtr) = string(versionName);
  isVersionVisit = true;

  CIRCUIT_FPRINTF(fout, "VERSION %s ;\n", versionName);
  return 0;
}

int units(defrCallbackType_e c, double d, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  *defUnitPtr = d;
  CIRCUIT_FPRINTF(fout, "UNITS DISTANCE MICRONS %g ;\n", d);
  return 0;
}

int casesens(defrCallbackType_e c, int d, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  if(d == 1) {
    CIRCUIT_FPRINTF(fout, "NAMESCASESENSITIVE ON ;\n", d);
  }
  else {
    CIRCUIT_FPRINTF(fout, "NAMESCASESENSITIVE OFF ;\n", d);
  }
  return 0;
}

static int cls(defrCallbackType_e c, void* cl, defiUserData ud) {
  defiSite* site;  // Site and Canplace and CannotOccupy
  defiBox* box;    // DieArea and
  defiPinCap* pc;
  defiPin* pin;
  int i, j, k;
  defiRow* row;
  defiTrack* track;
  defiGcellGrid* gcg;
  defiVia* via;
  defiRegion* re;
  defiGroup* group;
  defiComponentMaskShiftLayer* maskShiftLayer = NULL;
  defiScanchain* sc;
  defiIOTiming* iot;
  defiFPC* fpc;
  defiTimingDisable* td;
  defiPartition* part;
  defiPinProp* pprop;
  defiBlockage* block;
  defiSlot* slots;
  defiFill* fills;
  defiStyles* styles;
  int xl, yl, xh, yh;
  char *name, *a1, *b1;
  char **inst, **inPin, **outPin;
  int* bits;
  int size;
  int corner, typ;
  const char* itemT;
  char dir;
  defiPinAntennaModel* aModel;
  struct defiPoints points;

  checkType(c);
  if(ud != userData)
    dataError();
  switch(c) {
    case defrSiteCbkType:
      site = (defiSite*)cl;
      CIRCUIT_FPRINTF(fout, "SITE %s %g %g %s ", site->name(), site->x_orig(),
                      site->y_orig(), orientStr(site->orient()));
      CIRCUIT_FPRINTF(fout, "DO %g BY %g STEP %g %g ;\n", site->x_num(),
                      site->y_num(), site->x_step(), site->y_step());
      break;
    case defrCanplaceCbkType:
      site = (defiSite*)cl;
      CIRCUIT_FPRINTF(fout, "CANPLACE %s %g %g %s ", site->name(),
                      site->x_orig(), site->y_orig(),
                      orientStr(site->orient()));
      CIRCUIT_FPRINTF(fout, "DO %g BY %g STEP %g %g ;\n", site->x_num(),
                      site->y_num(), site->x_step(), site->y_step());
      break;
    case defrCannotOccupyCbkType:
      site = (defiSite*)cl;
      CIRCUIT_FPRINTF(fout, "CANNOTOCCUPY %s %g %g %s ", site->name(),
                      site->x_orig(), site->y_orig(),
                      orientStr(site->orient()));
      CIRCUIT_FPRINTF(fout, "DO %g BY %g STEP %g %g ;\n", site->x_num(),
                      site->y_num(), site->x_step(), site->y_step());
      break;
    case defrDieAreaCbkType:
      box = (defiBox*)cl;
      *defDieAreaPtr = *box;
      CIRCUIT_FPRINTF(fout, "DIEAREA %d %d %d %d ;\n", box->xl(), box->yl(),
                      box->xh(), box->yh());
      CIRCUIT_FPRINTF(fout, "DIEAREA ");
      points = box->getPoint();
      for(i = 0; i < points.numPoints; i++)
      CIRCUIT_FPRINTF(fout, "%d %d ", points.x[i], points.y[i]);
      CIRCUIT_FPRINTF(fout, ";\n");
      break;
    case defrPinCapCbkType:
      pc = (defiPinCap*)cl;
      if(testDebugPrint) {
        pc->print(fout);
      }
      else {
        CIRCUIT_FPRINTF(fout, "MINPINS %d WIRECAP %g ;\n", pc->pin(),
                        pc->cap());
        --numObjs;
        if(numObjs <= 0)
        CIRCUIT_FPRINTF(fout, "END DEFAULTCAP\n");
      }
      break;
    case defrPinCbkType:
      pin = (defiPin*)cl;

      // update pin data
      (*defPinMapPtr)[string(pin->pinName())] = defPinStorPtr->size();
      defPinStorPtr->push_back(*pin);

      if(testDebugPrint) {
        pin->print(fout);
      }
      else {
        CIRCUIT_FPRINTF(fout, "- %s + NET %s ", pin->pinName(), pin->netName());
        //         pin->changePinName("pinName");
        //         CIRCUIT_FPRINTF(fout, "%s ", pin->pinName());
        if(pin->hasDirection())
        CIRCUIT_FPRINTF(fout, "+ DIRECTION %s ", pin->direction());
        if(pin->hasUse())
        CIRCUIT_FPRINTF(fout, "+ USE %s ", pin->use());
        if(pin->hasNetExpr())
        CIRCUIT_FPRINTF(fout, "+ NETEXPR \"%s\" ", pin->netExpr());
        if(pin->hasSupplySensitivity())
        CIRCUIT_FPRINTF(fout, "+ SUPPLYSENSITIVITY %s ",
                        pin->supplySensitivity());
        if(pin->hasGroundSensitivity())
        CIRCUIT_FPRINTF(fout, "+ GROUNDSENSITIVITY %s ",
                        pin->groundSensitivity());
        if(pin->hasLayer()) {
          struct defiPoints points;
          for(i = 0; i < pin->numLayer(); i++) {
            CIRCUIT_FPRINTF(fout, "\n  + LAYER %s ", pin->layer(i));
            if(pin->layerMask(i))
            CIRCUIT_FPRINTF(fout, "MASK %d ", pin->layerMask(i));
            if(pin->hasLayerSpacing(i))
            CIRCUIT_FPRINTF(fout, "SPACING %d ", pin->layerSpacing(i));
            if(pin->hasLayerDesignRuleWidth(i))
            CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                            pin->layerDesignRuleWidth(i));
            pin->bounds(i, &xl, &yl, &xh, &yh);
            CIRCUIT_FPRINTF(fout, "%d %d %d %d ", xl, yl, xh, yh);
          }
          for(i = 0; i < pin->numPolygons(); i++) {
            CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", pin->polygonName(i));
            if(pin->polygonMask(i))
            CIRCUIT_FPRINTF(fout, "MASK %d ", pin->polygonMask(i));
            if(pin->hasPolygonSpacing(i))
            CIRCUIT_FPRINTF(fout, "SPACING %d ", pin->polygonSpacing(i));
            if(pin->hasPolygonDesignRuleWidth(i))
            CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                            pin->polygonDesignRuleWidth(i));
            points = pin->getPolygon(i);
            for(j = 0; j < points.numPoints; j++)
            CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
          }
          for(i = 0; i < pin->numVias(); i++) {
            if(pin->viaTopMask(i) || pin->viaCutMask(i) ||
                pin->viaBottomMask(i)) {
              CIRCUIT_FPRINTF(fout, "\n  + VIA %s MASK %d%d%d %d %d ",
                              pin->viaName(i), pin->viaTopMask(i),
                              pin->viaCutMask(i), pin->viaBottomMask(i),
                              pin->viaPtX(i), pin->viaPtY(i));
            }
            else {
              CIRCUIT_FPRINTF(fout, "\n  + VIA %s %d %d ", pin->viaName(i),
                              pin->viaPtX(i), pin->viaPtY(i));
            }
          }
        }
        if(pin->hasPort()) {
          struct defiPoints points;
          defiPinPort* port;
          for(j = 0; j < pin->numPorts(); j++) {
            port = pin->pinPort(j);
            CIRCUIT_FPRINTF(fout, "\n  + PORT");
            for(i = 0; i < port->numLayer(); i++) {
              CIRCUIT_FPRINTF(fout, "\n     + LAYER %s ", port->layer(i));
              if(port->layerMask(i))
              CIRCUIT_FPRINTF(fout, "MASK %d ", port->layerMask(i));
              if(port->hasLayerSpacing(i))
              CIRCUIT_FPRINTF(fout, "SPACING %d ", port->layerSpacing(i));
              if(port->hasLayerDesignRuleWidth(i))
              CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                              port->layerDesignRuleWidth(i));
              port->bounds(i, &xl, &yl, &xh, &yh);
              CIRCUIT_FPRINTF(fout, "%d %d %d %d ", xl, yl, xh, yh);
            }
            for(i = 0; i < port->numPolygons(); i++) {
              CIRCUIT_FPRINTF(fout, "\n     + POLYGON %s ",
                              port->polygonName(i));
              if(port->polygonMask(i))
              CIRCUIT_FPRINTF(fout, "MASK %d ", port->polygonMask(i));
              if(port->hasPolygonSpacing(i))
              CIRCUIT_FPRINTF(fout, "SPACING %d ", port->polygonSpacing(i));
              if(port->hasPolygonDesignRuleWidth(i))
              CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                              port->polygonDesignRuleWidth(i));
              points = port->getPolygon(i);
              for(k = 0; k < points.numPoints; k++)
              CIRCUIT_FPRINTF(fout, "( %d %d ) ", points.x[k], points.y[k]);
            }
            for(i = 0; i < port->numVias(); i++) {
              if(port->viaTopMask(i) || port->viaCutMask(i) ||
                  port->viaBottomMask(i)) {
                CIRCUIT_FPRINTF(fout, "\n     + VIA %s MASK %d%d%d ( %d %d ) ",
                                port->viaName(i), port->viaTopMask(i),
                                port->viaCutMask(i), port->viaBottomMask(i),
                                port->viaPtX(i), port->viaPtY(i));
              }
              else {
                CIRCUIT_FPRINTF(fout, "\n     + VIA %s ( %d %d ) ",
                                port->viaName(i), port->viaPtX(i),
                                port->viaPtY(i));
              }
            }
            if(port->hasPlacement()) {
              if(port->isPlaced()) {
                CIRCUIT_FPRINTF(fout, "\n     + PLACED ");
                CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", port->placementX(),
                                port->placementY(), orientStr(port->orient()));
              }
              if(port->isCover()) {
                CIRCUIT_FPRINTF(fout, "\n     + COVER ");
                CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", port->placementX(),
                                port->placementY(), orientStr(port->orient()));
              }
              if(port->isFixed()) {
                CIRCUIT_FPRINTF(fout, "\n     + FIXED ");
                CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", port->placementX(),
                                port->placementY(), orientStr(port->orient()));
              }
            }
          }
        }
        if(pin->hasPlacement()) {
          if(pin->isPlaced()) {
            CIRCUIT_FPRINTF(fout, "+ PLACED ");
            CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", pin->placementX(),
                            pin->placementY(), orientStr(pin->orient()));
          }
          if(pin->isCover()) {
            CIRCUIT_FPRINTF(fout, "+ COVER ");
            CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", pin->placementX(),
                            pin->placementY(), orientStr(pin->orient()));
          }
          if(pin->isFixed()) {
            CIRCUIT_FPRINTF(fout, "+ FIXED ");
            CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", pin->placementX(),
                            pin->placementY(), orientStr(pin->orient()));
          }
          if(pin->isUnplaced())
          CIRCUIT_FPRINTF(fout, "+ UNPLACED ");
        }
        if(pin->hasSpecial()) {
          CIRCUIT_FPRINTF(fout, "+ SPECIAL ");
        }
        if(pin->hasAPinPartialMetalArea()) {
          for(i = 0; i < pin->numAPinPartialMetalArea(); i++) {
            CIRCUIT_FPRINTF(fout, "ANTENNAPINPARTIALMETALAREA %d",
                            pin->APinPartialMetalArea(i));
            if(*(pin->APinPartialMetalAreaLayer(i)))
            CIRCUIT_FPRINTF(fout, " LAYER %s",
                            pin->APinPartialMetalAreaLayer(i));
            CIRCUIT_FPRINTF(fout, "\n");
          }
        }
        if(pin->hasAPinPartialMetalSideArea()) {
          for(i = 0; i < pin->numAPinPartialMetalSideArea(); i++) {
            CIRCUIT_FPRINTF(fout, "ANTENNAPINPARTIALMETALSIDEAREA %d",
                            pin->APinPartialMetalSideArea(i));
            if(*(pin->APinPartialMetalSideAreaLayer(i)))
            CIRCUIT_FPRINTF(fout, " LAYER %s",
                            pin->APinPartialMetalSideAreaLayer(i));
            CIRCUIT_FPRINTF(fout, "\n");
          }
        }
        if(pin->hasAPinDiffArea()) {
          for(i = 0; i < pin->numAPinDiffArea(); i++) {
            CIRCUIT_FPRINTF(fout, "ANTENNAPINDIFFAREA %d",
                            pin->APinDiffArea(i));
            if(*(pin->APinDiffAreaLayer(i)))
            CIRCUIT_FPRINTF(fout, " LAYER %s", pin->APinDiffAreaLayer(i));
            CIRCUIT_FPRINTF(fout, "\n");
          }
        }
        if(pin->hasAPinPartialCutArea()) {
          for(i = 0; i < pin->numAPinPartialCutArea(); i++) {
            CIRCUIT_FPRINTF(fout, "ANTENNAPINPARTIALCUTAREA %d",
                            pin->APinPartialCutArea(i));
            if(*(pin->APinPartialCutAreaLayer(i)))
            CIRCUIT_FPRINTF(fout, " LAYER %s",
                            pin->APinPartialCutAreaLayer(i));
            CIRCUIT_FPRINTF(fout, "\n");
          }
        }

        for(j = 0; j < pin->numAntennaModel(); j++) {
          aModel = pin->antennaModel(j);

          CIRCUIT_FPRINTF(fout, "ANTENNAMODEL %s\n", aModel->antennaOxide());

          if(aModel->hasAPinGateArea()) {
            for(i = 0; i < aModel->numAPinGateArea(); i++) {
              CIRCUIT_FPRINTF(fout, "ANTENNAPINGATEAREA %d",
                              aModel->APinGateArea(i));
              if(aModel->hasAPinGateAreaLayer(i))
              CIRCUIT_FPRINTF(fout, " LAYER %s",
                              aModel->APinGateAreaLayer(i));
              CIRCUIT_FPRINTF(fout, "\n");
            }
          }
          if(aModel->hasAPinMaxAreaCar()) {
            for(i = 0; i < aModel->numAPinMaxAreaCar(); i++) {
              CIRCUIT_FPRINTF(fout, "ANTENNAPINMAXAREACAR %d",
                              aModel->APinMaxAreaCar(i));
              if(aModel->hasAPinMaxAreaCarLayer(i))
              CIRCUIT_FPRINTF(fout, " LAYER %s",
                              aModel->APinMaxAreaCarLayer(i));
              CIRCUIT_FPRINTF(fout, "\n");
            }
          }
          if(aModel->hasAPinMaxSideAreaCar()) {
            for(i = 0; i < aModel->numAPinMaxSideAreaCar(); i++) {
              CIRCUIT_FPRINTF(fout, "ANTENNAPINMAXSIDEAREACAR %d",
                              aModel->APinMaxSideAreaCar(i));
              if(aModel->hasAPinMaxSideAreaCarLayer(i))
              CIRCUIT_FPRINTF(fout, " LAYER %s",
                              aModel->APinMaxSideAreaCarLayer(i));
              CIRCUIT_FPRINTF(fout, "\n");
            }
          }
          if(aModel->hasAPinMaxCutCar()) {
            for(i = 0; i < aModel->numAPinMaxCutCar(); i++) {
              CIRCUIT_FPRINTF(fout, "ANTENNAPINMAXCUTCAR %d",
                              aModel->APinMaxCutCar(i));
              if(aModel->hasAPinMaxCutCarLayer(i))
              CIRCUIT_FPRINTF(fout, " LAYER %s",
                              aModel->APinMaxCutCarLayer(i));
              CIRCUIT_FPRINTF(fout, "\n");
            }
          }
        }
        CIRCUIT_FPRINTF(fout, ";\n");
        --numObjs;
        if(numObjs <= 0)
        CIRCUIT_FPRINTF(fout, "END PINS\n");
      }
      break;
    case defrDefaultCapCbkType:
      i = (long)cl;
      CIRCUIT_FPRINTF(fout, "DEFAULTCAP %d\n", i);
      numObjs = i;
      break;
    case defrRowCbkType:
      row = (defiRow*)cl;
      defRowStorPtr->push_back(*row);
      (*defRowY2OrientMapPtr)[row->y()] = row->orient();

      CIRCUIT_FPRINTF(fout, "ROW %s %s %g %.0f %s ",
                      ignoreRowNames ? "XXX" : row->name(), row->macro(),
                      row->x(), row->y(), orientStr(row->orient()));

      if(row->hasDo()) {
        CIRCUIT_FPRINTF(fout, "DO %g BY %g ", row->xNum(), row->yNum());
        if(row->hasDoStep()) {
          CIRCUIT_FPRINTF(fout, "STEP %g %g ;\n", row->xStep(), row->yStep());
        }
        else {
          CIRCUIT_FPRINTF(fout, ";\n");
        }
      }
      else
      CIRCUIT_FPRINTF(fout, ";\n");
      if(row->numProps() > 0) {
        for(i = 0; i < row->numProps(); i++) {
          CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %s ", row->propName(i),
                          row->propValue(i));
          switch(row->propType(i)) {
            case 'R':
            CIRCUIT_FPRINTF(fout, "REAL ");
              break;
            case 'I':
            CIRCUIT_FPRINTF(fout, "INTEGER ");
              break;
            case 'S':
            CIRCUIT_FPRINTF(fout, "STRING ");
              break;
            case 'Q':
            CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
              break;
            case 'N':
            CIRCUIT_FPRINTF(fout, "NUMBER ");
              break;
          }
        }
        CIRCUIT_FPRINTF(fout, ";\n");
      }
      break;
    case defrTrackCbkType:
      track = (defiTrack*)cl;
      defTrackStorPtr->push_back(*track);
      if(track->firstTrackMask()) {
        if(track->sameMask()) {
          CIRCUIT_FPRINTF(fout,
                          "TRACKS %s %g DO %g STEP %g MASK %d SAMEMASK LAYER ",
                          track->macro(), track->x(), track->xNum(),
                          track->xStep(), track->firstTrackMask());
        }
        else {
          CIRCUIT_FPRINTF(fout, "TRACKS %s %g DO %g STEP %g MASK %d LAYER ",
                          track->macro(), track->x(), track->xNum(),
                          track->xStep(), track->firstTrackMask());
        }
      }
      else {
        CIRCUIT_FPRINTF(fout, "TRACKS %s %g DO %g STEP %g LAYER ",
                        track->macro(), track->x(), track->xNum(),
                        track->xStep());
      }
      for(i = 0; i < track->numLayers(); i++)
      CIRCUIT_FPRINTF(fout, "%s ", track->layer(i));
      CIRCUIT_FPRINTF(fout, ";\n");
      break;
    case defrGcellGridCbkType:
      gcg = (defiGcellGrid*)cl;
      defGcellGridStorPtr->push_back(*gcg);
      CIRCUIT_FPRINTF(fout, "GCELLGRID %s %d DO %d STEP %g ;\n", gcg->macro(),
                      gcg->x(), gcg->xNum(), gcg->xStep());
      break;
    case defrViaCbkType:
      via = (defiVia*)cl;
      if(testDebugPrint) {
        via->print(fout);
      }
      else {
        CIRCUIT_FPRINTF(fout, "- %s ", via->name());
        if(via->hasPattern())
        CIRCUIT_FPRINTF(fout, "+ PATTERNNAME %s ", via->pattern());
        for(i = 0; i < via->numLayers(); i++) {
          via->layer(i, &name, &xl, &yl, &xh, &yh);
          int rectMask = via->rectMask(i);

          if(rectMask) {
            CIRCUIT_FPRINTF(fout, "+ RECT %s + MASK %d %d %d %d %d \n", name,
                            rectMask, xl, yl, xh, yh);
          }
          else {
            CIRCUIT_FPRINTF(fout, "+ RECT %s %d %d %d %d \n", name, xl, yl, xh,
                            yh);
          }
        }
        // POLYGON
        if(via->numPolygons()) {
          struct defiPoints points;
          for(i = 0; i < via->numPolygons(); i++) {
            int polyMask = via->polyMask(i);

            if(polyMask) {
              CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s + MASK %d ",
                              via->polygonName(i), polyMask);
            }
            else {
              CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", via->polygonName(i));
            }
            points = via->getPolygon(i);
            for(j = 0; j < points.numPoints; j++)
            CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
          }
        }
        CIRCUIT_FPRINTF(fout, " ;\n");
        if(via->hasViaRule()) {
          char *vrn, *bl, *cl, *tl;
          int xs, ys, xcs, ycs, xbe, ybe, xte, yte;
          int cr, cc, xo, yo, xbo, ybo, xto, yto;
          (void)via->viaRule(&vrn, &xs, &ys, &bl, &cl, &tl, &xcs, &ycs, &xbe,
                             &ybe, &xte, &yte);
          CIRCUIT_FPRINTF(fout, "+ VIARULE '%s'\n",
                          ignoreViaNames ? "XXX" : vrn);
          CIRCUIT_FPRINTF(fout, "  + CUTSIZE %d %d\n", xs, ys);
          CIRCUIT_FPRINTF(fout, "  + LAYERS %s %s %s\n", bl, cl, tl);
          CIRCUIT_FPRINTF(fout, "  + CUTSPACING %d %d\n", xcs, ycs);
          CIRCUIT_FPRINTF(fout, "  + ENCLOSURE %d %d %d %d\n", xbe, ybe, xte,
                          yte);
          if(via->hasRowCol()) {
            (void)via->rowCol(&cr, &cc);
            CIRCUIT_FPRINTF(fout, "  + ROWCOL %d %d\n", cr, cc);
          }
          if(via->hasOrigin()) {
            (void)via->origin(&xo, &yo);
            CIRCUIT_FPRINTF(fout, "  + ORIGIN %d %d\n", xo, yo);
          }
          if(via->hasOffset()) {
            (void)via->offset(&xbo, &ybo, &xto, &yto);
            CIRCUIT_FPRINTF(fout, "  + OFFSET %d %d %d %d\n", xbo, ybo, xto,
                            yto);
          }
          if(via->hasCutPattern())
          CIRCUIT_FPRINTF(fout, "  + PATTERN '%s'\n", via->cutPattern());
        }
        --numObjs;
        if(numObjs <= 0)
        CIRCUIT_FPRINTF(fout, "END VIAS\n");
      }
      defViaStorPtr->push_back(*via);
      break;
    case defrRegionCbkType:
      re = (defiRegion*)cl;
      CIRCUIT_FPRINTF(fout, "- %s ", re->name());
      for(i = 0; i < re->numRectangles(); i++)
      CIRCUIT_FPRINTF(fout, "%d %d %d %d \n", re->xl(i), re->yl(i), re->xh(i),
                      re->yh(i));
      if(re->hasType())
      CIRCUIT_FPRINTF(fout, "+ TYPE %s\n", re->type());
      if(re->numProps()) {
        for(i = 0; i < re->numProps(); i++) {
          CIRCUIT_FPRINTF(fout, "+ PROPERTY %s %s ", re->propName(i),
                          re->propValue(i));
          switch(re->propType(i)) {
            case 'R':
            CIRCUIT_FPRINTF(fout, "REAL ");
              break;
            case 'I':
            CIRCUIT_FPRINTF(fout, "INTEGER ");
              break;
            case 'S':
            CIRCUIT_FPRINTF(fout, "STRING ");
              break;
            case 'Q':
            CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
              break;
            case 'N':
            CIRCUIT_FPRINTF(fout, "NUMBER ");
              break;
          }
        }
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      --numObjs;
      if(numObjs <= 0) {
        CIRCUIT_FPRINTF(fout, "END REGIONS\n");
      }
      break;
    case defrGroupNameCbkType:
      if((char*)cl) {
        CIRCUIT_FPRINTF(fout, "- %s", (char*)cl);
      }
      break;
    case defrGroupMemberCbkType:
      if((char*)cl) {
        CIRCUIT_FPRINTF(fout, " %s", (char*)cl);
      }
      break;
    case defrComponentMaskShiftLayerCbkType:
    CIRCUIT_FPRINTF(fout, "COMPONENTMASKSHIFT ");

      for(i = 0; i < maskShiftLayer->numMaskShiftLayers(); i++) {
        CIRCUIT_FPRINTF(fout, "%s ", maskShiftLayer->maskShiftLayer(i));
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      break;
    case defrGroupCbkType:
      group = (defiGroup*)cl;
      if(group->hasMaxX() | group->hasMaxY() | group->hasPerim()) {
        CIRCUIT_FPRINTF(fout, "\n  + SOFT ");
        if(group->hasPerim())
        CIRCUIT_FPRINTF(fout, "MAXHALFPERIMETER %d ", group->perim());
        if(group->hasMaxX())
        CIRCUIT_FPRINTF(fout, "MAXX %d ", group->maxX());
        if(group->hasMaxY())
        CIRCUIT_FPRINTF(fout, "MAXY %d ", group->maxY());
      }
      if(group->hasRegionName())
      CIRCUIT_FPRINTF(fout, "\n  + REGION %s ", group->regionName());
      if(group->hasRegionBox()) {
        int *gxl, *gyl, *gxh, *gyh;
        int size;
        group->regionRects(&size, &gxl, &gyl, &gxh, &gyh);
        for(i = 0; i < size; i++)
        CIRCUIT_FPRINTF(fout, "REGION %d %d %d %d ", gxl[i], gyl[i], gxh[i],
                        gyh[i]);
      }
      if(group->numProps()) {
        for(i = 0; i < group->numProps(); i++) {
          CIRCUIT_FPRINTF(fout, "\n  + PROPERTY %s %s ", group->propName(i),
                          group->propValue(i));
          switch(group->propType(i)) {
            case 'R':
            CIRCUIT_FPRINTF(fout, "REAL ");
              break;
            case 'I':
            CIRCUIT_FPRINTF(fout, "INTEGER ");
              break;
            case 'S':
            CIRCUIT_FPRINTF(fout, "STRING ");
              break;
            case 'Q':
            CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
              break;
            case 'N':
            CIRCUIT_FPRINTF(fout, "NUMBER ");
              break;
          }
        }
      }
      CIRCUIT_FPRINTF(fout, " ;\n");
      --numObjs;
      if(numObjs <= 0)
      CIRCUIT_FPRINTF(fout, "END GROUPS\n");
      break;
    case defrScanchainCbkType:
      sc = (defiScanchain*)cl;
      CIRCUIT_FPRINTF(fout, "- %s\n", sc->name());
      if(sc->hasStart()) {
        sc->start(&a1, &b1);
        CIRCUIT_FPRINTF(fout, "  + START %s %s\n", a1, b1);
      }
      if(sc->hasStop()) {
        sc->stop(&a1, &b1);
        CIRCUIT_FPRINTF(fout, "  + STOP %s %s\n", a1, b1);
      }
      if(sc->hasCommonInPin() || sc->hasCommonOutPin()) {
        CIRCUIT_FPRINTF(fout, "  + COMMONSCANPINS ");
        if(sc->hasCommonInPin())
        CIRCUIT_FPRINTF(fout, " ( IN %s ) ", sc->commonInPin());
        if(sc->hasCommonOutPin())
        CIRCUIT_FPRINTF(fout, " ( OUT %s ) ", sc->commonOutPin());
        CIRCUIT_FPRINTF(fout, "\n");
      }
      if(sc->hasFloating()) {
        sc->floating(&size, &inst, &inPin, &outPin, &bits);
        if(size > 0)
        CIRCUIT_FPRINTF(fout, "  + FLOATING\n");
        for(i = 0; i < size; i++) {
          CIRCUIT_FPRINTF(fout, "    %s ", inst[i]);
          if(inPin[i])
          CIRCUIT_FPRINTF(fout, "( IN %s ) ", inPin[i]);
          if(outPin[i])
          CIRCUIT_FPRINTF(fout, "( OUT %s ) ", outPin[i]);
          if(bits[i] != -1)
          CIRCUIT_FPRINTF(fout, "( BITS %d ) ", bits[i]);
          CIRCUIT_FPRINTF(fout, "\n");
        }
      }

      if(sc->hasOrdered()) {
        for(i = 0; i < sc->numOrderedLists(); i++) {
          sc->ordered(i, &size, &inst, &inPin, &outPin, &bits);
          if(size > 0)
          CIRCUIT_FPRINTF(fout, "  + ORDERED\n");
          for(j = 0; j < size; j++) {
            CIRCUIT_FPRINTF(fout, "    %s ", inst[j]);
            if(inPin[j])
            CIRCUIT_FPRINTF(fout, "( IN %s ) ", inPin[j]);
            if(outPin[j])
            CIRCUIT_FPRINTF(fout, "( OUT %s ) ", outPin[j]);
            if(bits[j] != -1)
            CIRCUIT_FPRINTF(fout, "( BITS %d ) ", bits[j]);
            CIRCUIT_FPRINTF(fout, "\n");
          }
        }
      }

      if(sc->hasPartition()) {
        CIRCUIT_FPRINTF(fout, "  + PARTITION %s ", sc->partitionName());
        if(sc->hasPartitionMaxBits())
        CIRCUIT_FPRINTF(fout, "MAXBITS %d ", sc->partitionMaxBits());
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      --numObjs;
      if(numObjs <= 0)
      CIRCUIT_FPRINTF(fout, "END SCANCHAINS\n");
      break;
    case defrIOTimingCbkType:
      iot = (defiIOTiming*)cl;
      CIRCUIT_FPRINTF(fout, "- ( %s %s )\n", iot->inst(), iot->pin());
      if(iot->hasSlewRise())
      CIRCUIT_FPRINTF(fout, "  + RISE SLEWRATE %g %g\n", iot->slewRiseMin(),
                      iot->slewRiseMax());
      if(iot->hasSlewFall())
      CIRCUIT_FPRINTF(fout, "  + FALL SLEWRATE %g %g\n", iot->slewFallMin(),
                      iot->slewFallMax());
      if(iot->hasVariableRise())
      CIRCUIT_FPRINTF(fout, "  + RISE VARIABLE %g %g\n",
                      iot->variableRiseMin(), iot->variableRiseMax());
      if(iot->hasVariableFall())
      CIRCUIT_FPRINTF(fout, "  + FALL VARIABLE %g %g\n",
                      iot->variableFallMin(), iot->variableFallMax());
      if(iot->hasCapacitance())
      CIRCUIT_FPRINTF(fout, "  + CAPACITANCE %g\n", iot->capacitance());
      if(iot->hasDriveCell()) {
        CIRCUIT_FPRINTF(fout, "  + DRIVECELL %s ", iot->driveCell());
        if(iot->hasFrom())
        CIRCUIT_FPRINTF(fout, "  FROMPIN %s ", iot->from());
        if(iot->hasTo())
        CIRCUIT_FPRINTF(fout, "  TOPIN %s ", iot->to());
        if(iot->hasParallel())
        CIRCUIT_FPRINTF(fout, "PARALLEL %g", iot->parallel());
        CIRCUIT_FPRINTF(fout, "\n");
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      --numObjs;
      if(numObjs <= 0)
      CIRCUIT_FPRINTF(fout, "END IOTIMINGS\n");
      break;
    case defrFPCCbkType:
      fpc = (defiFPC*)cl;
      CIRCUIT_FPRINTF(fout, "- %s ", fpc->name());
      if(fpc->isVertical())
      CIRCUIT_FPRINTF(fout, "VERTICAL ");
      if(fpc->isHorizontal())
      CIRCUIT_FPRINTF(fout, "HORIZONTAL ");
      if(fpc->hasAlign())
      CIRCUIT_FPRINTF(fout, "ALIGN ");
      if(fpc->hasMax())
      CIRCUIT_FPRINTF(fout, "%g ", fpc->alignMax());
      if(fpc->hasMin())
      CIRCUIT_FPRINTF(fout, "%g ", fpc->alignMin());
      if(fpc->hasEqual())
      CIRCUIT_FPRINTF(fout, "%g ", fpc->equal());
      for(i = 0; i < fpc->numParts(); i++) {
        fpc->getPart(i, &corner, &typ, &name);
        if(corner == 'B') {
          CIRCUIT_FPRINTF(fout, "BOTTOMLEFT ");
        }
        else {
          CIRCUIT_FPRINTF(fout, "TOPRIGHT ");
        }
        if(typ == 'R') {
          CIRCUIT_FPRINTF(fout, "ROWS %s ", name);
        }
        else {
          CIRCUIT_FPRINTF(fout, "COMPS %s ", name);
        }
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      --numObjs;
      if(numObjs <= 0)
      CIRCUIT_FPRINTF(fout, "END FLOORPLANCONSTRAINTS\n");
      break;
    case defrTimingDisableCbkType:
      td = (defiTimingDisable*)cl;
      if(td->hasFromTo())
      CIRCUIT_FPRINTF(fout, "- FROMPIN %s %s ", td->fromInst(), td->fromPin(),
                      td->toInst(), td->toPin());
      if(td->hasThru())
      CIRCUIT_FPRINTF(fout, "- THRUPIN %s %s ", td->thruInst(),
                      td->thruPin());
      if(td->hasMacroFromTo())
      CIRCUIT_FPRINTF(fout, "- MACRO %s FROMPIN %s %s ", td->macroName(),
                      td->fromPin(), td->toPin());
      if(td->hasMacroThru())
      CIRCUIT_FPRINTF(fout, "- MACRO %s THRUPIN %s %s ", td->macroName(),
                      td->fromPin());
      CIRCUIT_FPRINTF(fout, ";\n");
      break;
    case defrPartitionCbkType:
      part = (defiPartition*)cl;
      CIRCUIT_FPRINTF(fout, "- %s ", part->name());
      if(part->isSetupRise() | part->isSetupFall() | part->isHoldRise() |
          part->isHoldFall()) {
        // has turnoff
        CIRCUIT_FPRINTF(fout, "TURNOFF ");
        if(part->isSetupRise())
        CIRCUIT_FPRINTF(fout, "SETUPRISE ");
        if(part->isSetupFall())
        CIRCUIT_FPRINTF(fout, "SETUPFALL ");
        if(part->isHoldRise())
        CIRCUIT_FPRINTF(fout, "HOLDRISE ");
        if(part->isHoldFall())
        CIRCUIT_FPRINTF(fout, "HOLDFALL ");
      }
      itemT = part->itemType();
      dir = part->direction();
      if(strcmp(itemT, "CLOCK") == 0) {
        if(dir == 'T')  // toclockpin
        CIRCUIT_FPRINTF(fout, "+ TOCLOCKPIN %s %s ", part->instName(),
                        part->pinName());
        if(dir == 'F')  // fromclockpin
        CIRCUIT_FPRINTF(fout, "+ FROMCLOCKPIN %s %s ", part->instName(),
                        part->pinName());
        if(part->hasMin())
        CIRCUIT_FPRINTF(fout, "MIN %g %g ", part->partitionMin(),
                        part->partitionMax());
        if(part->hasMax())
        CIRCUIT_FPRINTF(fout, "MAX %g %g ", part->partitionMin(),
                        part->partitionMax());
        CIRCUIT_FPRINTF(fout, "PINS ");
        for(i = 0; i < part->numPins(); i++)
        CIRCUIT_FPRINTF(fout, "%s ", part->pin(i));
      }
      else if(strcmp(itemT, "IO") == 0) {
        if(dir == 'T')  // toiopin
        CIRCUIT_FPRINTF(fout, "+ TOIOPIN %s %s ", part->instName(),
                        part->pinName());
        if(dir == 'F')  // fromiopin
        CIRCUIT_FPRINTF(fout, "+ FROMIOPIN %s %s ", part->instName(),
                        part->pinName());
      }
      else if(strcmp(itemT, "COMP") == 0) {
        if(dir == 'T')  // tocomppin
        CIRCUIT_FPRINTF(fout, "+ TOCOMPPIN %s %s ", part->instName(),
                        part->pinName());
        if(dir == 'F')  // fromcomppin
        CIRCUIT_FPRINTF(fout, "+ FROMCOMPPIN %s %s ", part->instName(),
                        part->pinName());
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      --numObjs;
      if(numObjs <= 0)
      CIRCUIT_FPRINTF(fout, "END PARTITIONS\n");
      break;

    case defrPinPropCbkType:
      pprop = (defiPinProp*)cl;
      if(pprop->isPin()) {
        CIRCUIT_FPRINTF(fout, "- PIN %s ", pprop->pinName());
      }
      else {
        CIRCUIT_FPRINTF(fout, "- %s %s ", pprop->instName(), pprop->pinName());
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      if(pprop->numProps() > 0) {
        for(i = 0; i < pprop->numProps(); i++) {
          CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %s ", pprop->propName(i),
                          pprop->propValue(i));
          switch(pprop->propType(i)) {
            case 'R':
            CIRCUIT_FPRINTF(fout, "REAL ");
              break;
            case 'I':
            CIRCUIT_FPRINTF(fout, "INTEGER ");
              break;
            case 'S':
            CIRCUIT_FPRINTF(fout, "STRING ");
              break;
            case 'Q':
            CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
              break;
            case 'N':
            CIRCUIT_FPRINTF(fout, "NUMBER ");
              break;
          }
        }
        CIRCUIT_FPRINTF(fout, ";\n");
      }
      --numObjs;
      if(numObjs <= 0)
      CIRCUIT_FPRINTF(fout, "END PINPROPERTIES\n");
      break;
    case defrBlockageCbkType:
      block = (defiBlockage*)cl;
      defBlockageStorPtr->push_back(*block);
      if(testDebugPrint) {
        block->print(fout);
      }
      else {
        if(block->hasLayer()) {
          CIRCUIT_FPRINTF(fout, "- LAYER %s\n", block->layerName());
          if(block->hasComponent())
          CIRCUIT_FPRINTF(fout, "   + COMPONENT %s\n",
                          block->layerComponentName());
          if(block->hasSlots())
          CIRCUIT_FPRINTF(fout, "   + SLOTS\n");
          if(block->hasFills())
          CIRCUIT_FPRINTF(fout, "   + FILLS\n");
          if(block->hasPushdown())
          CIRCUIT_FPRINTF(fout, "   + PUSHDOWN\n");
          if(block->hasExceptpgnet())
          CIRCUIT_FPRINTF(fout, "   + EXCEPTPGNET\n");
          if(block->hasMask())
          CIRCUIT_FPRINTF(fout, "   + MASK %d\n", block->mask());
          if(block->hasSpacing())
          CIRCUIT_FPRINTF(fout, "   + SPACING %d\n", block->minSpacing());
          if(block->hasDesignRuleWidth())
          CIRCUIT_FPRINTF(fout, "   + DESIGNRULEWIDTH %d\n",
                          block->designRuleWidth());
        }
        else if(block->hasPlacement()) {
          CIRCUIT_FPRINTF(fout, "- PLACEMENT\n");
          if(block->hasSoft())
          CIRCUIT_FPRINTF(fout, "   + SOFT\n");
          if(block->hasPartial())
          CIRCUIT_FPRINTF(fout, "   + PARTIAL %g\n",
                          block->placementMaxDensity());
          if(block->hasComponent())
          CIRCUIT_FPRINTF(fout, "   + COMPONENT %s\n",
                          block->placementComponentName());
          if(block->hasPushdown())
          CIRCUIT_FPRINTF(fout, "   + PUSHDOWN\n");
        }

        for(i = 0; i < block->numRectangles(); i++) {
          CIRCUIT_FPRINTF(fout, "   RECT %d %d %d %d\n", block->xl(i),
                          block->yl(i), block->xh(i), block->yh(i));
        }

        for(i = 0; i < block->numPolygons(); i++) {
          CIRCUIT_FPRINTF(fout, "   POLYGON ");
          points = block->getPolygon(i);
          for(j = 0; j < points.numPoints; j++)
          CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
          CIRCUIT_FPRINTF(fout, "\n");
        }
        CIRCUIT_FPRINTF(fout, ";\n");
        --numObjs;
        if(numObjs <= 0)
        CIRCUIT_FPRINTF(fout, "END BLOCKAGES\n");
      }
      break;
    case defrSlotCbkType:
      slots = (defiSlot*)cl;
      if(slots->hasLayer())
      CIRCUIT_FPRINTF(fout, "- LAYER %s\n", slots->layerName());

      for(i = 0; i < slots->numRectangles(); i++) {
        CIRCUIT_FPRINTF(fout, "   RECT %d %d %d %d\n", slots->xl(i),
                        slots->yl(i), slots->xh(i), slots->yh(i));
      }
      for(i = 0; i < slots->numPolygons(); i++) {
        CIRCUIT_FPRINTF(fout, "   POLYGON ");
        points = slots->getPolygon(i);
        for(j = 0; j < points.numPoints; j++)
        CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
        CIRCUIT_FPRINTF(fout, ";\n");
      }
      CIRCUIT_FPRINTF(fout, ";\n");
      --numObjs;
      if(numObjs <= 0)
      CIRCUIT_FPRINTF(fout, "END SLOTS\n");
      break;
    case defrFillCbkType:
      fills = (defiFill*)cl;
      if(testDebugPrint) {
        fills->print(fout);
      }
      else {
        if(fills->hasLayer()) {
          CIRCUIT_FPRINTF(fout, "- LAYER %s", fills->layerName());
          if(fills->layerMask()) {
            CIRCUIT_FPRINTF(fout, " + MASK %d", fills->layerMask());
          }
          if(fills->hasLayerOpc())
          CIRCUIT_FPRINTF(fout, " + OPC");
          CIRCUIT_FPRINTF(fout, "\n");

          for(i = 0; i < fills->numRectangles(); i++) {
            CIRCUIT_FPRINTF(fout, "   RECT %d %d %d %d\n", fills->xl(i),
                            fills->yl(i), fills->xh(i), fills->yh(i));
          }
          for(i = 0; i < fills->numPolygons(); i++) {
            CIRCUIT_FPRINTF(fout, "   POLYGON ");
            points = fills->getPolygon(i);
            for(j = 0; j < points.numPoints; j++)
            CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
            CIRCUIT_FPRINTF(fout, ";\n");
          }
          CIRCUIT_FPRINTF(fout, ";\n");
        }
        --numObjs;
        if(fills->hasVia()) {
          CIRCUIT_FPRINTF(fout, "- VIA %s", fills->viaName());
          if(fills->viaTopMask() || fills->viaCutMask() ||
              fills->viaBottomMask()) {
            CIRCUIT_FPRINTF(fout, " + MASK %d%d%d", fills->viaTopMask(),
                            fills->viaCutMask(), fills->viaBottomMask());
          }
          if(fills->hasViaOpc())
          CIRCUIT_FPRINTF(fout, " + OPC");
          CIRCUIT_FPRINTF(fout, "\n");

          for(i = 0; i < fills->numViaPts(); i++) {
            points = fills->getViaPts(i);
            for(j = 0; j < points.numPoints; j++)
            CIRCUIT_FPRINTF(fout, " %d %d", points.x[j], points.y[j]);
            CIRCUIT_FPRINTF(fout, ";\n");
          }
          CIRCUIT_FPRINTF(fout, ";\n");
        }
        if(numObjs <= 0)
        CIRCUIT_FPRINTF(fout, "END FILLS\n");
      }
      break;
    case defrStylesCbkType:
      //            struct defiPoints points;
      styles = (defiStyles*)cl;
      CIRCUIT_FPRINTF(fout, "- STYLE %d ", styles->style());
      points = styles->getPolygon();
      for(j = 0; j < points.numPoints; j++)
      CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
      CIRCUIT_FPRINTF(fout, ";\n");
      --numObjs;
      if(numObjs <= 0)
      CIRCUIT_FPRINTF(fout, "END STYLES\n");
      break;

    default:
    CIRCUIT_FPRINTF(fout, "BOGUS callback to cls.\n");
      return 1;
  }
  return 0;
}

int dn(defrCallbackType_e c, const char* h, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();

  (*defDividerCharPtr) = string(h);
  isDividerCharVisit = true;

  CIRCUIT_FPRINTF(fout, "DIVIDERCHAR \"%s\" ;\n", h);
  return 0;
}

int ext(defrCallbackType_e t, const char* c, defiUserData ud) {
  char* name;

  checkType(t);
  if(ud != userData)
    dataError();

  switch(t) {
    case defrNetExtCbkType:
      name = address("net");
      break;
    case defrComponentExtCbkType:
      name = address("component");
      break;
    case defrPinExtCbkType:
      name = address("pin");
      break;
    case defrViaExtCbkType:
      name = address("via");
      break;
    case defrNetConnectionExtCbkType:
      name = address("net connection");
      break;
    case defrGroupExtCbkType:
      name = address("group");
      break;
    case defrScanChainExtCbkType:
      name = address("scanchain");
      break;
    case defrIoTimingsExtCbkType:
      name = address("io timing");
      break;
    case defrPartitionsExtCbkType:
      name = address("partition");
      break;
    default:
      name = address("BOGUS");
      return 1;
  }
  CIRCUIT_FPRINTF(fout, "  %s extension %s\n", name, c);
  return 0;
}

int extension(defrCallbackType_e c, const char* extsn, defiUserData ud) {
  checkType(c);
  if(ud != userData)
    dataError();
  CIRCUIT_FPRINTF(fout, "BEGINEXT %s\n", extsn);
  return 0;
}

void* mallocCB(size_t size) {
  return malloc(size);
}

void* reallocCB(void* name, size_t size) {
  return realloc(name, size);
}


BEGIN_LEFDEF_PARSER_NAMESPACE
extern long long nlines;
END_LEFDEF_PARSER_NAMESPACE
static int ccr1131444 = 0;

void lineNumberCB(long long lineNo) {
  // The CCR 1131444 tests ability of the DEF parser to count
  // input line numbers out of 32-bit int range. On the first callback
  // call 10G lines will be added to line counter. It should be
  // reflected in output.
  if(ccr1131444) {
    lineNo += 10000000000LL;
    defrSetNLines(lineNo);
    ccr1131444 = 0;
  }
  cout << "[DEF] Parsed " << lineNo / 1000 << "k number of lines!!" << endl;
}

int unUsedCB(defrCallbackType_e, void*, defiUserData) {
  CIRCUIT_FPRINTF(fout, "This callback is not used.\n");
  return 0;
}


///////////////////////////////////////////////////////////////////////
//
// Print function for DEF writer
//
///////////////////////////////////////////////////////////////////////

void ePlace::Parser::DumpDefVersion() {
  CIRCUIT_FPRINTF(fout, "VERSION %s ;\n",
                  (isVersionVisit) ? defVersion.c_str() : "5.8");
}

void ePlace::Parser::DumpDefDividerChar() {
  CIRCUIT_FPRINTF(fout, "DIVIDERCHAR \"%s\" ;\n",
                  (isDividerCharVisit) ? defDividerChar.c_str() : "/");
}

void ePlace::Parser::DumpDefBusBitChar() {
  CIRCUIT_FPRINTF(fout, "BUSBITCHARS \"%s\" ;\n",
                  (isBusBitCharVisit) ? defBusBitChar.c_str() : "[]");
}

void ePlace::Parser::DumpDefDesignName() {
  CIRCUIT_FPRINTF(fout, "DESIGN %s ;\n",
                  (isDesignNameVisit) ? defDesignName.c_str() : "noname");
}

void ePlace::Parser::DumpDefUnit() const {
  CIRCUIT_FPRINTF(fout, "UNITS DISTANCE MICRONS %g ;\n", defUnit);
}

void ePlace::Parser::DumpDefProp() {
  if(defPropStor.size() == 0) {
    return;
  }

  CIRCUIT_FPRINTF(fout, "\nPROPERTYDEFINITIONS\n");
  for(auto& p : defPropStor) {
    if(strcmp(p.propType(), "design") == 0) {
      CIRCUIT_FPRINTF(fout, "    DESIGN %s ", p.propName());
    }
    else if(strcmp(p.propType(), "net") == 0) {
      CIRCUIT_FPRINTF(fout, "    NET %s ", p.propName());
    }
    else if(strcmp(p.propType(), "component") == 0) {
      CIRCUIT_FPRINTF(fout, "    COMPONENT %s ", p.propName());
    }
    else if(strcmp(p.propType(), "specialnet") == 0) {
      CIRCUIT_FPRINTF(fout, "    SPECIALNET %s ", p.propName());
    }
    else if(strcmp(p.propType(), "group") == 0) {
      CIRCUIT_FPRINTF(fout, "    GROUP %s ", p.propName());
    }
    else if(strcmp(p.propType(), "row") == 0) {
      CIRCUIT_FPRINTF(fout, "    ROW %s ", p.propName());
    }
    else if(strcmp(p.propType(), "componentpin") == 0) {
      CIRCUIT_FPRINTF(fout, "    COMPONENTPIN %s ", p.propName());
    }
    else if(strcmp(p.propType(), "region") == 0) {
      CIRCUIT_FPRINTF(fout, "    REGION %s ", p.propName());
    }
    else if(strcmp(p.propType(), "nondefaultrule") == 0) {
      CIRCUIT_FPRINTF(fout, "    NONDEFAULTRULE %s ", p.propName());
    }

    if(p.dataType() == 'I')
    CIRCUIT_FPRINTF(fout, "INTEGER ");
    if(p.dataType() == 'R')
    CIRCUIT_FPRINTF(fout, "REAL ");
    if(p.dataType() == 'S')
    CIRCUIT_FPRINTF(fout, "STRING ");
    if(p.dataType() == 'Q')
    CIRCUIT_FPRINTF(fout, "STRING ");
    if(p.hasRange()) {
      CIRCUIT_FPRINTF(fout, "RANGE %g %g ", p.left(), p.right());
    }
    if(p.hasNumber())
    CIRCUIT_FPRINTF(fout, "%g ", p.number());
    if(p.hasString())
    CIRCUIT_FPRINTF(fout, "\"%s\" ", p.string());
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  CIRCUIT_FPRINTF(fout, "END PROPERTYDEFINITIONS\n\n");
}

void ePlace::Parser::DumpDefDieArea() {
  defiBox* box = &defDieArea;
  CIRCUIT_FPRINTF(fout, "DIEAREA ( %d %d ) ( %d %d ) ;\n\n", box->xl(),
                  box->yl(), box->xh(), box->yh());

  //    defiPoints points = defDieArea.getPoint();
  //    for (int i = 0; i < points.numPoints; i++)
  //        CIRCUIT_FPRINTF(fout, "( %d %d ) ", points.x[i], points.y[i]);
  //    CIRCUIT_FPRINTF(fout, ";\n");
}

void ePlace::Parser::DumpDefRow() {
  if(defRowStor.size() == 0) {
    return;
  }

  for(auto& curRow : defRowStor) {
    CIRCUIT_FPRINTF(fout, "ROW %s %s %.0f %.0f %s ",
                    ignoreRowNames ? "XXX" : curRow.name(), curRow.macro(),
                    curRow.x(), curRow.y(), orientStr(curRow.orient()));
    if(curRow.hasDo()) {
      CIRCUIT_FPRINTF(fout, "DO %g BY %g ", curRow.xNum(), curRow.yNum());
      if(curRow.hasDoStep()) {
        CIRCUIT_FPRINTF(fout, "STEP %g %g ;\n", curRow.xStep(), curRow.yStep());
      }
      else {
        CIRCUIT_FPRINTF(fout, ";\n");
      }
    }
    else
    CIRCUIT_FPRINTF(fout, ";\n");
    if(curRow.numProps() > 0) {
      for(int i = 0; i < curRow.numProps(); i++) {
        CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %s ", curRow.propName(i),
                        curRow.propValue(i));
        switch(curRow.propType(i)) {
          case 'R':
          CIRCUIT_FPRINTF(fout, "REAL ");
            break;
          case 'I':
          CIRCUIT_FPRINTF(fout, "INTEGER ");
            break;
          case 'S':
          CIRCUIT_FPRINTF(fout, "STRING ");
            break;
          case 'Q':
          CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
            break;
          case 'N':
          CIRCUIT_FPRINTF(fout, "NUMBER ");
            break;
        }
      }
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }
  CIRCUIT_FPRINTF(fout, "\n");
}

void ePlace::Parser::DumpDefTrack() {
  if(defTrackStor.size() == 0) {
    return;
  }

  for(auto& curTrack : defTrackStor) {
    if(curTrack.firstTrackMask()) {
      if(curTrack.sameMask()) {
        CIRCUIT_FPRINTF(fout,
                        "TRACKS %s %g DO %g STEP %g MASK %d SAMEMASK LAYER ",
                        curTrack.macro(), curTrack.x(), curTrack.xNum(),
                        curTrack.xStep(), curTrack.firstTrackMask());
      }
      else {
        CIRCUIT_FPRINTF(fout, "TRACKS %s %g DO %g STEP %g MASK %d LAYER ",
                        curTrack.macro(), curTrack.x(), curTrack.xNum(),
                        curTrack.xStep(), curTrack.firstTrackMask());
      }
    }
    else {
      CIRCUIT_FPRINTF(fout, "TRACKS %s %g DO %g STEP %g LAYER ",
                      curTrack.macro(), curTrack.x(), curTrack.xNum(),
                      curTrack.xStep());
    }
    for(int i = 0; i < curTrack.numLayers(); i++)
    CIRCUIT_FPRINTF(fout, "%s ", curTrack.layer(i));
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  CIRCUIT_FPRINTF(fout, "\n");
}

void ePlace::Parser::DumpDefGcellGrid() {
  if(defGcellGridStor.size() == 0) {
    return;
  }

  for(auto& curGcellGrid : defGcellGridStor) {
    CIRCUIT_FPRINTF(fout, "GCELLGRID %s %d DO %d STEP %g ;\n",
                    curGcellGrid.macro(), curGcellGrid.x(), curGcellGrid.xNum(),
                    curGcellGrid.xStep());
  }
  CIRCUIT_FPRINTF(fout, "\n");
}

void ePlace::Parser::DumpDefVia() {
  if(defViaStor.size() == 0) {
    return;
  }

  CIRCUIT_FPRINTF(fout, "VIAS %d ; \n", defViaStor.size());
  for(auto& curVia : defViaStor) {
    if(testDebugPrint) {
      curVia.print(fout);
    }
    else {
      CIRCUIT_FPRINTF(fout, "- %s \n", curVia.name());
      if(curVia.hasPattern())
      CIRCUIT_FPRINTF(fout, "  + PATTERNNAME %s ", curVia.pattern());
      for(int i = 0; i < curVia.numLayers(); i++) {
        int xl, yl, xh, yh;
        char* name;

        curVia.layer(i, &name, &xl, &yl, &xh, &yh);
        int rectMask = curVia.rectMask(i);

        if(rectMask) {
          CIRCUIT_FPRINTF(fout, "  + RECT %s + MASK %d %d %d %d %d \n", name,
                          rectMask, xl, yl, xh, yh);
        }
        else {
          CIRCUIT_FPRINTF(fout, "  + RECT %s ( %d %d ) ( %d %d ) \n", name, xl,
                          yl, xh, yh);
        }
        //                free(name);
      }
      // POLYGON
      if(curVia.numPolygons()) {
        struct defiPoints points;
        for(int i = 0; i < curVia.numPolygons(); i++) {
          int polyMask = curVia.polyMask(i);

          if(polyMask) {
            CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s + MASK %d ",
                            curVia.polygonName(i), polyMask);
          }
          else {
            CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", curVia.polygonName(i));
          }
          points = curVia.getPolygon(i);
          for(int j = 0; j < points.numPoints; j++)
          CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
        }
      }
      if(curVia.hasViaRule()) {
        char *vrn, *bl, *cl, *tl;
        int xs, ys, xcs, ycs, xbe, ybe, xte, yte;
        int cr, cc, xo, yo, xbo, ybo, xto, yto;
        (void)curVia.viaRule(&vrn, &xs, &ys, &bl, &cl, &tl, &xcs, &ycs, &xbe,
                             &ybe, &xte, &yte);

        CIRCUIT_FPRINTF(fout, "\n");
        CIRCUIT_FPRINTF(fout, "  + VIARULE '%s'\n",
                        ignoreViaNames ? "XXX" : vrn);
        CIRCUIT_FPRINTF(fout, "  + CUTSIZE %d %d\n", xs, ys);
        CIRCUIT_FPRINTF(fout, "  + LAYERS %s %s %s\n", bl, cl, tl);
        CIRCUIT_FPRINTF(fout, "  + CUTSPACING %d %d\n", xcs, ycs);
        CIRCUIT_FPRINTF(fout, "  + ENCLOSURE %d %d %d %d\n", xbe, ybe, xte,
                        yte);
        if(curVia.hasRowCol()) {
          (void)curVia.rowCol(&cr, &cc);
          CIRCUIT_FPRINTF(fout, "  + ROWCOL %d %d\n", cr, cc);
        }
        if(curVia.hasOrigin()) {
          (void)curVia.origin(&xo, &yo);
          CIRCUIT_FPRINTF(fout, "  + ORIGIN %d %d\n", xo, yo);
        }
        if(curVia.hasOffset()) {
          (void)curVia.offset(&xbo, &ybo, &xto, &yto);
          CIRCUIT_FPRINTF(fout, "  + OFFSET %d %d %d %d\n", xbo, ybo, xto, yto);
        }
        if(curVia.hasCutPattern())
        CIRCUIT_FPRINTF(fout, "  + PATTERN '%s'\n", curVia.cutPattern());
      }
    }
    CIRCUIT_FPRINTF(fout, " ;\n");
  }

  CIRCUIT_FPRINTF(fout, "END VIAS\n\n");
}

void ePlace::Parser::DumpDefComponentMaskShiftLayer() {
  if(!defComponentMaskShiftLayer.numMaskShiftLayers()) {
    return;
  }

  CIRCUIT_FPRINTF(fout, "COMPONENTMASKSHIFT ");

  for(int i = 0; i < defComponentMaskShiftLayer.numMaskShiftLayers(); i++) {
    CIRCUIT_FPRINTF(fout, "%s ", defComponentMaskShiftLayer.maskShiftLayer(i));
  }
  CIRCUIT_FPRINTF(fout, ";\n\n");
}

void ePlace::Parser::DumpDefComponent() {
  if(defComponentStor.size() == 0) {
    return;
  }

  CIRCUIT_FPRINTF(fout, "COMPONENTS %lu ;\n", defComponentStor.size());
  for(auto& curCo : defComponentStor) {
    if(testDebugPrint) {
      curCo.print(fout);
    }
    else {
      int i;

      //  missing GENERATE, FOREIGN
      CIRCUIT_FPRINTF(fout, "- %s %s ", curCo.id(), curCo.name());
      //    curCo.changeIdAndName("idName", "modelName");
      //    CIRCUIT_FPRINTF(fout, "%s %s ", curCo.id(),
      //            curCo.name());
      if(curCo.hasNets()) {
        for(i = 0; i < curCo.numNets(); i++)
        CIRCUIT_FPRINTF(fout, "%s ", curCo.net(i));
      }
      if(curCo.isFixed())
      CIRCUIT_FPRINTF(fout, "+ FIXED ( %d %d ) %s ", curCo.placementX(),
                      curCo.placementY(), orientStr(curCo.placementOrient()));
      if(curCo.isCover())
      CIRCUIT_FPRINTF(fout, "+ COVER ( %d %d ) %s ", curCo.placementX(),
                      curCo.placementY(), orientStr(curCo.placementOrient()));
      if(curCo.isPlaced())
      CIRCUIT_FPRINTF(fout, "+ PLACED ( %d %d ) %s ", curCo.placementX(),
                      curCo.placementY(), orientStr(curCo.placementOrient()));
      if(curCo.isUnplaced()) {
        CIRCUIT_FPRINTF(fout, "+ UNPLACED ");
        if((curCo.placementX() != -1) || (curCo.placementY() != -1))
        CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", curCo.placementX(),
                        curCo.placementY(),
                        orientStr(curCo.placementOrient()));
      }
      if(curCo.hasSource())
      CIRCUIT_FPRINTF(fout, "+ SOURCE %s ", curCo.source());
      if(curCo.hasGenerate()) {
        CIRCUIT_FPRINTF(fout, "+ GENERATE %s ", curCo.generateName());
        if(curCo.macroName() && *(curCo.macroName()))
        CIRCUIT_FPRINTF(fout, "%s ", curCo.macroName());
      }
      if(curCo.hasWeight())
      CIRCUIT_FPRINTF(fout, "+ WEIGHT %d ", curCo.weight());
      if(curCo.hasEEQ())
      CIRCUIT_FPRINTF(fout, "+ EEQMASTER %s ", curCo.EEQ());
      if(curCo.hasRegionName())
      CIRCUIT_FPRINTF(fout, "+ REGION %s ", curCo.regionName());
      if(curCo.hasRegionBounds()) {
        int *xl, *yl, *xh, *yh;
        int size;
        curCo.regionBounds(&size, &xl, &yl, &xh, &yh);
        for(i = 0; i < size; i++) {
          CIRCUIT_FPRINTF(fout, "+ REGION %d %d %d %d \n", xl[i], yl[i], xh[i],
                          yh[i]);
        }
      }
      if(curCo.maskShiftSize()) {
        CIRCUIT_FPRINTF(fout, "+ MASKSHIFT ");

        for(int i = curCo.maskShiftSize() - 1; i >= 0; i--) {
          CIRCUIT_FPRINTF(fout, "%d", curCo.maskShift(i));
        }
        CIRCUIT_FPRINTF(fout, "\n");
      }
      if(curCo.hasHalo()) {
        int left, bottom, right, top;
        (void)curCo.haloEdges(&left, &bottom, &right, &top);
        CIRCUIT_FPRINTF(fout, "+ HALO ");
        if(curCo.hasHaloSoft())
        CIRCUIT_FPRINTF(fout, "SOFT ");
        CIRCUIT_FPRINTF(fout, "%d %d %d %d\n", left, bottom, right, top);
      }
      if(curCo.hasRouteHalo()) {
        CIRCUIT_FPRINTF(fout, "+ ROUTEHALO %d %s %s\n", curCo.haloDist(),
                        curCo.minLayer(), curCo.maxLayer());
      }
      if(curCo.hasForeignName()) {
        CIRCUIT_FPRINTF(fout, "+ FOREIGN %s %d %d %s %d ", curCo.foreignName(),
                        curCo.foreignX(), curCo.foreignY(), curCo.foreignOri(),
                        curCo.foreignOrient());
      }
      if(curCo.numProps()) {
        for(i = 0; i < curCo.numProps(); i++) {
          CIRCUIT_FPRINTF(fout, "+ PROPERTY %s %s ", curCo.propName(i),
                          curCo.propValue(i));
          switch(curCo.propType(i)) {
            case 'R':
            CIRCUIT_FPRINTF(fout, "REAL ");
              break;
            case 'I':
            CIRCUIT_FPRINTF(fout, "INTEGER ");
              break;
            case 'S':
            CIRCUIT_FPRINTF(fout, "STRING ");
              break;
            case 'Q':
            CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
              break;
            case 'N':
            CIRCUIT_FPRINTF(fout, "NUMBER ");
              break;
          }
        }
      }
      CIRCUIT_FPRINTF(fout, ";\n");
    }
  }
  CIRCUIT_FPRINTF(fout, "END COMPONENTS\n\n");
}

void ePlace::Parser::DumpDefBlockage() {
  if(defBlockageStor.size() == 0) {
    return;
  }
  int i = 0, j = 0;
  defiPoints points;

  CIRCUIT_FPRINTF(fout, "BLOCKAGES %lu ;\n", defBlockageStor.size());

  for(auto& curBlockage : defBlockageStor) {
    if(curBlockage.hasLayer()) {
      CIRCUIT_FPRINTF(fout, "- LAYER %s\n", curBlockage.layerName());
      if(curBlockage.hasComponent())
      CIRCUIT_FPRINTF(fout, "   + COMPONENT %s\n",
                      curBlockage.layerComponentName());
      if(curBlockage.hasSlots())
      CIRCUIT_FPRINTF(fout, "   + SLOTS\n");
      if(curBlockage.hasFills())
      CIRCUIT_FPRINTF(fout, "   + FILLS\n");
      if(curBlockage.hasPushdown())
      CIRCUIT_FPRINTF(fout, "   + PUSHDOWN\n");
      if(curBlockage.hasExceptpgnet())
      CIRCUIT_FPRINTF(fout, "   + EXCEPTPGNET\n");
      if(curBlockage.hasMask())
      CIRCUIT_FPRINTF(fout, "   + MASK %d\n", curBlockage.mask());
      if(curBlockage.hasSpacing())
      CIRCUIT_FPRINTF(fout, "   + SPACING %d\n", curBlockage.minSpacing());
      if(curBlockage.hasDesignRuleWidth())
      CIRCUIT_FPRINTF(fout, "   + DESIGNRULEWIDTH %d\n",
                      curBlockage.designRuleWidth());
    }
    else if(curBlockage.hasPlacement()) {
      CIRCUIT_FPRINTF(fout, "- PLACEMENT\n");
      if(curBlockage.hasSoft())
      CIRCUIT_FPRINTF(fout, "   + SOFT\n");
      if(curBlockage.hasPartial())
      CIRCUIT_FPRINTF(fout, "   + PARTIAL %g\n",
                      curBlockage.placementMaxDensity());
      if(curBlockage.hasComponent())
      CIRCUIT_FPRINTF(fout, "   + COMPONENT %s\n",
                      curBlockage.placementComponentName());
      if(curBlockage.hasPushdown())
      CIRCUIT_FPRINTF(fout, "   + PUSHDOWN\n");
    }

    for(i = 0; i < curBlockage.numRectangles(); i++) {
      CIRCUIT_FPRINTF(fout, "   RECT ( %d %d ) ( %d %d ) \n", curBlockage.xl(i),
                      curBlockage.yl(i), curBlockage.xh(i), curBlockage.yh(i));
    }

    for(i = 0; i < curBlockage.numPolygons(); i++) {
      CIRCUIT_FPRINTF(fout, "   POLYGON ");
      points = curBlockage.getPolygon(i);
      for(j = 0; j < points.numPoints; j++)
      CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
      CIRCUIT_FPRINTF(fout, "\n");
    }
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  CIRCUIT_FPRINTF(fout, "END BLOCKAGES\n\n");
}

void ePlace::Parser::DumpDefPin() {
  if(defPinStor.size() == 0) {
    return;
  }

  int i = 0, j = 0, k = 0;
  int xl = 0, yl = 0, xh = 0, yh = 0;

  CIRCUIT_FPRINTF(fout, "PINS %lu ;\n", defPinStor.size());
  for(auto& curPin : defPinStor) {
    CIRCUIT_FPRINTF(fout, "- %s + NET %s ", curPin.pinName(), curPin.netName());
    //         curPin.changePinName("pinName");
    //         CIRCUIT_FPRINTF(fout, "%s ", curPin.pinName());
    if(curPin.hasDirection())
    CIRCUIT_FPRINTF(fout, "+ DIRECTION %s ", curPin.direction());
    if(curPin.hasUse())
    CIRCUIT_FPRINTF(fout, "+ USE %s ", curPin.use());
    if(curPin.hasNetExpr())
    CIRCUIT_FPRINTF(fout, "+ NETEXPR \"%s\" ", curPin.netExpr());
    if(curPin.hasSupplySensitivity())
    CIRCUIT_FPRINTF(fout, "+ SUPPLYSENSITIVITY %s ",
                    curPin.supplySensitivity());
    if(curPin.hasGroundSensitivity())
    CIRCUIT_FPRINTF(fout, "+ GROUNDSENSITIVITY %s ",
                    curPin.groundSensitivity());
    if(curPin.hasLayer()) {
      struct defiPoints points;
      for(i = 0; i < curPin.numLayer(); i++) {
        CIRCUIT_FPRINTF(fout, "\n  + LAYER %s ", curPin.layer(i));
        if(curPin.layerMask(i))
        CIRCUIT_FPRINTF(fout, "MASK %d ", curPin.layerMask(i));
        if(curPin.hasLayerSpacing(i))
        CIRCUIT_FPRINTF(fout, "SPACING %d ", curPin.layerSpacing(i));
        if(curPin.hasLayerDesignRuleWidth(i))
        CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                        curPin.layerDesignRuleWidth(i));
        curPin.bounds(i, &xl, &yl, &xh, &yh);
        CIRCUIT_FPRINTF(fout, "( %d %d ) ( %d %d ) ", xl, yl, xh, yh);
      }
      for(i = 0; i < curPin.numPolygons(); i++) {
        CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", curPin.polygonName(i));
        if(curPin.polygonMask(i))
        CIRCUIT_FPRINTF(fout, "MASK %d ", curPin.polygonMask(i));
        if(curPin.hasPolygonSpacing(i))
        CIRCUIT_FPRINTF(fout, "SPACING %d ", curPin.polygonSpacing(i));
        if(curPin.hasPolygonDesignRuleWidth(i))
        CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                        curPin.polygonDesignRuleWidth(i));
        points = curPin.getPolygon(i);
        for(j = 0; j < points.numPoints; j++)
        CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
      }
      for(i = 0; i < curPin.numVias(); i++) {
        if(curPin.viaTopMask(i) || curPin.viaCutMask(i) ||
            curPin.viaBottomMask(i)) {
          CIRCUIT_FPRINTF(fout, "\n  + VIA %s MASK %d%d%d %d %d ",
                          curPin.viaName(i), curPin.viaTopMask(i),
                          curPin.viaCutMask(i), curPin.viaBottomMask(i),
                          curPin.viaPtX(i), curPin.viaPtY(i));
        }
        else {
          CIRCUIT_FPRINTF(fout, "\n  + VIA %s %d %d ", curPin.viaName(i),
                          curPin.viaPtX(i), curPin.viaPtY(i));
        }
      }
    }
    if(curPin.hasPort()) {
      struct defiPoints points;
      defiPinPort* port;
      for(j = 0; j < curPin.numPorts(); j++) {
        port = curPin.pinPort(j);
        CIRCUIT_FPRINTF(fout, "\n  + PORT");
        for(i = 0; i < port->numLayer(); i++) {
          CIRCUIT_FPRINTF(fout, "\n     + LAYER %s ", port->layer(i));
          if(port->layerMask(i))
          CIRCUIT_FPRINTF(fout, "MASK %d ", port->layerMask(i));
          if(port->hasLayerSpacing(i))
          CIRCUIT_FPRINTF(fout, "SPACING %d ", port->layerSpacing(i));
          if(port->hasLayerDesignRuleWidth(i))
          CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                          port->layerDesignRuleWidth(i));
          port->bounds(i, &xl, &yl, &xh, &yh);
          CIRCUIT_FPRINTF(fout, "( %d %d ) ( %d %d ) ", xl, yl, xh, yh);
        }
        for(i = 0; i < port->numPolygons(); i++) {
          CIRCUIT_FPRINTF(fout, "\n     + POLYGON %s ", port->polygonName(i));
          if(port->polygonMask(i))
          CIRCUIT_FPRINTF(fout, "MASK %d ", port->polygonMask(i));
          if(port->hasPolygonSpacing(i))
          CIRCUIT_FPRINTF(fout, "SPACING %d ", port->polygonSpacing(i));
          if(port->hasPolygonDesignRuleWidth(i))
          CIRCUIT_FPRINTF(fout, "DESIGNRULEWIDTH %d ",
                          port->polygonDesignRuleWidth(i));
          points = port->getPolygon(i);
          for(k = 0; k < points.numPoints; k++)
          CIRCUIT_FPRINTF(fout, "( %d %d ) ", points.x[k], points.y[k]);
        }
        for(i = 0; i < port->numVias(); i++) {
          if(port->viaTopMask(i) || port->viaCutMask(i) ||
              port->viaBottomMask(i)) {
            CIRCUIT_FPRINTF(fout, "\n     + VIA %s MASK %d%d%d ( %d %d ) ",
                            port->viaName(i), port->viaTopMask(i),
                            port->viaCutMask(i), port->viaBottomMask(i),
                            port->viaPtX(i), port->viaPtY(i));
          }
          else {
            CIRCUIT_FPRINTF(fout, "\n     + VIA %s ( %d %d ) ",
                            port->viaName(i), port->viaPtX(i), port->viaPtY(i));
          }
        }
        if(port->hasPlacement()) {
          if(port->isPlaced()) {
            CIRCUIT_FPRINTF(fout, "\n     + PLACED ");
            CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", port->placementX(),
                            port->placementY(), orientStr(port->orient()));
          }
          if(port->isCover()) {
            CIRCUIT_FPRINTF(fout, "\n     + COVER ");
            CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", port->placementX(),
                            port->placementY(), orientStr(port->orient()));
          }
          if(port->isFixed()) {
            CIRCUIT_FPRINTF(fout, "\n     + FIXED ");
            CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", port->placementX(),
                            port->placementY(), orientStr(port->orient()));
          }
        }
      }
    }
    if(curPin.hasPlacement()) {
      if(curPin.isPlaced()) {
        CIRCUIT_FPRINTF(fout, "+ PLACED ");
        CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", curPin.placementX(),
                        curPin.placementY(), orientStr(curPin.orient()));
      }
      if(curPin.isCover()) {
        CIRCUIT_FPRINTF(fout, "+ COVER ");
        CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", curPin.placementX(),
                        curPin.placementY(), orientStr(curPin.orient()));
      }
      if(curPin.isFixed()) {
        CIRCUIT_FPRINTF(fout, "+ FIXED ");
        CIRCUIT_FPRINTF(fout, "( %d %d ) %s ", curPin.placementX(),
                        curPin.placementY(), orientStr(curPin.orient()));
      }
      if(curPin.isUnplaced())
      CIRCUIT_FPRINTF(fout, "+ UNPLACED ");
    }
    if(curPin.hasSpecial()) {
      CIRCUIT_FPRINTF(fout, "+ SPECIAL ");
    }
    if(curPin.hasAPinPartialMetalArea()) {
      for(i = 0; i < curPin.numAPinPartialMetalArea(); i++) {
        CIRCUIT_FPRINTF(fout, "ANTENNAPINPARTIALMETALAREA %d",
                        curPin.APinPartialMetalArea(i));
        if(*(curPin.APinPartialMetalAreaLayer(i)))
        CIRCUIT_FPRINTF(fout, " LAYER %s",
                        curPin.APinPartialMetalAreaLayer(i));
        CIRCUIT_FPRINTF(fout, "\n");
      }
    }
    if(curPin.hasAPinPartialMetalSideArea()) {
      for(i = 0; i < curPin.numAPinPartialMetalSideArea(); i++) {
        CIRCUIT_FPRINTF(fout, "ANTENNAPINPARTIALMETALSIDEAREA %d",
                        curPin.APinPartialMetalSideArea(i));
        if(*(curPin.APinPartialMetalSideAreaLayer(i)))
        CIRCUIT_FPRINTF(fout, " LAYER %s",
                        curPin.APinPartialMetalSideAreaLayer(i));
        CIRCUIT_FPRINTF(fout, "\n");
      }
    }
    if(curPin.hasAPinDiffArea()) {
      for(i = 0; i < curPin.numAPinDiffArea(); i++) {
        CIRCUIT_FPRINTF(fout, "ANTENNAPINDIFFAREA %d", curPin.APinDiffArea(i));
        if(*(curPin.APinDiffAreaLayer(i)))
        CIRCUIT_FPRINTF(fout, " LAYER %s", curPin.APinDiffAreaLayer(i));
        CIRCUIT_FPRINTF(fout, "\n");
      }
    }
    if(curPin.hasAPinPartialCutArea()) {
      for(i = 0; i < curPin.numAPinPartialCutArea(); i++) {
        CIRCUIT_FPRINTF(fout, "ANTENNAPINPARTIALCUTAREA %d",
                        curPin.APinPartialCutArea(i));
        if(*(curPin.APinPartialCutAreaLayer(i)))
        CIRCUIT_FPRINTF(fout, " LAYER %s", curPin.APinPartialCutAreaLayer(i));
        CIRCUIT_FPRINTF(fout, "\n");
      }
    }

    for(j = 0; j < curPin.numAntennaModel(); j++) {
      defiPinAntennaModel* aModel;
      aModel = curPin.antennaModel(j);

      CIRCUIT_FPRINTF(fout, "ANTENNAMODEL %s\n", aModel->antennaOxide());

      if(aModel->hasAPinGateArea()) {
        for(i = 0; i < aModel->numAPinGateArea(); i++) {
          CIRCUIT_FPRINTF(fout, "ANTENNAPINGATEAREA %d",
                          aModel->APinGateArea(i));
          if(aModel->hasAPinGateAreaLayer(i))
          CIRCUIT_FPRINTF(fout, " LAYER %s", aModel->APinGateAreaLayer(i));
          CIRCUIT_FPRINTF(fout, "\n");
        }
      }
      if(aModel->hasAPinMaxAreaCar()) {
        for(i = 0; i < aModel->numAPinMaxAreaCar(); i++) {
          CIRCUIT_FPRINTF(fout, "ANTENNAPINMAXAREACAR %d",
                          aModel->APinMaxAreaCar(i));
          if(aModel->hasAPinMaxAreaCarLayer(i))
          CIRCUIT_FPRINTF(fout, " LAYER %s", aModel->APinMaxAreaCarLayer(i));
          CIRCUIT_FPRINTF(fout, "\n");
        }
      }
      if(aModel->hasAPinMaxSideAreaCar()) {
        for(i = 0; i < aModel->numAPinMaxSideAreaCar(); i++) {
          CIRCUIT_FPRINTF(fout, "ANTENNAPINMAXSIDEAREACAR %d",
                          aModel->APinMaxSideAreaCar(i));
          if(aModel->hasAPinMaxSideAreaCarLayer(i))
          CIRCUIT_FPRINTF(fout, " LAYER %s",
                          aModel->APinMaxSideAreaCarLayer(i));
          CIRCUIT_FPRINTF(fout, "\n");
        }
      }
      if(aModel->hasAPinMaxCutCar()) {
        for(i = 0; i < aModel->numAPinMaxCutCar(); i++) {
          CIRCUIT_FPRINTF(fout, "ANTENNAPINMAXCUTCAR %d",
                          aModel->APinMaxCutCar(i));
          if(aModel->hasAPinMaxCutCarLayer(i))
          CIRCUIT_FPRINTF(fout, " LAYER %s", aModel->APinMaxCutCarLayer(i));
          CIRCUIT_FPRINTF(fout, "\n");
        }
      }
    }
    CIRCUIT_FPRINTF(fout, ";\n");
  }
  CIRCUIT_FPRINTF(fout, "END PINS\n\n");
}

void ePlace::Parser::DumpDefSpecialNet() {
  if(defSpecialNetStor.size() == 0) {
    return;
  }

  // Check SpecialNetCnt
  int sNetCnt = 0;
  for(auto& curNetType : defSpecialNetType) {
    if(curNetType == DEF_SPECIALNET_ORIGINAL) {
      sNetCnt++;
    }
  }
  CIRCUIT_FPRINTF(fout, "SPECIALNETS %d ;\n", sNetCnt);

  bool objectChange = true;
  for(auto& curNet : defSpecialNetStor) {
    int netType = defSpecialNetType[&curNet - &defSpecialNetStor[0]];

    if(netType == DEF_SPECIALNET_ORIGINAL) {
      // For net and special net.
      int i, j, x, y, z, count = 0, newLayer;
      char* layerName;
      double dist, left, right;
      defiPath* p;
      defiSubnet* s;
      int path;
      defiShield* shield;
      defiWire* wire;
      int numX, numY, stepX, stepY;

      // 5/6/2004 - don't need since I have a callback for the name
      if(objectChange) {
        CIRCUIT_FPRINTF(fout, "- %s ", curNet.name());

        count = 0;
        // compName & pinName
        for(i = 0; i < curNet.numConnections(); i++) {
          // set the limit of only 5 items print out in one line
          count++;
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          CIRCUIT_FPRINTF(fout, "( %s %s ) ", curNet.instance(i),
                          curNet.pin(i));
          if(curNet.pinIsSynthesized(i))
          CIRCUIT_FPRINTF(fout, "+ SYNTHESIZED ");
        }
      }

      // specialWiring
      if(curNet.numWires()) {
        for(i = 0; i < curNet.numWires(); i++) {
          newLayer = (objectChange) ? 0 : 1;
          wire = curNet.wire(i);
          if(objectChange) {
            CIRCUIT_FPRINTF(fout, "\n  + %s ", wire->wireType());
            if(strcmp(wire->wireType(), "SHIELD") == 0)
            CIRCUIT_FPRINTF(fout, "%s ", wire->wireShieldNetName());
          }
          for(j = 0; j < wire->numPaths(); j++) {
            p = wire->path(j);
            p->initTraverse();
            if(testDebugPrint) {
              p->print(fout);
            }
            else {
              while((path = (int)p->next()) != DEFIPATH_DONE) {
                count++;
                // Don't want the line to be too long
                if(count >= 5) {
                  CIRCUIT_FPRINTF(fout, "\n");
                  count = 0;
                }
                switch(path) {
                  case DEFIPATH_LAYER:
                    if(newLayer == 0) {
                      CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                      newLayer = 1;
                    }
                    else
                    CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                    break;
                  case DEFIPATH_VIA:
                  CIRCUIT_FPRINTF(fout, "%s ",
                                  ignoreViaNames ? "XXX" : p->getVia());
                    break;
                  case DEFIPATH_VIAROTATION:
                  CIRCUIT_FPRINTF(fout, "%s ",
                                  orientStr(p->getViaRotation()));
                    break;
                  case DEFIPATH_VIADATA:
                    p->getViaData(&numX, &numY, &stepX, &stepY);
                    CIRCUIT_FPRINTF(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                                    stepX, stepY);
                    break;
                  case DEFIPATH_WIDTH:
                  CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                    break;
                  case DEFIPATH_MASK:
                  CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
                    break;
                  case DEFIPATH_VIAMASK:
                  CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                                  p->getViaCutMask(), p->getViaBottomMask());
                    break;
                  case DEFIPATH_POINT:
                    p->getPoint(&x, &y);
                    CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                    break;
                  case DEFIPATH_FLUSHPOINT:
                    p->getFlushPoint(&x, &y, &z);
                    CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                    break;
                  case DEFIPATH_TAPER:
                  CIRCUIT_FPRINTF(fout, "TAPER ");
                    break;
                  case DEFIPATH_SHAPE:
                  CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
                    break;
                  case DEFIPATH_STYLE:
                  CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
                    break;
                }
              }
            }
          }
          CIRCUIT_FPRINTF(fout, "\n");
          count = 0;
        }
      }

      // POLYGON
      if(curNet.numPolygons()) {
        struct defiPoints points;

        for(i = 0; i < curNet.numPolygons(); i++) {
          if(curVer >= 5.8) {
            if(strcmp(curNet.polyRouteStatus(i), "") != 0) {
              CIRCUIT_FPRINTF(fout, "\n  + %s ", curNet.polyRouteStatus(i));
              if(strcmp(curNet.polyRouteStatus(i), "SHIELD") == 0) {
                CIRCUIT_FPRINTF(fout, "\n  + %s ",
                                curNet.polyRouteStatusShieldName(i));
              }
            }
            if(strcmp(curNet.polyShapeType(i), "") != 0) {
              CIRCUIT_FPRINTF(fout, "\n  + SHAPE %s ", curNet.polyShapeType(i));
            }
          }
          if(curNet.polyMask(i)) {
            CIRCUIT_FPRINTF(fout, "\n  + MASK %d + POLYGON % s ",
                            curNet.polyMask(i), curNet.polygonName(i));
          }
          else {
            CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", curNet.polygonName(i));
          }
          points = curNet.getPolygon(i);
          for(j = 0; j < points.numPoints; j++)
          CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
        }
      }
      // RECT
      if(curNet.numRectangles()) {
        for(i = 0; i < curNet.numRectangles(); i++) {
          if(curVer >= 5.8) {
            if(strcmp(curNet.rectRouteStatus(i), "") != 0) {
              CIRCUIT_FPRINTF(fout, "\n  + %s ", curNet.rectRouteStatus(i));
              if(strcmp(curNet.rectRouteStatus(i), "SHIELD") == 0) {
                CIRCUIT_FPRINTF(fout, "\n  + %s ",
                                curNet.rectRouteStatusShieldName(i));
              }
            }
            if(strcmp(curNet.rectShapeType(i), "") != 0) {
              CIRCUIT_FPRINTF(fout, "\n  + SHAPE %s ", curNet.rectShapeType(i));
            }
          }
          if(curNet.rectMask(i)) {
            CIRCUIT_FPRINTF(fout, "\n  + MASK %d + RECT %s %d %d %d %d",
                            curNet.rectMask(i), curNet.rectName(i),
                            curNet.xl(i), curNet.yl(i), curNet.xh(i),
                            curNet.yh(i));
          }
          else {
            CIRCUIT_FPRINTF(fout, "\n  + RECT %s %d %d %d %d",
                            curNet.rectName(i), curNet.xl(i), curNet.yl(i),
                            curNet.xh(i), curNet.yh(i));
          }
        }
      }
      // VIA
      if(curVer >= 5.8 && curNet.numViaSpecs()) {
        for(i = 0; i < curNet.numViaSpecs(); i++) {
          if(strcmp(curNet.viaRouteStatus(i), "") != 0) {
            CIRCUIT_FPRINTF(fout, "\n  + %s ", curNet.viaRouteStatus(i));
            if(strcmp(curNet.viaRouteStatus(i), "SHIELD") == 0) {
              CIRCUIT_FPRINTF(fout, "\n  + %s ",
                              curNet.viaRouteStatusShieldName(i));
            }
          }
          if(strcmp(curNet.viaShapeType(i), "") != 0) {
            CIRCUIT_FPRINTF(fout, "\n  + SHAPE %s ", curNet.viaShapeType(i));
          }
          if(curNet.topMaskNum(i) || curNet.cutMaskNum(i) ||
              curNet.bottomMaskNum(i)) {
            CIRCUIT_FPRINTF(fout, "\n  + MASK %d%d%d + VIA %s ",
                            curNet.topMaskNum(i), curNet.cutMaskNum(i),
                            curNet.bottomMaskNum(i), curNet.viaName(i));
          }
          else {
            CIRCUIT_FPRINTF(fout, "\n  + VIA %s ", curNet.viaName(i));
          }
          CIRCUIT_FPRINTF(fout, " %s", curNet.viaOrientStr(i));

          defiPoints points = curNet.getViaPts(i);

          for(int j = 0; j < points.numPoints; j++) {
            CIRCUIT_FPRINTF(fout, " %d %d", points.x[j], points.y[j]);
          }
          CIRCUIT_FPRINTF(fout, ";\n");
        }
      }

      if(curNet.hasSubnets()) {
        for(i = 0; i < curNet.numSubnets(); i++) {
          s = curNet.subnet(i);
          if(s->numConnections()) {
            if(s->pinIsMustJoin(0)) {
              CIRCUIT_FPRINTF(fout, "- MUSTJOIN ");
            }
            else {
              CIRCUIT_FPRINTF(fout, "- %s ", s->name());
            }
            for(j = 0; j < s->numConnections(); j++) {
              CIRCUIT_FPRINTF(fout, " ( %s %s )\n", s->instance(j), s->pin(j));
            }
          }

          // regularWiring
          if(s->numWires()) {
            for(i = 0; i < s->numWires(); i++) {
              wire = s->wire(i);
              CIRCUIT_FPRINTF(fout, "  + %s ", wire->wireType());
              for(j = 0; j < wire->numPaths(); j++) {
                p = wire->path(j);
                p->print(fout);
              }
            }
          }
        }
      }

      if(curNet.numProps()) {
        for(i = 0; i < curNet.numProps(); i++) {
          if(curNet.propIsString(i))
          CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %s ", curNet.propName(i),
                          curNet.propValue(i));
          if(curNet.propIsNumber(i))
          CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %g ", curNet.propName(i),
                          curNet.propNumber(i));
          switch(curNet.propType(i)) {
            case 'R':
            CIRCUIT_FPRINTF(fout, "REAL ");
              break;
            case 'I':
            CIRCUIT_FPRINTF(fout, "INTEGER ");
              break;
            case 'S':
            CIRCUIT_FPRINTF(fout, "STRING ");
              break;
            case 'Q':
            CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
              break;
            case 'N':
            CIRCUIT_FPRINTF(fout, "NUMBER ");
              break;
          }
          CIRCUIT_FPRINTF(fout, "\n");
        }
      }

      // SHIELD
      count = 0;
      // testing the SHIELD for 5.3, obsolete in 5.4
      if(curNet.numShields()) {
        for(i = 0; i < curNet.numShields(); i++) {
          shield = curNet.shield(i);
          CIRCUIT_FPRINTF(fout, "\n  + SHIELD %s ", shield->shieldName());
          newLayer = 0;
          for(j = 0; j < shield->numPaths(); j++) {
            p = shield->path(j);
            p->initTraverse();
            while((path = (int)p->next()) != DEFIPATH_DONE) {
              count++;
              // Don't want the line to be too long
              if(count >= 5) {
                CIRCUIT_FPRINTF(fout, "\n");
                count = 0;
              }
              switch(path) {
                case DEFIPATH_LAYER:
                  if(newLayer == 0) {
                    CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                    newLayer = 1;
                  }
                  else
                  CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                  break;
                case DEFIPATH_VIA:
                CIRCUIT_FPRINTF(fout, "%s ",
                                ignoreViaNames ? "XXX" : p->getVia());
                  break;
                case DEFIPATH_VIAROTATION:
                CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
                  break;
                case DEFIPATH_WIDTH:
                CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                  break;
                case DEFIPATH_MASK:
                CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
                  break;
                case DEFIPATH_VIAMASK:
                CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                                p->getViaCutMask(), p->getViaBottomMask());
                  break;
                case DEFIPATH_POINT:
                  p->getPoint(&x, &y);
                  CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                  break;
                case DEFIPATH_FLUSHPOINT:
                  p->getFlushPoint(&x, &y, &z);
                  CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                  break;
                case DEFIPATH_TAPER:
                CIRCUIT_FPRINTF(fout, "TAPER ");
                  break;
                case DEFIPATH_SHAPE:
                CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
                  break;
                case DEFIPATH_STYLE:
                CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
                  break;
              }
            }
          }
        }
      }

      // layerName width
      if(curNet.hasWidthRules()) {
        for(i = 0; i < curNet.numWidthRules(); i++) {
          curNet.widthRule(i, &layerName, &dist);
          CIRCUIT_FPRINTF(fout, "\n  + WIDTH %s %g ", layerName, dist);
        }
      }

      // layerName spacing
      if(curNet.hasSpacingRules()) {
        for(i = 0; i < curNet.numSpacingRules(); i++) {
          curNet.spacingRule(i, &layerName, &dist, &left, &right);
          if(left == right) {
            CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g ", layerName, dist);
          }
          else {
            CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g RANGE %g %g ", layerName,
                            dist, left, right);
          }
        }
      }

      if(curNet.hasFixedbump())
      CIRCUIT_FPRINTF(fout, "\n  + FIXEDBUMP ");
      if(curNet.hasFrequency())
      CIRCUIT_FPRINTF(fout, "\n  + FREQUENCY %g ", curNet.frequency());
      if(curNet.hasVoltage())
      CIRCUIT_FPRINTF(fout, "\n  + VOLTAGE %g ", curNet.voltage());
      if(curNet.hasWeight())
      CIRCUIT_FPRINTF(fout, "\n  + WEIGHT %d ", curNet.weight());
      if(curNet.hasCap())
      CIRCUIT_FPRINTF(fout, "\n  + ESTCAP %g ", curNet.cap());
      if(curNet.hasSource())
      CIRCUIT_FPRINTF(fout, "\n  + SOURCE %s ", curNet.source());
      if(curNet.hasPattern())
      CIRCUIT_FPRINTF(fout, "\n  + PATTERN %s ", curNet.pattern());
      if(curNet.hasOriginal())
      CIRCUIT_FPRINTF(fout, "\n  + ORIGINAL %s ", curNet.original());
      if(curNet.hasUse())
      CIRCUIT_FPRINTF(fout, "\n  + USE %s ", curNet.use());

      CIRCUIT_FPRINTF(fout, ";\n");

      objectChange = true;
    }
    else if(netType == DEF_SPECIALNET_PARTIAL_PATH) {
      int i, j, x, y, z, count, newLayer;
      char* layerName;
      double dist, left, right;
      defiPath* p;
      defiSubnet* s;
      int path;
      defiShield* shield;
      defiWire* wire;
      int numX, numY, stepX, stepY;

      //      if (c != defrSNetPartialPathCbkType)
      //        return 1;
      //      if (ud != userData) dataError();

      //      CIRCUIT_FPRINTF (fout, "SPECIALNET partial data\n");

      count = 0;
      if(objectChange) {
        CIRCUIT_FPRINTF(fout, "- %s ", curNet.name());

        // compName & pinName
        for(i = 0; i < curNet.numConnections(); i++) {
          // set the limit of only 5 items print out in one line
          count++;
          if(count >= 5) {
            CIRCUIT_FPRINTF(fout, "\n");
            count = 0;
          }
          CIRCUIT_FPRINTF(fout, "( %s %s ) ", curNet.instance(i),
                          curNet.pin(i));
          if(curNet.pinIsSynthesized(i))
          CIRCUIT_FPRINTF(fout, "+ SYNTHESIZED ");
        }
      }

      // specialWiring
      // POLYGON
      if(curNet.numPolygons()) {
        struct defiPoints points;
        for(i = 0; i < curNet.numPolygons(); i++) {
          CIRCUIT_FPRINTF(fout, "\n  + POLYGON %s ", curNet.polygonName(i));
          points = curNet.getPolygon(i);
          for(j = 0; j < points.numPoints; j++)
          CIRCUIT_FPRINTF(fout, "%d %d ", points.x[j], points.y[j]);
        }
      }
      // RECT
      if(curNet.numRectangles()) {
        for(i = 0; i < curNet.numRectangles(); i++) {
          CIRCUIT_FPRINTF(fout, "\n  + RECT %s %d %d %d %d", curNet.rectName(i),
                          curNet.xl(i), curNet.yl(i), curNet.xh(i),
                          curNet.yh(i));
        }
      }

      // COVER, FIXED, ROUTED or SHIELD
      if(curNet.numWires()) {
        for(i = 0; i < curNet.numWires(); i++) {
          newLayer = (objectChange) ? 0 : 1;
          wire = curNet.wire(i);
          if(objectChange) {
            CIRCUIT_FPRINTF(fout, "\n  + %s ", wire->wireType());
            if(strcmp(wire->wireType(), "SHIELD") == 0)
            CIRCUIT_FPRINTF(fout, "%s ", wire->wireShieldNetName());
          }
          for(j = 0; j < wire->numPaths(); j++) {
            p = wire->path(j);
            p->initTraverse();
            while((path = (int)p->next()) != DEFIPATH_DONE) {
              count++;
              // Don't want the line to be too long
              if(count >= 5) {
                CIRCUIT_FPRINTF(fout, "\n");
                count = 0;
              }
              switch(path) {
                case DEFIPATH_LAYER:
                  if(newLayer == 0) {
                    CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                    newLayer = 1;
                  }
                  else
                  CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                  break;
                case DEFIPATH_VIA:
                CIRCUIT_FPRINTF(fout, "%s ",
                                ignoreViaNames ? "XXX" : p->getVia());
                  break;
                case DEFIPATH_VIAROTATION:
                CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
                  break;
                case DEFIPATH_VIADATA:
                  p->getViaData(&numX, &numY, &stepX, &stepY);
                  CIRCUIT_FPRINTF(fout, "DO %d BY %d STEP %d %d ", numX, numY,
                                  stepX, stepY);
                  break;
                case DEFIPATH_WIDTH:
                CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                  break;
                case DEFIPATH_MASK:
                CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
                  break;
                case DEFIPATH_VIAMASK:
                CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                                p->getViaCutMask(), p->getViaBottomMask());
                  break;
                case DEFIPATH_POINT:
                  p->getPoint(&x, &y);
                  CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                  break;
                case DEFIPATH_FLUSHPOINT:
                  p->getFlushPoint(&x, &y, &z);
                  CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                  break;
                case DEFIPATH_TAPER:
                CIRCUIT_FPRINTF(fout, "TAPER ");
                  break;
                case DEFIPATH_SHAPE:
                CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
                  break;
                case DEFIPATH_STYLE:
                CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
                  break;
              }
            }
          }
          CIRCUIT_FPRINTF(fout, "\n");
          count = 0;
        }
      }

      if(curNet.hasSubnets()) {
        for(i = 0; i < curNet.numSubnets(); i++) {
          s = curNet.subnet(i);
          if(s->numConnections()) {
            if(s->pinIsMustJoin(0)) {
              CIRCUIT_FPRINTF(fout, "- MUSTJOIN ");
            }
            else {
              CIRCUIT_FPRINTF(fout, "- %s ", s->name());
            }
            for(j = 0; j < s->numConnections(); j++) {
              CIRCUIT_FPRINTF(fout, " ( %s %s )\n", s->instance(j), s->pin(j));
            }
          }

          // regularWiring
          if(s->numWires()) {
            for(i = 0; i < s->numWires(); i++) {
              wire = s->wire(i);
              CIRCUIT_FPRINTF(fout, "  + %s ", wire->wireType());
              for(j = 0; j < wire->numPaths(); j++) {
                p = wire->path(j);
                p->print(fout);
              }
            }
          }
        }
      }

      if(curNet.numProps()) {
        for(i = 0; i < curNet.numProps(); i++) {
          if(curNet.propIsString(i))
          CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %s ", curNet.propName(i),
                          curNet.propValue(i));
          if(curNet.propIsNumber(i))
          CIRCUIT_FPRINTF(fout, "  + PROPERTY %s %g ", curNet.propName(i),
                          curNet.propNumber(i));
          switch(curNet.propType(i)) {
            case 'R':
            CIRCUIT_FPRINTF(fout, "REAL ");
              break;
            case 'I':
            CIRCUIT_FPRINTF(fout, "INTEGER ");
              break;
            case 'S':
            CIRCUIT_FPRINTF(fout, "STRING ");
              break;
            case 'Q':
            CIRCUIT_FPRINTF(fout, "QUOTESTRING ");
              break;
            case 'N':
            CIRCUIT_FPRINTF(fout, "NUMBER ");
              break;
          }
          CIRCUIT_FPRINTF(fout, "\n");
        }
      }

      // SHIELD
      count = 0;
      // testing the SHIELD for 5.3, obsolete in 5.4
      if(curNet.numShields()) {
        for(i = 0; i < curNet.numShields(); i++) {
          shield = curNet.shield(i);
          CIRCUIT_FPRINTF(fout, "\n  + SHIELD %s ", shield->shieldName());
          newLayer = 0;
          for(j = 0; j < shield->numPaths(); j++) {
            p = shield->path(j);
            p->initTraverse();
            while((path = (int)p->next()) != DEFIPATH_DONE) {
              count++;
              // Don't want the line to be too long
              if(count >= 5) {
                CIRCUIT_FPRINTF(fout, "\n");
                count = 0;
              }
              switch(path) {
                case DEFIPATH_LAYER:
                  if(newLayer == 0) {
                    CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                    newLayer = 1;
                  }
                  else
                  CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                  break;
                case DEFIPATH_VIA:
                CIRCUIT_FPRINTF(fout, "%s ",
                                ignoreViaNames ? "XXX" : p->getVia());
                  break;
                case DEFIPATH_VIAROTATION:
                  if(newLayer) {
                    CIRCUIT_FPRINTF(fout, "%s ",
                                    orientStr(p->getViaRotation()));
                  }
                  else {
                    CIRCUIT_FPRINTF(fout, "Str %s ", p->getViaRotationStr());
                  }
                  break;
                case DEFIPATH_WIDTH:
                CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                  break;
                case DEFIPATH_MASK:
                CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
                  break;
                case DEFIPATH_VIAMASK:
                CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                                p->getViaCutMask(), p->getViaBottomMask());
                  break;
                case DEFIPATH_POINT:
                  p->getPoint(&x, &y);
                  CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                  break;
                case DEFIPATH_FLUSHPOINT:
                  p->getFlushPoint(&x, &y, &z);
                  CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                  break;
                case DEFIPATH_TAPER:
                CIRCUIT_FPRINTF(fout, "TAPER ");
                  break;
                case DEFIPATH_SHAPE:
                CIRCUIT_FPRINTF(fout, "+ SHAPE %s ", p->getShape());
                  break;
                case DEFIPATH_STYLE:
                CIRCUIT_FPRINTF(fout, "+ STYLE %d ", p->getStyle());
              }
            }
          }
        }
      }

      // layerName width
      if(curNet.hasWidthRules()) {
        for(i = 0; i < curNet.numWidthRules(); i++) {
          curNet.widthRule(i, &layerName, &dist);
          CIRCUIT_FPRINTF(fout, "\n  + WIDTH %s %g ", layerName, dist);
        }
      }

      // layerName spacing
      if(curNet.hasSpacingRules()) {
        for(i = 0; i < curNet.numSpacingRules(); i++) {
          curNet.spacingRule(i, &layerName, &dist, &left, &right);
          if(left == right) {
            CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g ", layerName, dist);
          }
          else {
            CIRCUIT_FPRINTF(fout, "\n  + SPACING %s %g RANGE %g %g ", layerName,
                            dist, left, right);
          }
        }
      }

      if(curNet.hasFixedbump())
      CIRCUIT_FPRINTF(fout, "\n  + FIXEDBUMP ");
      if(curNet.hasFrequency())
      CIRCUIT_FPRINTF(fout, "\n  + FREQUENCY %g ", curNet.frequency());
      if(curNet.hasVoltage())
      CIRCUIT_FPRINTF(fout, "\n  + VOLTAGE %g ", curNet.voltage());
      if(curNet.hasWeight())
      CIRCUIT_FPRINTF(fout, "\n  + WEIGHT %d ", curNet.weight());
      if(curNet.hasCap())
      CIRCUIT_FPRINTF(fout, "\n  + ESTCAP %g ", curNet.cap());
      if(curNet.hasSource())
      CIRCUIT_FPRINTF(fout, "\n  + SOURCE %s ", curNet.source());
      if(curNet.hasPattern())
      CIRCUIT_FPRINTF(fout, "\n  + PATTERN %s ", curNet.pattern());
      if(curNet.hasOriginal())
      CIRCUIT_FPRINTF(fout, "\n  + ORIGINAL %s ", curNet.original());
      if(curNet.hasUse())
      CIRCUIT_FPRINTF(fout, "\n  + USE %s ", curNet.use());

      CIRCUIT_FPRINTF(fout, "\n");

      if(objectChange) {
        objectChange = false;
      }
    }
  }

  CIRCUIT_FPRINTF(fout, "END SPECIALNETS\n\n");
}

void ePlace::Parser::DumpDefNet() {
  if(defNetStor.size() == 0) {
    return;
  }

  // For net and special net.
  int i, j, k, w, x, y, z, count, newLayer;
  defiPath* p;
  defiSubnet* s;
  int path;
  defiVpin* vpin;
  // defiShield *noShield;
  defiWire* wire;

  CIRCUIT_FPRINTF(fout, "NETS %lu ;\n", defNetStor.size());

  for(auto& curNet : defNetStor) {
    CIRCUIT_FPRINTF(fout, "- %s \n", curNet.name());

    if(curNet.pinIsMustJoin(0))
    CIRCUIT_FPRINTF(fout, "- MUSTJOIN \n");

    count = 0;
    // compName & pinName
    for(i = 0; i < curNet.numConnections(); i++) {
      // set the limit of only 5 items per line
      if(count >= 4) {
        CIRCUIT_FPRINTF(fout, "\n");
        count = 0;
      }

      if(count == 0) {
        CIRCUIT_FPRINTF(fout, "  ");
      }

      count++;

      CIRCUIT_FPRINTF(fout, "( %s %s ) ", curNet.instance(i), curNet.pin(i));
      //      curNet.changeInstance("newInstance", i);
      //      curNet.changePin("newPin", i);
      //      CIRCUIT_FPRINTF(fout, "( %s %s ) ", curNet.instance(i),
      //              curNet.pin(i));
      if(curNet.pinIsSynthesized(i))
      CIRCUIT_FPRINTF(fout, "+ SYNTHESIZED ");
    }

    if(curNet.hasNonDefaultRule())
    CIRCUIT_FPRINTF(fout, "+ NONDEFAULTRULE %s\n", curNet.nonDefaultRule());

    for(i = 0; i < curNet.numVpins(); i++) {
      vpin = curNet.vpin(i);
      CIRCUIT_FPRINTF(fout, "  + %s", vpin->name());
      if(vpin->layer())
      CIRCUIT_FPRINTF(fout, " %s", vpin->layer());
      CIRCUIT_FPRINTF(fout, " %d %d %d %d", vpin->xl(), vpin->yl(), vpin->xh(),
                      vpin->yh());
      if(vpin->status() != ' ') {
        CIRCUIT_FPRINTF(fout, " %c", vpin->status());
        CIRCUIT_FPRINTF(fout, " %d %d", vpin->xLoc(), vpin->yLoc());
        if(vpin->orient() != -1)
        CIRCUIT_FPRINTF(fout, " %s", orientStr(vpin->orient()));
      }
      CIRCUIT_FPRINTF(fout, "\n");
    }

    fflush(stdout);
    // regularWiring
    if(curNet.numWires()) {
      for(i = 0; i < curNet.numWires(); i++) {
        newLayer = 0;
        wire = curNet.wire(i);
        CIRCUIT_FPRINTF(fout, "\n  + %s ", wire->wireType());
        count = 0;
        for(j = 0; j < wire->numPaths(); j++) {
          p = wire->path(j);
          p->initTraverse();
          while((path = (int)p->next()) != DEFIPATH_DONE) {
            count++;
            // Don't want the line to be too long
            if(count >= 5) {
              CIRCUIT_FPRINTF(fout, "\n");
              count = 0;
            }
            switch(path) {
              case DEFIPATH_LAYER:
                if(newLayer == 0) {
                  CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                  newLayer = 1;
                }
                else
                CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                break;
              case DEFIPATH_MASK:
              CIRCUIT_FPRINTF(fout, "MASK %d ", p->getMask());
                break;
              case DEFIPATH_VIAMASK:
              CIRCUIT_FPRINTF(fout, "MASK %d%d%d ", p->getViaTopMask(),
                              p->getViaCutMask(), p->getViaBottomMask());
                break;
              case DEFIPATH_VIA:
              CIRCUIT_FPRINTF(fout, "%s ",
                              ignoreViaNames ? "XXX" : p->getVia());
                break;
              case DEFIPATH_VIAROTATION:
              CIRCUIT_FPRINTF(fout, "%s ", orientStr(p->getViaRotation()));
                break;
              case DEFIPATH_RECT:
                p->getViaRect(&w, &x, &y, &z);
                CIRCUIT_FPRINTF(fout, "RECT ( %d %d %d %d ) ", w, x, y, z);
                break;
              case DEFIPATH_VIRTUALPOINT:
                p->getVirtualPoint(&x, &y);
                CIRCUIT_FPRINTF(fout, "VIRTUAL ( %d %d ) ", x, y);
                break;
              case DEFIPATH_WIDTH:
              CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                break;
              case DEFIPATH_POINT:
                p->getPoint(&x, &y);
                CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                break;
              case DEFIPATH_FLUSHPOINT:
                p->getFlushPoint(&x, &y, &z);
                CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                break;
              case DEFIPATH_TAPER:
              CIRCUIT_FPRINTF(fout, "TAPER ");
                break;
              case DEFIPATH_TAPERRULE:
              CIRCUIT_FPRINTF(fout, "TAPERRULE %s ", p->getTaperRule());
                break;
              case DEFIPATH_STYLE:
              CIRCUIT_FPRINTF(fout, "STYLE %d ", p->getStyle());
                break;
            }
          }
        }
        CIRCUIT_FPRINTF(fout, "\n");
        count = 0;
      }
    }

    // SHIELDNET
    if(curNet.numShieldNets()) {
      for(i = 0; i < curNet.numShieldNets(); i++)
      CIRCUIT_FPRINTF(fout, "\n  + SHIELDNET %s", curNet.shieldNet(i));
    }

    if(curNet.hasSubnets()) {
      for(i = 0; i < curNet.numSubnets(); i++) {
        s = curNet.subnet(i);
        CIRCUIT_FPRINTF(fout, "\n");

        if(s->numConnections()) {
          if(s->pinIsMustJoin(0)) {
            CIRCUIT_FPRINTF(fout, "- MUSTJOIN ");
          }
          else {
            CIRCUIT_FPRINTF(fout, "  + SUBNET %s ", s->name());
          }
          for(j = 0; j < s->numConnections(); j++) {
            CIRCUIT_FPRINTF(fout, " ( %s %s )\n", s->instance(j), s->pin(j));
          }

          // regularWiring
          if(s->numWires()) {
            for(k = 0; k < s->numWires(); k++) {
              newLayer = 0;
              wire = s->wire(k);
              CIRCUIT_FPRINTF(fout, "  %s ", wire->wireType());
              count = 0;
              for(j = 0; j < wire->numPaths(); j++) {
                p = wire->path(j);
                p->initTraverse();
                while((path = (int)p->next()) != DEFIPATH_DONE) {
                  count++;
                  // Don't want the line to be too long
                  if(count >= 5) {
                    CIRCUIT_FPRINTF(fout, "\n");
                    count = 0;
                  }
                  switch(path) {
                    case DEFIPATH_LAYER:
                      if(newLayer == 0) {
                        CIRCUIT_FPRINTF(fout, "%s ", p->getLayer());
                        newLayer = 1;
                      }
                      else
                      CIRCUIT_FPRINTF(fout, "NEW %s ", p->getLayer());
                      break;
                    case DEFIPATH_VIA:
                    CIRCUIT_FPRINTF(fout, "%s ",
                                    ignoreViaNames ? "XXX" : p->getVia());
                      break;
                    case DEFIPATH_VIAROTATION:
                    CIRCUIT_FPRINTF(fout, "%s ", p->getViaRotationStr());
                      break;
                    case DEFIPATH_WIDTH:
                    CIRCUIT_FPRINTF(fout, "%d ", p->getWidth());
                      break;
                    case DEFIPATH_POINT:
                      p->getPoint(&x, &y);
                      CIRCUIT_FPRINTF(fout, "( %d %d ) ", x, y);
                      break;
                    case DEFIPATH_FLUSHPOINT:
                      p->getFlushPoint(&x, &y, &z);
                      CIRCUIT_FPRINTF(fout, "( %d %d %d ) ", x, y, z);
                      break;
                    case DEFIPATH_TAPER:
                    CIRCUIT_FPRINTF(fout, "TAPER ");
                      break;
                    case DEFIPATH_TAPERRULE:
                    CIRCUIT_FPRINTF(fout, "TAPERRULE  %s ",
                                    p->getTaperRule());
                      break;
                    case DEFIPATH_STYLE:
                    CIRCUIT_FPRINTF(fout, "STYLE  %d ", p->getStyle());
                      break;
                  }
                }
              }
            }
          }
        }
      }
    }

    if(curNet.numProps()) {
      CIRCUIT_FPRINTF(fout, "\n  + PROPERTY ");
      for(i = 0; i < curNet.numProps(); i++) {
        CIRCUIT_FPRINTF(fout, "%s ", curNet.propName(i));
        switch(curNet.propType(i)) {
          case 'R':
          CIRCUIT_FPRINTF(fout, "%.6f ", curNet.propNumber(i));
            break;
          case 'I':
          CIRCUIT_FPRINTF(fout, "%g INTEGER ", curNet.propNumber(i));
            break;
          case 'S':
          CIRCUIT_FPRINTF(fout, "%s STRING ", curNet.propValue(i));
            break;
          case 'Q':
          CIRCUIT_FPRINTF(fout, "%s QUOTESTRING ", curNet.propValue(i));
            break;
          case 'N':
          CIRCUIT_FPRINTF(fout, "%g NUMBER ", curNet.propNumber(i));
            break;
        }
      }
      CIRCUIT_FPRINTF(fout, "\n");
    }

    if(curNet.hasWeight())
    CIRCUIT_FPRINTF(fout, "+ WEIGHT %d ", curNet.weight());
    if(curNet.hasCap())
    CIRCUIT_FPRINTF(fout, "+ ESTCAP %.6f ", curNet.cap());
    if(curNet.hasSource())
    CIRCUIT_FPRINTF(fout, "+ SOURCE %s ", curNet.source());
    if(curNet.hasFixedbump())
    CIRCUIT_FPRINTF(fout, "+ FIXEDBUMP ");
    if(curNet.hasFrequency())
    CIRCUIT_FPRINTF(fout, "+ FREQUENCY %.6f ", curNet.frequency());
    if(curNet.hasPattern())
    CIRCUIT_FPRINTF(fout, "+ PATTERN %s ", curNet.pattern());
    if(curNet.hasOriginal())
    CIRCUIT_FPRINTF(fout, "+ ORIGINAL %s ", curNet.original());
    if(curNet.hasUse())
    CIRCUIT_FPRINTF(fout, "+ USE %s ", curNet.use());

    CIRCUIT_FPRINTF(fout, ";\n");
  }
  CIRCUIT_FPRINTF(fout, "END NETS\n\n");
}

void ePlace::Parser::DumpDefDone() {
  CIRCUIT_FPRINTF(fout, "END DESIGN\n");
}

// below is for additional Check
void ePlace::Parser::DumpDefComponentPinToNet() {
  for(auto& curComponent : defComponentPinToNet) {
    int idx = &curComponent - &defComponentPinToNet[0];
    cout << "Comp: " << defComponentStor[idx].id() << endl;
    for(auto& curPin : curComponent) {
      cout << defNetStor[curPin.second].name() << " " << curPin.first << ", ";
    }
    cout << endl;
  }
}


////////////////////////////////////////////////////////////////////////
///////////////////////// Parsering Function ///////////////////////////
////////////////////////////////////////////////////////////////////////

int ePlace::Parser::ParseLef(vector<string> &lefStor) {
  //    char* outFile;
  bool isVerbose;
  FILE* f;
  int res;
  //    int noCalls = 0;

  //  long start_mem;
  //    int num;
  int status;
  int retStr = 0;
  //    int numInFile = 0;
  //    int fileCt = 0;
  int relax = 0;
  //    const char* version = "N/A";
  //    int setVer = 0;
  //    int msgCb = 0;

  // start_mem = (long)sbrk(0);

  char* userData = strdup("(lefrw-5100)");

  fout = stdout;

  lefVersionPtr = &(this->lefVersion);
  lefDividerPtr = &(this->lefDivider);
  lefBusBitCharPtr = &(this->lefBusBitChar);

  lefUnitPtr = &(this->lefUnit);
  lefManufacturingGridPtr = &(this->lefManufacturingGrid);

  lefLayerStorPtr = &(this->lefLayerStor);
  lefSiteStorPtr = &(this->lefSiteStor);
  lefMacroStorPtr = &(this->lefMacroStor);
  lefViaStorPtr = &(this->lefViaStor);

  // vector of vector
  lefPinStorPtr = &(this->lefPinStor);
  lefObsStorPtr = &(this->lefObsStor);

  lefPinMapStorPtr = &(this->lefPinMapStor);

  //    lefMacroToPinPtr = &(this->lefMacroToPin);

  lefMacroMapPtr = &(this->lefMacroMap);
  lefViaMapPtr = &(this->lefViaMap);
  lefLayerMapPtr = &(this->lefLayerMap);
  lefSiteMapPtr = &(this->lefSiteMap);

  // sets the parser to be case sensitive...
  // default was supposed to be the case but false...
  // lefrSetCaseSensitivity(true);

  lefrInitSession(1);

  lefrSetWarningLogFunction(printWarning);
  lefrSetAntennaInputCbk(antennaCB);
  lefrSetAntennaInoutCbk(antennaCB);
  lefrSetAntennaOutputCbk(antennaCB);
  lefrSetArrayBeginCbk(arrayBeginCB);
  lefrSetArrayCbk(arrayCB);
  lefrSetArrayEndCbk(arrayEndCB);
  lefrSetBusBitCharsCbk(busBitCharsCB);
  lefrSetCaseSensitiveCbk(caseSensCB);
  lefrSetFixedMaskCbk(fixedMaskCB);
  lefrSetClearanceMeasureCbk(clearanceCB);
  lefrSetDensityCbk(densityCB);
  lefrSetDividerCharCbk(dividerCB);
  lefrSetNoWireExtensionCbk(noWireExtCB);
  lefrSetNoiseMarginCbk(noiseMarCB);
  lefrSetEdgeRateThreshold1Cbk(edge1CB);
  lefrSetEdgeRateThreshold2Cbk(edge2CB);
  lefrSetEdgeRateScaleFactorCbk(edgeScaleCB);
  lefrSetExtensionCbk(extensionCB);
  lefrSetNoiseTableCbk(noiseTableCB);
  lefrSetCorrectionTableCbk(correctionCB);
  lefrSetDielectricCbk(dielectricCB);
  lefrSetIRDropBeginCbk(irdropBeginCB);
  lefrSetIRDropCbk(irdropCB);
  lefrSetIRDropEndCbk(irdropEndCB);
  lefrSetLayerCbk(layerCB);
  lefrSetLibraryEndCbk(doneCB);
  lefrSetMacroBeginCbk(macroBeginCB);
  lefrSetMacroCbk(macroCB);
  lefrSetMacroClassTypeCbk(macroClassTypeCB);
  lefrSetMacroOriginCbk(macroOriginCB);
  lefrSetMacroSizeCbk(macroSizeCB);
  lefrSetMacroFixedMaskCbk(macroFixedMaskCB);
  lefrSetMacroEndCbk(macroEndCB);
  lefrSetManufacturingCbk(manufacturingCB);
  lefrSetMaxStackViaCbk(maxStackViaCB);
  lefrSetMinFeatureCbk(minFeatureCB);
  lefrSetNonDefaultCbk(nonDefaultCB);
  lefrSetObstructionCbk(obstructionCB);
  lefrSetPinCbk(pinCB);
  lefrSetPropBeginCbk(propDefBeginCB);
  lefrSetPropCbk(propDefCB);
  lefrSetPropEndCbk(propDefEndCB);
  lefrSetSiteCbk(siteCB);
  lefrSetSpacingBeginCbk(spacingBeginCB);
  lefrSetSpacingCbk(spacingCB);
  lefrSetSpacingEndCbk(spacingEndCB);
  lefrSetTimingCbk(timingCB);
  lefrSetUnitsCbk(unitsCB);
  lefrSetUseMinSpacingCbk(useMinSpacingCB);
  lefrSetUserData((void*)3);
  if(!retStr)
    lefrSetVersionCbk(versionCB);
  else
    lefrSetVersionStrCbk(versionStrCB);
  lefrSetViaCbk(viaCB);
  lefrSetViaRuleCbk(viaRuleCB);
  lefrSetInputAntennaCbk(antennaCB);
  lefrSetOutputAntennaCbk(antennaCB);
  lefrSetInoutAntennaCbk(antennaCB);

  //    if (msgCb) {
  //        lefrSetLogFunction(errorCB);
  //        lefrSetWarningLogFunction(warningCB);
  //    }

  lefrSetMallocFunction(mallocCB);
  lefrSetReallocFunction(reallocCB);
  lefrSetFreeFunction(freeCB);

  if(isVerbose) {
    lefrSetLineNumberFunction(lineNumberCB);
    lefrSetDeltaNumberLines(500);
  }

  lefrSetRegisterUnusedCallbacks();

  if(relax)
    lefrSetRelaxMode();

  //    if (setVer)
  //        (void)lefrSetVersionValue(version);

  lefrSetAntennaInoutWarnings(30);
  lefrSetAntennaInputWarnings(30);
  lefrSetAntennaOutputWarnings(30);
  lefrSetArrayWarnings(30);
  lefrSetCaseSensitiveWarnings(30);
  lefrSetCorrectionTableWarnings(30);
  lefrSetDielectricWarnings(30);
  lefrSetEdgeRateThreshold1Warnings(30);
  lefrSetEdgeRateThreshold2Warnings(30);
  lefrSetEdgeRateScaleFactorWarnings(30);
  lefrSetInoutAntennaWarnings(30);
  lefrSetInputAntennaWarnings(30);
  lefrSetIRDropWarnings(30);
  lefrSetLayerWarnings(30);
  lefrSetMacroWarnings(30);
  lefrSetMaxStackViaWarnings(30);
  lefrSetMinFeatureWarnings(30);
  lefrSetNoiseMarginWarnings(30);
  lefrSetNoiseTableWarnings(30);
  lefrSetNonDefaultWarnings(30);
  lefrSetNoWireExtensionWarnings(30);
  lefrSetOutputAntennaWarnings(30);
  lefrSetPinWarnings(30);
  lefrSetSiteWarnings(30);
  lefrSetSpacingWarnings(30);
  lefrSetTimingWarnings(30);
  lefrSetUnitsWarnings(30);
  lefrSetUseMinSpacingWarnings(30);
  lefrSetViaRuleWarnings(30);
  lefrSetViaWarnings(30);

  (void)lefrSetShiftCase();  // will shift name to uppercase if caseinsensitive

  // is set to off or not set
  lefrSetOpenLogFileAppend();

  for(auto& lefName : lefStor) {
    lefrReset();

    if((f = fopen(lefName.c_str(), "r")) == 0) {
      fprintf(stderr, "\n**ERROR: Couldn't open input file '%s'\n",
              lefName.c_str());
      exit(1);
    }

    (void)lefrEnableReadEncrypted();

    status = lefwInit(fout);  // initialize the lef writer,
    // need to be called 1st
    if(status != LEFW_OK)
      return 1;

    fout = NULL;
    res = lefrRead(f, lefName.c_str(), (void*)userData);

    if(res) {
      CIRCUIT_FPRINTF(stderr, "Reader returns bad status.\n", lefName.c_str());
      return res;
    }

    (void)lefrPrintUnusedCallbacks(fout);
    (void)lefrReleaseNResetMemory();
  }

  (void)lefrUnsetCallbacks();

  // Unset all the callbacks
  void lefrUnsetAntennaInputCbk();
  void lefrUnsetAntennaInoutCbk();
  void lefrUnsetAntennaOutputCbk();
  void lefrUnsetArrayBeginCbk();
  void lefrUnsetArrayCbk();
  void lefrUnsetArrayEndCbk();
  void lefrUnsetBusBitCharsCbk();
  void lefrUnsetCaseSensitiveCbk();
  void lefrUnsetFixedMaskCbk();
  void lefrUnsetClearanceMeasureCbk();
  void lefrUnsetCorrectionTableCbk();
  void lefrUnsetDensityCbk();
  void lefrUnsetDielectricCbk();
  void lefrUnsetDividerCharCbk();
  void lefrUnsetEdgeRateScaleFactorCbk();
  void lefrUnsetEdgeRateThreshold1Cbk();
  void lefrUnsetEdgeRateThreshold2Cbk();
  void lefrUnsetExtensionCbk();
  void lefrUnsetInoutAntennaCbk();
  void lefrUnsetInputAntennaCbk();
  void lefrUnsetIRDropBeginCbk();
  void lefrUnsetIRDropCbk();
  void lefrUnsetIRDropEndCbk();
  void lefrUnsetLayerCbk();
  void lefrUnsetLibraryEndCbk();
  void lefrUnsetMacroBeginCbk();
  void lefrUnsetMacroCbk();
  void lefrUnsetMacroClassTypeCbk();
  void lefrUnsetMacroEndCbk();
  void lefrUnsetMacroOriginCbk();
  void lefrUnsetMacroSizeCbk();
  void lefrUnsetManufacturingCbk();
  void lefrUnsetMaxStackViaCbk();
  void lefrUnsetMinFeatureCbk();
  void lefrUnsetNoiseMarginCbk();
  void lefrUnsetNoiseTableCbk();
  void lefrUnsetNonDefaultCbk();
  void lefrUnsetNoWireExtensionCbk();
  void lefrUnsetObstructionCbk();
  void lefrUnsetOutputAntennaCbk();
  void lefrUnsetPinCbk();
  void lefrUnsetPropBeginCbk();
  void lefrUnsetPropCbk();
  void lefrUnsetPropEndCbk();
  void lefrUnsetSiteCbk();
  void lefrUnsetSpacingBeginCbk();
  void lefrUnsetSpacingCbk();
  void lefrUnsetSpacingEndCbk();
  void lefrUnsetTimingCbk();
  void lefrUnsetUseMinSpacingCbk();
  void lefrUnsetUnitsCbk();
  void lefrUnsetVersionCbk();
  void lefrUnsetVersionStrCbk();
  void lefrUnsetViaCbk();
  void lefrUnsetViaRuleCbk();

  //    fclose(fout);

  // Release allocated singleton data.
  //    lefrClear();

  return res;
}
int ePlace::Parser::ParseDef(string filename) {
  bool isVerbose;
  //    int num = 99;
  char* inFile[6];
  FILE* f;
  int res;
  //    int noCalls = 0;
  //  long start_mem;
  //    int numInFile = 0;
  int fileCt = 0;
  int noNetCb = 0;

  //  start_mem = (long)sbrk(0);
  string local_filename = filename;

  inFile[0] = strdup(local_filename.c_str());
  fout = stdout;
  userData = (void*)0x01020304;

  defVersionPtr = &(this->defVersion);
  defDividerCharPtr = &(this->defDividerChar);
  defBusBitCharPtr = &(this->defBusBitChar);
  defDesignNamePtr = &(this->defDesignName);

  defPropStorPtr = &(this->defPropStor);
  defUnitPtr = &(this->defUnit);

  defDieAreaPtr = &(this->defDieArea);
  defRowStorPtr = &(this->defRowStor);
  defTrackStorPtr = &(this->defTrackStor);
  defGcellGridStorPtr = &(this->defGcellGridStor);
  defViaStorPtr = &(this->defViaStor);

  defComponentMaskShiftLayerPtr = &(this->defComponentMaskShiftLayer);
  defComponentStorPtr = &(this->defComponentStor);
  defNetStorPtr = &(this->defNetStor);
  defSpecialNetStorPtr = &(this->defSpecialNetStor);
  defPinStorPtr = &(this->defPinStor);
  defBlockageStorPtr = &(this->defBlockageStor);

  defComponentMapPtr = &(this->defComponentMap);
  defPinMapPtr = &(this->defPinMap);
  defRowY2OrientMapPtr = &(this->defRowY2OrientMap);

  defComponentPinToNetPtr = &(this->defComponentPinToNet);

  defrInitSession(0);

  defrSetWarningLogFunction(printWarning);

  defrSetUserData((void*)3);
  defrSetDesignCbk(dname);
  defrSetTechnologyCbk(tname);
  defrSetExtensionCbk(extension);
  defrSetDesignEndCbk(done);
  defrSetPropDefStartCbk(propstart);
  defrSetPropCbk(prop);
  defrSetPropDefEndCbk(propend);

  /* Test for CCR 766289*/
  if(!noNetCb)
    defrSetNetCbk(netf);

  defrSetNetNameCbk(netNamef);
  defrSetNetNonDefaultRuleCbk(nondefRulef);
  defrSetNetSubnetNameCbk(subnetNamef);
  defrSetNetPartialPathCbk(netpath);
  defrSetSNetCbk(snetf);
  defrSetSNetPartialPathCbk(snetpath);
  if(setSNetWireCbk)
    defrSetSNetWireCbk(snetwire);
  defrSetComponentMaskShiftLayerCbk(compMSL);
  defrSetComponentCbk(compf);
  defrSetAddPathToNet();
  defrSetHistoryCbk(hist);
  defrSetConstraintCbk(constraint);
  defrSetAssertionCbk(constraint);
  defrSetArrayNameCbk(an);
  defrSetFloorPlanNameCbk(fn);
  defrSetDividerCbk(dn);
  defrSetBusBitCbk(bbn);
  defrSetNonDefaultCbk(ndr);

  defrSetAssertionsStartCbk(constraintst);
  defrSetConstraintsStartCbk(constraintst);
  defrSetComponentStartCbk(cs);
  defrSetPinPropStartCbk(cs);
  defrSetNetStartCbk(cs);
  defrSetStartPinsCbk(cs);
  defrSetViaStartCbk(cs);
  defrSetRegionStartCbk(cs);
  defrSetSNetStartCbk(cs);
  defrSetGroupsStartCbk(cs);
  defrSetScanchainsStartCbk(cs);
  defrSetIOTimingsStartCbk(cs);
  defrSetFPCStartCbk(cs);
  defrSetTimingDisablesStartCbk(cs);
  defrSetPartitionsStartCbk(cs);
  defrSetBlockageStartCbk(cs);
  defrSetSlotStartCbk(cs);
  defrSetFillStartCbk(cs);
  defrSetNonDefaultStartCbk(cs);
  defrSetStylesStartCbk(cs);

  // All of the extensions point to the same function.
  defrSetNetExtCbk(ext);
  defrSetComponentExtCbk(ext);
  defrSetPinExtCbk(ext);
  defrSetViaExtCbk(ext);
  defrSetNetConnectionExtCbk(ext);
  defrSetGroupExtCbk(ext);
  defrSetScanChainExtCbk(ext);
  defrSetIoTimingsExtCbk(ext);
  defrSetPartitionsExtCbk(ext);

  defrSetUnitsCbk(units);
  defrSetVersionStrCbk(versStr);
  defrSetCaseSensitiveCbk(casesens);

  // The following calls are an example of using one function "cls"
  // to be the callback for many DIFFERENT types of constructs.
  // We have to cast the function type to meet the requirements
  // of each different set function.
  defrSetSiteCbk((defrSiteCbkFnType)cls);
  defrSetCanplaceCbk((defrSiteCbkFnType)cls);
  defrSetCannotOccupyCbk((defrSiteCbkFnType)cls);
  defrSetDieAreaCbk((defrBoxCbkFnType)cls);
  defrSetPinCapCbk((defrPinCapCbkFnType)cls);
  defrSetPinCbk((defrPinCbkFnType)cls);

  defrSetPinPropCbk((defrPinPropCbkFnType)cls);
  defrSetDefaultCapCbk((defrIntegerCbkFnType)cls);
  defrSetRowCbk((defrRowCbkFnType)cls);
  defrSetTrackCbk((defrTrackCbkFnType)cls);
  defrSetGcellGridCbk((defrGcellGridCbkFnType)cls);
  defrSetViaCbk((defrViaCbkFnType)cls);
  defrSetRegionCbk((defrRegionCbkFnType)cls);
  defrSetGroupNameCbk((defrStringCbkFnType)cls);
  defrSetGroupMemberCbk((defrStringCbkFnType)cls);
  defrSetGroupCbk((defrGroupCbkFnType)cls);
  defrSetScanchainCbk((defrScanchainCbkFnType)cls);
  defrSetIOTimingCbk((defrIOTimingCbkFnType)cls);
  defrSetFPCCbk((defrFPCCbkFnType)cls);
  defrSetTimingDisableCbk((defrTimingDisableCbkFnType)cls);
  defrSetPartitionCbk((defrPartitionCbkFnType)cls);
  defrSetBlockageCbk((defrBlockageCbkFnType)cls);
  defrSetSlotCbk((defrSlotCbkFnType)cls);
  defrSetFillCbk((defrFillCbkFnType)cls);
  defrSetStylesCbk((defrStylesCbkFnType)cls);

  defrSetAssertionsEndCbk(endfunc);
  defrSetComponentEndCbk(endfunc);
  defrSetConstraintsEndCbk(endfunc);
  defrSetNetEndCbk(endfunc);
  defrSetFPCEndCbk(endfunc);
  defrSetFPCEndCbk(endfunc);
  defrSetGroupsEndCbk(endfunc);
  defrSetIOTimingsEndCbk(endfunc);
  defrSetNetEndCbk(endfunc);
  defrSetPartitionsEndCbk(endfunc);
  defrSetRegionEndCbk(endfunc);
  defrSetSNetEndCbk(endfunc);
  defrSetScanchainsEndCbk(endfunc);
  defrSetPinEndCbk(endfunc);
  defrSetTimingDisablesEndCbk(endfunc);
  defrSetViaEndCbk(endfunc);
  defrSetPinPropEndCbk(endfunc);
  defrSetBlockageEndCbk(endfunc);
  defrSetSlotEndCbk(endfunc);
  defrSetFillEndCbk(endfunc);
  defrSetNonDefaultEndCbk(endfunc);
  defrSetStylesEndCbk(endfunc);

  defrSetMallocFunction(mallocCB);
  defrSetReallocFunction(reallocCB);
  defrSetFreeFunction(freeCB);

  // defrSetRegisterUnusedCallbacks();

  // Testing to set the number of warnings
  defrSetAssertionWarnings(3);
  defrSetBlockageWarnings(3);
  defrSetCaseSensitiveWarnings(3);
  defrSetComponentWarnings(3);
  defrSetConstraintWarnings(0);
  defrSetDefaultCapWarnings(3);
  defrSetGcellGridWarnings(3);
  defrSetIOTimingWarnings(3);
  defrSetNetWarnings(3);
  defrSetNonDefaultWarnings(3);
  defrSetPinExtWarnings(3);
  defrSetPinWarnings(3);
  defrSetRegionWarnings(3);
  defrSetRowWarnings(3);
  defrSetScanchainWarnings(3);
  defrSetSNetWarnings(3);
  defrSetStylesWarnings(3);
  defrSetTrackWarnings(3);
  defrSetUnitsWarnings(3);
  defrSetVersionWarnings(3);
  defrSetViaWarnings(3);

  if(isVerbose) {
    defrSetLongLineNumberFunction(lineNumberCB);
    defrSetDeltaNumberLines(10000);
  }

  (void)defrSetOpenLogFileAppend();

  if((f = fopen(inFile[fileCt], "r")) == 0) {
    fprintf(stderr, "**\nERROR: Couldn't open input file '%s'\n",
            inFile[fileCt]);
    exit(1);
  }
  // Set case sensitive to 0 to start with, in History & PropertyDefinition
  // reset it to 1.

  fout = NULL;
  res = defrRead(f, inFile[fileCt], userData, 1);

  if(res) {
    CIRCUIT_FPRINTF(stderr, "Reader returns bad status.\n", inFile[fileCt]);
    return res;
  }

  // Testing the aliases API.
  defrAddAlias("alias1", "aliasValue1", 1);

  defiAlias_itr aliasStore;
  const char* alias1Value = NULL;

  while(aliasStore.Next()) {
    if(strcmp(aliasStore.Key(), "alias1") == 0) {
      alias1Value = aliasStore.Data();
    }
  }

  if(!alias1Value || strcmp(alias1Value, "aliasValue1")) {
    CIRCUIT_FPRINTF(stderr, "**\nERROR: Aliases don't work\n");
  }

  (void)defrPrintUnusedCallbacks(fout);
  (void)defrReleaseNResetMemory();

  (void)defrUnsetCallbacks();
  (void)defrSetUnusedCallbacks(unUsedCB);

  fflush(stdout);
  //    printf("Component Dump Check\n");

  //    DumpDefComponentPinToNet();
  //    int testInput = 0;
  //    scanf("%d", &testInput);

  // Unset all the callbacks
  defrUnsetArrayNameCbk();
  defrUnsetAssertionCbk();
  defrUnsetAssertionsStartCbk();
  defrUnsetAssertionsEndCbk();
  defrUnsetBlockageCbk();
  defrUnsetBlockageStartCbk();
  defrUnsetBlockageEndCbk();
  defrUnsetBusBitCbk();
  defrUnsetCannotOccupyCbk();
  defrUnsetCanplaceCbk();
  defrUnsetCaseSensitiveCbk();
  defrUnsetComponentCbk();
  defrUnsetComponentExtCbk();
  defrUnsetComponentStartCbk();
  defrUnsetComponentEndCbk();
  defrUnsetConstraintCbk();
  defrUnsetConstraintsStartCbk();
  defrUnsetConstraintsEndCbk();
  defrUnsetDefaultCapCbk();
  defrUnsetDesignCbk();
  defrUnsetDesignEndCbk();
  defrUnsetDieAreaCbk();
  defrUnsetDividerCbk();
  defrUnsetExtensionCbk();
  defrUnsetFillCbk();
  defrUnsetFillStartCbk();
  defrUnsetFillEndCbk();
  defrUnsetFPCCbk();
  defrUnsetFPCStartCbk();
  defrUnsetFPCEndCbk();
  defrUnsetFloorPlanNameCbk();
  defrUnsetGcellGridCbk();
  defrUnsetGroupCbk();
  defrUnsetGroupExtCbk();
  defrUnsetGroupMemberCbk();
  defrUnsetComponentMaskShiftLayerCbk();
  defrUnsetGroupNameCbk();
  defrUnsetGroupsStartCbk();
  defrUnsetGroupsEndCbk();
  defrUnsetHistoryCbk();
  defrUnsetIOTimingCbk();
  defrUnsetIOTimingsStartCbk();
  defrUnsetIOTimingsEndCbk();
  defrUnsetIOTimingsExtCbk();
  defrUnsetNetCbk();
  defrUnsetNetNameCbk();
  defrUnsetNetNonDefaultRuleCbk();
  defrUnsetNetConnectionExtCbk();
  defrUnsetNetExtCbk();
  defrUnsetNetPartialPathCbk();
  defrUnsetNetSubnetNameCbk();
  defrUnsetNetStartCbk();
  defrUnsetNetEndCbk();
  defrUnsetNonDefaultCbk();
  defrUnsetNonDefaultStartCbk();
  defrUnsetNonDefaultEndCbk();
  defrUnsetPartitionCbk();
  defrUnsetPartitionsExtCbk();
  defrUnsetPartitionsStartCbk();
  defrUnsetPartitionsEndCbk();
  defrUnsetPathCbk();
  defrUnsetPinCapCbk();
  defrUnsetPinCbk();
  defrUnsetPinEndCbk();
  defrUnsetPinExtCbk();
  defrUnsetPinPropCbk();
  defrUnsetPinPropStartCbk();
  defrUnsetPinPropEndCbk();
  defrUnsetPropCbk();
  defrUnsetPropDefEndCbk();
  defrUnsetPropDefStartCbk();
  defrUnsetRegionCbk();
  defrUnsetRegionStartCbk();
  defrUnsetRegionEndCbk();
  defrUnsetRowCbk();
  defrUnsetScanChainExtCbk();
  defrUnsetScanchainCbk();
  defrUnsetScanchainsStartCbk();
  defrUnsetScanchainsEndCbk();
  defrUnsetSiteCbk();
  defrUnsetSlotCbk();
  defrUnsetSlotStartCbk();
  defrUnsetSlotEndCbk();
  defrUnsetSNetWireCbk();
  defrUnsetSNetCbk();
  defrUnsetSNetStartCbk();
  defrUnsetSNetEndCbk();
  defrUnsetSNetPartialPathCbk();
  defrUnsetStartPinsCbk();
  defrUnsetStylesCbk();
  defrUnsetStylesStartCbk();
  defrUnsetStylesEndCbk();
  defrUnsetTechnologyCbk();
  defrUnsetTimingDisableCbk();
  defrUnsetTimingDisablesStartCbk();
  defrUnsetTimingDisablesEndCbk();
  defrUnsetTrackCbk();
  defrUnsetUnitsCbk();
  defrUnsetVersionCbk();
  defrUnsetVersionStrCbk();
  defrUnsetViaCbk();
  defrUnsetViaExtCbk();
  defrUnsetViaStartCbk();
  defrUnsetViaEndCbk();

  //    fclose(fout);

  // Release allocated singleton data.
  defrClear();

  free(inFile[0]);
  return res;
}



