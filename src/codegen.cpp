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
	throw std::exception("not supported yet");
}

llvm::Value * CodeGenerator::GenIfStatement(IfStatement *pEl) {
	throw std::exception("not supported yet");
}

llvm::Value * CodeGenerator::GenForStatement(ForStatement *pEl) {
	throw std::exception("not supported yet");
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
	case Statement::S_FOR:
	case Statement::S_WHILE:
	case Statement::S_REPEAT:
	default:
		throw std::exception();
	};
}

llvm::Value * CodeGenerator::GenVar(Var *pVar) {
	switch (pVar->_type) {
	case Var::CHAR:
	case Var::REAL:
	case Var::INTEGER:
	case Var::BOOLEAN:
	case Var::VOID:
	default:
		return nullptr;
	};

}
/*


llvm::Value * CodeGenerator::GenConst(Const *pNode) {
	switch (pNode->_type) {
	case Const::NUMBER:
		if (pNode->_val.find('.') != string::npos)
			return llvm::ConstantFP::get(m_Context, llvm::APFloat(atof(pNode->_val.c_str())));
		else
			return llvm::ConstantInt::get(m_Context, llvm::APInt(sizeof(int) * 8, atoi(pNode->_val.c_str())));
		break;
	case Const::ID:
		break;
	case Const::STRING:
		break;
	}

	return nullptr;
}
llvm::Value * CodeGenerator::GenProcedure() { return nullptr; }

llvm::Value * CodeGenerator::GenParamList() { return nullptr; }
llvm::Value * CodeGenerator::GenExpression() { return nullptr; }
llvm::Value * CodeGenerator::GenSimpleExpression() { return nullptr; }
llvm::Value * CodeGenerator::GenTerm() { return nullptr; }
llvm::Value * CodeGenerator::GenFactor() { return nullptr; }
*/

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