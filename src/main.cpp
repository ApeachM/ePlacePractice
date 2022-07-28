#include <iostream>
#include "Circuit.h"
#include <fstream>
#include <vector>
#include <sstream>
#include <string>

using namespace std;
using namespace ePlace;
int main() {
  Circuit circuit;
  string lefName = "../bench/ispd18/ispd18_test1/ispd18_test1.input.lef";
  string defName = "../bench/ispd18/ispd18_test1/ispd18_test1.input.def";

  circuit.parsing(lefName, defName);
  circuit.addCellList();
  circuit.addNetList();
  cout <<"test" << endl;

  //open file
/*

  ifstream fin;
//  fin.open("/home/minjae/CLionProjects/ePlacePractice/bench/ispd18/ispd18_test1/ispd18_test1.input.def",ifstream::in);
  fin.open("../bench/ispd18/ispd18_test1/ispd18_test1.input.def",ifstream::in);
  string line;
  char cmp[4];

  if (fin.is_open()==0) {
    cerr << "Could not open the file"  << endl;
    return EXIT_FAILURE;
  }
  else {
    while(getline(fin,line,';')) {
      cout << line << endl;
      cmp[0]=line[0];
      cmp[1]=line[1];
      cmp[2]=line[2];
      cmp[3]=line[3];

      istringstream ss(line);
      vector <string> words;
      string word;

      while(getline(ss,word,' ')){
        words.push_back(word);
      }

      if(strcmp(cmp,"VERS")==1)
      {

      }
      else if(strcmp(cmp,"DIVI")==1)
      {
          circuit.lefDivider=line[13];
      }
      else if(strcmp(cmp,"BUSB")==1)
      {

      }
    }
  }
*/


}
