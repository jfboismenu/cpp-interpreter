// Copyright (c) 2023 Jean-François Boismenu

#include <memory>

namespace luno {
namespace line_parser {

class File {
 public:
  const char* const filename;
  const char* const content;
};

class Line {
 public:
  const File* const file;
  const char* const start;
  const int length;
  const int number;
};

}  // namespace line_parser

}  // namespace luno

using namespace luno::line_parser;

int main(int, char**) {
  File file{"filename.txt", "content.txt"};
  Line line{&file, "filename.txt", 10, 10};
  (void)line;
  return 0;
}
