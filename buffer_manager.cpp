#include "buffer_manager.h"

#include <cstring>
#include <fstream>

#include "const.h"
#include "exception.h"

void Page::initialize() {
  buffer = new char[PAGESIZE];
  file_name = "";
  block_id = -1;
  use_count = 0;
  pin_count = 0;
  dirty = 0;
}

void Page::setFileName(std::string _file_name) { file_name = _file_name; }

std::string Page::getFileName() { return file_name; }

void Page::setBlockId(int _block_id) { block_id = _block_id; }

int Page::getBlockId() { return block_id; }

void Page::addUse() { ++use_count; }

int Page::getUseCount() { return use_count; }

void Page::setPin() { ++pin_count; }

void Page::unPin() { --pin_count; }

int Page::getPinCount() { return pin_count; }

void Page::setDirty(bool _dirty) { dirty = _dirty; }

bool Page::getDirty() { return dirty; }

char* Page::getBuffer() { return buffer; }

BufferManager::BufferManager() {
  frames = nullptr;
  heap_id = nullptr;
  frame_size = 0;
}

BufferManager::BufferManager(int _frame_size) { initialize(_frame_size); }

void BufferManager::initialize(int _frame_size) {
  frames = new Page[_frame_size];
  heap_id = new bheap::point_iterator[_frame_size];
  frame_size = _frame_size;
  for (int i = 0; i < _frame_size; ++i) {
    free_nodes.push(i);
  }
}

void BufferManager::addUse(int _id) {
  auto& p = frames[_id];
  p.addUse();
  heap.modify(heap_id[_id], p);
}

char* BufferManager::getPage(std::string _file_name, int _block_id) {
  int id = getPageId(_file_name, _block_id);
  if (id == -1) {
    id = getEmptyPageId();
    loadDiskBlock(id, _file_name, _block_id);
  }
  addUse(id);
  return frames[id].getBuffer();
}

void BufferManager::modifyPage(int _page_id) {
  auto& p = frames[_page_id];
  p.setDirty(true);
}

void BufferManager::pinPage(int _page_id) {
  auto& p = frames[_page_id];
  if (p.getPinCount() == 0) {
    heap.erase(heap_id[_page_id]);
  }
  p.setPin();
}

int BufferManager::unpinPage(int _page_id) {
  auto& p = frames[_page_id];
  p.unPin();
  if (p.getPinCount() == 0) {
    heap_id[_page_id] = heap.push(p);
    return -1;
  }
  return 0;
}

int BufferManager::flushPage(int _page_id, std::string _file_name, int _block_id) {
  auto& p = frames[_page_id];
  std::ofstream ofs("./data/" + _file_name);
  ofs.seekp(_block_id * PAGESIZE);
  ofs.write(p.getBuffer(), PAGESIZE);
  free_nodes.push(_page_id);
}

int BufferManager::getPageId(std::string _file_name, int _block_id) {
  auto it = frame_id.find({_file_name, _block_id});
  if (it == frame_id.end()) {
    return -1;
  } else {
    return it->second;
  }
}

int BufferManager::getEmptyPageId() {
  if (!free_nodes.empty()) {
    int ret = free_nodes.front();
    free_nodes.pop();
    return ret;
  }
  try {
    if (heap.size() == 0) {
      throw buffer_is_full();
    }
    auto it = heap.top();
    heap.pop();
    int id = frame_id[{it.getFileName(), it.getBlockId()}];
    if (frames[id].getDirty() == true) {
      flushPage(id, it.getFileName(), it.getBlockId());
    }
    return id;
  } catch (const buffer_is_full& bif) {
    std::cerr << "Buffer is full!" << std::endl;
  }
}

int BufferManager::loadDiskBlock(int _page_id, std::string _file_name, int _block_id) {
  std::ifstream ifs("./data/" + _file_name);
  if (ifs.fail()) {
    return -1;
  }
  ifs.seekg(_block_id * PAGESIZE);
  auto& p = frames[_page_id];
  ifs.read(p.getBuffer(), PAGESIZE);
  p.setBlockId(_block_id);
  p.setFileName(_file_name);
  return 0;
}