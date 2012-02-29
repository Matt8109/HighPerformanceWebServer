#include <fcntl.h>
#include <errno.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>
#include <string>

#include <tr1/random>

#include "buffer.hpp"
#include "callback.hpp"
#include "file_cache.hpp"
#include "logging.hpp"
#include "thread.hpp"
#include "test_unit.hpp"

namespace {

using std::string;
using std::vector;

using base::Buffer;
using base::Callback;
using base::FileCache;
using base::LogMessage;
using base::makeCallableOnce;
using base::makeThread;

typedef FileCache::CacheHandle CacheHandle;

//
// Support for creating test files
//

class FileFixture {
public:
  FileFixture(const char** files, size_t files_size)
    : files_(files), files_size_(files_size) {  }
  ~FileFixture() { }

  void startUp();
  void tearDown();

private:
  const char** files_;
  size_t files_size_;

  vector<int> fds_;
  vector<string> names_;

  void createFile(const char* file, size_t size, char c);
  void deleteFiles();

};

class Tester {
public:
  Tester() { }
  ~Tester() { }

};

void FileFixture::startUp() {
  for (size_t i = 0; i < files_size_; i += 3) {
    size_t size = atoi(files_[i+1]);
    createFile(files_[i], size, *files_[i+2]);
  }
}

void FileFixture::tearDown() {
  for (size_t i = 0; i < fds_.size(); i++) {
    close(fds_[i]);
    unlink(names_[i].c_str());
  }
}

void FileFixture::createFile(const char* name, size_t size, char c) {
  unlink(name);
  int fd = creat(name, S_IRUSR | S_IRGRP);
  if (fd < 0) {
    LOG(LogMessage::ERROR) << "can't create file " << string(name)
                           << ": " << strerror(errno);
    close(fd);
    unlink(name);
    exit(1);
  }

  string content(size, c);
  int bytes = ::write(fd, content.c_str(), size);
  if (bytes < 0) {
    close(fd);
    unlink(name);
    LOG(LogMessage::ERROR) << "can't write to " << string(name)
                           << ": " << *strerror(errno);
    exit(1);
  }

  fds_.push_back(fd);
  names_.push_back(name);
}

//
// Test cases
//

TEST(Basic, Init) {
  FileCache file_cache(500);

  EXPECT_EQ(file_cache.maxSize(), 500);
  EXPECT_EQ(file_cache.bytesUsed(), 0);
  EXPECT_EQ(file_cache.hits(), 0);
  EXPECT_EQ(file_cache.pins(), 0);
}

TEST(Statistics, Basic) {
  int error = 0;
  Buffer* buff;
  CacheHandle handle;
  FileCache file_cache(50 << 20); //50 megs

  handle = file_cache.pin("a.html", &buff, &error);

  EXPECT_NEQ(handle, 0); // will be non-zero if file was read

  handle = file_cache.pin("a.html", &buff, &error);

  EXPECT_NEQ(handle, 0); // will be non-zero if file was read

  EXPECT_EQ(file_cache.maxSize(), 50 << 20);
  EXPECT_EQ(file_cache.bytesUsed(), 2500);
  EXPECT_EQ(file_cache.hits(), 1);
  EXPECT_EQ(file_cache.pins(), 1);
  EXPECT_EQ(error, 0); // will be non-zero if file was read
}

TEST(Statistics, CacheThrash) {
  int error = 0;
  Buffer* buff;
  CacheHandle handle;
  FileCache file_cache(2501); //prevents any one item from staying in the cache

  // all of these files should be forced out of the cache due to the small
  // cache size, thus ensuring that we dont have any cache hits
  for (int i = 0; i < 10; i++) {
    handle = file_cache.pin("a.html", &buff, &error);
    file_cache.unpin(handle);

    handle = file_cache.pin("1.html", &buff, &error);
    file_cache.unpin(handle);

    handle = file_cache.pin("b.html", &buff, &error);
    file_cache.unpin(handle);
  }

  EXPECT_EQ(file_cache.hits(), 0);
  EXPECT_EQ(file_cache.pins(), 0);
}

TEST(Statistics, LargeCacheNoPurges) {
  int error = 0;
  Buffer* buff;
  FileCache file_cache(50 << 20); //50 megs

  for (int i = 0; i < 11; i++) {
    file_cache.pin("a.html", &buff, &error);
    file_cache.pin("b.html", &buff, &error);
    file_cache.pin("c.html", &buff, &error);
    file_cache.pin("1.html", &buff, &error);
    file_cache.pin("2.html", &buff, &error);
    file_cache.pin("3.html", &buff, &error);
    file_cache.pin("4.html", &buff, &error);
    file_cache.pin("5.html", &buff, &error);
  }

  EXPECT_EQ(file_cache.hits(), 50); // 5 items, requested 11 times, first misses
  EXPECT_EQ(file_cache.pins(), 8);
}

TEST(Statistics, Unpinning) {
  int error = 0;
  Buffer* buff;
  CacheHandle handle;
  FileCache file_cache(50 << 20); //50 megs

  handle = file_cache.pin("a.html", &buff, &error);
  handle = file_cache.pin("a.html", &buff, &error);
  file_cache.unpin(handle);

  handle = file_cache.pin("b.html", &buff, &error);
  handle = file_cache.pin("1.html", &buff, &error);

  // request all the file again to test hits
  handle = file_cache.pin("b.html", &buff, &error);
  handle = file_cache.pin("1.html", &buff, &error);

  EXPECT_EQ(file_cache.hits(), 3);
  EXPECT_EQ(file_cache.pins(), 2);
}

}  // unnamed namespace

int main(int argc, char *argv[]) {

  // Creates tests throught the fixtures on the local directory.  The
  // test cases can refer to these files.  The fixture will clean up
  // at when it is tore town.
  const char* files[] =  { "a.html", "2500", "x",
                           "b.html", "2500", "y",
                           "c.html", "5000", "z",
                           "1.html", "2048", "1",
                           "2.html", "2048", "2",
                           "3.html", "2048", "3",
                           "4.html", "2048", "4",
                           "5.html", "2048", "5"  };
  size_t size = sizeof(files)/sizeof(files[0]);
  FileFixture ff(files, size);
  ff.startUp();

  int res = RUN_TESTS(argc, argv);

  ff.tearDown();
  return res;
}
