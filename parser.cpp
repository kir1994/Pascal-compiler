#include "parser.h"

#include <cctype>
#include <ios>
#include <sstream>

// Лексический анализатор
// =====================================================================

static int isnewline(int c) { return (c == '\n' || c == '\r'); }

int Lexer::GetToken() {
	// Пропускаем ведущие пробелы и символы табуляции.
	while (_lastChar == ' ' || _lastChar == '\t') {
		_lastChar = _input.get();
	}
	// Если буква - значит, идентификатор или ключевое слово.
	if (isalpha(_lastChar)) {
		_IDName = _lastChar;
		while (isalnum((_lastChar = _input.get()))) {
			_IDName += _lastChar;
		}
		// Ключевые слова
		if (KEYWORDS.find(_IDName) != KEYWORDS.end())
			return _ID;

		// Если никакое ключевое слово не подошло, это идентификатор.
		//return _ID;
	}
	  // Если цифра - значит, константа
	if (isdigit(_lastChar))
	{
		_isFloat = false;
		_stringValue = _lastChar;
		while (isdigit((_lastChar = _input.get()))) 
			_stringValue += _lastChar;
		if (_lastChar == '.')
		{
			_isFloat = true;
			_stringValue += _lastChar;
			while (isdigit((_lastChar = _input.get())))
				_stringValue += _lastChar;
		}
		return _CONST;
	}

  // Если перевод строки, возвращаем специальный маркер.
	// Несколько символов перевода строки считаем одним.
	if (isnewline(_lastChar)) 
	{
		while (isnewline((_lastChar = _input.get())));
		return _NEWLINE;
	}

	// Проверим, не дошли ли мы до конца файла.
	if (_input.eof())
		return _EOF;

	// Возвращаем сам символ, переходя при этом к следующему.
	int ThisChar = _lastChar;
	_lastChar = _input.get();
	return ThisChar;
}



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
