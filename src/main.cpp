#include <iostream>

#include "const.h"
#include "interpreter.h"

BufferManager buffer_manager(MAXFRAMESIZE);
Interpreter query;

int main(int argc, const char* argv[]) {
  std::cout << ">>> Welcome to MiniSQL" << std::endl;
  while (1) {
    if (!query.getQuery(cin)) {
      break;
    }
  }
  return 0;
}
