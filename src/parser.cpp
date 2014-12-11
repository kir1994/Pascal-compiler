#include "parser.h"

using namespace std;

Var::TYPE Str2Type(const std::string& type)
{
	if (type == "real")
		return Var::REAL;
	else if (type == "integer")
		return Var::INTEGER;
	else if (type == "char")
		return Var::CHAR;
	else if (type == "Boolean")
		return Var::BOOLEAN;
	else
		throw exception();
}
Condition::OP Str2Cond(const std::string& cond)
{
	if (cond == "=")
		return Condition::EQ;
	else if (cond == "<>")
		return Condition::NEQ;
	else if (cond == ">")
		return Condition::GT;
	else if (cond == "<")
		return Condition::LT;
	else if (cond == "<=")
		return Condition::LE;
	else if (cond == ">=")
		return Condition::GE;
	else if (cond == "IN")
		return Condition::IN; 
	else
		throw exception();

}
BinaryOp::OP Str2Arithm(const std::string& op)
{
	if (op == "*")
		return BinaryOp::MUL;
	else if (op == "/")
		return BinaryOp::DIV;
	else if (op == "+")
		return BinaryOp::ADD;
	else if (op == "-")
		return BinaryOp::SUB;
	else if (op == "^" || op == "and")
		return BinaryOp::AND;
	else if (op == "div")
		return BinaryOp::INT_DIV;
	else if (op == "mod")
		return BinaryOp::MOD;
	else if (op == "or")
		return BinaryOp::OR;
	else
		throw exception();
}

void Parser::Parse()
{
	_ast = new Root;
	try
	{
		if (Is(T_PROGRAM))
		{
			ShouldBe(T_ID);
			_ast->progName = GetCurrentValue();
			MustBe(T_SEMICOLON);
		}

		_ast->_blk = ParseBlock();
		_isValid = true;
	}
	catch (exception)
	{
		_isValid = false;
		cout << "Error found on line: " << _lex->GetLine();
	}
}

StatementSeq * Parser::ParseStmntSeq()
{
	StatementSeq *seq = new StatementSeq;
	if(!Is(T_BEGIN))
		throw exception();
	do
	{
		NextToken();
		seq->AddStatement(ParseStatement());
	} while (Is(T_SEMICOLON));
	MustBe(T_END);

	return seq;
}

Block * Parser::ParseBlock()
{
	Block *blk = new Block();
	Scope *scp = new Scope();

	if (Is(T_CONST))
	{
		Const *cnst;
		NextToken();
		do
		{
			cnst = ParseConst();
			if (!scp->Add(cnst->GetID(), cnst))
				throw exception();
			MustBe(T_SEMICOLON);
		} while (Is(T_ID));
	}
	if (Is(T_VAR))
	{
		NextToken();
		do
		{
			vector<string> vars;
			if (!Is(T_ID))
				throw exception();
			vars.push_back(GetCurrentValue());
			while (Is(T_COMMA))
			{
				ShouldBe(T_ID);
				vars.push_back(GetCurrentValue());
			}
			MustBe(T_COLON);
			if (!Is(T_VARTYPE))
				throw exception();
			Var::TYPE type = Str2Type(GetCurrentValue());
			for (auto& i : vars)
				if(!scp->Add(i, new Var(i, type)))
					throw exception();
			MustBe(T_SEMICOLON);
		} while (Is(T_ID));
	}
	while (Is(T_PROC) || Is(T_FUNC))
	{
		if (Is(T_PROC))
		{
			Procedure *p = ParseProcedure();
			//p->SetParentScope(scp);
			p->_blk->SetParentScope(scp);
			if(scp->Add(p->GetID(), p))
				throw exception();
		}
		if (Is(T_FUNC))
		{
			Function *f = ParseFunction();
			//f->SetParentScope(scp);
			f->_blk->SetParentScope(scp);
			if(scp->Add(f->GetID(), f))
				throw exception();
		}
	}
	blk->seq = ParseStmntSeq();
	blk->SetScope(scp);
	return blk;
}

Const * Parser::ParseConst()
{
	Const *cnst = new Const();

	if (Is(T_ID))
		cnst->SetID(GetCurrentValue());
	else
		throw std::exception();

	if (!Is(T_COND) || GetCurrentValue()[0] != '=')
		throw exception();
	if (Is(T_STRING))
		cnst->SetType(Const::STRING);
	else if (Is(T_UNUMBER))
		cnst->SetType(Const::NUMBER);
	else if (Is(T_ID))
		cnst->SetType(Const::ID);
	else if (Is(T_TERMOP))
	{
		char c = GetCurrentValue()[0];
		if (c != '-' && c != '+')
			throw exception();
		cnst->SetNeg(c == '-');
		if (Is(T_UNUMBER))
			cnst->SetType(Const::NUMBER);
		else if (Is(T_ID))
			cnst->SetType(Const::ID);
		else
			throw exception();
	}
	else
		throw exception();

	cnst->SetVal(GetCurrentValue());

	return cnst;
}

Function * Parser::ParseFunction()
{
	Function *f = new Function();
	//Идентификатор
	MustBe(T_ID);
	f->_ID = GetCurrentValue();
	//Распознаем список параметров
	f->_params = ParseParamList();
	// Возвращаемый тип
	MustBe(T_COLON);
	MustBe(T_VARTYPE);
	f->_return_type = Str2Type(GetCurrentValue());
	MustBe(T_SEMICOLON);
	f->_blk = ParseBlock();
	MustBe(T_SEMICOLON);

	return f;
}

Procedure * Parser::ParseProcedure()
{
	Procedure *p = new Procedure();
	//Идентификатор
	MustBe(T_ID);
	p->_ID = GetCurrentValue();
	//Распознаем список параметров
	p->_params = ParseParamList();
	MustBe(T_SEMICOLON);
	p->_blk = ParseBlock();
	MustBe(T_SEMICOLON);

	return p;
}

Statement * Parser::ParseStatement()
{
	if (Is(T_BEGIN))
		return ParseStmntSeq();
	else if (Is(T_IF))
	{
		NextToken();
		Expression *cond = ParseExpression();
		MustBe(T_THEN);
		Statement *thenStnt = ParseStatement();
		Statement *elseStnt = nullptr;
		if (Is(T_ELSE))
		{
			NextToken();
			elseStnt = ParseStatement();
		}
		return new IfStatement(cond, thenStnt, elseStnt);
	}
	else if (Is(T_FOR))
	{
		ShouldBe(T_ID);
		string var = GetCurrentValue();

		MustBe(T_ASSIGN);

		Expression *expr = ParseExpression();

		ForStatement::TYPE type;

		if (Is(T_TO))
			type = ForStatement::TO;
		else if (Is(T_DOWNTO))
			type = ForStatement::DOWNTO;
		else
			throw std::exception();
		NextToken();
		Expression *finExpr = ParseExpression();

		MustBe(T_DO);

		Statement *st = ParseStatement();

		return new ForStatement(var, expr, finExpr, st, type);
	}
	else if(Is(T_WHILE))
	{
		Expression *cond = ParseExpression();
		MustBe(T_DO);
		Statement *st = ParseStatement();
		return new WhileStatement(cond, st);
	}
	else if (Is(T_ID))
	{
		string var = GetCurrentValue();
		if (Is(T_ASSIGN))
		{
			NextToken();
			Expression *expr = ParseExpression();
			return new AssignStatement(var, expr);
		}
		else
		{
			vector<Expression *> params;
			if (Is(T_LBR))
			{
				do
				{
					NextToken();
					params.push_back(ParseExpression());
				} while (Is(T_COMMA));
				MustBe(T_RBR);
			}
			return new ProcCallStatement(var, params);
		}
	}
	else if (Is(T_REPEAT))
	{
		StatementSeq *seq = new StatementSeq();
		do
		{
			NextToken();
			seq->AddStatement(ParseStatement());
		} while (Is(T_SEMICOLON));
		MustBe(T_UNTIL);
		Expression *expr = ParseExpression();

		return new RepeatStatement(expr, seq);
	}
	throw exception();
}

Expression * Parser::ParseExpression()
{
	Expression *left = ParseSimpleExpression();
	if (Is(T_COND))
	{
		Condition::OP op = Str2Cond(GetCurrentValue());
		Expression * right = ParseSimpleExpression();
		return new Condition(left, right, op);
	}
	return left;
}

Expression * Parser::ParseSimpleExpression()
{
	bool isNeg = false;
	if (Is(T_TERMOP))
	{
		char c = GetCurrentValue()[0];
		if (c != '+' && c != '-')
			throw exception();
		isNeg = (c == '-');
	}
	Expression *left = ParseTerm();
	while (Is(T_TERMOP))
	{
		BinaryOp::OP op = Str2Arithm(GetCurrentValue());
		Expression *right = ParseTerm();
		left = new BinaryOp(left, right, op);
	}
	if (isNeg)
		left->Negate();
	return left;
}

Expression * Parser::ParseTerm()
{
	Expression *left = ParseFactor();

	while (Is(T_FACTOROP))
	{
		BinaryOp::OP op = Str2Arithm(GetCurrentValue());
		Expression *right = ParseFactor();
		left = new BinaryOp(left, right, op);
	}
	return left;
}

Expression * Parser::ParseFactor()
{
	if (Is(T_ID))
	{
		string id = GetCurrentValue();
		if (!Is(T_LBR))
			return new ExprID(id);
		vector<Expression *> vec;
		do
		{
			NextToken();
			vec.push_back(ParseExpression());
		} while (Is(T_COMMA));
		MustBe(T_RBR);

		return new FuncCallExpr(id, vec);
	}
	else if (Is(T_STRING))
		return new ExprConst(ExprConst::STRING, GetCurrentValue());
	else if (Is(T_UNUMBER))
		return new ExprConst(ExprConst::NUMBER, GetCurrentValue());
	else if (Is(T_NIL))
		return new ExprConst(ExprConst::NIL, GetCurrentValue());
	else if (Is(T_NEG))
	{
		NextToken();
		Expression *ex = ParseFactor();
		ex->Negate();
		return ex;
	}
	else if (Is(T_LBR))
	{
		Expression *ex = ParseExpression();
		MustBe(T_RBR);
		NextToken();
		return ex;
	}
	throw exception();
}

ParamList * Parser::ParseParamList()
{
	ParamList *pList = new ParamList;
	if (Is(T_LBR))
	{
		do
		{
			NextToken();
			if (Is(T_PROC))
			{
				do
				{
					ShouldBe(T_ID);
					pList->_params.push_back({ GetCurrentValue(), new ParamType() });
				} while (Is(T_COMMA));
			}
			else if (Is(T_FUNC) || Is(T_VAR) || Is(T_ID))
			{
				bool isFunc = Is(T_FUNC);
				bool byRef = Is(T_VAR);
				vector<string> ids;
				do
				{
					ShouldBe(T_ID);
					ids.push_back(GetCurrentValue());
				} while (Is(T_COMMA));
				MustBe(T_COLON);
				MustBe(T_VARTYPE);
				Var::TYPE type = Str2Type(GetCurrentValue());

				for (auto& i : ids)
				{
					if (isFunc)
						pList->_params.push_back({ i, new ParamType(type) });
					else
						pList->_params.push_back({ i, new ParamType(type, (byRef) ? ParamType::REF : ParamType::VALUE) });
				}
			}
			else
				throw exception();
		} while (Is(T_SEMICOLON));
		MustBe(T_RBR);
	}
	return pList;
}




