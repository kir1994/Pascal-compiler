#pragma once

#include "ast.h"
#include <string>
#include <istream>
#include <iostream>
#include <map>
#include <unordered_set>


enum Token {
  // Конец файла
  T_EOF,
  // Виды констант
  T_STRING,
  T_UNUMBER,
  T_ID,
  // Простой тип
  T_VARTYPE,
  //Служебные слова
  T_CONST,
  T_PROGRAM,
  T_LABEL,
  T_VAR,
  T_FUNC,
  T_PROC,
  T_BEGIN,
  T_END,
  T_TRUE,
  T_FALSE,
  // Различные операции
  T_FACTOROP,
  T_TERMOP,
  T_COND,
  // Условный оператор IF
  T_IF,
  T_THEN,
  T_ELSE,
  // Операторы цикла
  T_FOR,
  T_TO,
  T_DOWNTO,
  T_DO,
  T_WHILE,
  T_REPEAT,
  T_UNTIL,
  //;
  T_SEMICOLON,
  //:
  T_COLON,
  //,
  T_COMMA,
  //:=
  T_ASSIGN,
  //~
  T_NEG,
  //(
  T_LBR,
  //)
  T_RBR,
  //nil
  T_NIL
};

static std::unordered_set<std::string> NAMES = {
	"nil", "in", "if", "then", "else", "case", 
	"of", "repeat", "until", "while", 
	"do", "for", "to", "downto", 
	"begin", "end", "with", "goto",
	"const", "var", "array", "record", 
	"set", "file", "function", "procedure", 
	"label", "packed", "program", "true", "false"
	
};
static std::unordered_set<std::string> TYPE_NAMES = {
	"integer", "char", "Boolean", "real"
};
static std::unordered_set<std::string> COND_NAMES = {
	"=", "<>", ">", ">=", 
	"<", "<=", "in"
};
static std::unordered_set<std::string> FACTOR_NAMES = {
	"*", "/", "div", 
	"mod", "and", "^"
};

static std::unordered_set<std::string> TERM_NAMES = {
	"+", "-", "or",
};


// Лексический анализатор
class Lexer {
  std::istream &_input;
  int _lastChar;

  unsigned _curLine;

public:
  Lexer(std::istream &input)
	  : _input(input), _lastChar(' '), _IDName(""), _stringValue(""), _curLine(0) { }

  ~Lexer() {}

  // Получение очередной лексемы из потока
  Token GetToken();
  unsigned GetLine() { return _curLine; }

  std::string _IDName;
  std::string _stringValue;
};

// Синтаксический анализатор.
// =====================================================================

class Parser {
public:
	std::istream &_input;
	Lexer *_lex;
	Root *_ast;
	Token _currentToken;

public:
	Parser(std::istream &input) : _input(input), _isValid(true) {
		_lex = new Lexer(_input);
		NextToken();
  }

	~Parser() 
	{ 
		delete _lex; 
		delete _ast;
	}

	void Parse();

	std::string GetCurrentValue()
	{
		std::string res = _lex->_stringValue;
		NextToken();
		return res;
	}

	void ParseDeclarations(Function *func);
	Statement * ParseStatement();
	StatementSeq * ParseStmntSeq();
	Const * ParseConst();
	Function * ParseFunction(Function *par, bool isFunc);
	ParamList * ParseParamList();
	Expression * ParseExpression();
	Expression * ParseSimpleExpression();
	Expression * ParseTerm();
	Expression * ParseFactor();

	bool _isValid;
	bool IsSuccess() const { return _isValid; }

	void NextToken() { _currentToken = _lex->GetToken(); }

	bool Is(const Token& tok)//Проверка, равен ли текущий токен tok
	{
		return (_currentToken == tok);
	}
	void ShouldBe(const Token& tok)//Считать токен, он должен быть равен tok
	{
		NextToken();
		if (!Is(tok))
			throw std::exception();
	}
	void MustBe(const Token& tok)//Текущий токен должен быть равен tok. Считать следующий
	{
		if (!Is(tok))
			throw std::exception();
		NextToken();
	}
};

