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
  Node* node;
  sync_root.rLock();

  CacheHandle h = checkInCache(file_name, buf, node, error);

  if (h) { // the file was in the cache
    __sync_fetch_and_add(&node->pin_count, 1);
    __sync_fetch_and_add(&hit_count, 1);
  }
  else {   // the file wasnt in the cache
    sync_root.unlock(); // give up the read lock

    // try reading the file, dont want to grab a lock while waiting on the fs
    int file_size;
    Node* node;

    file_size = readFile(file_name, buf, error);

    sync_root.wLock(); 

    if (!*error) {  
      h = checkInCache(file_name, buf, node, error);
      if (h) {
        // someone added the file before we did
        __sync_fetch_and_add(&node->pin_count, 1);
        __sync_fetch_and_add(&hit_count, 1);
      } else {
        // ok we need to add the new node
        node = new Node(file_name);
        node->file_size = file_size;
        node->file_name = file_name;

        cache_map.insert(CacheMap::value_type(&node->file_name, node));
      }
    } else {
      failed_count++;
      delete node;
      delete buf;
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
                                    Node* node, 
                                    int* error) {
  CacheHandle h = 0;
  CacheMap::iterator it = cache_map.find(&file_name);

 if (it != cache_map.end()) { // already in the cache
    h = it->first;
    node = it->second;
    *buf = node->buf;
    *error = 0;
   return h;
  } else {
    return 0;             // no dice
  }
}

bool FileCache::clearSpace(int space_needed) {
  if (bytes_used + space_needed <= max_size)
    return true;  

  queue<pair<CacheHandle, Node*> > free_queue;

  for (CacheMap::iterator it = cache_map.begin(); 
       it != cache_map.end(); 
       it++) {
    if (it->second->pin_count == 0) {
      pair<CacheHandle, Node*> pair;
      pair.first = it->first;
      pair.second = it->second;

      free_queue.push(pair);
    }
  }

  // now try removing until we have space
  while (bytes_used + space_needed > max_size && free_queue.size() != 0) {
    pair<CacheHandle, Node*> pair = free_queue.front();
    free_queue.pop();

    cache_map.erase(pair.first);
    bytes_used -= pair.second->file_size;

    delete pair.second->buf; // delete file bug
    delete pair.second;      // delete the node itself
  }

  if (bytes_used + space_needed <= max_size)
    return true;  // cleared out enough space
  else
    return false; // couldnt free the space
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