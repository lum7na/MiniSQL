#include <iostream>

#include "const.h"
#include "interpreter.h"

BufferManager buffer_manager(MAXFRAMESIZE);
Interpreter query;

int main(int argc, const char* argv[]) {
  system("mkdir -p database");
  system("mkdir -p ./database/index");
  system("mkdir -p ./database/catalog");
  system("mkdir -p ./database/data");
  system("touch ./database/catalog/catalog_file");
  if (argc > 1) {
    if (string(argv[1]) == "without_io") {
      query.closeIO();
    }
  }
  std::cout << ">>> Welcome to MiniSQL" << std::endl;
  try {
    while (1) {
      if (!query.getQuery(cin)) {
        break;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Exception Detected: " + string(e.what()) << std::endl;
  }
  return 0;
}
