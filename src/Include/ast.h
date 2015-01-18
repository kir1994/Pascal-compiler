#pragma once

#include <vector>
#include <string>
#include <map>

class Expression;
class Function;
class Const;
class Node;
class ScopableNode;

class Scope
{
public:
	std::string name;
	Scope *pParScope;

	std::map <std::string, ScopableNode *> scope;

	Scope(const std::string &name) : name(name), pParScope(nullptr) {}

	std::string GetScopeName() {
		std::string res = "";

		Scope *pCur = this;

		while (pCur != nullptr) {
			res = name + "::" + res;
			pCur = pParScope;
		}

		return res;
	}

	void SetParScope(Scope *parScope)
	{
		pParScope = parScope;
	}

	bool IsRoot() {
		return pParScope == nullptr;
	}

	bool Add(std::string var, ScopableNode* val)
	{
		if(scope.find(var) == scope.end()) {
			scope[var]=val;
			return true;
		}
		else
			return false;
	}

	template<class T>
	T * Get(std::string var)
	{
		T *pRes = nullptr;
		
		auto res = scope.find(var);

		if (res == scope.end() || (pRes = dynamic_cast<T *>(res->second)) == nullptr) {
			if (pParScope != nullptr)
				pRes = pParScope->Get<T>(var);
			else return nullptr;
		}

		return pRes;
	}
};

class Node
{
public:
	virtual ~Node() {}
};

class ScopableNode : public Node {
public:
	enum TYPE {VAR, FUNC};

	TYPE _type;

	ScopableNode(TYPE type) : _type(type) {}

	virtual ~ScopableNode() {}
};

class Var : public ScopableNode
{
public:
	enum TYPE { CHAR, REAL, INTEGER, BOOLEAN, VOID };

	TYPE _type;
	bool isConst, isRef;

	Var(TYPE type, bool isConst = false, bool isRef = false) : ScopableNode(ScopableNode::VAR), _type(type), isConst(isConst), isRef(isRef) {}

	Var() : Var(VOID) {}
};

class Const : public Var {
public:
	bool isNeg, isSet;

	Const(Var::TYPE t) : Var(t, true), isNeg(false) {}

	void SetNeg(bool neg) { isNeg = neg; }

	virtual ~Const() {}
};

class ConstInteger : public Const
{
public:
	unsigned long long _val;
	ConstInteger(int i) : Const(Var::INTEGER), _val(i) {}
};

class ConstBoolean : public Const
{
public:
	bool _val;
	ConstBoolean(bool i) : Const(Var::BOOLEAN), _val(i) {}
};

class ConstReal : public Const
{
public:
	double _val;
	ConstReal(double i) : Const(Var::REAL), _val(i) {}
};

class ParamList {
public:
	typedef std::pair<std::string, Var *> func_param;
	std::vector<func_param> _params;

	virtual ~ParamList() {
		for (auto &i : _params)
			delete i.second;
	}
};

class Expression
{
public:
	enum TYPE{E_BINARY, E_COND, E_CONST, E_ID, E_FUNCCALL};
	bool isNeg;
	TYPE _type;
	Var *_pVar;

	Expression(TYPE type) :_type(type), isNeg(false), _pVar(nullptr) {}

	void Negate() { isNeg = true; }

	const Var * GetVar(Scope *scp) {
		if (_pVar == nullptr)
			CalculateVar(scp);

		return _pVar;
	}

	virtual void CalculateVar(Scope *scp) = 0;

	virtual ~Expression() {
		delete _pVar;
	}
};

class FuncCallExpr : public Expression {
public:
	std::string _name;
	std::vector<Expression *> _params;

	FuncCallExpr(const std::string& s, const std::vector<Expression *>& par) : Expression(E_FUNCCALL), _name(s), _params(par) {}

	virtual ~FuncCallExpr() {
		for (auto &i : _params)
			delete i;
	}
	void CalculateVar(Scope *scp) final {
		throw std::exception("not supported yet");
	}
};

class BinaryOp : public Expression {
public:
	enum OP { MUL, DIV, INT_DIV, MOD, AND, ADD, SUB, OR};

	Expression *_left;
	OP _op;
	Expression *_right;

	BinaryOp(Expression *l, Expression *r, OP o) : Expression(E_BINARY), _left(l), _right(r), _op(o) {}

	void CalculateVar(Scope *scp) final {
		auto LeftV = _left->GetVar(scp);
		auto RightV = _right->GetVar(scp);

		if (LeftV->_type != RightV->_type)
			throw std::exception("invalid type");

		if (LeftV->_type >= Var::VOID)
			throw std::exception("invalid type");

		if ((_op == INT_DIV || _op == MOD || _op == AND || _op == OR) &&
			LeftV->_type != Var::INTEGER)
			throw std::exception("invalid type");

		if (_op == DIV)
			_pVar = new Var(Var::REAL);
		else
			_pVar = new Var(LeftV->_type);
	}

	virtual ~BinaryOp() {
		delete _left;
		delete _right;
	}
};
class ExprID : public Expression {
public:

	std::string id;

	ExprID(const std::string& name) : Expression(E_ID), id(name) {}
	void CalculateVar(Scope *scp) final {
		auto inc = scp->Get<Var>(id);

		if (inc == nullptr)
			throw std::exception();

		_pVar = new Var(*inc);
	}
};

class ExprConst : public Expression {
public:
	Const *_val;

	ExprConst(Const *val = nullptr) : Expression(E_CONST), _val(val) {}

	void CalculateVar(Scope *scp) final {
		_pVar = new Var(*_val);
	}

	virtual ~ExprConst() {
		delete _val;
	}
};

class Condition : public Expression {
public:
	enum OP { EQ, NEQ, LT, LE, GT, GE, IN };
	Expression *_left;

	OP _op;
	Expression *_right;

	Condition(Expression *l, Expression *r, OP o) : Expression(E_COND), _left(l), _right(r), _op(o) {}

	void CalculateVar(Scope *scp) final {
		auto LeftV = _left->GetVar(scp);
		auto RightV = _right->GetVar(scp);

		if (_op == IN) {
			throw std::exception("not supported yet");
		}

		if (LeftV->_type != RightV->_type)
			throw std::exception("invalid type");

		if (LeftV->_type >= Var::VOID)
			throw std::exception("invalid type");

		_pVar = new Var(Var::BOOLEAN);
	}

	virtual ~Condition() {
		delete _left;
		delete _right;
	}
};

class Statement : public Node {
public:
	enum TYPE{S_SEQ, S_IF, S_FOR, S_WHILE, S_ASSIGN, S_PROCCALL, S_REPEAT};

	TYPE _type;
	Statement(TYPE type) : _type(type) {}

	virtual ~Statement() {}
};

class IfStatement : public Statement {
public:
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
public:
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
public:
	std::vector<Statement *> statements;

	StatementSeq() : Statement(S_SEQ) {}

	void AddStatement(Statement *st)
	{
		statements.push_back(st);
	}

	virtual ~StatementSeq() {
		for (auto &i : statements)
			delete i;
	}
};
class RepeatStatement : public Statement {
public:
	Expression *_condition;

	StatementSeq *_st;

	RepeatStatement(Expression *cond, StatementSeq *st) : Statement(S_REPEAT), _condition(cond), _st(st) {}

	virtual ~RepeatStatement()
	{
		delete _condition;
		delete _st;
	}
};
class AssignStatement : public Statement {
public:
	std::string _var;
	Expression *_expr;

	AssignStatement(const std::string& var, Expression * expr) : Statement(S_ASSIGN), _var(var), _expr(expr) {}

	virtual ~AssignStatement() {
		delete _expr;
	}
};
class ProcCallStatement : public Statement {
public:
	std::string _id;
	std::vector<Expression *> _params;

	ProcCallStatement(const std::string& var, const std::vector<Expression *> par) : Statement(S_PROCCALL), _id(var), _params(par) {}

	virtual ~ProcCallStatement() {
		for (auto &i : _params)
			delete i;
	}
};

class ForStatement : public Statement {

public:
	enum TYPE{ TO, DOWNTO };

	std::string _var;
	Expression *_from;
	Expression *_to;

	Statement *_do;
	TYPE _type;

	ForStatement(const std::string& var, Expression * from, Expression * to, Statement *d, TYPE type) :
		Statement(S_FOR), _var(var), _from(from), _to(to), _do(d), _type(type) {}

	virtual ~ForStatement()
	{
		delete _from;
		delete _to;
		delete _do;
	}
};

class Function : public ScopableNode {
public:
	std::string _ID; // Имя функции
	ParamList *_params; // Список параметров

	Var *_prtype; // Возращаемое значение (VOID для процедуры)

	Scope scp; // Текущая область видимости

	std::vector<std::pair<std::string, Var *>> Vars; // Объявления переменных и констант
	std::vector<std::pair<std::string, Function *>> Funcs; // Объявления вложенных функций

	StatementSeq *seq; // Тело функции

	bool add(const std::string &name, ScopableNode *pNode) {
		if (!scp.Add(name, pNode))
			return false;

		switch (pNode->_type) {
		case ScopableNode::VAR:
			Vars.push_back({ name, dynamic_cast<Var *>(pNode) });
			break;
		case ScopableNode::FUNC:
			Funcs.push_back({ name, dynamic_cast<Function *>(pNode) });
			break;
		}
		return true;
	}

	Function(const std::string& id, ParamList *par, Var::TYPE rtype = Var::VOID) : ScopableNode(ScopableNode::FUNC),  scp(_ID), _ID(id), _params(par) {
		_prtype = new Var(rtype);
	}

	virtual ~Function() {
		for (auto &i : Vars)
			delete i.second;
		for (auto &i : Funcs)
			delete i.second;

		delete _prtype;
		delete seq;
	}

	unsigned GetNumOfParams() const {
		if (_params != nullptr)
			return _params->_params.size();

		return 0;
	}


	std::string GetID()
	{
		return _ID;
	}
};

class Root : public Function
{
public:

	Root() : Function("", nullptr, Var::INTEGER) {
	}

	virtual ~Root()
	{
	}
};
