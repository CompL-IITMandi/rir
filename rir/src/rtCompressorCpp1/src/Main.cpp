#define MAX_RT_STACK_SIZE 1000000
#define BLACKLIST_THRESHOLD 10

#include <string>
#include "RshMethod.h"
#include "RshJsonParser.h"

bool validInputPath(char * inputPath) {
  char * t = inputPath;
  while (*t != '\0') {
    if (*t == '.') {
      if (*(++t) == 'l' && *(++t) == 'o' && *(++t) == 'g' && *(++t) == 'g') return true;
      return false;
    }
    t++;
  }
  std::cout << std::endl;
  return false;
}

bool validOutputPath(char * inputPath) {
  char * t = inputPath;
  while (*t != '\0') {
    if (*t == '.') {
      if (*(++t) == 'j' && *(++t) == 's' && *(++t) == 'o' && *(++t) == 'n') return true;
      return false;
    }
    t++;
  }
  std::cout << std::endl;
  return false;
}

int main(int argc, char** argv) {
  srand(time(0));
  std::string del = ",";
  char* inputPath = argv[1];
  char* outputPath = argv[2];
  int level = std::stoi(argv[3]);
  int verbose = std::stoi(argv[4]);  
  
  if (!validInputPath(inputPath)) {
    std::cerr << "ERR: Input file path not specified [with .logg extension]" << std::endl;
    exit(0);
  }
  if (!validOutputPath(outputPath)) {
    std::cerr << "ERR: Output file path not specified [with .json extension]" << std::endl;
    exit(0);
  }
  std::ifstream loggFile;
  loggFile.open(inputPath);
  if( !loggFile ) {
    std::cerr << "ERR: Unable to open logg file" << std::endl;
    exit(1);
  }
  RshJsonParser parser(del, verbose);
  // Start processing the file
  std::string line;
  while(getline(loggFile, line)){
    parser.processLine(line);
  }
  loggFile.close();

  switch (level)
  {
  case 0:
    parser.saveVizJson(outputPath);
    break;
  case 1:
    parser.saveVizJson(outputPath);
    parser.saveLists(outputPath);
    break;
  case 2:
    parser.saveVizJson(outputPath);
    parser.saveCG(outputPath);
    break;
  case 3:
    parser.saveVizJson(outputPath);
    parser.saveLists(outputPath);
    parser.saveCG(outputPath);
    break;
  
  default:
    break;
  }
  
  // Print results
  
  
  
}