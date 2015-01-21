#include <iostream>
#include <fstream>

#include "codegen.h"
#include "parser.h"

int main(int argc, char **argv) {
  std::ifstream input;
  input.open("123.txt");

  Parser *P = new Parser(input);
  P->Parse();
  if (!P->IsSuccess()) {
	  std::cout << "Parser failed... :(\n";
	  return 1;
  }

  CodeGenerator *pGen = CodeGenerator::getInstance();
  pGen->Generate(P);

  pGen->Dump();

  int res = pGen->Execute();

  std::cout << "Result: " << res << std::endl;

  pGen->Release();

  return 0;
}
