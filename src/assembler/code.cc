#include "assembler/code.h"

#include "assembler/block_table.h"
#include "assembler/literal_table.h"
#include "assembler/symbol_table.h"

using std::string;

namespace sicxe {
namespace assembler {

Code::Code() : text_file_(nullptr), start_address_(0), end_address_(0), entry_point_(0) {}
Code::~Code() {}

const TextFile* Code::text_file() const {
  return text_file_;
}

void Code::set_text_file(const TextFile* file) {
  text_file_ = file;
}

const Code::NodeList& Code::nodes() const {
  return nodes_;
}

Code::NodeList* Code::mutable_nodes() {
  return &nodes_;
}

const SymbolTable* Code::symbol_table() const {
  return symbol_table_.get();
}

void Code::set_symbol_table(SymbolTable* table) {
  symbol_table_.reset(table);
}

const BlockTable* Code::block_table() const {
  return block_table_.get();
}

void Code::set_block_table(BlockTable* table) {
  block_table_.reset(table);
}

const LiteralTable* Code::literal_table() const {
  return literal_table_.get();
}

void Code::set_literal_table(LiteralTable* table) {
  literal_table_.reset(table);
}

const string& Code::program_name() const {
  return program_name_;
}

void Code::set_program_name(const std::string& name) {
  program_name_ = name;
}

uint32 Code::start_address() const {
  return start_address_;
}

void Code::set_start_address(uint32 the_start_address) {
  start_address_ = the_start_address;
}

uint32 Code::end_address() const {
  return end_address_;
}

void Code::set_end_address(uint32 the_end_address) {
  end_address_ = the_end_address;
}

uint32 Code::entry_point() const {
  return entry_point_;
}

void Code::set_entry_point(uint32 the_entry_point) {
  entry_point_ = the_entry_point;
}

}  // namespace assembler
}  // namespace sicxe
