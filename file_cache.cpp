#include "file_cache.hpp"
#include "logging.hpp"

namespace base {

FileCache::FileCache(int max_size_temp)
    : bytes_used(0),
      failed_count(0),
      hit_count(0),
      max_size(max_size_temp),
      pin_count(0) { }

// REQUIRES: No ongoing pin. This code assumes no one is using the
// cache anymore
FileCache::~FileCache() {
  for (CacheMap::iterator it = cache_map.begin(); 
       it != cache_map.end(); 
       it++) {
    delete it->second->buf;
    delete it->second;
  }  
}

FileCache::CacheHandle FileCache::pin(const string& file_name,
                                      Buffer** buf,
                                      int* error) {
  Node* node = NULL;
  Buffer* temp_buf;
  int* error_num = new int;
  *error_num =0;

  __sync_fetch_and_add(&pin_count, 1);

  sync_root.rLock();

  CacheHandle h = checkInCache(file_name, &temp_buf, &node, error_num);

  if (h) {
    __sync_fetch_and_add(&node->pin_count, 1);
    __sync_fetch_and_add(&hit_count, 1);
  }
  else { 
    sync_root.unlock(); // dont hold the lock while waiting on fs

    int file_size;
    Node* node = NULL;

    file_size = readFile(file_name, &temp_buf, error_num);

    sync_root.wLock(); 

    if (!*error_num) {  
      h = checkInCache(file_name, &temp_buf, &node, error_num);
      if (h) {
        // someone added the file before we did
        __sync_fetch_and_add(&node->pin_count, 1);
        __sync_fetch_and_add(&hit_count, 1);
      } else { // ok we need to add the new node
        if (clearSpace(file_size)) {
          node = new Node(file_name);
          node->file_size = file_size;
          node->buf = temp_buf;

          cache_map.insert(CacheMap::value_type(&node->file_name, node));
          bytes_used += node->file_size;

          h = reinterpret_cast<CacheHandle>(&node->file_name);
        } else {  // out of space          
          failed_count++;
          delete temp_buf;
        }
      }
    } else {
      failed_count++;
      delete node;
    }
  }

   sync_root.unlock();

   if (h != 0) {
     Buffer* ret_buf = new Buffer;
     ret_buf->copyFrom(temp_buf);
     *buf = ret_buf;
  } else {
    *buf = NULL;
  }

  if (error)
    *error = *error_num;
  delete error_num;

  return h;
}

void FileCache::unpin(CacheHandle h) {
  sync_root.rLock();

  CacheMap::iterator it = cache_map.find(h);
  if (it != cache_map.end()) {
    h = it->first;
    Node* node = it->second;

    if (node->pin_count != 0)
     __sync_fetch_and_sub(&node->pin_count, 1);
    else
       LOG(LogMessage::FATAL) << "Attempted to unpin unpinned item.";
  } else {
    LOG(LogMessage::FATAL) << "Nonexistant cache handle.";
  }

  sync_root.unlock();

}

FileCache::CacheHandle FileCache::checkInCache(const string& file_name, 
                                              Buffer** buf,
                                              Node** node, 
                                              int* error) {
  CacheHandle h = 0;
  CacheMap::iterator it = cache_map.find(&file_name);

 if (it != cache_map.end()) {
    h = it->first;
    *node = it->second;
    *buf = (*node)->buf;
    *error = 0;
    return h;
  } else {
    return 0;
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

    delete pair.second->buf; 
    delete pair.second;
  }

  if (bytes_used + space_needed <= max_size)
    return true; 
  else
    return false;
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
      *buf = new_buf;  // set the buffer
  }

  return file_size;
}

}  // namespace base