#include <iostream>

#include "interpreter.h"

BufferManager buffer_manager;

int main(int argc, const char* argv[]) {
  std::cout << ">>> Welcome to MiniSQL" << std::endl;
  while (1) {
    Interpreter query;
    query.getQuery();
  }
  return 0;
}
