#pragma once

#include <vector>
#include <string>
#include <set>
#include <map>

enum VAR_TYPES {CHAR, REAL, INTEGER, BOOLEAN};

class Block;
class Expression;
class Function;
class Procedure;

class ParamType
{
	enum TYPE{ SIMPLE, FUNC, PROC };
	ParamType::TYPE _type;
	VAR_TYPES _simple;
	VAR_TYPES _return_type;	
public:
	ParamType(VAR_TYPES type) : _type(SIMPLE), _simple(type) {}
	ParamType() : _type(PROC) {}
	ParamType(VAR_TYPES type, VAR_TYPES ret_type) : _type(FUNC), _simple(type), _return_type(ret_type) {}
};

class Var
{
	std::string _name;
	VAR_TYPES _type;
	std::string _val;

	bool isSet;

public:
	Var(std::string name, VAR_TYPES type) : _name(name), isSet(false), _type(type) {	}

};

typedef std::map<std::string, Var *> cur_scope;

typedef std::pair<void *, cur_scope> scope;

typedef std::pair<std::string, ParamType *> func_param;

class Node
{	
public:
	//virtual void Parse(void *pParser);
	//virtual void GenerateCode();
};

class Label : public Node
{
	unsigned _val;
public:
	Label(unsigned val) : _val(val) {}
};

class Const : public Node {
	bool isNeg;
	std::string _val;
	std::string _ID;
	enum TYPE { STRING, ID, NUMBER};
	TYPE _type;
public:
	virtual void Parse(void *pParser);
};

class Type : public Node {
	VAR_TYPES _type;
};

class ParamList : public Node {

	std::vector<func_param> _params;
public:
	virtual void Parse(void *pParser);
};

class Factor : public Node {
	
	bool isNeg;
};

enum TERM_OP { MUL, DIV, INT_DIV, MOD, AND };
class Term : public Node {
	Factor * _left;

	std::vector<std::pair<TERM_OP, Factor *>> _operands;
public:
	virtual void Parse(void *pParser);
	virtual ~Term()
	{
		delete _left;

		for (auto& i : _operands)
			delete i.second;
	}
};

enum SEXPR_OP {PLUS, MINUS, OR};
class SimpleExpression : public Node {
	bool isNeg;
	Term _left;

	std::vector <std::pair<SEXPR_OP, Term>> _operands;
public:
	virtual void Parse(void *pParser);
};

enum EXPR_OP {EQ, NEQ, LT, LE, GT, GE, IN};
class Expression : public Node {
	SimpleExpression _left;

	EXPR_OP _op;
	SimpleExpression *_right;
public:
	virtual void Parse(void *pParser);
	Expression() : _right(nullptr) {}
	virtual ~Expression()
	{
		delete _right;
	}
};

class FactorFunc : public Factor {
	std::string _funcName;
	std::vector<Expression *> _params;
public:
	virtual ~FactorFunc()
	{
		for (auto& i : _params)
			delete i;
	}
};
class FactorExpr : public Factor {
	Expression *_expr;
public:
	virtual ~FactorExpr()
	{
		delete _expr;
	}
};
class FactorVar : public Factor {
	std::string _varName;
};
class FactorConst : public Factor {
	Const *_const;
public:
	virtual ~FactorConst()
	{
		delete _const;
	}
};
class Statement : public Node {

};
class IfStatement : public Statement {

	Expression *_condition;

	Statement *_then;
	Statement *_else;

public:
	virtual ~IfStatement()
	{
		delete _condition;
		delete _then;
		delete _else;
	}
};
class WhileStatement : public Statement {
	Expression *_condition;

	Statement *_st;

public:
	virtual ~WhileStatement()
	{
		delete _condition;
		delete _st;
	}
};
enum CNT_TYPES{TO, DOWNTO};
class ForStatement : public Statement {

	std::string _init_var;
	Expression *_init_var_expression;
	Expression *_end_var_expression;

	Statement *_st;
	CNT_TYPES _type;

	VAR_TYPES _left;
	VAR_TYPES _right;
public:
	virtual ~ForStatement()
	{
		delete _init_var_expression;
		delete _end_var_expression;
		delete _st;
	}
};

class Block : public Node
{
	std::vector<unsigned> labels;
	std::vector<Const> constants;
	//std::vector<Type> types; Пока не делаем-с
	std::vector<Function> functions;
	std::vector<Procedure> procedures;

	std::vector<Statement *> statements;

	scope _scp;

public:
	virtual void Parse(void *pParser);
	virtual ~Block()
	{
		for (auto& i : statements)
			delete i;
	}

	Block(scope *cur_scp) {
		_scp.first = cur_scp;
	}
};

class Procedure : public Node {
	std::string _name;
	ParamList _params;
	scope _scp;
	Block *_blk;
public:
	virtual void Parse(void *pParser);
	Procedure(scope *par_scope) : _blk(nullptr)
	{
		_scp.first = par_scope;
	}
	virtual ~Procedure() {
		delete _blk;
	}
};
class Function : public Node {
	std::string _name;
	ParamList _params;

	scope _scp;
	Block *_blk;
	VAR_TYPES _return_type;
public:
	virtual void Parse(void *pParser);
	Function(scope *par_scope) : _blk(nullptr)
	{
		_scp.first = par_scope;
	}
	virtual ~Function() {
		delete _blk;
	}
};


class Root : public Node
{
	Block *_blk;
public:
	virtual void Parse(void *pParser);
	virtual ~Root()
	{
		delete _blk;
	}
};
