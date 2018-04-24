#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#ifndef PROCESS_METEOR_HEADER_GUARD
#define PROCESS_METEOR_HEADER_GUARD

typedef struct fileMetaData {
  int length;
  int alnCount;
  int *blankLines; 
} fmd;

typedef struct wordAlignment {
  int sen2start, sen2len; //ref sentence
  int sen1start, sen1len; //test sentence
} waln;

typedef struct alignmentData {
  std::string *sen1; //(in words) 
  std::string *sen2; //(in words) 
  int sen1l, sen2l;
  waln *alns; //every word alnmnt 
  int alnc;
} alnData;
void mismatch(std::string found, std::string expected);
void getFileMetaData(std::string filename, fmd *pfmd);
void readMeteorFile(std::string filename, fmd *pfmd, std::string *plines);
int wordCount(std::string line);
void wordSplit(std::string line, std::string *words, int len);
void processMeteor(std::string *lines, fmd *pfmd, alnData **dataArr);
void displayAlnData(alnData *data);
void displayAlnData(alnData **dataArr, int len);

#endif
