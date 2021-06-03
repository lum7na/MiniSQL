#include "buffer_manager.h"

#include <cstring>

#include "const.h"

void Page::initialize() {
  buffer = new char[PAGESIZE];
  file_name = "";
  block_id = -1;
  use_count = 0;
  dirty = 0;
  avaliable = 1;
}

void Page::setFileName(std::string _file_name) { file_name = _file_name; }

std::string Page::getFileName() { return file_name; }

void Page::setBlockId(int _block_id) { block_id = _block_id; }

int Page::getBlockId() { return block_id; }

void Page::setUseCount(int _use_count) { use_count = _use_count; }

int Page::getUseCount() { return use_count; }

void Page::setDirty(bool _dirty) { dirty = _dirty; }

bool Page::getDirty() { return dirty; }

void Page::setAvaliable(bool _avaliable) { avaliable = _avaliable; }

bool Page::getAvaliable() { return avaliable; }

char* Page::getBuffer() { return buffer; }

BufferManager::BufferManager() {
  frames = nullptr;
  frame_size = 0;
}

BufferManager::BufferManager(int _frame_size) { initialize(_frame_size); }

void BufferManager::initialize(int _frame_size) {
  frames = new Page[_frame_size];
  frame_size = _frame_size;
}

char* BufferManager::getPage(std::string _file_name, int _block_id) {}