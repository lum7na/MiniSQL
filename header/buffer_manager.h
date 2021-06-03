#ifndef _BUFFER_MANAGER_H_
#define _BUFFER_MANAGER_H_ 1

#include <cstdio>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/priority_queue.hpp>
#include <ext/pb_ds/tree_policy.hpp>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

#include "const.h"

// Page类。磁盘文件中的每一块对应内存中的一个页（page)
class Page {
 public:
  // 构造函数和一些存取控制函数。可忽略。
  Page();
  void initialize();
  void setFileName(std::string _file_name);
  std::string getFileName();
  void setBlockId(int _block_id);
  int getBlockId();
  void addUse();
  int getUseCount();
  void setPin();
  void unPin();
  int getPinCount();
  void setDirty(bool _dirty);
  bool getDirty();
  void setAvaliable(bool _avaliable);
  bool getAvaliable();
  char* getBuffer();

  bool operator<(Page& r) { return getUseCount() < r.getUseCount(); }

 private:
  char* buffer;           //每一页都是一个大小为PAGESIZE字节的数组
  std::string file_name;  //页所对应的文件名
  int block_id;           //页在所在文件中的块号(磁盘中通常叫块)
  int use_count;          //记录被使用的次数
  int pin_count;          //记录被钉住的次数。被钉住的意思就是不可以被替换
  bool dirty;             // dirty记录页是否被修改
  bool avaliable;         // avaliable标示页是否可以被使用(即将磁盘块load进该页)
};

// BufferManager类。对外提供操作缓冲区的接口。
class BufferManager {
 public:
  //  构造函数
  BufferManager();
  BufferManager(int _frame_size);
  // 析构函数
  ~BufferManager();
  // 通过页号得到页的句柄(一个页的头地址)
  char* getPage(std::string _file_name, int _block_id);
  // 标记page_id所对应的页已经被修改
  void modifyPage(int _page_id);
  // 钉住一个页
  void pinPage(int _page_id);
  // 解除一个页的钉住状态(需要注意的是一个页可能被多次钉住，该函数只能解除一次)
  // 如果对应页的pin_count_为0，则返回-1
  int unpinPage(int _page_id);
  // 将对应内存页写入对应文件的对应块。这里的返回值为int，但感觉其实没什么用，可以设为void
  int flushPage(int _page_id, std::string _file_name, int _block_id);
  // 获取对应文件的对应块在内存中的页号，没有找到返回-1
  int getPageId(std::string _file_name, int _block_id);

 private:
  Page* frames;                      //缓冲池，实际上就是一个元素为Page的数组，实际内存空间将分配在堆上
  int frame_size;                    //记录总页数
  void initialize(int _frame_size);  //实际初始化函数
  // 获取一个闲置的页的页号(内部封装了时钟替换策略，但使用者不需要知道这些)
  int getEmptyPageId();
  // 讲对应文件的对应块载入对应内存页，对于文件不存在返回-1，否则返回0
  int loadDiskBlock(int _page_id, std::string _file_name, int _block_id);

  typedef __gnu_pbds::priority_queue<Page, std::less<Page>, __gnu_pbds::binomial_heap_tag> bheap;
  bheap heap;
  bheap::point_iterator* heap_id;
  __gnu_pbds::gp_hash_table<std::pair<std::string, int>, int> frame_id;
  std::queue<int> free_nodes;
};

#endif

// 如何使用?
// 首先实例化一个BufferManager类，然后通过getPage接口得到对应文件的对应块
// 在内存中的句柄，得到句柄之后即可读取或修改页的内容。之后通过getPageId接口
// 获取块在内存中的页号。需要注意的是，如果修改了对应页的内容，需要调用
// modifyPage接口来标记该页已经被修改，否则可能修改会丢失。
// 另外，如果当前操作需要持续使用某一页，则需要通过pinPage接口将该页钉住，防止
// 被替换。如果不需要再使用该页，通过unpinPage接口将该页解除。
// 通过flushPage接口可以将内存中的一页写到文件中的一块。
