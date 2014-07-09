#ifndef ASSEMBLER_BLOCK_TABLE_H
#define ASSEMBLER_BLOCK_TABLE_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "common/macros.h"
#include "common/types.h"

namespace sicxe {

class TextFile;

namespace assembler {

class BlockTable {
 public:
  DISALLOW_COPY_AND_MOVE(BlockTable);

  struct Entry {
    Entry(int the_block, const std::string& the_name);

    int block;
    std::string name;

    uint32 start;
    uint32 size;
  };

  typedef std::vector<std::unique_ptr<Entry> > EntryVector;

  BlockTable();
  ~BlockTable();

  const Entry* GetBlock(int block) const;
  Entry* GetBlock(int block);
  const Entry* FindName(const std::string& name) const;
  Entry* FindNameOrCreate(const std::string& name);
  void OrderBlocks(uint32* start, uint32* end);

  void OutputToTextFile(TextFile* file) const;

  const EntryVector& entries() const;

 private:
  EntryVector entries_;
  std::map<std::string, Entry*> name_map_;
};

}  // namespace assembler
}  // namespace sicxe

#endif  // ASSEMBLER_BLOCK_TABLE_H
