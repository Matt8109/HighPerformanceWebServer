#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#include <tr1/random>

#include "buffer.hpp"
#include "callback.hpp"
#include "file_cache.hpp"
#include "logging.hpp"
#include "test_unit.hpp"
#include "thread.hpp"


namespace {

using std::string;
using std::vector;

using base::Buffer;
using base::Callback;
using base::FileCache;
using base::LogMessage;
using base::makeThread;
using base::makeCallableOnce;
using base::makeCallableMany;

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
  Tester(FileCache* tempFileCache)
      : fileCache(tempFileCache) { }
  ~Tester() { }

  void PinFiles() {
    int error = 0;
    Buffer* buff;
    CacheHandle cacheHandles[8];

    fileCache->pin("a.html", &buff, &error);

    for (int i = 0; i < 10; i++) {
      int x = 0;
      cacheHandles[x++] = fileCache->pin("a.html", &buff, &error);
      cacheHandles[x++] = fileCache->pin("b.html", &buff, &error);
      cacheHandles[x++] = fileCache->pin("c.html", &buff, &error);
      cacheHandles[x++] = fileCache->pin("1.html", &buff, &error);
      cacheHandles[x++] = fileCache->pin("2.html", &buff, &error);
      cacheHandles[x++] = fileCache->pin("3.html", &buff, &error);
      cacheHandles[x++] = fileCache->pin("4.html", &buff, &error);
      cacheHandles[x++] = fileCache->pin("5.html", &buff, &error);

      for (int i = 1; i < 8; i++) {
        if (cacheHandles[i] != NULL)          // only delete if it was added
          fileCache->unpin(cacheHandles[i]);
      }
    }  
  }

private:
  FileCache* fileCache;
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
  EXPECT_EQ(file_cache.failed(), 0);
}

TEST(Basic, FileRead) { // test we are actually returning the right file
  int error = 0;
  Buffer* buff;
  FileCache file_cache(50 << 20); //50 megs

  file_cache.pin("a.html", &buff, &error);

  string read_string(buff->readPtr(), buff->readSize());
  EXPECT_EQ(read_string[0], 'x');

  file_cache.pin("1.html", &buff, &error);

  read_string = string(buff->readPtr(), buff->readSize());
  EXPECT_EQ(read_string[0], '1');

  //try loading a file that doesnt exist
  file_cache.pin("12345.html", &buff, &error);
  EXPECT_NEQ(error, 0);
  EXPECT_EQ(file_cache.failed(), 1);
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
  EXPECT_EQ(file_cache.pins(), 2);
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
  EXPECT_EQ(file_cache.pins(), 30);
  EXPECT_EQ(file_cache.bytesUsed(), 2500); //only b.html should still be there
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

  EXPECT_EQ(file_cache.hits(), 80); // 8 items, requested 11 times, first misses
  EXPECT_EQ(file_cache.pins(), 88);
  EXPECT_EQ(file_cache.bytesUsed(), 20240);
}

TEST(Statistics, Unpinning) {
  int error = 0;
  Buffer* buff;
  CacheHandle handle;
  FileCache file_cache(50 << 20); //50 megs

  file_cache.pin("a.html", &buff, &error);
  handle = file_cache.pin("a.html", &buff, &error);
  file_cache.unpin(handle);

  file_cache.pin("b.html", &buff, &error);
  file_cache.pin("1.html", &buff, &error);

  // request all the file again to test hits
  file_cache.pin("b.html", &buff, &error);
  file_cache.pin("1.html", &buff, &error);

  EXPECT_EQ(file_cache.hits(), 3);
  EXPECT_EQ(file_cache.pins(), 6);
}

// makes sure we are puring unpinned items
TEST(Statistics, PurgeUnpinned) {
  int error = 0;
  Buffer* buff;
  CacheHandle handle;
  FileCache file_cache(5001);

  file_cache.pin("a.html", &buff, &error);
  handle = file_cache.pin("b.html", &buff, &error);

  file_cache.unpin(handle);

  file_cache.pin("1.html", &buff, &error);
  file_cache.pin("a.html", &buff, &error);

  EXPECT_EQ(file_cache.hits(), 1);
  EXPECT_EQ(file_cache.pins(), 4);
}

TEST(Statistics, FullFailToAdd) {
  int error = 0;
  Buffer* buff;
  CacheHandle handle;
  FileCache file_cache(250);

  file_cache.pin("a.html", &buff, &error);
  handle = file_cache.pin("b.html", &buff, &error);

  // the two conditions when  the cache is full
  EXPECT_EQ(handle, 0);
  EXPECT_EQ(error, 0);
}

TEST(MultipleActors, PinsAndUnpinsLargeCache) {
  FileCache file_cache(50 << 20);
  Tester tester(&file_cache);

  Callback<void>* pinCallback = makeCallableMany(&Tester::PinFiles, &tester);

  pthread_t pinThreadOne = makeThread(pinCallback);
  pthread_t pinThreadTwo = makeThread(pinCallback);

  pthread_join(pinThreadOne, NULL);
  pthread_join(pinThreadTwo, NULL);

  EXPECT_EQ(file_cache.pins(), 162); // at least one file should be pinned
  EXPECT_GT(file_cache.hits(), 19); // one file is constantly hit
  EXPECT_EQ(file_cache.bytesUsed(), 20240);

  delete pinCallback;
}

TEST(MultipleActors, PinsAndUnpinsSmallCache) {
  FileCache file_cache(10240);
  Tester tester(&file_cache);

  Callback<void>* pinCallback = makeCallableMany(&Tester::PinFiles, &tester);

  pthread_t pinThreadOne = makeThread(pinCallback);
  pthread_t pinThreadTwo = makeThread(pinCallback);
  pthread_t pinThreadThree = makeThread(pinCallback);

  pthread_join(pinThreadOne, NULL);
  pthread_join(pinThreadTwo, NULL);
  pthread_join(pinThreadThree, NULL);

  EXPECT_EQ(file_cache.pins(), 243); // at least one file should be pinned
  EXPECT_GT(file_cache.hits(), 19); // one file is constantly hit
  EXPECT_GT(10240, file_cache.bytesUsed()); // expect cache size less than max

  delete pinCallback;
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
