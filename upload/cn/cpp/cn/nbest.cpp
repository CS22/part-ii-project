#include <iostream>
#include <string>
#include <math.h>

#include "../meteor/process_meteor.h"
#include  "./tdefs.h"

#include <fst/fstlib.h>

//maximum difference between generated path and averagge
const int MAX_LEN_DIFF = 5; 

float penaltyFunction(float expected, int actual) {
  return pow((expected - actual),2);
}

class PathData {
  private:
    Label *labels;
    int maxLen;
    size_t len;
    Weight weight;
    Weight newWeight;
    int ranking;
    std::string path;

    void makePath(std::string **sparseLabels);
    float lengthPenalise(float avgLen) { return penaltyFunction(avgLen, this->pathLen); };
    Weight fixWeight(float avgLen) { return (newWeight = weight.Value() + lengthPenalise(avgLen)); }

    size_t pathLen;

  public:
    explicit PathData(int maxStates);
    ~PathData() { delete[] labels; };
    void operator=(const PathData &orig);
    size_t getLen() { return len; };
//    size_t lenActual();
    Weight getWeight() { return weight; };
    Weight getNewWeight() { return newWeight; };
    int getRank() { return ranking; };

    size_t getPathLen() { return pathLen; };

    size_t addLabel(Label l) { labels[len++] = l; return len; };
    Weight addWeight(const Weight &addW) { return (weight = weight.Value() + addW.Value()); };
    void setRank(int r) { ranking = r; };
    void print(std::string prefix);

    void finalise(float avgLength, std::string **sparseLabels);
    std::string getPath() { return path; };
};
PathData::PathData(int maxStates) {
  if(maxStates > 0) {
    this->labels = new Label[maxStates];
    this->maxLen  = maxStates;
    this->len=0; this->weight=0;

    this->ranking = this->maxLen; // at most maxLen-1 (eventually; arcs# < states#)

    for(int i=0; i<maxStates; i++)
      labels[i] = 0;
  }
}
void PathData::finalise(float avgLength, std::string **sparseLabels) {
  fixWeight(avgLength);
  makePath(sparseLabels);
}
void PathData::makePath(std::string **sparseLabels) {
  std::string out = "";
  this->pathLen=0;
  for(int i=0; i<len; i++)
    if(labels[i] != 0) {
      this->pathLen++;
      out += *sparseLabels[labels[i]] + " ";
    }
  this->path = out;
}
void PathData::operator=(const PathData &orig) {
  this->maxLen = orig.maxLen;
  this->len = orig.len;
  this->weight = orig.weight;
  this->ranking = orig.ranking;

  // labels length could be len but do maxlen for consistency 
  this->labels = new Label[this->maxLen]; // DYN! //
  for(int i=0; i<len; i++)
    this->labels[i] = orig.labels[i];
}
void PathData::print(std::string prefix="\t") {
  std::cerr << prefix << "Max length: " << maxLen << std::endl;
  std::cerr << prefix << "(Length):   " << len << std::endl;
  std::cerr << prefix << "Actual len: " << pathLen << std::endl;
  std::cerr << prefix << "Weight:     " << weight << std::endl;
  std::cerr << prefix << "Ranking:    " << ranking << std::endl;
  for(int i=0; i<len; i++)
    std::cerr << prefix << "labels["<<i<<"]: " << labels[i] << std::endl;
}


class Finals {
  private:
    int finalCount;
    int maxCount;
    StateId *finalStates;
    SVF *cn;
    void getFinalStates();
  public:
    Finals(SVF *cn, int maxStates): cn(cn), maxCount(maxStates) {
      getFinalStates(); };
    bool checkFinal(StateId state);
    ~Finals() { delete[] finalStates; };
    int getFC() { return finalCount; };//temp
};
void Finals::getFinalStates() {
  this->finalStates = new StateId[this->maxCount]; 
  int i=0;

  for(fst::StateIterator<SVF> siter(*cn); !siter.Done(); siter.Next()) {
    StateId s = siter.Value();
    if(cn->Final(s) != Weight::Zero()) {
      this->finalStates[i++] = s;
    }
  }
  this->finalCount = i;
}
bool Finals::checkFinal(StateId state) {
  for(int i=0; i<this->finalCount; i++) {
    if(state == this->finalStates[i])
      return true;
  }
  return false;
}


void recPathFind(PathData *currentPath, PathData **paths, int *pathCount,
                 SVF* cn, StateId state, Finals *finals,
                 float avgLen, std::string **sparseLabels) {
  
  if(finals->checkFinal(state)) {
    currentPath->finalise(avgLen, sparseLabels);
    if(abs(avgLen - currentPath->getPathLen()) <= MAX_LEN_DIFF) { //only add if it's withing bounds
      paths[(*pathCount)++] = currentPath;
    std::cout << currentPath->getNewWeight() << " | " << currentPath->getWeight()  << " | "<<currentPath->getPath()<< std::endl;
  }

  PathData *newPath;
  for(fst::ArcIterator<SVF> aiter(*cn, state); !aiter.Done(); aiter.Next()) {
    const fst::StdArc &arc = aiter.Value();
    newPath = new PathData(0);
    *newPath = *currentPath;
    newPath->addLabel(arc.ilabel);
    newPath->addWeight(arc.weight);

    recPathFind(newPath, paths, pathCount, cn, arc.nextstate, finals, avgLen, sparseLabels);
  }
}

int getPaths(SVF *cn, Finals *finals, PathData **paths, int n,
             float avgLen, std::string **sparseLabels) {
/*/ For a given shortestpaths CN, find the strings, path lengths, and path weights,
 *  and return the actual number of paths (out of a maximum of n). Do this by
 *  recursive function calls */

  StateId initialState = cn->Start();
  PathData *initialPath = paths[0]; //default and will also be overwritten
  int pathCount=0; int *pc = &pathCount;
  recPathFind(initialPath, paths, pc,
              cn, initialState, finals,
              avgLen, sparseLabels);

  return *pc;
}


void getBest(SVF* cn, int nBest, int nBestFinal, int maxStates, float avgLength,std::string **sparseLabels) {
  SVF nBestCN;
  fst::ShortestPath(*cn, &nBestCN, nBest, true); // true: unique only
  nBestCN.Write("paths.cn");

  PathData **paths = new PathData*[nBest];
  for(int i=0; i < nBest; i++) {
    paths[i] = new PathData(maxStates);
  }

  Finals finals(&nBestCN, maxStates);
  int nActual = getPaths(&nBestCN, &finals, paths, nBest,
                         avgLength, sparseLabels);

  std::cerr << "MAX PATHS: " << nBest << ", ACTUAL: " << nActual << std::endl;
//  for(int i=0; i<nActual; i++) {
//    paths[i]->finalise(avgLength, sparseLabels);
////    std::cout << i << ": " 
//    std::cout << paths[i]->getNewWeight() << " | " << paths[i]->getWeight()  << " | "<<paths[i]->getPath()<< std::endl;
//  }

}
