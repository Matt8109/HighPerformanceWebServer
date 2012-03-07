#ifndef MCP_BASE_FILE_CACHE_HEADER
#define MCP_BASE_FILE_CACHE_HEADER

#include <cerrno>
#include <fcntl.h>
#include <queue>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <tr1/unordered_map>
#include <unistd.h>

#include "buffer.hpp"
#include "lock.hpp"

namespace base {

using std::pair;
using std::queue;
using std::string;
using std::tr1::hash;
using std::tr1::unordered_map;

struct Node {
public:
  Node(const string& temp_file_name)
      : file_name(temp_file_name),
        pin_count(0),
        file_size(0) { }
  ~Node() {}

  const string file_name;
  Buffer* buf;   // owned here
  int pin_count; // current number of pins on the object
  int file_size;      // size of the object in bytes
};

 // A map from a file_name to its node. Because we want to save
 // space, we use the file_name inside the Node as the key (as a
 // string*) rather than to duplicate that (as a string).
 struct HashStrPtr {
   size_t operator()(const string* a) const {
     hash<string> hasher;
     return hasher(*a);
   }
 };

 struct EqStrPtr {
   bool operator()(const string* a, const string* b) const {
     return *a == *b;
   }
 };

 typedef unordered_map<const string*, Node*, HashStrPtr, EqStrPtr> CacheMap;


// The FileCache maintains a map from file names to their contents,
// stored as 'Buffer's. The sum of all Buffer's stored in the map
// never exceeds the size determined for the cache (at construction
// time).
//
// A request to pin a file that is already in the cache is supposed to
// be very fast. We do so by leveraging reader-writer locks. A cache
// hit needs only to grab a read lock on the map -- it doesn't change
// it -- and to increment the pin count for that file -- which can be
// done with an atomic fetch-and-add.
//
// A cache miss is slower, but we expect them to be less frequent. The
// buffers are connected together in a single-linked FIFO list. That's
// right, for the lab 2 there is no special eviction policy. When
// space is needed, we traverse the list and evict the first
// non-pinned buffer we find. And repeat until enough space was
// cleared.
//
// If not enough unpinned space is found to fit a new request, then
// the pin request may fail.
//
// Thread safety:
//   + pin() and unpin() can be done from different threads
//   + ~FileCache is NOT thread-safe. The caller has to be sure there
//     are no ongoing cache readers before disposing of the cache
//
// Usage:
//   FileCache my_cache(50<<20 /* 50MB */);
//
//   Buffer* buf;
//   int error;
//   CacheHandle h = my_cache.pin("a_file.html", &buf, &error);
//   if (h != 0) {
//     can read contents of *buf
//   } else if (error == 0) {
//     no space on cache. will need to read the file on your own
//   } else /* error < 0 */ {
//     other error reading the file; error contains errno
//   }
//
class FileCache {
public:
  typedef const string* CacheHandle;

  explicit FileCache(int max_size_temp);
  ~FileCache();

  CacheHandle pin(const string& file_name, Buffer** buf, int* error);
  void unpin(CacheHandle h);

  // accessors

  int bytesUsed() const { return bytes_used; }
  int failed() const    { return failed_count; }
  int hits() const      { return hit_count; }
  int maxSize() const   { return max_size; }
  int pins() const      { return pin_count; }

private:
  int bytes_used;
  int failed_count;
  int hit_count;
  int max_size;
  int pin_count;
  RWMutex sync_root;
  CacheMap cache_map;

  CacheHandle checkInCache(const string& file_name, 
                           Buffer** buf,
                           Node** node, 
                           int* error);
  bool clearSpace(int space_needed);
  int readFile(const string& file_name, Buffer** buf, int* error);

  // Non-copyable, non-assignable
  FileCache(FileCache&);
  FileCache& operator=(FileCache&);
};

} // namespace base

#endif // MCP_BASE_FILE_CACHE_HEADER
