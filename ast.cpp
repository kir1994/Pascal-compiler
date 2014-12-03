#include "parser.h"
#include <stdlib.h>

void Root::Parse(void *_pParser)
{
	Parser *pParser = (Parser *)_pParser;

	if (pParser->Is(T_PROGRAM))
	{
		pParser->ShouldBe(T_ID);
		pParser->ShouldBe(T_SEMICOLON);
		pParser->NextToken();
	}

	this->_blk = new Block(nullptr);
	return this->_blk->Parse(_pParser);
}

void Block::Parse(void *_pParser)
{
	Parser *pParser = (Parser *)_pParser;

	if (pParser->Is(T_LABEL))
	{	
		do
		{
			pParser->ShouldBe(T_UINT);
			this->labels.push_back(atoi(pParser->GetCurrentValue().c_str()));
		} while (pParser->Is(T_COMMA));

		pParser->MustBe(T_SEMICOLON);
	}
	if (pParser->Is(T_CONST))
	{
		Const cnst;
		do
		{
			pParser->NextToken();
			cnst.Parse(_pParser);
			this->constants.push_back(cnst);
		} while (pParser->Is(T_SEMICOLON));
	}
	if (pParser->Is(T_VAR))
	{
		do
		{
			std::vector<std::string> vars;
			pParser->ShouldBe(T_ID);
			vars.push_back(pParser->GetCurrentValue());
			while (pParser->Is(T_COMMA))
			{
				pParser->ShouldBe(T_ID);
				vars.push_back(pParser->GetCurrentValue());
			}
			pParser->MustBe(T_COLON);
			pParser->MustBe(T_VARTYPE);
			VAR_TYPES type = Str2Type(pParser->GetCurrentValue());
			for (auto& i : vars)
				this->_scp.second[i] = new Var(i, type);
			pParser->MustBe(T_SEMICOLON);
		} while (pParser->Is(T_VAR));
	}
	while (pParser->Is(T_PROC) || pParser->Is(T_FUNC))
	{
		if (pParser->Is(T_PROC))
		{
			Procedure p(&(this->_scp));
			
			p.Parse(_pParser);
			this->procedures.push_back(p);
		}
		if (pParser->Is(T_FUNC))
		{
			Function f(&(this->_scp));

			f.Parse(_pParser);
			this->functions.push_back(f);
		}
	}
	pParser->MustBe(T_BEGIN);
	do
	{
		Statement *st = pParser->ParseStatement();
		this->statements.push_back(st);
	} while (pParser->Is(T_SEMICOLON));
	pParser->ShouldBe(T_END);
}

void Const::Parse(void *_pParser)
{
	this->isNeg = false;
	Parser *pParser = (Parser *)_pParser;

	pParser->MustBe(T_ID);

	this->_ID = pParser->GetCurrentValue();
	pParser->MustBe(T_ASSIGN);
	pParser->NextToken();

	if (pParser->Is(T_STRING))
		this->_type = Const::STRING;
	else if (pParser->Is(T_UNUMBER))
		this->_type = Const::NUMBER;
	else if (pParser->Is(T_ID))
		this->_type = Const::ID;
	else if (pParser->Is(T_SIGN))
	{
		char c = pParser->GetCurrentValue()[0];
		this->isNeg = (c == '-');
		if (pParser->Is(T_UNUMBER))
			this->_type = Const::NUMBER;
		else if (pParser->Is(T_ID))
			this->_type = Const::ID; 
		else
			throw std::exception();
	}
	else
		throw std::exception();

	this->_val = pParser->GetCurrentValue();
}

void Function::Parse(void *_pParser)
{
	Parser *pParser = (Parser *)_pParser;

	pParser->MustBe(T_ID);

	this->_name = pParser->GetCurrentValue();
	//Распознаем список параметров
	this->_params.Parse(_pParser);
	// Возвращаемый тип
	pParser->MustBe(T_COLON);
	pParser->MustBe(T_VARTYPE);

	this->_return_type = Str2Type(pParser->GetCurrentValue());

	pParser->MustBe(T_SEMICOLON);

	this->_blk = new Block(&(this->_scp));
	this->_blk->Parse(_pParser);

	pParser->MustBe(T_SEMICOLON);
}

void Procedure::Parse(void *_pParser)
{
	Parser *pParser = (Parser *)_pParser;

	pParser->MustBe(T_ID);

	this->_name = pParser->GetCurrentValue();
	//Распознаем список параметров
	this->_params.Parse(_pParser);

	pParser->MustBe(T_SEMICOLON);

	this->_blk = new Block(&(this->_scp));
	this->_blk->Parse(_pParser);

	pParser->MustBe(T_SEMICOLON);
}

void Expression::Parse(void *_pParser)
{
	Parser *pParser = (Parser *)_pParser;

	this->_left.Parse(_pParser);
	if (pParser->Is(T_EXPR_OP))
	{
		this->_op = Str2ExprOp(pParser->GetCurrentValue());
		this->_right = new SimpleExpression();
		this->_right->Parse(_pParser);
	}
}

void SimpleExpression::Parse(void *_pParser)
{
	Parser *pParser = (Parser *)_pParser;
	
	if (pParser->Is(T_SIGN))
	{
		char c = pParser->GetCurrentValue()[0];
		this->isNeg = (c == '-');
	}
	this->_left.Parse(_pParser);

	while (pParser->Is(T_SEXPR_OP))
	{
		SEXPR_OP op = Str2SExprOp(pParser->GetCurrentValue());
		Term term;
		term.Parse(_pParser);
		this->_operands.push_back({ op, term });
	}
}

void Term::Parse(void *_pParser)
{
	Parser *pParser = (Parser *)_pParser;

	this->_left = pParser->ParseFactor();

	while (pParser->Is(T_TERM_OP))
	{
		TERM_OP op = Str2TermOp(pParser->GetCurrentValue());

		Factor *fact = pParser->ParseFactor();
	}
}

void ParamList::Parse(void *_pParser)
{
	Parser *pParser = (Parser *)_pParser;
	
	if (pParser->Is(T_LBR))
	{
		do
		{
			pParser->NextToken();
			if (pParser->Is(T_PROC))
			{
				do
				{
					pParser->ShouldBe(T_ID);
					this->_params.push_back({ pParser->GetCurrentValue(), new ParamType() });
				} while (pParser->Is(T_COMMA));
			}
			else if (pParser->Is(T_FUNC) || pParser->Is(T_VAR))
			{
				bool isFunc = pParser->Is(T_FUNC);
				std::vector<std::string> ids;
				do
				{
					pParser->ShouldBe(T_ID);
					ids.push_back(pParser->GetCurrentValue());
				} while (pParser->Is(T_COMMA));
				pParser->MustBe(T_COLON);
				pParser->MustBe(T_VARTYPE);
				VAR_TYPES type = Str2Type(pParser->GetCurrentValue());

				for (auto& i : ids)
				{
					if (isFunc)
						this->_params.push_back({ i, new ParamType(type, type) });
					else
						this->_params.push_back({ i, new ParamType(type) });
				}
			}
			else
				throw std::exception();
		} while (pParser->Is(T_SEMICOLON));
		pParser->MustBe(T_RBR);
	}
}




