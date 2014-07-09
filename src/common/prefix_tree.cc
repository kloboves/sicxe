#include "common/prefix_tree.h"
#include "common/types.h"

using std::string;
using std::vector;

namespace sicxe {

PrefixTree::PrefixTree() : root_(new Node) {}
PrefixTree::~PrefixTree() {}

PrefixTree::Node::Node() : is_end(false) {}

bool PrefixTree::Insert(const std::string& str) {
  if (str.empty()) {
    return false;
  }
  Node* node = root_.get();
  for (size_t i = 0; i < str.size(); i++) {
    uint8 c = static_cast<uint8>(str[i]);
    if (node->children[c] == nullptr) {
      node->children[c].reset(new Node);
    }
    node = node->children[c].get();
  }
  if (node->is_end) {
    return false;
  }
  node->is_end = true;
  return true;
}

void PrefixTree::Find(const string& str, vector<string>* result) const {
  if (str.empty()) {
    return;
  }

  const Node* node = root_.get();
  for (size_t i = 0; i < str.size(); i++) {
    uint8 c = static_cast<uint8>(str[i]);
    if (node->children[c] == nullptr) {
      return;
    }
    node = node->children[c].get();
  }

  if (node->is_end) {
    // exact match, return only this entry
    result->push_back(str);
    return;
  }

  // search recursively for all prefix matches
  string current = str;
  FindRecursive(node, &current, result);
}

void PrefixTree::FindRecursive(const Node* node, string* current,
                               vector<string>* result) const {
  if (node->is_end) {
    result->push_back(*current);
  }

  for (size_t i = 0; i < (1 << 8); i++) {
    if (node->children[i] != nullptr) {
      current->push_back(static_cast<char>(i));
      FindRecursive(node->children[i].get(), current, result);
      current->pop_back();
    }
  }
}


}  // namespace sicxe
