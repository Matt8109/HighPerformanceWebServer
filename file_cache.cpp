#include "file_cache.hpp"
#include "logging.hpp"

namespace base {

FileCache::FileCache(int max_size_temp)
    : max_size(max_size_temp) {
}

// REQUIRES: No ongoing pin. This code assumes no one is using the
// cache anymore
FileCache::~FileCache() {
}

FileCache::CacheHandle FileCache::pin(const string& file_name,
                                      Buffer** buf,
                                      int* error) {
  CacheHandle h = 0;

  sync_root.rLock();
  CacheMap::iterator it = cache_map.find(&file_name);
 
 if (it != cache_map.end()) {

    h = it->first;
    Node* node = it->second;
    *buf = node->buf;

   __sync_fetch_and_add(&hit_count, 1);

  } else {
    // the file wasnt in the cache
    sync_root.unlock(); // give up the read lock
    sync_root.wLock();  // and get the write lock


  }


  sync_root.unlock();
  __sync_fetch_and_add(&pin_count, 1);

  return h;
}

void FileCache::unpin(CacheHandle h) {
}

int readFile(const string& file_name, Buffer** buff, int* error) {
  int fd = open(file_name.c_str(), O_RDONLY);
  if (fd < 0) {
    LOG(LogMessage::WARNING) << "could not open " << file_name
                             << ": " << strerror(errno);
 
  } else {
    struct stat stat_buf;
    fstat(fd, &stat_buf);
  }
}

}  // namespace base