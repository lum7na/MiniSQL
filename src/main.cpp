#include <iostream>

#include "const.h"
#include "interpreter.h"

BufferManager buffer_manager(MAXFRAMESIZE);
Interpreter query;

int main(int argc, const char* argv[]) {
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
