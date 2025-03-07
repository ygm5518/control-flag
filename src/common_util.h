// Copyright (c) 2021 Niranjan Hasabnis and Justin Gottschlich
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef SRC_COMMON_UTIL_H_
#define SRC_COMMON_UTIL_H_

#include <sys/time.h>
#include <tree_sitter/api.h>
#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <exception>
#include <iostream>

#include "parser.h"

//----------------------------------------------------------------------------
class cf_string_exception: public std::exception {
 public:
  explicit cf_string_exception(const std::string& message) :
    message_(message) {}
  virtual const char* what() const noexcept { return message_.c_str(); }
 private:
  std::string message_;
};

class cf_file_access_exception: public cf_string_exception {
 public:
  explicit cf_file_access_exception(const std::string& error) :
    cf_string_exception("File access failed: " + error) {}
};

class cf_parse_error: public cf_string_exception {
 public:
  explicit cf_parse_error(const std::string& expression) :
    cf_string_exception("Parse error in expression:" + expression) {}
};

class cf_unexpected_situation: public cf_string_exception {
 public:
  explicit cf_unexpected_situation(const std::string& error) :
    cf_string_exception("Assert failed: " + error) {}
};

inline void cf_assert(bool value, const std::string& message) {
  if (value == false) throw cf_unexpected_situation(message);
}

inline void cf_assert(bool value, const std::string& message,
                     const TSNode& node) {
  cf_assert(value, message + ts_node_string(node));
}

//----------------------------------------------------------------------------

typedef TSNode code_block_t;
typedef std::vector<TSNode> code_blocks_t;

/// Memory-managed TSTree object representing the abstract syntax tree of given
/// source code.
class TSTreeDeleter {
 public:
  void operator()(TSTree* tree) {
    ts_tree_delete(tree);
  }
};
using ManagedTSTree = std::unique_ptr<TSTree, TSTreeDeleter>;

/// Parse source code from specified file and return TSNode object for the root
/// as well as original source code content.
template <Language L>
ManagedTSTree GetTSTree(const std::string& source_file,
                        std::string& source_file_contents);

/// Parse source code from specified string and return TSNode
template <Language L>
ManagedTSTree GetTSTree(const std::string& source_code,
                        bool report_parse_errors = false);

template <Language G>
void CollectCodeBlocksOfInterest(const TSNode& root_node,
                                 code_blocks_t& code_blocks);

template <Language G>
void CollectCodeBlocksOfInterest(const ManagedTSTree& tree,
                                 code_blocks_t& code_blocks);

//----------------------------------------------------------------------------
// A simple microsec precision timer for profiling

class Timer {
 public:
  inline int StartTimer() {
    struct timezone *tz = NULL;
    return gettimeofday(&start_tv_, tz);
  }

  inline int StopTimer() {
    struct timezone *tz = NULL;
    return gettimeofday(&end_tv_, tz);
  }

  inline struct timeval TimerDiffToTimeval() const {
    static const suseconds_t kMicroSecs = 1000000;
    auto timeval2microsec = [&](const struct timeval& tv) {
      return tv.tv_sec * kMicroSecs + tv.tv_usec;
    };

    suseconds_t diff_microsec = timeval2microsec(end_tv_) -
                                timeval2microsec(start_tv_);
    struct timeval diff_tv = {.tv_sec = diff_microsec / kMicroSecs,
                              .tv_usec = diff_microsec % kMicroSecs};
    return diff_tv;
  }

  inline std::string TimerDiff() const {
    struct timeval diff_tv = TimerDiffToTimeval();
    std::ostringstream ret_string;
    ret_string << diff_tv.tv_sec << ".";
    ret_string.width(3);
    ret_string.fill('0');
    // Print only 3 decimal points in millisecs.
    ret_string << diff_tv.tv_usec / 1000;
    return ret_string.str();
  }

 private:
    struct timeval start_tv_;
    struct timeval end_tv_;
};

#endif  // SRC_COMMON_UTIL_H_
