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
  sync_root.rLock();

  CacheHandle h = checkInCache(file_name, buf, error);

  if (!h) { // the file wasnt in the cache
    sync_root.unlock(); // give up the read lock

    // try reading the file, dont need the lock yet
    Node* node = new Node;
    node->file_size = readFile(file_name, buf, error);

    sync_root.wLock();  // and get the write lock

    if (!*error) {      // ok we read the file
      if (checkInCache(file_name, buf, error)) {

      }
    }
  }

   __sync_fetch_and_add(&pin_count, 1);

   sync_root.unlock();

  return h;
}

void FileCache::unpin(CacheHandle h) {
}

FileCache::CacheHandle FileCache::checkInCache(const string& file_name, 
                                    Buffer** buf, 
                                    int* error) {
  CacheHandle h = 0;
  CacheMap::iterator it = cache_map.find(&file_name);

 if (it != cache_map.end()) { // already in the cache
    h = it->first;
    Node* node = it->second;
    *buf = node->buf;
    *error = 0;

   __sync_fetch_and_add(&hit_count, 1);

   return h;
  } else {
    return 0;             // no dice
  }
}

int FileCache::readFile(const string& file_name, Buffer** buf, int* error) {
  int file_size = 0;
  int fd = open(file_name.c_str(), O_RDONLY);

  if (fd < 0) {
    LOG(LogMessage::WARNING) << "could not open " << file_name
                             << ": " << strerror(errno);
    *error = errno; // pass the error back
  } else {
    struct stat stat_buf;
    fstat(fd, &stat_buf);

    //read the file into the buffer
    Buffer* new_buf = new Buffer;
    size_t to_read = stat_buf.st_size;
    file_size = to_read;
    while (to_read > 0) {
      new_buf->reserve(new_buf->BlockSize);

      int bytes_read = read(fd, new_buf->writePtr(), new_buf->writeSize());

      if (bytes_read < 0 ) {

        LOG(LogMessage::ERROR) << "cant read file " << file_name
                               << ": " << strerror(errno);
        close(fd);
        delete new_buf;
        *error = errno;
      }

      to_read -= bytes_read;
    }

    if (to_read != 0) {
      LOG(LogMessage::WARNING) << "file change while reading " << file_name;
    }

    new_buf->advance(stat_buf.st_size - to_read);

    if (!*error)
      buf = &new_buf;  // set the buffer
  }

  return file_size;
}

}  // namespace base