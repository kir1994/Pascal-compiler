#pragma once

#include <vector>
#include <string>
#include <map>

class Block;
class Expression;
class Function;
class Const;
class Node;

class Scope
{
	Scope *pParScope;
	std::map <std::string, Node *> scope;

public:
	Scope(Scope *parScope) : pParScope(parScope) {}
	Scope() : pParScope(nullptr) {}

	void SetParScope(Scope *parScope)
	{
		pParScope = parScope;
	}
	bool Add(std::string var, Node* val)
	{
		if(scope.find(var) == scope.end()) {
			scope[var]=val;
			return true;
		}
		else
			return false;
	};
	Node * Get(std::string var)
	{
		auto res =scope.find(var);
		if (res != scope.end())
			return res->second;
		else
		{
			if(pParScope == nullptr)
				return nullptr;
			else
				return pParScope->Get(var);
		}
	}
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

//class Type
//{
//public:
//	enum TYPE{T_SIMPLE};
//
//	TYPE _type;
//
//	Type(TYPE t) : _type(t) {}
//};
//class SimpleType : public Type
//{
//public:
//	enum TYPE{T_CHAR, T_BOOLEAN, T_INTEGER, T_REAL};
//	TYPE _type;
//	SimpleType(TYPE t) : Type(T_SIMPLE), _type(t) {}
//};

class Var : public Node
{
public:
	enum TYPE { CHAR, REAL, INTEGER, BOOLEAN, VOID };
private:
	//std::string _name;
	TYPE _type;
	/*std::string _realVal;

	Expression *_val;*/

	bool isSet;

public:
	Var(/*std::string name, */TYPE type) : /*_name(name), */isSet(false), _type(type) {	}
	Var() :isSet(false), _type(VOID) {}
	/*void Assign(Expression *val)
	{
		_val = val;
		isSet = true;
	}
	void Assign(const std::string& val)
	{
		_realVal = val;
		isSet = true;
	}*/
};
class SimpleInteger : public Var
{
public:
	int _val;
	SimpleInteger(const int& i = 0) : Var(INTEGER), _val(i) {}
};
class SimpleReal : public Var
{
public:
	double _val;
	SimpleReal(const double& i = 0) : Var(REAL), _val(i) {}
};
class SimpleBoolean : public Var
{
public:
	bool _val;
	SimpleBoolean(const bool& i = false) : Var(BOOLEAN), _val(i) {}
};
class SimpleChar : public Var
{
public:
	char _val;
	SimpleChar(const char& i = 0) : Var(CHAR), _val(i) {}
};


class ParamType
{
public:
	enum TYPE {VAR, VAR_REF, FUNC};
	TYPE _type;

	Var::TYPE _rvtype;

	ParamType(const TYPE& t) : _type(t) {}
	ParamType(const TYPE& t, const Var::TYPE& rtype) : _type(t), _rvtype(rtype) {}
};

class Const : public Node {
public:
	enum TYPE { STRING, ID, UREAL, UINT };
private:
	bool isNeg;
	std::string _val;
	TYPE _type;
public:
	Const() : isNeg(false), _val("") {}
	Const(std::string val) : isNeg(false), _val(val), _type(STRING){}
	Const(TYPE t, std::string val = "") : isNeg(false), _val(val), _type(t) {}
	void SetNeg(bool neg) { isNeg = neg; }
	void SetVal(std::string val) { _val = val; }
	void SetType(TYPE t) { _type = t; }
};

class ConstInteger : public Const
{
public:
	int _val;
	ConstInteger(int i) : Const(UINT), _val(i) {}
};
class ConstReal : public Const
{
public:
	double _val;
	ConstReal(double i) : Const(UREAL), _val(i) {}
};


class ParamList : public Node {
public:
	typedef std::pair<std::string, ParamType *> func_param;
	std::vector<func_param> _params;
};

class Expression
{
public:
	enum TYPE{E_BINARY, E_COND, E_CONST, E_ID, E_FUNCCALL};
	bool isNeg;
	TYPE _type;

	Expression(TYPE type) :_type(type), isNeg(false) {}

	void Negate() { isNeg = true; }
};
class FuncCallExpr : public Expression {
public:
	std::string _name;
	std::vector<Expression *> _params;

	FuncCallExpr(const std::string& s, const std::vector<Expression *>& par) : Expression(E_FUNCCALL), _name(s), _params(par) {}
};
class BinaryOp : public Expression {
public:
	enum OP { MUL, DIV, INT_DIV, MOD, AND, ADD, SUB, OR};

	Expression *_left;
	OP _op;
	Expression *_right;

	BinaryOp(Expression *l, Expression *r, OP o) : Expression(E_BINARY), _left(l), _right(r), _op(o) {}
};
class ExprID : public Expression {
public:

	std::string id;

	ExprID(const std::string& name) : Expression(E_ID), id(name) {}
};
class ExprConst : public Expression {
public:
	enum TYPE{STRING, NUMBER, NIL};
	//std::string _val;
	Const *_val;
	TYPE _type;

	ExprConst(TYPE t, Const *val = nullptr) : Expression(E_CONST), _val(val), _type(t) {}
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
	enum TYPE{S_SEQ, S_IF, S_FOR, S_WHILE, S_ASSIGN, S_PROCCALL, S_REPEAT};

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
class RepeatStatement : public Statement {
	Expression *_condition;

	StatementSeq *_st;

public:
	RepeatStatement(Expression *cond, StatementSeq *st) : Statement(S_REPEAT), _condition(cond), _st(st) {}
	virtual ~RepeatStatement()
	{
		delete _condition;
		delete _st;
	}
};
class AssignStatement : public Statement {
	std::string _var;
	Expression *_expr;

public:
	AssignStatement(const std::string& var, Expression * expr) : Statement(S_ASSIGN), _var(var), _expr(expr) {}
};
class ProcCallStatement : public Statement {
	std::string _id;
	std::vector<Expression *> _params;

public:
	ProcCallStatement(const std::string& var, const std::vector<Expression *> par) : Statement(S_PROCCALL), _id(var), _params(par) {}
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

class Function : public Node {
public:
	std::string _ID;
	ParamList *_params;
	Block *_blk;

	Var::TYPE _rtype;

	bool isProc;

	Function(const std::string& id, ParamList *par, Block *blk, Var::TYPE rtype) : _ID(id), _params(par), _blk(blk), isProc(false), _rtype(rtype) {}
	Function(const std::string& id, ParamList *par, Block *blk) : _ID(id), _params(par), _blk(blk), isProc(true) {}

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
