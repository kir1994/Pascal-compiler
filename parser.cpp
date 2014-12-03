#include "parser.h"

#include <cctype>
#include <ios>
#include <sstream>

// Лексический анализатор
// =====================================================================

static int isnewline(int c) { return (c == '\n' || c == '\r'); }


//
//void Parser::SkipAssign() {
//  while (CurrentToken != tok_newline && CurrentToken != tok_eof) {
//    NextToken();
//  }
//}
//
//void Parser::SkipToEndToken() {
//  while (CurrentToken != tok_eof && CurrentToken != tok_end) {
//    NextToken();
//  }
//}
