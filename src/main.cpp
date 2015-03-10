#include <iostream>
#include <fstream>

#include "codegen.h"
#include "parser.h"

int main(int argc, char **argv) {
  std::ifstream input;
  //input.open("../tests/test13.pas");
  if(argc == 2)
  	input.open(argv[1]);
  Parser *P = new Parser(input);
  P->Parse();
  if (!P->IsSuccess()) {
	  std::cout << "Parser failed... :(\n";
	  return 1;
  }

  CodeGenerator *pGen = CodeGenerator::getInstance();

  

  if (pGen->Generate(P)) {
	  pGen->Dump();
	  int res = pGen->Execute();
	  std::cout << std::endl << "Result: " << res << std::endl;
  }

  pGen->Release();

  return 0;
}
