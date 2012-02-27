#include "file_cache.hpp"
#include "logging.hpp"

namespace base {

FileCache::FileCache(int max_size){
}

// REQUIRES: No ongoing pin. This code assumes no one is using the
// cache anymore
FileCache::~FileCache() {
}

FileCache::CacheHandle FileCache::pin(const string& file_name,
                                      Buffer** buf,
                                      int* error) {
  CacheHandle h = 0;
  return h;
}

void FileCache::unpin(CacheHandle h) {
}

}  // namespace base
