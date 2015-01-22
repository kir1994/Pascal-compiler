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
	else if (type == "boolean")
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
			_ast->_ID = GetCurrentValue();
			_ast->scp.name = _ast->_ID;
			_ast->scp.Add(_ast->_ID, _ast->_prtype);
			MustBe(T_SEMICOLON);
		}

		ParseDeclarations(_ast);
		_ast->seq = ParseStmntSeq();
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

void Parser::ParseDeclarations(Function *func)
{
	if (Is(T_CONST))
	{
		Const *cnst;
		ShouldBe(T_ID);
		do
		{
			string str = GetCurrentValue();
			cnst = ParseConst();
			if (!func->add(str, cnst))
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
			{
				switch (type)
				{
				case Var::BOOLEAN:
					if (!func->add(i, new Var(Var::BOOLEAN)))
						throw exception();
					break;
				case Var::INTEGER:
					if (!func->add(i, new Var(Var::INTEGER)))
						throw exception();
					break;
				case Var::CHAR:
					if (!func->add(i, new Var(Var::CHAR)))
						throw exception();
					break;
				case Var::REAL:
					if (!func->add(i, new Var(Var::REAL)))
						throw exception();
					break;
				}
			}
			MustBe(T_SEMICOLON);
		} while (Is(T_ID));
	}
	while (Is(T_PROC) || Is(T_FUNC))
	{
		Function *f = ParseFunction(func, Is(T_FUNC));
		if (!func->add(f->GetID(), f))
			throw exception();
	}
}

Const *ParseConstNumber(const string& val, bool neg = false)
{
	bool sign = neg;
	double res;
	int fst;
	int snd;
	unsigned cnt = 0;
	bool isPt = false;
	while (cnt < val.size() && isdigit(val[cnt])) ++cnt;
	fst = stoi(val.substr(0, cnt));
	const unsigned offs1 = cnt + 1;
	if (cnt == val.size())
		return  (!sign) ? (new ConstInteger(fst)) : (new ConstInteger(-fst));
	if (val[cnt] == '.')
	{
		cnt++;
		isPt = true;
		while (cnt < val.size() && isdigit(val[cnt])) ++cnt;
		string ptPart = val.substr(offs1, cnt);
		res = stoi(ptPart);
		for (unsigned i = 0; i < ptPart.size(); ++i)
			res /= 10;
		res += fst;
		if (cnt == val.size())
			return (!sign) ? (new ConstReal(res)) : (new ConstReal(-res));
	}
	if (val[cnt] == 'E')
	{
		cnt++;
		if (!isdigit(val[cnt]))
		{
			sign ^= (val[cnt] == '-');
			cnt++;
		}
		snd = stoi(val.substr(cnt, val.size()));
		if (isPt)
		{
			for (int i = 0; i < snd; ++i)
				res *= 10;
			return (!sign) ? (new ConstReal(res)) : (new ConstReal(-res));
		}
		else
		{
			for (int i = 0; i < snd; ++i)
				fst *= 10;
			return  (!sign) ? (new ConstInteger(fst)) : (new ConstInteger(-fst));
		}
	}
	throw exception();
}

Const * Parser::ParseConst()
{

	if (!Is(T_COND) || GetCurrentValue()[0] != '=')
		throw exception();
	//if (Is(T_STRING))
	//	return new ConstString(GetCurrentValue());
	else if (Is(T_UNUMBER))
		return ParseConstNumber(GetCurrentValue());
	//else if (Is(T_ID)) {
	//	std::string val = GetCurrentValue();
	//	return new ConstID(GetCurrentValue());
	//}
	else if (Is(T_FALSE) || Is(T_TRUE))
	{
		bool b = Is(T_TRUE);
		NextToken();
		return new ConstBoolean(b);
	}
	else if (Is(T_TERMOP))
	{
		char c = GetCurrentValue()[0];
		if (c != '-' && c != '+')
			throw exception();
		if (Is(T_UNUMBER))
			return ParseConstNumber(GetCurrentValue(), (c == '-'));
		/*else if (Is(T_ID))
		{
			Const *cnst;
			cnst = new ConstID(GetCurrentValue());
			cnst->SetNeg(c == '-');

			return cnst;
		}*/
		else
			throw exception();
	}
	else
		throw exception();
}

Function * Parser::ParseFunction(Function *par, bool isFunc)
{
	//Идентификатор
	ShouldBe(T_ID);
	string id = GetCurrentValue();
	//Распознаем список параметров
	ParamList* pList = ParseParamList();
	for (auto &i : pList->_params)
		par->scp.Add(i.first, i.second);

	Var::TYPE rtype = Var::VOID;
	if (isFunc)
	{
		// Возвращаемый тип
		MustBe(T_COLON);
		if (!Is(T_VARTYPE))
			throw exception();
		rtype = Str2Type(GetCurrentValue());
	}
	MustBe(T_SEMICOLON);
	Function *pFunc = new Function(id, pList, rtype);
	pFunc->scp.SetParScope(&par->scp);
	ParseDeclarations(pFunc);
	pFunc->scp.Add(pFunc->_ID, pFunc->_prtype);
	pFunc->seq = ParseStmntSeq();
	MustBe(T_SEMICOLON);
	return pFunc;
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
	else return new Statement(Statement::TYPE::S_EMPTY);
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
	//else if (Is(T_STRING))
	//	return new ExprConst(new ConstString(GetCurrentValue()));
	else if (Is(T_UNUMBER))
		return new ExprConst(ParseConstNumber(GetCurrentValue()));
	/*else if (Is(T_NIL))
		return new ExprConst(ExprConst::NIL);*/
	else if (Is(T_FALSE) || Is(T_TRUE))
	{
		bool b = Is(T_TRUE);
		NextToken();
		return new ExprConst(new ConstBoolean(b));
	}
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
			
			if (Is(T_VAR) || Is(T_ID))
			{
				bool byRef = Is(T_VAR);
				if (Is(T_VAR))
					NextToken();
				vector<string> ids;
				//MustBe(T_ID);
				ids.push_back(GetCurrentValue());
				while (Is(T_COMMA))
				{
					ShouldBe(T_ID);
					ids.push_back(GetCurrentValue());
				}
				MustBe(T_COLON);

				if (!Is(T_VARTYPE))
					throw exception();

				Var::TYPE type = Str2Type(GetCurrentValue());

				for (auto& i : ids)
					pList->_params.push_back({ i, new Var(type, false, byRef)});
			}
			else
				throw exception();
		} while (Is(T_SEMICOLON));
		MustBe(T_RBR);
	}
	return pList;
}




