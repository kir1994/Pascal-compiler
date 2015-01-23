#include "codegen.h"

CodeGenerator *CodeGenerator::m_pInst = nullptr;

CodeGenerator::CodeGenerator() : m_Context(llvm::getGlobalContext()), 
								 m_pCurScope(nullptr), m_pOurFPM(nullptr), m_pExe(nullptr) 
{
	m_pBuilder = new llvm::IRBuilder<>(m_Context);
}

CodeGenerator::~CodeGenerator() {
	delete m_pExe;
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

	// ������� ��� ��������� �������� ������ :)
	if (pEl->isNeg == true) {
		auto Type = pEl->GetVar(m_pCurScope)->_type;
		if (Type == Var::REAL)
			pRes = m_pBuilder->CreateFSub(llvm::Constant::getNullValue(pRes->getType()), pRes, "negate");
		else
			pRes = m_pBuilder->CreateSub(llvm::Constant::getNullValue(pRes->getType()), pRes, "negate");
	}

	return pRes;
}

llvm::Value * CodeGenerator::GenExprID(ExprID *pEl, bool getRef) {

	Var *pNode = m_pCurScope->Get<Var>(pEl->id);
	llvm::Value *pV = m_ValueMap[pNode];

	if (pV == nullptr)
		throw std::exception("Unknown variable name");

	if (pNode->isRef)
		pV = m_pBuilder->CreateLoad(pV, pEl->id.c_str());

	if (getRef == false)
		pV = m_pBuilder->CreateLoad(pV, pEl->id.c_str());

	return pV;
}

llvm::Constant * CodeGenerator::GetConstValue(const Const *pC) {
	switch (pC->_type) {
	case Const::INTEGER:
	{
		auto inc = dynamic_cast<const ConstInteger *>(pC);
		return llvm::ConstantInt::get(m_Context, llvm::APInt(sizeof(int) * 8, inc->_val));
	}
	case Const::REAL:
	{
		auto inc = dynamic_cast<const ConstReal *>(pC);
		return llvm::ConstantFP::get(m_Context, llvm::APFloat(inc->_val));
	}
	case Const::BOOLEAN:
	{
		auto inc = dynamic_cast<const ConstBoolean *>(pC);
		return llvm::ConstantInt::get(m_Context, llvm::APInt(1, inc->_val));
	}
	default:
		throw std::exception("undefined const expression");
	}
}

llvm::Value * CodeGenerator::GenExprConst(ExprConst *pEl) {
	return GetConstValue(pEl->_val);
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
	auto pFunc = m_pCurScope->Get<Function>(pEl->_name);

	if (pFunc == nullptr)
		throw std::exception("Unknown function referenced");

	if (pFunc->_params->_params.size() != pEl->_params.size())
		throw std::exception("Incorrect # arguments passed");

	std::vector<llvm::Value *> ArgsV;

	for (unsigned i = 0, e = pEl->_params.size(); i != e; ++i) {
		ArgsV.push_back(ExpressionCaster(pEl->_params[i], pFunc->_params->_params[i].second));
		if (ArgsV.back() == 0) return 0;
	}

	llvm::Function *pFunction = llvm::dyn_cast<llvm::Function>(m_ValueMap[pFunc]);
	return m_pBuilder->CreateCall(pFunction, ArgsV);
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
	m_pBuilder->CreateStore(pFrom, pForV); // ����������� ��������� ��������

	// ���������� ��� �����
	llvm::Value *pStepV;
	ConstInteger Step((pEl->_type == ForStatement::TO ? 1 : -1));
	pStepV = GetConstValue(&Step);

	llvm::BasicBlock *pCondBB = llvm::BasicBlock::Create(m_Context, "loopcond", TheFunction);
	llvm::BasicBlock *pBodyBB = llvm::BasicBlock::Create(m_Context, "loop", TheFunction);
	llvm::BasicBlock *pAfterBB = llvm::BasicBlock::Create(m_Context, "afterloop", TheFunction);

	// ��������� ������� ������
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
	
	// ���������� ���� �����
	m_pBuilder->SetInsertPoint(pBodyBB);
	llvm::Value *pBody = GenStatement(pEl->_do);

	llvm::Value *pNextVar = m_pBuilder->CreateAdd(pCurVar, pStepV, "nextvar");
	m_pBuilder->CreateStore(pNextVar, pForV);
	m_pBuilder->CreateBr(pCondBB);

	// ����� �� �����
	m_pBuilder->SetInsertPoint(pAfterBB);

	return nullptr;
}

llvm::Value * CodeGenerator::GenWhileStatement(WhileStatement *pEl) {
	llvm::Function *TheFunction = m_pBuilder->GetInsertBlock()->getParent();

	llvm::BasicBlock *pCondBB = llvm::BasicBlock::Create(m_Context, "loopcond", TheFunction);
	llvm::BasicBlock *pBodyBB = llvm::BasicBlock::Create(m_Context, "loopbody", TheFunction);
	llvm::BasicBlock *pAfterBB = llvm::BasicBlock::Create(m_Context, "afterloop", TheFunction);

	// ��������� ������� ������
	m_pBuilder->CreateBr(pCondBB);
	m_pBuilder->SetInsertPoint(pCondBB);
	Var VarBool(Var::BOOLEAN);
	llvm::Value *pCond = ExpressionCaster(pEl->_condition, &VarBool);

	m_pBuilder->CreateCondBr(pCond, pBodyBB, pAfterBB);

	// ���������� ���� �����
	m_pBuilder->SetInsertPoint(pBodyBB);
	llvm::Value *pBody = GenStatement(pEl->_st);
	m_pBuilder->CreateBr(pCondBB);

	// ����� �� �����
	m_pBuilder->SetInsertPoint(pAfterBB);

	return nullptr;
}

llvm::Value * CodeGenerator::GenRepeatStatement(RepeatStatement *pEl) {
	llvm::Function *TheFunction = m_pBuilder->GetInsertBlock()->getParent();

	llvm::BasicBlock *pBodyBB = llvm::BasicBlock::Create(m_Context, "loopbody", TheFunction);
	llvm::BasicBlock *pCondBB = llvm::BasicBlock::Create(m_Context, "loopcond", TheFunction);
	llvm::BasicBlock *pAfterBB = llvm::BasicBlock::Create(m_Context, "afterloop", TheFunction);

	// ���������� ���� �����
	m_pBuilder->CreateBr(pBodyBB);
	m_pBuilder->SetInsertPoint(pBodyBB);
	llvm::Value *pBody = GenStatement(pEl->_st);
	m_pBuilder->CreateBr(pCondBB);

	// ��������� ������� ������
	m_pBuilder->SetInsertPoint(pCondBB);
	Var VarBool(Var::BOOLEAN);
	llvm::Value *pCond = ExpressionCaster(pEl->_condition, &VarBool);

	m_pBuilder->CreateCondBr(pCond, pAfterBB, pBodyBB);

	// ����� �� �����
	m_pBuilder->SetInsertPoint(pAfterBB);

	return nullptr;
}

llvm::Value * CodeGenerator::GenAssignStatement(AssignStatement *pEl) {
	Var *pVar = m_pCurScope->Get<Var>(pEl->_var);

	if (pVar == nullptr)
		throw std::exception((std::string("unknown variable '") + pEl->_var + "'").c_str());
	
	bool ref = pVar->isRef;
	pVar->isRef = false;

	llvm::Value *pAssignValue = ExpressionCaster(pEl->_expr, pVar);

	llvm::Value *pVarValue = m_ValueMap[pVar];

	if (ref)
		pVarValue = m_pBuilder->CreateLoad(pVarValue);

	m_pBuilder->CreateStore(pAssignValue, pVarValue);

	pVar->isRef = ref;
	return nullptr;
}


llvm::Function * CodeGenerator::GenFunctionHeader(Function *pFunc) {
	llvm::Type *pReturnType = GetType(pFunc->_prtype); // ��� ������������� ��������

	unsigned NumOfParams = pFunc->GetNumOfParams(); // ���-�� ���������� �������

	vector<llvm::Type *> pParamTypes(NumOfParams); // ������ ����� ����������

	for (unsigned i = 0; i < NumOfParams; ++i)
		pParamTypes[i] = GetType(pFunc->_params->_params[i].second);
	llvm::FunctionType *FuncType = llvm::FunctionType::get(pReturnType, pParamTypes, false);

	llvm::Function *pFunction = llvm::Function::Create(FuncType, llvm::Function::ExternalLinkage, pFunc->GetID(), m_pMainModule);

	if (pFunction->getName() != pFunc->GetID()) {
		// ���� ���������� ������� � ����� �� ������
		pFunction->eraseFromParent();
		pFunction = m_pMainModule->getFunction(pFunc->GetID());

		if (!pFunction->empty() || pFunction->arg_size() != NumOfParams) {
			// ������. ��������������� �������
			throw std::exception("Function redefenition");
		}
	}

	// ������������� ����� ��� ���� ����������
	unsigned i = 0;
	for (llvm::Function::arg_iterator it = pFunction->arg_begin(); i < NumOfParams; ++it, ++i) {
		string &name = pFunc->_params->_params[i].first;
		it->setName(name);
	}

	return pFunction;
}

llvm::Value * CodeGenerator::GenFunctionBody(Function *pFunc) {
	m_pCurScope = &pFunc->scp; // ������������� ������� ������� ���������

	llvm::Function *pFunction = m_pMainModule->getFunction(pFunc->_ID);

	llvm::BasicBlock *pBB = llvm::BasicBlock::Create(m_Context, "entry", pFunction);
	m_pBuilder->SetInsertPoint(pBB);

	// �������� ������ ��� ��������� � ��������� �� � ������� ��������
	unsigned i = 0;
	for (llvm::Function::arg_iterator it = pFunction->arg_begin(); i < pFunc->GetNumOfParams(); ++it, ++i) {
		auto &inc = pFunc->_params->_params[i];
		llvm::Value *pAlloca = CreateEntryBlockAlloca(pFunction, inc.first, inc.second);
		m_pBuilder->CreateStore(it, pAlloca);
		m_ValueMap[inc.second] = pAlloca;
	}

	// �������� ������ ��� ���������� :)
	for (const auto &i : pFunc->Vars) {
		llvm::Value *pAlloca = CreateEntryBlockAlloca(pFunction, i.first, i.second, m_pCurScope->IsRoot());
		//m_pBuilder->CreateStore(new llvm::Integer, pAlloca);
		m_ValueMap[i.second] = pAlloca;
	}

	// ������������� ����������� ��������
	llvm::Value *pRetVal = nullptr;
	if (pFunc->_prtype->_type != Var::VOID) {
		pRetVal = CreateEntryBlockAlloca(pFunction, pFunc->_ID, pFunc->_prtype);
		m_ValueMap[pFunc->_prtype] = pRetVal;
	}

	llvm::Value *pBody = GenStmntSeq(pFunc->seq);

	if (pRetVal != nullptr)
		m_pBuilder->CreateRet(m_pBuilder->CreateLoad(pRetVal));
	else
		m_pBuilder->CreateRetVoid();

	llvm::verifyFunction(*pFunction);
	m_pOurFPM->run(*pFunction);
	
	//pFunction->dump();
	return pFunction;
}

llvm::Value * CodeGenerator::GenFunction(Function *pFunc) {

	m_ValueMap[pFunc] = GenFunctionHeader(pFunc);
	
	// ������������ ��������� �������
	for (auto &i : pFunc->Funcs) {
		m_ValueMap[i.second] = GenFunctionHeader(i.second);
	}

	
	llvm::Value *pRes = GenFunctionBody(pFunc);
	
	for(auto &i : pFunc->Funcs)
		GenFunction(i.second);

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
	case Statement::S_WHILE:
	{
		WhileStatement *pWhile = dynamic_cast<WhileStatement *>(pStmt);
		return GenWhileStatement(pWhile);
	}
	case Statement::S_REPEAT:
	{
		RepeatStatement *pWhile = dynamic_cast<RepeatStatement *>(pStmt);
		return GenRepeatStatement(pWhile);
	}
	case Statement::S_EMPTY:
		return nullptr;
	
	
	default:
		throw std::exception();
	};
}

llvm::Value * CodeGenerator::ExpressionCaster(Expression *pExp, const Var *pTo) {
	llvm::Value *pExpValue;
	if (pTo->isRef) {
		ExprID *pE = dynamic_cast<ExprID *>(pExp);
		if (pE == nullptr)
			throw std::exception("cannot pass by reference");
		pExpValue = GenExprID(pE, true);
		return pExpValue;
	}

	pExpValue = GenExpression(pExp);
	const Var *pType = pExp->GetVar(m_pCurScope);

	llvm::Instruction::CastOps CastOp;
	std::string CastName = "cast";

	if (pType->_type != pTo->_type) {
		if (pTo->isRef)
			throw std::exception("reference casting attempt");

		llvm::Type *pDestT = GetType(pTo);

		switch (pType->_type) {
		case Var::REAL:
			CastName += "real";
			if (pTo->_type == Var::BOOLEAN)
				return m_pBuilder->CreateFCmpONE(pExpValue, llvm::ConstantFP::getNullValue(pExpValue->getType()));
			else
				CastOp = llvm::Instruction::CastOps::FPToSI;
			break;
		case Var::INTEGER:
		case Var::CHAR:
			CastName += "int";
			if (pTo->_type == Var::REAL)
				CastOp = llvm::Instruction::CastOps::SIToFP;
			else if (pTo->_type == Var::BOOLEAN)
				return m_pBuilder->CreateICmpNE(pExpValue, llvm::ConstantInt::getNullValue(pExpValue->getType()));
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
	llvm::Type *pT;
	switch (pV->_type) {
	case Var::BOOLEAN:
		pT =  llvm::IntegerType::get(m_Context, 1);
		break;
	case Var::CHAR:
		pT = llvm::IntegerType::get(m_Context, 8);
		break;
	case Var::INTEGER:
		pT = llvm::IntegerType::get(m_Context, sizeof(int) * 8);
		break;
	case Var::REAL:
		pT = llvm::Type::getDoubleTy(m_Context);
		break;
	case Var::VOID:
		pT = llvm::Type::getVoidTy(m_Context);
		break;
	}

	if (pV->isRef)
		pT = llvm::PointerType::get(pT, 0);

	return pT;
}



CodeGenerator * CodeGenerator::getInstance() {
	if (m_pInst == nullptr) {
		llvm::InitializeNativeTarget();
		m_pInst = new CodeGenerator();
	}
	return m_pInst;
}

bool CodeGenerator::Generate(Parser *pP) {
	m_pMainModule = new llvm::Module(pP->_ast->_ID, m_Context);

	// ������������� ������������
	m_pOurFPM = new llvm::FunctionPassManager(m_pMainModule);
	m_pOurFPM->add(new llvm::DataLayoutPass(m_pMainModule));
	m_pOurFPM->add(llvm::createBasicAliasAnalysisPass());
	m_pOurFPM->add(llvm::createPromoteMemoryToRegisterPass());
	m_pOurFPM->add(llvm::createInstructionCombiningPass());
	m_pOurFPM->add(llvm::createReassociatePass());
	m_pOurFPM->add(llvm::createGVNPass());
	m_pOurFPM->add(llvm::createCFGSimplificationPass());

	m_pOurFPM->doInitialization();

	// ������ ���������� ���������
	m_pExe = llvm::EngineBuilder(m_pMainModule).setErrorStr(&m_ErrorString).create();

	bool Success = true;
	try {
		GenRoot(pP->_ast);
	}
	catch (std::exception &ex) {
		cout << "Generation failed: " << ex.what() << endl;
		Success = false;
	}
	delete m_pOurFPM;

	return Success;
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
	std::string name = m_pMainModule->getModuleIdentifier();
	llvm::Function *pFunction = m_pExe->FindFunctionNamed(name.c_str());
	std::vector<std::string> v;
	int res = m_pExe->runFunctionAsMain(pFunction, v, nullptr);

	return res;
}