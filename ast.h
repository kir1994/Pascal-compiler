#pragma once

#include <vector>
#include <string>
#include <set>
#include <map>

enum VAR_TYPES {CHAR, REAL, INTEGER};

class Block;
class Expression;

class Var
{
	std::string _name;
	VAR_TYPES _type;

	std::string _val;
};

typedef std::map<std::string, Var> cur_scope;
typedef std::pair<cur_scope *, cur_scope> scope;
typedef std::pair<std::string, VAR_TYPES> func_param;

class Node
{
public:
	virtual void Fill();
	virtual void GenerateCode();
};

class Label : public Node
{
	unsigned _val;
public:
	Label(unsigned val) : _val(val) {}
};

class Const : public Node {
	std::string _val;
	enum TYPE { STRING, INTEGER, REAL };
	TYPE _type;
public:
	Const(std::string val, TYPE type) : _val(val), _type(type) {}
};

class Type : public Node {
	VAR_TYPES _type;
};

class Procedure : public Node {
	std::string _name;
	std::vector<func_param> _params;

	Block *_blk;
};
class Function : public Procedure {
	VAR_TYPES _return_type;
};

class Factor : public Node {
	
	bool isNeg;
};
class FactorFunc : public Factor {
	std::string _funcName;
	std::vector<Expression *> _params;
};
class FactorExpr : public Factor {
	Expression *_expr;
};
class FactorVar : public Factor {
	std::string _varName;
};
class FactorConst : public Factor {
	Const *_const;
};

enum TERM_OP { MUL, DIV, INT_DIV, MOD, AND };
class Term : public Node {
	Factor * _left;

	std::vector<std::pair<TERM_OP, Factor *>> _operands;
};

enum SEXPR_OP {PLUS, MINUS, OR};
class SimpleExpession : public Node {
	char sign;
	Term * _left;

	std::vector <std::pair<SEXPR_OP, Term *>> _operands;
};

enum EXPR_OP {EQ, NEQ, LT, LE, GT, GE, IN};
class Expression : public Node {
	SimpleExpession *_left;

	EXPR_OP _op;
	SimpleExpession *_right;
};

class Statement : public Node {

};
class IfStatement : public Statement {

	Expression *_condition;

	Statement *_then;
	Statement *_else;
};
class WhileStatement : public Statement {
	Expression *_condition;

	Statement *_st;
};
enum CNT_TYPES{TO, DOWNTO};
class ForStatement : public WhileStatement {

	std::string _init_var;
	Expression *_init_var_expression;

	CNT_TYPES _type;

	VAR_TYPES _left;
	VAR_TYPES _right;
};

class Block : public Node
{
	std::vector<Label> labels;
	std::vector<Const> constants;
	std::vector<Type> types;
	std::vector<Function> _func;
	std::vector<Procedure> _proc;

	std::vector<Statement> st;

	scope _scp;

public:

	Block(cur_scope *cur_scp) {
		_scp.first = cur_scp;
	}
	virtual void Fill();
};


class Root : public Node
{
	Block *_blk;
public:
	virtual void Fill();
};
