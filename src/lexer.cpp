#include "parser.h"

static int isNewline(int c) { return (c == '\n' || c == '\r'); }
static int isSpace(int c) { return (c == ' ' || c == '\t'); }

Token Lexer::GetToken()
{
	//Пропускаем ненужные гадости
	while (isNewline(_lastChar) || isSpace(_lastChar))
	{
		if (isNewline(_lastChar))
			_curLine++;
		_lastChar = _input.get();
	}
	// Проверка на наличие ключевого слова или идентификатора
	if (isalpha(_lastChar)) {
		_stringValue = _lastChar;
		while (isalnum((_lastChar = _input.get())))
			_stringValue += _lastChar;
		if (TYPE_NAMES.find(_stringValue) != TYPE_NAMES.end())
			return T_VARTYPE;
		else if (COND_NAMES.find(_stringValue) != COND_NAMES.end())
			return T_COND;
		else if (FACTOR_NAMES.find(_stringValue) != FACTOR_NAMES.end())
			return T_FACTOROP;
		else if (TERM_NAMES.find(_stringValue) != TERM_NAMES.end())
			return T_TERMOP;
		else if (NAMES.find(_stringValue) == NAMES.end())
			return T_ID;
		else if (_stringValue == "if")
			return T_IF;
		else if (_stringValue == "else")
			return T_ELSE;
		else if (_stringValue == "then")
			return T_THEN;
		else if (_stringValue == "begin")
			return T_BEGIN;
		else if (_stringValue == "end")
			return T_END;
		else if (_stringValue == "nil")
			return T_NIL;
		else if (_stringValue == "const")
			return T_CONST;
		else if (_stringValue == "function")
			return T_FUNC;
		else if (_stringValue == "procedure")
			return T_PROC;
		else if (_stringValue == "var")
			return T_VAR;
		else if (_stringValue == "program")
			return T_PROGRAM;
		else if (_stringValue == "label")
			return T_LABEL;
		else if (_stringValue == "for")
			return T_FOR;
		else if (_stringValue == "to")
			return T_TO;
		else if (_stringValue == "downto")
			return T_DOWNTO;
		else if (_stringValue == "do")
			return T_DO;
		else if (_stringValue == "while")
			return T_WHILE;
		else if (_stringValue == "while")
			return T_WHILE;
		else if (_stringValue == "repeat")
			return T_REPEAT;
		else if (_stringValue == "until")
			return T_UNTIL;
		else if (_stringValue == "true")
			return T_TRUE;
		else if (_stringValue == "false")
			return T_FALSE;
	}
	//Если строка
	else if (_lastChar == '\'')
	{
		_stringValue = "";
		_lastChar = _input.get();
		while (_lastChar != '\'' && !_input.eof())
		{
			if (_lastChar == '\n')
				_curLine++;
			_stringValue += _lastChar;
			_lastChar = _input.get();
		}
		if (_lastChar != '\'')
			throw std::exception();
		_lastChar = _input.get();
		return T_STRING;
	}
	//Если число
	else if (isdigit(_lastChar)) {
		_stringValue = _lastChar;
		while (isdigit((_lastChar = _input.get())))
			_stringValue += _lastChar;
		if (_lastChar == '.')
		{
			_stringValue += _lastChar;
			while (isdigit((_lastChar = _input.get())))
				_stringValue += _lastChar;
		}
		if (_lastChar == 'E')
		{
			_stringValue += _lastChar;
			_lastChar = _input.get();
			if (_lastChar == '+' || _lastChar == '-')
			{
				_stringValue += _lastChar;
				_lastChar = _input.get();
			}
			if (isdigit(_lastChar))
			{
				do
				{
					_stringValue += _lastChar;
					_lastChar = _input.get();

				} while (isdigit(_lastChar));
			}
			else
				throw std::exception();
		}

		return T_UNUMBER;
	}
	else if (_lastChar == '(')
	{
		_stringValue = _lastChar;
		_lastChar = _input.get();
		return T_LBR;
	}
	else if (_lastChar == ')')
	{
		_stringValue = _lastChar;
		_lastChar = _input.get();
		return T_RBR;
	}
	else if (_lastChar == ';')
	{
		_stringValue = _lastChar;
		_lastChar = _input.get();
		return T_SEMICOLON;
	}
	else if (_lastChar == ':')
	{
		_stringValue = _lastChar;
		_lastChar = _input.get();
		if (_lastChar == '=')
		{
			_stringValue += _lastChar;
			_lastChar = _input.get();
			return T_ASSIGN;
		}
		else
			return T_COLON;
	}
	else if (_lastChar == ',')
	{
		_stringValue = _lastChar;
		_lastChar = _input.get();
		return T_COMMA;
	}
	else if (_lastChar == '~')
	{
		_stringValue = _lastChar;
		_lastChar = _input.get();
		return T_NEG;
	}
	else if (TERM_NAMES.find((_stringValue = _lastChar)) != TERM_NAMES.end())
	{
		_stringValue = _lastChar;
		_lastChar = _input.get();
		return T_TERMOP;
	}
	else if (FACTOR_NAMES.find((_stringValue = _lastChar)) != FACTOR_NAMES.end())
	{
		_stringValue = _lastChar;
		_lastChar = _input.get();
		return T_FACTOROP;
	}
	else if (COND_NAMES.find((_stringValue = _lastChar)) != COND_NAMES.end())
	{
		_stringValue = _lastChar;
		if ((_lastChar == '>' || _lastChar == '<') && ((_lastChar = _input.get()) == '='))
			_stringValue += _lastChar;
		_lastChar = _input.get();
		return T_COND;
	}
	// Проверим, не дошли ли мы до конца файла.
	if (_input.eof())
		return T_EOF;

	throw std::exception();
}