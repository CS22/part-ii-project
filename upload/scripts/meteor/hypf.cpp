#include <iostream>
#include <fstream>
#include <string>

/********************************************************************
                    ============================                      
                    = Hypothesis file creation =                     
                    ============================                      
                      Must be called within the
                          desired directory

********************************************************************/

int fileLength(std::string fn) {
  std::ifstream fs(fn);

  std::string l;
  int c=0;
  if(fs.is_open()) 
    while(getline(fs,l)) {
      c++;
    }
  else
    std::cerr << "ERROR OPENING FILE #0E" << std::endl;
  
  return c;
}

void getFile(std::string fn, std::string* ls) {
  std::ifstream fs(fn);

  std::string l;
  int c=0;
  if(fs.is_open()) 
    while(getline(fs,l)) {
      ls[c] = l;
      c++;
    }
  else
    std::cerr << "ERROR OPENING FILE #3E" << std::endl;
}

void primHypos(std::string fn, int fc) {
/*  Write primary hypothesis files, each containing
    ONE of the file's FL lines, FC-1 times (as the
    2ndary hyp. file will contain the other FC-1
    files' (e.g. kth) lines                         */

  int fl = fileLength(fn);
  std::string* flines = new std::string[fl]; //\\DYNA\\//
  getFile(fn, flines);

  for(int k=0; k<fl; k++) { // will be making a file for each line in current
    std::string line=flines[k];
    std::string outname="prim."+std::to_string(k)+"."+fn;
    std::ofstream fs(outname);
    if(fs.is_open())
      for(int i=0; i<fc-1; i++) fs << line << std::endl;
    else
      std::cerr << "ERROR OPENING FILE #1E" << std::endl;
  }

  delete[] flines;
}

void secHypos(std::string* fnames, int fc, std::string hypfn) {
/*  Write secondary hypothesis files, each containing
    ONE of FC-1 files' (e.g.) Kth lines, excluding the
    line of the primary hypothesis they are tied to.    */

  int fl = fileLength(fnames[0]); //they'll all be the same

  int hypi;
  std::string** flinesArr = new std::string*[fc]; //\\DYNA\\//
  for(int fi=0; fi<fc; fi++) 
    if(fnames[fi] != hypfn) {
      flinesArr[fi] = new std::string[fl]; //\\DYNA\\//
      getFile(fnames[fi], flinesArr[fi]);
    } else
      hypi=fi;

  for(int k=0; k<fl; k++) { //for each line in (all) file(s)
    std::string outname="scnd."+std::to_string(k)+"."+hypfn;
    std::ofstream fs(outname);
    if(fs.is_open()) {
      for(int fi=0; fi<fc; fi++)
        if(fi != hypi) fs << flinesArr[fi][k] << std::endl;
    } else
      std::cerr << "ERROR OPENING FILE #2E" << std::endl;
  }

  for(int fi=0; fi<fc; fi++) 
    if(fi!=hypi)
      delete[] flinesArr[fi];
  delete[] flinesArr;
} 


int main(int argc, char** argv) {
  int filec=argc-1;
  std::string* files = new std::string[filec];
  for(int i=0; i<filec; i++)
    files[i] = std::string(argv[i+1]);

  for(int primi=0; primi<filec; primi++) {
    std::cout << "Processing " << files[primi] << "..." << std::endl;
    primHypos(files[primi], filec);
    secHypos(files, filec, files[primi]);
  }

  delete[] files;

}
