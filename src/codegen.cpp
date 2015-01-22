#include "codegen.h"

CodeGenerator *CodeGenerator::m_pInst = nullptr;

CodeGenerator::CodeGenerator() : m_Context(llvm::getGlobalContext()), m_pCurScope(nullptr) {
	m_pBuilder = new llvm::IRBuilder<>(m_Context);
}

CodeGenerator::~CodeGenerator() {
}

llvm::Value * CodeGenerator::GenRoot(Root *pRoot) {
	return GenFunction(pRoot);
}


llvm::Value * CodeGenerator::GenStmntSeq(StatementSeq *pSeq) {
	llvm::Function *TheFunction = m_pBuilder->GetInsertBlock()->getParent();

	for (auto &i : pSeq->statements) {
		GenStatement(i);
	}

	return nullptr;
}

llvm::Value * CodeGenerator::GenExpression(Expression *pEl) {

	llvm::Value *pRes;
	switch (pEl->_type) {
	case Expression::E_BINARY:
		pRes = GenBinaryOp(dynamic_cast<BinaryOp *>(pEl));
		break;
	case Expression::E_COND:
		pRes = GenCondition(dynamic_cast<Condition *>(pEl));
		break;
	case Expression::E_CONST:
		pRes = GenExprConst(dynamic_cast<ExprConst *>(pEl));
		break;
	case Expression::E_ID:
		pRes = GenExprID(dynamic_cast<ExprID *>(pEl));
		break;
	case Expression::E_FUNCCALL:
		pRes = GenFuncCallExpr(dynamic_cast<FuncCallExpr *>(pEl));
		break;
	default:
		throw std::exception("undefined expression type");
	}

	// Костыль для обработки унарного минуса :)
	if (pEl->isNeg == true) {
		auto Type = pEl->GetVar(m_pCurScope)->_type;
		if (Type == Var::REAL)
			pRes = m_pBuilder->CreateFSub(llvm::Constant::getNullValue(pRes->getType()), pRes, "negate");
		else
			pRes = m_pBuilder->CreateSub(llvm::Constant::getNullValue(pRes->getType()), pRes, "negate");
	}

	return pRes;
}

llvm::Value * CodeGenerator::GenExprID(ExprID *pEl) {

	ScopableNode *pNode = m_pCurScope->Get<Var>(pEl->id);
	llvm::Value *pV = m_ValueMap[pNode];

	if (pV == nullptr)
		throw std::exception("Unknown variable name");

	return m_pBuilder->CreateLoad(pV, pEl->id.c_str());
}

llvm::Value * CodeGenerator::GenExprConst(ExprConst *pEl) {
	Const *pC = pEl->_val;
	
	switch (pC->_type) {
	case Const::INTEGER:
	{
		auto inc = dynamic_cast<ConstInteger *>(pC);
		return llvm::ConstantInt::get(m_Context, llvm::APInt(sizeof(int) * 8, inc->_val));
	}
	case Const::REAL:
	{
		auto inc = dynamic_cast<ConstReal *>(pC);
		return llvm::ConstantFP::get(m_Context, llvm::APFloat(inc->_val));
	}
	case Const::BOOLEAN:
	{
		auto inc = dynamic_cast<ConstBoolean *>(pC);
		return llvm::ConstantInt::get(m_Context, llvm::APInt(1, inc->_val));
	}
	default:
		throw std::exception("undefined const expression");
	}
}

llvm::Value * CodeGenerator::GenBinaryOp(BinaryOp *pEl) {

	auto *pType = pEl->GetVar(m_pCurScope);

	llvm::Value *pLeft = ExpressionCaster(pEl->_left, pType);
	llvm::Value *pRight = ExpressionCaster(pEl->_right, pType);


	switch (pEl->_op) {
	case BinaryOp::MUL:
		return pType->_type == Var::REAL ? 
			m_pBuilder->CreateFMul(pLeft, pRight, "mulreal") :
			m_pBuilder->CreateMul(pLeft, pRight, "mulint");
	case BinaryOp::ADD:
		return pType->_type == Var::REAL ?
			m_pBuilder->CreateFAdd(pLeft, pRight, "addreal") :
			m_pBuilder->CreateAdd(pLeft, pRight, "addint");
	case BinaryOp::SUB:
		return pType->_type == Var::REAL ?
			m_pBuilder->CreateFSub(pLeft, pRight, "subreal") :
			m_pBuilder->CreateSub(pLeft, pRight, "subint");
	case BinaryOp::DIV:
		return m_pBuilder->CreateFDiv(pLeft, pRight, "divreal");
		break;
	case BinaryOp::INT_DIV:
		return m_pBuilder->CreateSDiv(pLeft, pRight, "divint");
	case BinaryOp::MOD:
		return m_pBuilder->CreateSRem(pLeft, pRight, "modint");
	case BinaryOp::AND:
		return m_pBuilder->CreateAnd(pLeft, pRight, "andint");
	case BinaryOp::OR:
		return m_pBuilder->CreateOr(pLeft, pRight, "orint");
	default:
		throw std::exception("not supported yet");
	}

}

llvm::Value * CodeGenerator::GenProcCallStatement(ProcCallStatement *pEl) {
	
	FuncCallExpr expr(pEl->_id, pEl->_params);

	return GenFuncCallExpr(&expr);
}

llvm::Value * CodeGenerator::GenFuncCallExpr(FuncCallExpr *pEl) {
	llvm::Function *pFunc = m_pMainModule->getFunction(pEl->_name);

	if (pFunc == nullptr)
		throw std::exception("Unknown function referenced");

	if (pFunc->arg_size() != pEl->_params.size())
		throw std::exception("Incorrect # arguments passed");

	std::vector<llvm::Value *> ArgsV;

	for (unsigned i = 0, e = pEl->_params.size(); i != e; ++i) {
		ArgsV.push_back(GenExpression(pEl->_params[i]));
		if (ArgsV.back() == 0) return 0;
	}

	return m_pBuilder->CreateCall(pFunc, ArgsV, "calltmp");
}

llvm::Value * CodeGenerator::GenCondition(Condition *pEl) {
	const Var *pLeftT = pEl->_left->GetVar(m_pCurScope);
	const Var *pRightT = pEl->_right->GetVar(m_pCurScope);
	const Var *pBestT = Expression::GetBestType(pLeftT, pRightT);

	llvm::Value *pLeftV = ExpressionCaster(pEl->_left, pBestT);
	llvm::Value *pRightV = ExpressionCaster(pEl->_right, pBestT);

	llvm::CmpInst::Predicate Op;

	switch (pEl->_op) {
	case Condition::EQ:
		switch (pBestT->_type) {
		case Var::REAL:
			Op = llvm::CmpInst::Predicate::FCMP_OEQ; break;
		default:
			Op = llvm::CmpInst::Predicate::ICMP_EQ;
		}

		break;
	case Condition::NEQ:
		switch (pBestT->_type) {
		case Var::REAL:
			Op = llvm::CmpInst::Predicate::FCMP_ONE; break;
		default:
			Op = llvm::CmpInst::Predicate::ICMP_NE;
		}

		break;
	case Condition::LT:
		switch (pBestT->_type) {
		case Var::REAL:
			Op = llvm::CmpInst::Predicate::FCMP_OLT; break;
		case Var::BOOLEAN:
			Op = llvm::CmpInst::Predicate::ICMP_ULT; break;
		default:
			Op = llvm::CmpInst::Predicate::ICMP_SLT;
		}

		break;
	case Condition::LE:
		switch (pBestT->_type) {
		case Var::REAL:
			Op = llvm::CmpInst::Predicate::FCMP_OLE; break;
		case Var::BOOLEAN:
			Op = llvm::CmpInst::Predicate::ICMP_ULE; break;
		default:
			Op = llvm::CmpInst::Predicate::ICMP_SLE;
		}

		break;
	case Condition::GT:
		switch (pBestT->_type) {
		case Var::REAL:
			Op = llvm::CmpInst::Predicate::FCMP_OGT; break;
		case Var::BOOLEAN:
			Op = llvm::CmpInst::Predicate::ICMP_UGT; break;
		default:
			Op = llvm::CmpInst::Predicate::ICMP_SGT;
		}

		break;
	case Condition::GE:
		switch (pBestT->_type) {
		case Var::REAL:
			Op = llvm::CmpInst::Predicate::FCMP_OGE; break;
		case Var::BOOLEAN:
			Op = llvm::CmpInst::Predicate::ICMP_UGE; break;
		default:
			Op = llvm::CmpInst::Predicate::ICMP_SGE;
		}

		break;
	case Condition::IN:
	default:
		throw exception("not supported yet");
	}

	if (Op <= llvm::CmpInst::Predicate::LAST_FCMP_PREDICATE)
		return m_pBuilder->CreateFCmp(Op, pLeftV, pRightV);
	else
		return m_pBuilder->CreateICmp(Op, pLeftV, pRightV);
}

llvm::Value * CodeGenerator::GenIfStatement(IfStatement *pEl) {
	Var VarBool(Var::BOOLEAN);
	llvm::Value *pCondV = ExpressionCaster(pEl->_cond, &VarBool);

	llvm::Function *TheFunction = m_pBuilder->GetInsertBlock()->getParent();

	llvm::BasicBlock *pThenBB = llvm::BasicBlock::Create(m_Context, "then", TheFunction);
	llvm::BasicBlock *pElseBB = llvm::BasicBlock::Create(m_Context, "else");
	llvm::BasicBlock *pMergeBB = llvm::BasicBlock::Create(m_Context, "ifcont");

	m_pBuilder->CreateCondBr(pCondV, pThenBB, pElseBB);

	m_pBuilder->SetInsertPoint(pThenBB);
	llvm::Value *pThenV = GenStatement(pEl->_then);

	m_pBuilder->CreateBr(pMergeBB);
	pThenBB = m_pBuilder->GetInsertBlock();

	TheFunction->getBasicBlockList().push_back(pElseBB);
	m_pBuilder->SetInsertPoint(pElseBB);

	llvm::Value *pElseV = GenStatement(pEl->_else);
	m_pBuilder->CreateBr(pMergeBB);

	pElseBB = m_pBuilder->GetInsertBlock();
	TheFunction->getBasicBlockList().push_back(pMergeBB);
	m_pBuilder->SetInsertPoint(pMergeBB);

	return nullptr;
}

llvm::Value * CodeGenerator::GenForStatement(ForStatement *pEl) {
	llvm::Function *TheFunction = m_pBuilder->GetInsertBlock()->getParent();

	Var *pForT = m_pCurScope->Get<Var>(pEl->_var);
	if (!pForT->Is(Var::INTEGER))
		throw exception("incorrect variable type");
	llvm::Value *pForV = m_ValueMap[pForT];
	llvm::Value *pFrom = ExpressionCaster(pEl->_from, pForT);
	m_pBuilder->CreateStore(pFrom, pForV); // Присваиваем начальное значение

	// Определяем шаг цикла
	llvm::Value *pStepV;
	if (pEl->_type == ForStatement::TO)
		pStepV = llvm::ConstantInt::get(m_Context, llvm::APInt(sizeof(int) * 8, 1));
	else
		pStepV = llvm::ConstantInt::get(m_Context, llvm::APInt(sizeof(int) * 8, -1));

	
	llvm::BasicBlock *pCondBB = llvm::BasicBlock::Create(m_Context, "loopcond", TheFunction);
	llvm::BasicBlock *pBodyBB = llvm::BasicBlock::Create(m_Context, "loop", TheFunction);
	llvm::BasicBlock *pAfterBB = llvm::BasicBlock::Create(m_Context, "afterloop", TheFunction);

	// Проверяем условие выхода
	m_pBuilder->CreateBr(pCondBB);
	m_pBuilder->SetInsertPoint(pCondBB);
	llvm::Value *pTo = ExpressionCaster(pEl->_to, pForT);
	llvm::Value *pEndCond;

	llvm::Value *pCurVar = m_pBuilder->CreateLoad(pForV, pEl->_var.c_str());
	if (pEl->_type == ForStatement::TO)
		pEndCond = m_pBuilder->CreateICmpSLE(pCurVar, pTo, "endloop");
	else
		pEndCond = m_pBuilder->CreateICmpSGE(pCurVar, pTo, "endloop");

	m_pBuilder->CreateCondBr(pEndCond, pBodyBB, pAfterBB);
	
	// Генерируем тело цикла
	m_pBuilder->SetInsertPoint(pBodyBB);
	llvm::Value *pBody = GenStatement(pEl->_do);

	llvm::Value *pNextVar = m_pBuilder->CreateAdd(pCurVar, pStepV, "nextvar");
	m_pBuilder->CreateStore(pNextVar, pForV);
	m_pBuilder->CreateBr(pCondBB);

	// Выход из цикла
	m_pBuilder->SetInsertPoint(pAfterBB);

	return nullptr;
}

llvm::Value * CodeGenerator::GenAssignStatement(AssignStatement *pEl) {
	Var *pVar = m_pCurScope->Get<Var>(pEl->_var);

	llvm::Value *pAssignValue = ExpressionCaster(pEl->_expr, pVar);

	llvm::Value *pVarValue = m_ValueMap[pVar];

	m_pBuilder->CreateStore(pAssignValue, pVarValue);

	return nullptr;
}


llvm::Function * CodeGenerator::GenFunctionHeader(Function *pFunc) {
	llvm::Type *pReturnType = GetType(pFunc->_prtype); // Тип возвращаемого значения

	unsigned NumOfParams = pFunc->GetNumOfParams(); // Кол-во параметров функции

	vector<llvm::Type *> pParamTypes(NumOfParams); // Массив типов параметров

	for (unsigned i = 0; i < NumOfParams; ++i)
		pParamTypes[i] = GetType(pFunc->_params->_params[i].second);

	llvm::FunctionType *FuncType = llvm::FunctionType::get(pReturnType, pParamTypes, false);

	llvm::Function *pFunction = llvm::Function::Create(FuncType, llvm::Function::ExternalLinkage, pFunc->GetID(), m_pMainModule);

	if (pFunction->getName() != pFunc->GetID()) {
		// Если существует фнукция с таким же именем
		pFunction->eraseFromParent();
		pFunction = m_pMainModule->getFunction(pFunc->GetID());

		if (!pFunction->empty() || pFunction->arg_size() != NumOfParams) {
			// Ошибка. Переопределение функции
			throw std::exception("Function redefenition");
		}
	}

	// Устанавливаем имена для всех параметров
	unsigned i = 0;
	for (llvm::Function::arg_iterator it = pFunction->arg_begin(); i < NumOfParams; ++it, ++i) {
		string &name = pFunc->_params->_params[i].first;
		it->setName(name);
	}

	return pFunction;
}

llvm::Value * CodeGenerator::GenFunctionBody(Function *pFunc) {
	m_pCurScope = &pFunc->scp; // Устанавливаем текущую область видимости

	llvm::Function *pFunction = m_pMainModule->getFunction(pFunc->_ID);

	llvm::BasicBlock *pBB = llvm::BasicBlock::Create(m_Context, "entry", pFunction);
	m_pBuilder->SetInsertPoint(pBB);

	// Выделяем память под параметры и добавляем их в таблицу символов
	unsigned i = 0;
	for (llvm::Function::arg_iterator it = pFunction->arg_begin(); i < pFunc->GetNumOfParams(); ++it, ++i) {
		auto &inc = pFunc->_params->_params[i];
		llvm::Value *pAlloca = CreateEntryBlockAlloca(pFunction, inc.first, inc.second);
		m_pBuilder->CreateStore(it, pAlloca);
		m_ValueMap[inc.second] = pAlloca;
	}

	// Выделяем память под переменные :)
	for (const auto &i : pFunc->Vars) {
		llvm::Value *pAlloca = CreateEntryBlockAlloca(pFunction, i.first, i.second, m_pCurScope->IsRoot());
		//m_pBuilder->CreateStore(new llvm::Integer, pAlloca);
		m_ValueMap[i.second] = pAlloca;
	}

	// Устанавливаем возращаемое значение
	llvm::Value *pRetVal = nullptr;
	if (pFunc->_prtype->_type != Var::VOID) {
		pRetVal = CreateEntryBlockAlloca(pFunction, pFunc->_ID, pFunc->_prtype);
		m_ValueMap[pFunc->_prtype] = pRetVal;
	}

	llvm::Value *pBody = GenStmntSeq(pFunc->seq);

	m_pBuilder->CreateRet(m_pBuilder->CreateLoad(pRetVal));
	llvm::verifyFunction(*pFunction);

	return pFunction;
}

llvm::Value * CodeGenerator::GenFunction(Function *pFunc) {
	// Обрабатываем вложенные функции
	for (auto &i : pFunc->Funcs)
		GenFunctionHeader(i.second);

	GenFunctionHeader(pFunc);
	llvm::Value *pRes = GenFunctionBody(pFunc);
	
	for(auto &i : pFunc->Funcs)
		GenFunctionBody(i.second);

	//pFunction->dump();
	return pRes;
}


llvm::Value * CodeGenerator::GenStatement(Statement *pStmt) {
	switch (pStmt->_type) {
	case Statement::S_SEQ:
	{
		StatementSeq *pSeq = dynamic_cast<StatementSeq *>(pStmt);
		return GenStmntSeq(pSeq);
	}
	case Statement::S_ASSIGN:
	{
		AssignStatement *pAssign = dynamic_cast<AssignStatement *>(pStmt);
		return GenAssignStatement(pAssign);
	}
	case Statement::S_PROCCALL:
	{
		ProcCallStatement *pCall = dynamic_cast<ProcCallStatement *>(pStmt);
		return GenProcCallStatement(pCall);
	}
	case Statement::S_IF:
	{
		IfStatement *pIf = dynamic_cast<IfStatement *>(pStmt);
		return GenIfStatement(pIf);
	}
	case Statement::S_FOR:
	{
		ForStatement *pFor = dynamic_cast<ForStatement *>(pStmt);
		return GenForStatement(pFor);
	}
	case Statement::S_EMPTY:
		return nullptr;
	case Statement::S_WHILE:
	case Statement::S_REPEAT:
	default:
		throw std::exception();
	};
}

llvm::Value * CodeGenerator::ExpressionCaster(Expression *pExp, const Var *pTo) {

	llvm::Value *pExpValue = GenExpression(pExp);

	auto pType = pExp->GetVar(m_pCurScope);

	llvm::Instruction::CastOps CastOp;
	std::string CastName = "cast";

	if (pType->_type != pTo->_type) {
		llvm::Type *pDestT = GetType(pTo);

		switch (pType->_type) {
		case Var::REAL:
			CastName += "real";
			if (pTo->_type != Var::BOOLEAN)
				CastOp = llvm::Instruction::CastOps::FPToSI;
			else
				CastOp = llvm::Instruction::CastOps::FPToUI;
			break;
		case Var::INTEGER:
		case Var::CHAR:
			CastName += "int";
			if (pTo->_type == Var::REAL)
				CastOp = llvm::Instruction::CastOps::SIToFP;
			else
				return m_pBuilder->CreateIntCast(pExpValue, pDestT, true, CastName);
			break;
		case Var::BOOLEAN:
			CastName += "bool";
			if (pTo->_type == Var::REAL)
				CastOp = llvm::Instruction::CastOps::UIToFP;
			else
				return m_pBuilder->CreateIntCast(pExpValue, pDestT, false, CastName);

		default:
			std::exception("illegal type cast");
		}

		pExpValue = m_pBuilder->CreateCast(CastOp, pExpValue, pDestT, CastName);
	}

	return pExpValue;
}

llvm::Type * CodeGenerator::GetType(const Var *pV) {
	switch (pV->_type) {
	case Var::BOOLEAN:
		return llvm::IntegerType::get(m_Context, 1);
	case Var::CHAR:
		return llvm::IntegerType::get(m_Context, 8);
	case Var::INTEGER:
		return llvm::IntegerType::get(m_Context, sizeof(int) * 8);
	case Var::REAL:
		return llvm::Type::getDoubleTy(m_Context);
	case Var::VOID:
		return llvm::Type::getVoidTy(m_Context);
	}

	return nullptr;
}



CodeGenerator * CodeGenerator::getInstance() {
	if (m_pInst == nullptr) {
		llvm::InitializeNativeTarget();
		m_pInst = new CodeGenerator();
	}
	return m_pInst;
}

void CodeGenerator::Generate(Parser *pP) {
	m_pMainModule = new llvm::Module(pP->_ast->_ID, m_Context);
	GenRoot(pP->_ast);
}

void CodeGenerator::Release() {
	if (m_pInst != nullptr) {
		delete m_pInst;
		m_pInst = nullptr;
	}
}

void CodeGenerator::Dump() {
	m_pMainModule->dump();
}

int CodeGenerator::Execute() {
	llvm::ExecutionEngine *pEe = llvm::EngineBuilder(m_pMainModule).setErrorStr(&m_ErrorString).create();
	std::string name = m_pMainModule->getModuleIdentifier();
	llvm::Function *pFunction = pEe->FindFunctionNamed(name.c_str());
	std::vector<std::string> v;
	int res = pEe->runFunctionAsMain(pFunction, v, nullptr);
	delete pEe;

	return res;
}