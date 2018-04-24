#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

/**************************************************************
          Process METEOR C++ functions


This file contains various functions needed to transform
the output of a METEOR alignment to the form needed to use
it with confusion networks.


**************************************************************/

typedef struct fileMetaData {
  int length;
  int alnCount;
  int *blankLines; 
} fmd;

typedef struct wordAlignment {
/*  Contains data from one
    line of METEOR output   */
  int sen2start, sen2len; //ref sentence
  int sen1start, sen1len; //test sentence
} waln;

typedef struct alignmentData {
/*  Contains full information
    of one an alignment ("block")
    from a file                   */
  std::string *sen1; //(in words) 
  std::string *sen2; //(in words) 
  int sen1l, sen2l;
  waln *alns; //every word alnmnt 
  int alnc;
} alnData;

void mismatch(std::string found, std::string expected) {
/*  Helpful error if == fails; needs std::to_string() 
    for non string arguments.                           */
  std::cerr << "Mismatch! "<<found<<" found when "<<expected<<" expected!!\n";
}

//\\ PART OF RETURNED STRUCT DYNAMICALLY ALLOCATED \\//
void getFileMetaData(std::string filename, fmd *pfmd) {
/*  Does a sweep of the given file and finds its 
    length and number of blank lines 
    (== no. alignments) writing to stderr if can't */

  int length=0;
  int alnCount=0; //last alignment still has trailing newline
  std::string line;
  std::ifstream myfile(filename);
  if(myfile.is_open()) {
    while(getline(myfile,line)) {
      if(line == "")
        alnCount++;
      length++;
    }
    myfile.close();
  }
  else std::cerr << "Error opening file\n"; 

  pfmd->length = length;
  pfmd->alnCount = alnCount;
}

void readMeteorFile(std::string filename, fmd *pfmd, std::string *plines) {
/*  Read lines from given filename (with metadata)
    (which it then completes by recording blank line positions,
     which are the boundaries of the alignments, which will
     allow correct sizes of alignment arrays.)                  */

  int linecount=0, blanklinecount=0;

  std::string line;
  std::ifstream myfile(filename);
  if(myfile.is_open()) {
    while(getline(myfile,line)) {
      plines[linecount] = line;
      if(line == "") {
        pfmd->blankLines[blanklinecount] = linecount;
        blanklinecount++;
      }
      linecount++;
    }
    myfile.close();
  }
  else std::cerr << "Error opening file\n"; 

  if(linecount == pfmd->length);
  else
    mismatch(std::to_string(linecount), std::to_string(pfmd->length));

  if(blanklinecount == pfmd->alnCount);
  else
    mismatch(std::to_string(blanklinecount), std::to_string(pfmd->alnCount));
}

int wordCount(std::string line) {
  if(line=="") return 0;

  int wc=1;
  for(int i=0; i<line.length(); i++)
    if(line.at(i) == ' ') wc++;

  return wc;
}

//\\ EDITED ARR DYNAMICALLY ALLOCATED \\//
void wordSplit(std::string line, std::string *words, int len) {
/* Splits a given string into a string array of its words   */
  std::istringstream iss(line);
  for(int w=0; w<len; w++) {
    iss >> words[w];
  }
}

void processMeteor(std::string *lines, fmd *pfmd, alnData **dataArr) {
/*  Given METEOR file contents and its metadata,
    file contents will be processed and stored

    ALIGNMENTS START FROM 1. ALL ELSE STARTS FROM 0. */

  int al=0; //line within current alignment
  int alnNum, _;
  int l2s, l2l, l1s, l1l; //start, length
  std::string *sen1;
  std::string *sen2;
  int sen1l, sen2l;
  waln *alns;
  int alnLen;

  for(int i=0; i<pfmd->length; i++) {
    std::string line = lines[i];
    if(line == "") {
      al=-1; //will be 0 next time round (after ++ at end)

      // <alnNum-1> as alnNum 1 indexed but blankLines 0 indexed
      if(pfmd->blankLines[alnNum-1] == i);
      else
        mismatch(std::to_string(pfmd->blankLines[alnNum-1]), std::to_string(i));

    } else if (al == 0) { // Alignment N
      sscanf(line.c_str(), "Alignment %d", &alnNum);

    } else if (al == 1) { // Sentence 1
      sen1l = wordCount(line);
      sen1 = new std::string[sen1l];
      wordSplit(line, sen1, sen1l);

    } else if (al == 2) { // Sentence 2
      sen2l = wordCount(line);
      sen2 = new std::string[sen2l];
      wordSplit(line, sen2, sen2l);

    } else if (al == 3) { // Table header
    } else if (al > 3) { // Line <al-4> of alignment data
      int alni = al-4; //alignment index

      //e.g. 1:1   3:2   0   1.0
      sscanf(line.c_str(), "%d:%d\t\t\t%d:%d\t\t\t%d\t\t\t%d.%d",
                            &l2s,&l2l, &l1s,&l1l, &_, &_,&_);

      //ALIGNMENT NUMBER 1-INDEXED!!!
      if(alni == 0) { // need to init arrays for this aln
        // Length of alignment `block' known
        alnLen = pfmd->blankLines[alnNum-1];
        if(alnNum-1 > 0) //0-index means 1st blank index correct aln length
          alnLen -= (pfmd->blankLines[alnNum-1-1] + 1); //-= +1 as blank not included
        //number of alignment data is 4 smaller (aln#, sen1, sen2, table header)
        alnLen -= 4;

        alns = new waln[alnLen];
      }
      alns[alni] = {l2s, l2l, l1s, l1l};
    }
    dataArr[alnNum-1]->sen1=sen1;
    dataArr[alnNum-1]->sen1l=sen1l;
    dataArr[alnNum-1]->sen2=sen2;
    dataArr[alnNum-1]->sen2l=sen2l;
    dataArr[alnNum-1]->alns=alns;
    dataArr[alnNum-1]->alnc=alnLen;
    al++;
  }
}

void displayAlnData(alnData *data) {
  std::cout << "[[ ";
  for(int i=0; i<data->sen1l; i++)
    std::cout << data->sen1[i] << " ";
  std::cout << "]]\n";

  std::cout << "[[ ";
  for(int i=0; i<data->sen2l; i++)
    std::cout << data->sen2[i] << " ";
  std::cout << "]]";

  for(int i=0; i<data->alnc; i++) {
    if(i%6 == 0) //fixed width rows
      std::cout << std::endl;
    std::cout << "("  << data->alns[i].sen2start << ":"
                      << data->alns[i].sen2len   << " "
                      << data->alns[i].sen1start << ":"
                      << data->alns[i].sen1len   << ")\t";
  }
  std::cout << "\n\n";
}

void displayAlnData(alnData **dataArr, int len) {
  for(int i=0; i<len; i++) {
    std::cout << "[" << i << "]\n";
    displayAlnData(dataArr[i]);
  }
}
/*
int main(int argc, char **argv) {
  int filec=argc-1;
  std::string *files = new std::string[filec];
  for(int i=0; i<filec; i++)
    files[i] = std::string(argv[i+1]);

  for(int fi=0; fi<filec; fi++) {
    fmd meta;
    getFileMetaData(files[fi], &meta);
    meta.blankLines = new int[meta.alnCount]; 
    
    std::string *plines = new std::string[meta.length]; 
    readMeteorFile(files[fi], &meta, plines);

    alnData **data = new alnData *[meta.alnCount]; 
    for(int i=0; i<meta.alnCount; i++)
      data[i] = new alnData; 
    processMeteor(plines, &meta, data);

    std::cout << files[fi] << std::endl;
    displayAlnData(data, meta.alnCount);

    delete[] plines;
    delete[] meta.blankLines;
    for(int i=0; i<meta.alnCount; i++) {
      delete[] data[i]->sen1;
      delete[] data[i]->sen2;
      delete[] data[i]->alns;
      delete[] data[i];
    }
    delete[] data;
  }

  delete[] files;
}*/
