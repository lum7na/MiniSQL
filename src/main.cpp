#include <iostream>
#include <fstream>

#include "const.h"
#include "interpreter.h"

BufferManager buffer_manager(MAXFRAMESIZE);
Interpreter query;

int main(int argc, const char* argv[]) {
  // 请在Linux或MacOS系统下使用如下命令重新编译
  system("mkdir -p database");
  system("mkdir -p ./database/index");
  system("mkdir -p ./database/catalog");
  system("mkdir -p ./database/data");
  system("touch ./database/catalog/catalog_file");
  
  /*
  // 请在Windows系统下使用如下命令重新编译
  system("mkdir database");
  system("mkdir .\\database\\index");
  system("mkdir .\\database\\catalog");
  system("mkdir .\\database\\data");
  fstream fs;
  fs.open(".\\database\\catalog\\catalog_file", ios::in);
  if (!fs)
  {
    ofstream fout(".\\database\\catalog\\catalog_file");
    fout.close();
  }
  else
  {
    fs.close();
  }
  */

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
