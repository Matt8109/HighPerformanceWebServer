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

TEST(Group, Case) {
  EXPECT_TRUE(true);
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
