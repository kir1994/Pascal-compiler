#pragma once

#include "commondf.h"

#include <llvm/Analysis/Passes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/PassManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Transforms/Scalar.h>

#include <unordered_map>
#include <deque>

#include "parser.h"

class CodeGenerator {
private:
	static CodeGenerator *m_pInst;
	CodeGenerator();
	virtual ~CodeGenerator();
private:
	llvm::LLVMContext &m_Context;
	llvm::Module *m_pMainModule;
	llvm::IRBuilder<> *m_pBuilder;
	string m_ErrorString;

	std::unordered_map<ScopableNode *, llvm::Value *> m_ValueMap;
	
	Scope *m_pCurScope;

	llvm::Value * GenRoot(Root *pEl);

	llvm::Value * GenStatement(Statement *pEl);
	llvm::Value * GenStmntSeq(StatementSeq *pEl);
	llvm::Value * GenForStatement(ForStatement *pEl);
	llvm::Value * GenProcCallStatement(ProcCallStatement *pEl);
	llvm::Value * GenAssignStatement(AssignStatement *pEl);
	llvm::Value * GenRepeatStatement(RepeatStatement *pEl);
	llvm::Value * GenWhileStatement(WhileStatement *pEl);
	llvm::Value * GenIfStatement(IfStatement *pEl);

	llvm::Value * GenExpression(Expression *pEl);
	llvm::Value * GenExprConst(ExprConst *pEl);
	llvm::Value * GenExprID(ExprID *pEl);
	llvm::Value * GenBinaryOp(BinaryOp *pEl);
	llvm::Value * GenFuncCallExpr(FuncCallExpr *pEl);
	llvm::Value * GenCondition(Condition *pEl);
	

	llvm::Value * GenConst(Const *pEl);
	llvm::Value * GenVar(Var *pVar);
	
	llvm::Function * GenFunctionHeader(Function *pFunc);
	llvm::Value * GenFunctionBody(Function *pFunc);
	llvm::Value * GenFunction(Function *pEl);

	llvm::Type * GetType(const Var *pV);

	llvm::Value * ExpressionCaster(Expression *pExp, const Var *pTo) {

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

	llvm::Value *CreateEntryBlockAlloca(llvm::Function *TheFunction,
		const std::string &VarName, Var *pVarType, bool IsGlobal = false) {

		if (IsGlobal) {
			return new llvm::GlobalVariable(*m_pMainModule, GetType(pVarType), false, llvm::GlobalVariable::LinkageTypes::CommonLinkage,
				llvm::Constant::getNullValue(GetType(pVarType)), VarName);
		}

		llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
			TheFunction->getEntryBlock().begin());

		return TmpB.CreateAlloca(GetType(pVarType), 0,
			VarName.c_str());
	}

public:

	static CodeGenerator * getInstance();
	void Generate(Parser *pP);
	void Release();
	void Dump();
	int Execute();
};