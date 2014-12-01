#pragma once

#include "ast.h"
#include <string>
#include <istream>
#include <iostream>
#include <map>

enum Token {
  // Конец файла
  _EOF = -1,
  // Перевод строки
  _NEWLINE = -2,
  // Константа
  _CONST = -3,
  // Идентификатор
  _ID = -4,
  // Лексемы условного оператора
  _IF = -5,
  _ELSE = -6,
  _END = -7,
};

static std::string NAMES[] = {
	"div", "mod", "nil", "in",
	"if", "then", "else", "case", 
	"of", "repeat", "until", "while", 
	"do", "for", "to", "downto", 
	"begin", "end", "with", "goto"
	"const", "var", "array", "record", 
	"set", "file", "function", "procedure", 
	"label", "packed", "proqram"
};

// Лексический анализатор.
class Lexer {
  std::istream &_input;
  int _lastChar;
  bool _isFloat;
  std::map<std::string, bool> KEYWORDS;


public:
  Lexer(std::istream &input)
	  : _input(input), _lastChar(' '), _IDName(""), _stringValue("") {
	  for (int i = 0; i < 30; ++i)
		  KEYWORDS[NAMES[i]] = true;
  }

  ~Lexer() {}

  // Получение очередной лексемы из потока.
  int GetToken();

  // Публично доступные атрибуты лексемы.
  std::string _IDName;
  std::string _stringValue;
};

// Синтаксический анализатор.
// =====================================================================

class Parser {
	std::istream &_input;
	Lexer *_lex;
	int _currentToken;

public:
	Parser(std::istream &input) : _input(input), _isValid(true) {
		_lex = new Lexer(_input);
    NextToken();
  }

	~Parser() { delete _lex; }

	Root *_ast;

	void Parse();

	bool IsSuccess() const { return _isValid; }

private:

  void NextToken() { _currentToken = _lex->GetToken(); }

  void SkipNewline() {
	  while (_currentToken == _NEWLINE) {
      NextToken();
    }
  }

  // Обработка ошибок
  bool _isValid;

  void SkipAssign();
  void SkipToEndToken();
};
