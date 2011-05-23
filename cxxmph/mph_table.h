#include "mph_index.h"

// String to string map working on mmap'ed memory

class MPHTable {
 public:
  typedef StringPiece key_type;
  typedef StringPiece data_type;
  typedef std::pair<StringPiece, StringPiece> value_type;
  template <class ForwardIterator>
  bool Reset(ForwardIterator begin, ForwardIterator end);
 private:
  char* data_;
  vector<uint64_t> offsets_;
  MPHIndex index_;
};
