#include <iostream>
#include <fstream>

#include "parser.h"

int main(int argc, char **argv) {
  std::ifstream input;
  input.open("../tests/test.pas");
  Parser *P = new Parser(input);
  P->Parse();
  if (P->IsSuccess())
	  std::cout << "Success!\n";
  else
	  std::cout << "Fail...:(\n";

  return 0;
}
