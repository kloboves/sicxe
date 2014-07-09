#ifndef ASSEMBLER_NODE_VISITOR_H
#define ASSEMBLER_NODE_VISITOR_H

#include "assembler/node.h"

namespace sicxe {
namespace assembler {

class NodeVisitor {
 public:
  virtual void VisitNode(node::Empty* node) = 0;
  virtual void VisitNode(node::InstructionF1* node) = 0;
  virtual void VisitNode(node::InstructionF2* node) = 0;
  virtual void VisitNode(node::InstructionFS34* node) = 0;
  virtual void VisitNode(node::DirectiveStart* node) = 0;
  virtual void VisitNode(node::DirectiveEnd* node) = 0;
  virtual void VisitNode(node::DirectiveOrg* node) = 0;
  virtual void VisitNode(node::DirectiveEqu* node) = 0;
  virtual void VisitNode(node::DirectiveUse* node) = 0;
  virtual void VisitNode(node::DirectiveLtorg* node) = 0;
  virtual void VisitNode(node::DirectiveBase* node) = 0;
  virtual void VisitNode(node::DirectiveNobase* node) = 0;
  virtual void VisitNode(node::DirectiveExtdef* node) = 0;
  virtual void VisitNode(node::DirectiveExtref* node) = 0;
  virtual void VisitNode(node::DirectiveMemInit* node) = 0;
  virtual void VisitNode(node::DirectiveMemReserve* node) = 0;
  virtual void VisitNode(node::DirectiveInternalLiteral* node) = 0;
};

class ConstNodeVisitor {
 public:
  virtual void VisitNode(const node::Empty* node) = 0;
  virtual void VisitNode(const node::InstructionF1* node) = 0;
  virtual void VisitNode(const node::InstructionF2* node) = 0;
  virtual void VisitNode(const node::InstructionFS34* node) = 0;
  virtual void VisitNode(const node::DirectiveStart* node) = 0;
  virtual void VisitNode(const node::DirectiveEnd* node) = 0;
  virtual void VisitNode(const node::DirectiveOrg* node) = 0;
  virtual void VisitNode(const node::DirectiveEqu* node) = 0;
  virtual void VisitNode(const node::DirectiveUse* node) = 0;
  virtual void VisitNode(const node::DirectiveLtorg* node) = 0;
  virtual void VisitNode(const node::DirectiveBase* node) = 0;
  virtual void VisitNode(const node::DirectiveNobase* node) = 0;
  virtual void VisitNode(const node::DirectiveExtdef* node) = 0;
  virtual void VisitNode(const node::DirectiveExtref* node) = 0;
  virtual void VisitNode(const node::DirectiveMemInit* node) = 0;
  virtual void VisitNode(const node::DirectiveMemReserve* node) = 0;
  virtual void VisitNode(const node::DirectiveInternalLiteral* node) = 0;
};

}  // namespace assembler
}  // namespace sicxe

#endif  // ASSEMBLER_NODE_VISITOR_H
