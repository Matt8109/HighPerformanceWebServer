#include "lock_free_hash_table.hpp"
#include "markable_pointer.hpp"

namespace lock_free {

class LockFreeHashTable {
public:
  explicit LockFreeHashTable(int num_threads);

  // The destructor is not thread-safe
  ~LockFreeHashTable();

private:
  uint count;       // total item count
  uint size;        // current table size

};

}