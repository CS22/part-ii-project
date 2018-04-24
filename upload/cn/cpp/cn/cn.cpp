#include <iostream>
#include <fstream>
#include <string>

#include  "./tdefs.h"
#include  "./symbols.cpp"
#include  "./nbest.cpp"
#include "../meteor/process_meteor.h"

#ifndef FST_HEADER_GUARD
#define FST_HEADER_GUARD
#include <fst/fstlib.h>
#endif

void initialiseCN(SVF *cn, alnData *data, int len, Label **labs) {
  cn->AddState();

  for(int w=0; w<data->sen2l; w++) {
    cn->AddState(); //state w+1
    cn->AddArc(w, fst::StdArc(
      labs[len][w], // Input label
      labs[len][w], // Output label
      -1,  // Weight, -ve => "bigger" is better
      w+1) // Next state, one above current
    ); 
  }
}


void createArcOrAddWeight(SVF *cn, StateId state0, StateId state1, 
    Label label, Weight w) {
  bool exists=false;
  // If an arc with the right label exists, just add the new weight on
  for(fst::MutableArcIterator<SVF> aiter(cn, state0); !aiter.Done(); aiter.Next()) {
    fst::StdArc arc = aiter.Value();
    if(arc.ilabel == label && arc.nextstate == state1) {
      exists=true;
      aiter.SetValue(fst::StdArc(
        arc.ilabel, arc.olabel,
        arc.weight.Value() + w.Value(), arc.nextstate)
      );
    }
  }

  // Otherwise add a new arc with the given weight
  if(!exists)
    cn->AddArc(state0, fst::StdArc(
      label, label, w, state1)
    );
}

StateId insertArc(SVF *cn, StateId state0, StateId state1, Label label, int hypn) {
/*/ "Insert" an arc by redirecting arcs to a new state, adding the new arc from that state */
/*/ Or, if state0 < 0; add state [at start] but no arc redirecting */
/*/ Or, if state1 < 0; add state [at end] but no arc redirecting */
/*/ Then return new state's ID. */

  // Add the new state
  StateId newState = cn->AddState();

  // Change all previous arcs from 0->1 to this new state
  if(state0 >= 0 && state1 >= 0) {
    for(fst::MutableArcIterator<SVF> aiter(cn, state0); !aiter.Done(); aiter.Next()) {
      fst::StdArc arc = aiter.Value();
      if(arc.nextstate == state1)
        aiter.SetValue(fst::StdArc(
          arc.ilabel, arc.olabel,
          arc.weight, newState)
        );
    }
  }

  // Add the new desired transition
  if(state0 <= 0 && state1 <= 0) {
    std::cerr << "Both state0 and state1 were less than 0!!" << std::endl;
  } else if( (state0 >= 0 && state1 >= 0) || (state0 <= 0) ) {
    state0=newState;
  } else if(state1 <= 0) {
    state1=newState;
  }

  cn->AddArc(state0, fst::StdArc(label, label, -1, state1));

  // Add additional eps transitions in number equivalent to the number of hyps done so far,
  //  i.e. an eps for every path that would chose *not* to use this new transition
  if(hypn) createArcOrAddWeight(cn, state0, state1, 0, -hypn);
//  else std::cerr << "Couldn't add epsiolns - 0 weight" << std::endl;

  return newState;
}

void makeFinalAndStart(SVF *cn) {
  bool *isDest = new bool[cn->NumStates()]; 
  for(int i=0; i<cn->NumStates(); i++) isDest[i]=false;

  bool *isSource = new bool[cn->NumStates()]; 
  for(int i=0; i<cn->NumStates(); i++) isSource[i]=false;

  for(fst::StateIterator<SVF> siter(*cn); !siter.Done(); siter.Next()) {
    for(fst::ArcIterator<SVF> aiter(*cn, siter.Value()); !aiter.Done(); aiter.Next()) {
      // If there are any arcs (i.e. we've reached here) then isSource)
      isSource[siter.Value()] = true;
      // Each arc's destination states isDest
      isDest[aiter.Value().nextstate] = true;
    }
  }

  for(int i=0; i<cn->NumStates(); i++) {
    if(! isDest[i] )
      cn->SetStart(i);
    if(! isSource[i] )
      cn->SetFinal(i, 0);
  }

  delete[] isDest;
  delete[] isSource;

}

StateId getNextState(SVF *cn, StateId state) {
/*/ Only works on linear CNs (luckily ours has to be), returns
    next state by simply giving the destination of the given
    state's first arc                                          */

  for(fst::StateIterator<SVF> siter(*cn); !siter.Done(); siter.Next())
    if(siter.Value() == state) //have to do this to satisfy openfst lib conventions(constraints)
      for(fst::ArcIterator<SVF> aiter(*cn, state); !aiter.Done(); aiter.Next())
        return aiter.Value().nextstate;

  //otherwise, error
  return -1;
}

bool checkForLabel(SVF *cn, StateId state, Label label) {
  for(fst::StateIterator<SVF> siter(*cn); !siter.Done(); siter.Next())
    if(siter.Value() == state)
      for(fst::ArcIterator<SVF> aiter(*cn, state); !aiter.Done(); aiter.Next())
        if(aiter.Value().ilabel == label)
          return true;
  return false;
}

void getCN(SVF *cn, alnData **data, int len, Label **labels, std::string fname) {

  initialiseCN(cn, data[0], len, labels);

  for(int alni=0; alni<len; alni++) {
    alnData *aln = data[alni];
    int hypn = alni+1; //as sen2 is hyp 0

    int maxsenlen = 9 * aln->sen2l; // sentence length ratio < 9 (from moses)
    // These marked states will be the ones that *won't* need <eps> arcs added at end of pass
    int *markedStates = new int[maxsenlen]; 
    // 0 = unmarked, 1 = marked but no 2h arc, 2 = marked and 2h arc
    for(int i=0;i<9*aln->sen2l;i++) markedStates[i]=0;

    // (1) Add all *aligned* words [to CN] ////////////////////////////////////////////////////
    for(int wi=0; wi<aln->alnc; wi++) { 
      waln wa = aln->alns[wi];

      StateId state; Label label; StateId ns;
      int lend = wa.sen2len - wa.sen1len;
      if(lend==0) { // Align each 2h word to Ph word
        for(int d=0; d<wa.sen2len; d++) {
          state = wa.sen2start + d; // State n has arc with Ph n'th word
          label = labels[alni][wa.sen1start + d];
          ns    = getNextState(cn, state);

          markedStates[state] = 2;  
          createArcOrAddWeight(cn, state, ns, label, -1); 
        }
      } else if(lend > 0) { 
        for(int d=0; d<lend; d++) {
          state = wa.sen2start + d;
          label = labels[alni][wa.sen1start + d];
          ns    = getNextState(cn, state);
  
          markedStates[state] = 2;
          createArcOrAddWeight(cn, state, ns, label, -1);
        }
        for(int d=lend; d<wa.sen2len; d++) { // ... mark the rest
          state = wa.sen2start + d;
          markedStates[state] = 1;
        }
      } else if(lend < 0) { 
        for(int d=0; d<-lend; d++) {
          state = wa.sen2start + d;
          label = labels[alni][wa.sen1start + d];
          ns    = getNextState(cn, state);
  
          markedStates[state] = 2;
          createArcOrAddWeight(cn, state, ns, label, -1);
        }
        for(int d=-lend; d<wa.sen1len; d++) { // (Insert/add)+mark the rest as new states before next Ph state
          label = labels[alni][wa.sen1start + d];
          if(ns < aln->sen2l) { // Need to insert new state as the next one is a Ph state
            state = insertArc(cn, state, ns, label, hypn);
            markedStates[state] = 2;
          }
          else { // We can align with previous 2h state that's already there
            state = ns;
            ns    = getNextState(cn, state);
            if(ns < 0) { // We're at the end of the CN!!
              ns = insertArc(cn, state, -1, label, hypn);
              markedStates[state] = 2;
            } else {
              createArcOrAddWeight(cn, state, ns, label, -1);
              markedStates[state] = 2;
            }
          }
        }
      }
        
    } 

    // (2) Add all *unaligned* words [to CN] ///////////////////////////////////////////////////////
    bool *aligned = new bool[aln->sen1l]; 
    for(int i=0; i<aln->sen1l; i++) aligned[i]=false;
    int firstAligned = aln->sen1l;
    for(int wi=0; wi<aln->alnc; wi++) {
      waln wa = aln->alns[wi];
      if(firstAligned > wa.sen1start) firstAligned = wa.sen1start;
      for(int d=0; d<wa.sen1len; d++) {
        aligned[wa.sen1start + d] = true;
      }
    }

    int i=0;
    while(i < aln->sen1l) {
      if(aligned[i] == true) {
        i++; 
      } else if(i == 0) { // First one is a special case

        StateId ns;
        for(StateId s=0; s < cn->NumStates(); s++) {
          if( (markedStates[s]==2) && (checkForLabel(cn, s, labels[alni][firstAligned])) ) {
            ns = s;
            break;
          }
        }

        StateId state = -1;
        for(StateId s=0; s < cn->NumStates(); s++) {
          if(getNextState(cn, s) == ns) {
            state = s; break;
          }
        }

        for(i=firstAligned-1; i>=0; i--) {
          // If prior state is PH then ins, if SH then add
          if(state < aln->sen2l) {
            state = insertArc(cn, state, ns, labels[alni][i], hypn);
            markedStates[state] = 2;
          } else {
            createArcOrAddWeight(cn, state, ns, labels[alni][i], -1);
            markedStates[state] = 2;
            ns = state;
            for(StateId s=0; s < cn->NumStates(); s++) {
              if(getNextState(cn, s) == ns) {
                state = s; break;
              }
            }
          }
        }
        i=firstAligned;

      } else {
        // Find state to insert after
        for(StateId s=0; s < cn->NumStates(); s++) {
          if( (markedStates[s]==2) && (checkForLabel(cn, s, labels[alni][i-1])) ) {
            // Insert as many as we can
            StateId ns = getNextState(cn, s);
            StateId state = s;
            while(aligned[i] == false && i < aln->sen1l) {
              Label l = labels[alni][i];
              if(ns < aln->sen2l) { // it's a PH state so insert before
                state = insertArc(cn, state, ns, l, hypn);
                markedStates[state] = 2;
              } else { // We can align with previous 2h state that's already there
                state = ns;
                ns    = getNextState(cn, state);
                if(ns < 0) { // We're at the end of the CN!!
                  ns = insertArc(cn, state, -1, l, hypn);
                  markedStates[state] = 2;
                } else {
                  createArcOrAddWeight(cn, state, ns, l, -1);
                  markedStates[state] = 2;
                }
              }
              i++;
            }
            break;
          }
        }
      }
    }
  

    delete[] aligned;

    // (3) Add <eps> arcs to all unmarked states ///////////////////////////////////////////////////
    for(fst::StateIterator<SVF> siter(*cn); !siter.Done(); siter.Next()) {
      StateId state = siter.Value();
      if(getNextState(cn, state) > -1) {
        StateId ns = getNextState(cn, state);
        if(!markedStates[state]) { // Need to add eps!
          createArcOrAddWeight(cn, state, ns, 0, -1);
        }
      }
    }
    
    delete[] markedStates;
  }

  makeFinalAndStart(cn);

  for(int i=0; i<=len; i++)
    delete[] labels[i];
  delete[] labels;
}

void lengthTrack(alnData **data, int len, int *tlen, int *c) {
/* Maintain total length, must be 0 initialised before first call */
  *tlen += data[0]->sen2l; (*c)++;
  for(int i=0; i<len; i++) {
    *tlen += data[i]->sen1l; (*c)++;
  }
}

void initProcessFiles(std::string *files, int filec, fmd **metas, alnData ***datas, int *totalHypLength, int *hypCount) {

  for(int fi=0; fi<filec; fi++) {
    fmd *meta = new fmd;
    metas[fi] = meta;
    getFileMetaData(files[fi], meta);
    meta->blankLines = new int[meta->alnCount]; 
    
    std::string *plines = new std::string[meta->length]; 
    readMeteorFile(files[fi], meta, plines);

    alnData **data = new alnData *[meta->alnCount]; 
    datas[fi] = data;
    for(int i=0; i<meta->alnCount; i++)
      data[i] = new alnData; 
    processMeteor(plines, meta, data);
    std::cerr << files[fi] << std::endl;
//    displayAlnData(data, meta->alnCount);

    delete[] plines;
    delete[] meta->blankLines;

    // For averaging, length penalties
    lengthTrack(data, meta->alnCount, totalHypLength, hypCount);
  }

}

int getStateCount(SVF* cn) {
/*/ Must be given a standard for alignment CN (i.e. linear) */
  int count=0;
  for(fst::StateIterator<SVF> siter(*cn); !siter.Done(); siter.Next()) 
    count++;
  return count;
}

int main(int argc, char **argv) {

  int filec=argc-1;
  std::string *files = new std::string[filec];
  for(int i=0; i<filec; i++)
    files[i] = std::string(argv[i+1]);
  
  SVF *cns = new SVF[filec]; 
  SVF unionCN;

  int totalHypLength=0;
  int hypCount=0;

  fmd **metas = new fmd*[filec]; 
  alnData ***datas = new alnData**[filec]; 
  Label ***labelss = new Label**[filec]; 

  std::string prefix = longestCommonPrefix(files, filec);
  if(prefix.c_str()[prefix.length()-1] != '.')
    prefix += ".";

  initProcessFiles(files, filec, metas, datas, &totalHypLength, &hypCount);
  float avgLen = totalHypLength / hypCount;
  std::cerr << "Average hyp length: " << avgLen << std::endl;

  std::string fname = createSymbols(files, datas, metas, filec, labelss, prefix);
  std::string **sparseLabels;
  int splLen = getSparseLabelLength(fname);
  sparseLabels = new std::string*[splLen];
  getSparseLabelArray(fname, sparseLabels);

  int maxStateCount=0;
  for(int fi=0; fi<filec; fi++) {
    Label **labels = labelss[fi]; alnData **data = datas[fi]; fmd *meta = metas[fi]; 
    SVF cn = cns[fi];
    getCN(&cn, data, meta->alnCount, labels, files[fi]);

    int sc=getStateCount(&cn);
    if(sc>maxStateCount) maxStateCount=sc;

    fst::Union(&unionCN, cn);
    cn.Write(files[fi]+".cn");
  }

  getBest(&unionCN, 10000, 500, 2*maxStateCount, avgLen, sparseLabels);

  unionCN.Write(prefix+"ucn");

  delete[] cns;
  delete[] files;
}
