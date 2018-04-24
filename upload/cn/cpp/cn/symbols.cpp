#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <climits>

#include "../meteor/process_meteor.h"

#include <fst/fstlib.h>

typedef fst::StdArc::Label Label;

const int MAX_SENTENCE_LENGTH = 80;
const int MAX_SENTENCE_COUNT = 6 - 1 - 1; // Languages - Pivot - Target(English)
const int MAX_SYMBOL_COUNT = MAX_SENTENCE_LENGTH * MAX_SENTENCE_COUNT;


void er(std::string msg) { std::cerr << msg << std::endl; }


int getBiggestSentence(alnData **data, int len) {
  int b=data[0]->sen2l;
  for(int i=0; i<len; i++) {
    int l = data[i]->sen1l;
    if(l > b) b = l;
  }
  
  return b;
}

std::string longestCommonPrefix(std::string *strs, int len) {
  // end will be the first point they differ.
  int end = strs[0].length();
  for(int si=1; si<len; si++) {
    std::string str1 = strs[si-1]; const char *s1 = str1.c_str();
    std::string str2 = strs[si]; const char *s2 = str2.c_str();

    end = str2.length() < end ? str2.length() : end;

    for(int i=0; i < end; i++) {
      if(s1[i] != s2[i]) {
        end = i;
        break;
      }
    }
  }
  return strs[0].substr(0, end);
}

void initLabels(Label ***labelss, int index, int len, int big) {
  //(we will) Store sen2 in labels[len], 0->(len-1) are the sen1`s
  Label **labels = new Label*[len+1]; 
  labelss[index] = labels;
  for(int i=0; i<=len; i++)
    labels[i] = new Label[big];
}

void getSymbols(Label ***labelss, alnData ***datas, fmd **metas, int filec, std::stringstream *symbols) {
/*/ Store an FST symbol index, with the first "len" arrays containing
    the indices for each of the len "sen1"s, and the "len+1"th item
    containing the indices for "sen2", which is the same for all alns. */
  for(int fi=0; fi<filec; fi++) {  
    Label **labels = labelss[fi]; alnData **data = datas[fi]; fmd *meta = metas[fi]; // re defs
    for(int alni=0; alni <= meta->alnCount; alni++) {
      int senlen; std::string* sen;
      if(alni == meta->alnCount) { // we're doing sen2
        senlen = data[0]->sen2l;
        sen    = data[0]->sen2;
      } else { // we're doing sen1
        senlen = data[alni]->sen1l;
        sen    = data[alni]->sen1;
      }

      for(int seni=0; seni<senlen; seni++) {
        bool unq=true; 
        Label defaultLabel = MAX_SYMBOL_COUNT*fi + MAX_SENTENCE_LENGTH*alni + seni + 1;
        std::string word=sen[seni]; 
        if(fi==0 && alni==0 && seni==0) {
          // Very begining
          *symbols << word<<" "<< defaultLabel << std::endl;
          labels[alni][seni] = defaultLabel;
        }
        else {
          //check words up to this one!
          for(int pfi=0; pfi<=fi; pfi++) {
            Label **plabels = labelss[pfi]; alnData **pdata = datas[pfi]; fmd *pmeta = metas[pfi]; // 'prior' re defs
            int maxpalni = (pfi < fi) ? pmeta->alnCount : alni;
            for(int palni=0; palni <= maxpalni; palni++) {
              int psenlen; std::string* psen;
              if(palni == meta->alnCount  ||  palni == pmeta->alnCount) { // sen2
                psenlen = pdata[0]->sen2l;
                psen    = pdata[0]->sen2;
              } else { // sen1
                psenlen = pdata[palni]->sen1l;
                psen    = pdata[palni]->sen1;
              }

              int maxpseni = (pfi == fi && palni == alni) ? seni : psenlen;
              for(int pseni=0; pseni < maxpseni; pseni++) {
                std::string pword = psen[pseni];
                
                if(word == pword) { // clash, need to reassign word
                  unq = false;
                  labels[alni][seni] = plabels[palni][pseni];
                }
                if(!unq) break;
              }
              if(!unq) break;
            }
            if(!unq) break;
          }

          if(unq) {
            *symbols << word<<" "<< defaultLabel << std::endl;
            labels[alni][seni] = defaultLabel;
          }
        }
      }
    }
  }
}

int getSparseLabelLength(std::string file) {
  std::string line, prevline;
  std::ifstream ifs(file);
  if(ifs.is_open()) {
    while(getline(ifs,line)) {
      prevline = line;
    }
    ifs.close();
  }
  else std::cerr << "Error opening file\n"; 

  int len;
  sscanf(prevline.c_str(), "%*s %d", &len);
  return len+1;
}

void getSparseLabelArray(std::string file, std::string **sparse) {
  int index=0, i;
  char word[100];

  std::string line, prevline;
  std::ifstream ifs2(file);
  if(ifs2.is_open()) {
    while(getline(ifs2,line)) {
      sscanf(line.c_str(), "%s %d", word, &i);
      while(i > index) {
        sparse[index++] = NULL;
      }
      sparse[index++] = new std::string(word);
    }
    ifs2.close();
  }
  else std::cerr << "Error opening file\n"; 
}

std::string createSymbols(std::string *files, alnData ***datas, fmd **metas, int filec, Label ***labelss, std::string prefix) {
/*/ Interface function; will create labels array and all symbol files. */
  std::stringstream ss;
  std::stringstream *syms = &ss;
  *syms << "<eps> 0" << std::endl;
  for(int fi=0; fi<filec; fi++) {
    const int OFFSET = fi * MAX_SYMBOL_COUNT;
    int len = metas[fi]->alnCount;
    int big = getBiggestSentence(datas[fi], len);

    // Get basic label indices
    initLabels(labelss, fi, len, big);
  }

  getSymbols(labelss, datas, metas, filec, syms);
  std::string filename = prefix + "sym";
  std::ofstream ofs(filename);
  ofs << ss.rdbuf();
  ofs.close();

  return filename;
}

