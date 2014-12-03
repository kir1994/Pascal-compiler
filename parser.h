#pragma once

#include "ast.h"
#include <string>
#include <istream>
#include <iostream>
#include <map>

enum Token {
  // Конец файла
  T_EOF = -1,
  // Перевод строки
  T_NEWLINE = -2,
  // Константа
  T_CONST = -3,
  // Значение метки
  T_UINT,
  T_STRING,
  T_UNUMBER,
  T_ID,
  T_CONSTVAL,
  T_VARTYPE,
  //Начало программы
  T_PROGRAM,
  T_LABEL,
  T_VAR,
  T_FUNC,
  T_PROC,
  T_BEGIN,
  T_EXPR_OP,
  T_SEXPR_OP,
  T_TERM_OP,
  // ;
  T_SEMICOLON,
  // :
  T_COLON,
  //,
  T_COMMA,
  //=
  T_ASSIGN,
  //'
  T_QUOTE,
  //(
  T_LBR,
  //)
  T_RBR,
  //-
  T_SIGN,
  T_END
};

VAR_TYPES Str2Type(const std::string& type);
EXPR_OP Str2ExprOp(const std::string& type);
SEXPR_OP Str2SExprOp(const std::string& type);
TERM_OP Str2TermOp(const std::string& type);

static std::string NAMES[] = {
	"div", "mod", "nil", "in",
	"if", "then", "else", "case", 
	"of", "repeat", "until", "while", 
	"do", "for", "to", "downto", 
	"begin", "end", "with", "goto"
	"const", "var", "array", "record", 
	"set", "file", "function", "procedure", 
	"label", "packed", "program",
	"integer", "char", "Boolean", "real"
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
	  for (int i = 0; i < 34; ++i)
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
	Root *_ast;
	std::string _tokenValue;
	int _currentToken;

public:
	Parser(std::istream &input) : _input(input), _isValid(true) {
		_lex = new Lexer(_input);
		_ast = new Root;
		NextToken();
  }

	~Parser() 
	{ 
		delete _lex; 
		delete _ast;
	}

	void Parse()
	{
		_ast->Parse(this);

		_isValid = true;
	}

	std::string GetCurrentValue()
	{
		std::string res = _tokenValue;
		NextToken();
		return res;
	}

	Statement * ParseStatement();
	Factor * ParseFactor();

	bool IsSuccess() const { return _isValid; }

	//void NextToken() { _currentToken = _lex->GetToken(); }

	/*void SkipNewline() {
	  while (_currentToken == T_NEWLINE)
		NextToken();
	}*/

	bool Is(const Token& tok);//Проверка, равен ли текущий токен tok
	void ShouldBe(const Token& tok);//Считать токен, он должен быть равен tok
	void MustBe(const Token& tok);//Текущий токен должен быть равен tok. Считать следующий
	void NextToken();

  // Обработка ошибок
  bool _isValid;

  /*void SkipAssign();
  void SkipToEndToken();*/
};

