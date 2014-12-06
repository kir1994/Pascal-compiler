#pragma once

#include <vector>
#include <string>
#include <map>

class Block;
class Expression;
class Function;
class Procedure;
class Const;

class Var
{
public:
	enum TYPE { CHAR, REAL, INTEGER, BOOLEAN };
private:
	std::string _name;
	TYPE _type;
	std::string _realVal;

	Expression *_val;

	bool isSet;

public:
	Var(std::string name, TYPE type) : _name(name), isSet(false), _type(type) {	}

	void Assign(Expression *val)
	{
		_val = val;
		isSet = true;
	}
};


class ParamType
{
	enum TYPE{ SIMPLE, FUNC, PROC };
	ParamType::TYPE _type;
	Var::TYPE _simple;
	Var::TYPE _return_type;
public:
	ParamType(Var::TYPE type) : _type(SIMPLE), _simple(type) {}
	ParamType() : _type(PROC) {}
	ParamType(Var::TYPE type, Var::TYPE ret_type) : _type(FUNC), _simple(type), _return_type(ret_type) {}
};

class Scope
{
	Scope *pParScope;

	std::map<std::string, Var *> VarScope;
	std::map<std::string, Const *> ConstScope;
	std::map<std::string, Function *> FunctionScope;
	std::map<std::string, Procedure *> ProcedureScope;

public:
	Scope(Scope *parScope) : pParScope(parScope) {}
	Scope() : pParScope(nullptr) {}

	void SetParScope(Scope *parScope)
	{
		pParScope = parScope;
	}

#define DeclareAddGet(x) bool Add##x##(std::string var,##x##*val)\
	{\
	if(##x##Scope.find(var) ==##x##Scope.end()){\
	x##Scope[var]=val;\
	return true;\
	}\
	else\
	return false;\
	};\
	x##*##Get##x##(std::string var)\
	{\
	auto res = ##x##Scope.find(var);\
	if (res != ##x##Scope.end())\
	return res->second;\
	else\
	{\
	if(pParScope == nullptr)\
	return nullptr;\
	else\
	return pParScope->Get##x##(var);\
	}\
	}
	DeclareAddGet(Var);
	DeclareAddGet(Const);
	DeclareAddGet(Function);
	DeclareAddGet(Procedure);
	
#define Add(x) Add##x
#define Get(x) Get##x

//#define DeclareGet(x) x##*##Get##x##(std::string var)\
//	{\
//	auto res = ##x##Scope.find(var);\
//	if (res != ##x##Scope.end())\
//	return res->second;\
//	else\
//	{\
//	if(pParScope == nullptr)\
//	return nullptr;\
//	else\
//	return pParScope->Get##x##(var);\
//	}\
//	}
//	DeclareAdd(Var);
//	DeclareAdd(Const);
//	DeclareAdd(Function);
//	DeclareAdd(Procedure);
//	DeclareGet(Var);
//	DeclareGet(Const);
//	DeclareGet(Function);
//	DeclareGet(Procedure);
};

class Node
{	
	Scope *_scp;

public:
	Node(Scope *scp = nullptr) : _scp(scp) {}
	void SetScope(Scope *scp)
	{
		_scp = scp;
	}
	void SetParentScope(Scope *scp)
	{
		if (_scp != nullptr)
			_scp->SetParScope(scp);
		else
			throw std::exception();
	}
	Scope *getScope() { return _scp; }
};

class Const : public Node {
public:
	enum TYPE { STRING, ID, NUMBER };
private:
	bool isNeg;
	std::string _val;
	std::string _ID;
	TYPE _type;
public:
	Const() : isNeg(false), _val(""), _ID("") {}
	void SetNeg(bool neg) { isNeg = neg; }
	void SetVal(std::string val) { _val = val; }
	void SetID(std::string ID) { _ID = ID; }
	void SetType(TYPE t) { _type = t; }
	std::string GetID() { return _ID; }
};

class ParamList : public Node {
public:
	typedef std::pair<std::string, ParamType *> func_param;
	std::vector<func_param> _params;
};

class Expression
{
public:
	enum TYPE{E_BINARY, E_COND, E_CONST, E_VAR};
	bool isNeg;
	TYPE _type;

	Expression(TYPE type) :_type(type), isNeg(false) {}

	void Negate() { isNeg = true; }
};

class BinaryOp : public Expression {
public:
	enum OP { MUL, DIV, INT_DIV, MOD, AND, ADD, SUB, OR};

	Expression *_left;
	OP _op;
	Expression *_right;

	BinaryOp(Expression *l, Expression *r, OP o) : Expression(E_BINARY), _left(l), _right(r), _op(o) {}
};
class ExprVar : public Expression {
public:

	std::string varName;

	ExprVar(const std::string& name) : Expression(E_VAR), varName(name) {}
};
class ExprConst : public Expression {
public:
	enum TYPE{STRING, NUMBER, NIL};
	std::string _val;
	TYPE _type;

	ExprConst(TYPE t, const std::string& val = "") : Expression(E_CONST), _val(val), _type(t) {}
};
class Condition : public Expression {
public:
	enum OP { EQ, NEQ, LT, LE, GT, GE, IN };
	Expression *_left;

	OP _op;
	Expression *_right;

	Condition(Expression *l, Expression *r, OP o) : Expression(E_COND), _left(l), _right(r), _op(o) {}
};

class Statement : public Node {
public:
	enum TYPE{S_SEQ, S_IF, S_FOR, S_WHILE, S_ASSIGN, S_PROCCALL};

	TYPE _type;
	Statement(TYPE type) : _type(type) {}
};
class IfStatement : public Statement {

	Expression *_cond;

	Statement *_then;
	Statement *_else;

public:
	IfStatement(Expression *expr, Statement *th, Statement *el) : Statement(S_IF), _cond(expr), _then(th), _else(el) {}
	virtual ~IfStatement()
	{
		delete _cond;
		delete _then;
		delete _else;
	}
};
class WhileStatement : public Statement {
	Expression *_condition;

	Statement *_st;

public:
	WhileStatement(Expression *cond, Statement *st) : Statement(S_WHILE), _condition(cond), _st(st) {}
	virtual ~WhileStatement()
	{
		delete _condition;
		delete _st;
	}
};
class AssignStatement : public Statement {
	std::string _var;
	Expression *_expr;

public:
	AssignStatement(std::string var, Expression * expr) : Statement(S_ASSIGN), _var(var), _expr(expr) {}
};
class StatementSeq : public Statement
{
	std::vector<Statement *> statements;
public:
	StatementSeq() : Statement(S_SEQ) {}
	void AddStatement(Statement *st)
	{
		statements.push_back(st);
	}
};
class ForStatement : public Statement {

public:
	enum TYPE{ TO, DOWNTO };
private:
	std::string _var;
	Expression *_from;
	Expression *_to;

	Statement *_do;
	TYPE _type;

public:
	ForStatement(const std::string& var, Expression * from, Expression * to, Statement *d, TYPE type) :
		Statement(S_FOR), _var(var), _from(from), _to(to), _do(d), _type(type) {}
	virtual ~ForStatement()
	{
		delete _from;
		delete _to;
		delete _do;
	}
};

class Block : public Node
{
public:
	StatementSeq * seq;
};

class Procedure : public Node {
public:
	std::string _ID;
	ParamList *_params;
	Block *_blk;

	std::string GetID()
	{
		return _ID;
	}
};
class Function : public Node {
public:
	std::string _ID;
	ParamList *_params;

	Block *_blk;
	Var::TYPE _return_type;

	std::string GetID()
	{
		return _ID;
	}
};


class Root : public Node
{
public:
	std::string progName;
	Block *_blk;
	virtual ~Root()
	{
		delete _blk;
	}
};
