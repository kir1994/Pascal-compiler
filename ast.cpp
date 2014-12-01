//#include "ast.h"
//#include <ostream>
//
//// Красивая печать.
//// ================
//
//static const int INDENT_SIZE = 2;
//
//static void PrintIndent(std::ostream &out, int indent) {
//  std::string Indent(indent * INDENT_SIZE, ' ');
//  out << Indent;
//}
//
//static void PrintBinaryOp(BinaryOp op, std::ostream &out) {
//  switch (op) {
//  case ADD:
//    out << " + ";
//    break;
//  case SUB:
//    out << " - ";
//    break;
//  default:
//    break;
//  }
//}
//
//
//// Печать выражений.
//// --------------------------------------------------------------------
//
//void ExprErrorNode::Format(std::ostream &out) { out << "<" << Message << ">"; }
//
//void ConstNode::Format(std::ostream &out) { out << Val; }
//
//void VarNode::Format(std::ostream &out) { out << Name; }
//
//void BinaryNode::Format(std::ostream &out) {
//  LHS->Format(out);
//  PrintBinaryOp(Op, out);
//  RHS->Format(out);
//}
//
//// Печать операторов
//// --------------------------------------------------------------------
//
//void StmtErrorNode::Format(std::ostream &out, int indent) {
//  PrintIndent(out, indent);
//  out << "<" << Message << ">" << std::endl;
//}
//
//void SeqNode::Format(std::ostream &out, int indent) {
//  for (auto stmt : Statements) {
//    stmt->Format(out, indent);
//  }
//}
//
//void AssignNode::Format(std::ostream &out, int indent) {
//  PrintIndent(out, indent);
//  out << Name << " = ";
//  RHS->Format(out);
//  out << std::endl;
//}
//
//void IfNode::Format(std::ostream &out, int indent) {
//  PrintIndent(out, indent);
//  out << "if ";
//  Cond->Format(out);
//  PrintCompareOp(Op, out);
//  out << std::endl;
//  Then->Format(out, indent + 1);
//
//  if (Else != nullptr) {
//    PrintIndent(out, indent);
//    out << "else" << std::endl;
//    Else->Format(out, indent + 1);
//  }
//
//  out << "end" << std::endl;
//}
//
//void PrintNode::Format(std::ostream &out, int indent) {
//  PrintIndent(out, indent);
//  out << "print ";
//  RHS->Format(out);
//  out << std::endl;
//}
//
//void InputNode::Format(std::ostream &out, int indent) {
//  PrintIndent(out, indent);
//  out << "input " << Name;
//  out << std::endl;
//}
