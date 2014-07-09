#ifndef COMMON_PREFIX_TREE_H
#define COMMON_PREFIX_TREE_H

#include <memory>
#include <string>
#include <vector>
#include "common/macros.h"

namespace sicxe {

class PrefixTree {
 public:
  DISALLOW_COPY_AND_MOVE(PrefixTree);

  PrefixTree();
  ~PrefixTree();

  bool Insert(const std::string& str);
  void Find(const std::string& str, std::vector<std::string>* result) const;

 private:
  struct Node {
    Node();

    bool is_end;
    std::unique_ptr<Node> children[1 << 8];
  };

  void FindRecursive(const Node* node, std::string* current,
                     std::vector<std::string>* result) const;

  std::unique_ptr<Node> root_;
};

}  // namespace sicxe

#endif  // COMMON_PREFIX_TREE_H
