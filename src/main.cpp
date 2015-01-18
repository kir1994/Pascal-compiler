#include <iostream>
#include <fstream>

#include "codegen.h"
#include "parser.h"

int main(int argc, char **argv) {
  std::ifstream input;
  input.open("..\\tests\\test3.pas");
  Parser *P = new Parser(input);
  P->Parse();
  if (P->IsSuccess())
	  std::cout << "Success!\n";
  else
	  std::cout << "\nFail...:(\n";

  CodeGenerator *pGen = CodeGenerator::getInstance();
  pGen->Generate(P);

  pGen->Dump();

  int res = pGen->Execute();

  std::cout << "Result: " << res << std::endl;

  pGen->Release();
  return 0;
}
