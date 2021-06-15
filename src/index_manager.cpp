#include "index_manager.h"

//构造函数
//功能：根据输入的table_name创建索引文件
IndexManager::IndexManager(std::string table_name) {
  CatalogManager catalog_manager;
  Attribute attr = catalog_manager.getAttribute(table_name);

  for (int idx = 0; idx < attr.num; idx++)
    if (attr.has_index[idx]) {
      std::string index_path = "INDEX_FILE_" + attr.name[idx] + "_" + table_name;
      createIndex(index_path, attr.type[idx]);
    }
}

//析构函数
IndexManager::~IndexManager() {
  for (intMap::iterator iter = indexIntMap.begin(); iter != indexIntMap.end(); iter++)
    if (iter->second) {
      iter->second->writtenbackToDiskAll();
      delete iter->second;
    }

  for (floatMap::iterator iter = indexFloatMap.begin(); iter != indexFloatMap.end(); iter++)
    if (iter->second) {
      iter->second->writtenbackToDiskAll();
      delete iter->second;
    }

  for (stringMap::iterator iter = indexStringMap.begin(); iter != indexStringMap.end(); iter++)
    if (iter->second) {
      iter->second->writtenbackToDiskAll();
      delete iter->second;
    }
}

//输入：Index文件名(路径)，索引的key的类型
//输出：void
//功能：创建索引文件及B+树
//异常：
void IndexManager::createIndex(std::string file_path, int type) {
  if (type < -1) std::cout << "Invalid data type!" << std::endl;

  switch (type) {
    case -1: {
      BPlusTree<int> *bplustree = new BPlusTree<int>(file_path, getKeySize(type), getDegree(type));
      indexIntMap.insert(intMap::value_type(file_path, bplustree));
      break;
    }
    case 0: {
      BPlusTree<float> *bplustree = new BPlusTree<float>(file_path, getKeySize(type), getDegree(type));
      indexFloatMap.insert(floatMap::value_type(file_path, bplustree));
      break;
    }
    default: {
      BPlusTree<std::string> *bplustree = new BPlusTree<std::string>(file_path, getKeySize(type), getDegree(type));
      indexStringMap.insert(stringMap::value_type(file_path, bplustree));
      break;
    }
  }
}

//输入：Index文件名(路径)，索引的key的类型
//输出：void
//功能：删除索引、B+树及文件
//异常：
void IndexManager::dropIndex(std::string file_path, int type) {
  if (type < -1) std::cout << "Invalid data type!" << std::endl;

  switch (type) {
    case -1: {
      intMap::iterator iter = indexIntMap.find(file_path);
      if (iter == indexIntMap.end()) {
        std::cout << "Fail to find the index to be dropped." << std::endl;
      } else {
        delete iter->second;
        indexIntMap.erase(iter);
      }
    }
    case 0: {
      floatMap::iterator iter = indexFloatMap.find(file_path);
      if (iter == indexFloatMap.end()) {
        std::cout << "Fail to find the index to be dropped." << std::endl;
      } else {
        delete iter->second;
        indexFloatMap.erase(iter);
      }
    }
    default: {
      stringMap::iterator iter = indexStringMap.find(file_path);
      if (iter == indexStringMap.end()) {
        std::cout << "Fail to find the index to be dropped." << std::endl;
      } else {
        delete iter->second;
        indexStringMap.erase(iter);
      }
    }
  }
}

//输入：Index文件名(路径)，索引的key(包含类型)
//输出：根据给出的data返回对应的value
//功能：创建索引文件及B+树
//异常：
int IndexManager::findIndex(std::string file_path, Data data) {
  int ret;

  if (data.type < -1) std::cout << "Invalid data type!" << std::endl;

  switch (data.type) {
    case -1: {
      intMap::iterator iter = indexIntMap.find(file_path);
      if (iter == indexIntMap.end())
        std::cout << "Fail to find the index." << std::endl;
      else
        ret = iter->second->searchVal(data.datai);
    }
    case 0: {
      floatMap::iterator iter = indexFloatMap.find(file_path);
      if (iter == indexFloatMap.end())
        std::cout << "Fail to find the index." << std::endl;
      else
        ret = iter->second->searchVal(data.dataf);
    }
    default: {
      stringMap::iterator iter = indexStringMap.find(file_path);
      if (iter == indexStringMap.end())
        std::cout << "Fail to find the index." << std::endl;
      else
        ret = iter->second->searchVal(data.datas);
    }
  }

  return ret;
}

//输入：Index文件名(路径)，索引的key(包含类型)，block_id
//输出：void
//功能：在指定索引中插入一个key
//异常：
void IndexManager::insertIndex(std::string file_path, Data data, int block_id) {
  if (data.type < -1) std::cout << "Invalid data type!" << std::endl;

  switch (data.type) {
    case -1: {
      intMap::iterator iter = indexIntMap.find(file_path);
      if (iter == indexIntMap.end())
        std::cout << "Fail to find the index." << std::endl;
      else
        iter->second->insertKey(data.datai, block_id);
      break;
    }
    case 0: {
      floatMap::iterator iter = indexFloatMap.find(file_path);
      if (iter == indexFloatMap.end())
        std::cout << "Fail to find the index." << std::endl;
      else
        iter->second->insertKey(data.dataf, block_id);
      break;
    }
    default: {
      stringMap::iterator iter = indexStringMap.find(file_path);
      if (iter == indexStringMap.end())
        std::cout << "Fail to find the index." << std::endl;
      else
        iter->second->insertKey(data.datas, block_id);
    }
  }
}

//输入：Index文件名(路径)，索引的key(包含类型)
//输出：void
//功能：在索引中删除相应的Key
//异常：
void IndexManager::deleteIndexByKey(std::string file_path, Data data) {
  if (data.type < -1) std::cout << "Invalid data type!" << std::endl;

  switch (data.type) {
    case -1: {
      intMap::iterator iter = indexIntMap.find(file_path);
      if (iter == indexIntMap.end())
        std::cout << "Fail to find the index." << std::endl;
      else
        iter->second->deleteKey(data.datai);
      break;
    }
    case 0: {
      floatMap::iterator iter = indexFloatMap.find(file_path);
      if (iter == indexFloatMap.end())
        std::cout << "Fail to find the index." << std::endl;
      else
        iter->second->deleteKey(data.dataf);
      break;
    }
    default: {
      stringMap::iterator iter = indexStringMap.find(file_path);
      if (iter == indexStringMap.end())
        std::cout << "Fail to find the index." << std::endl;
      else
        iter->second->deleteKey(data.datas);
    }
  }
}

//输入：Index文件名(路径)，索引的key1(包含类型)，索引的key2(包含类型)，返回的vector引用
//输出：void
//功能：范围查找，返回一定范围内的value
//异常：
void IndexManager::searchRange(std::string file_path, Data data1, Data data2, std::vector<int> &vals) {
  // if (data1.type != data2.type) throw data_type_conflict();
  // flag有三种类型，0表示在data1与data2之间搜索，1表示从-INF搜索到data2，2表示从data1搜索到INF
  int flag = 0;
  if (data1.type == -2)
    flag = 1;
  else if (data2.type == -2)
    flag = 1;

  switch (data1.type) {
    case -1: {
      intMap::iterator iter = indexIntMap.find(file_path);
      if (iter == indexIntMap.end())
        std::cout << "Fail to find the index." << std::endl;
      else
        iter->second->searchRange(data1.datai, data2.datai, vals, flag);
      break;
    }
    case 0: {
      floatMap::iterator iter = indexFloatMap.find(file_path);
      if (iter == indexFloatMap.end())
        std::cout << "Fail to find the index." << std::endl;
      else
        iter->second->searchRange(data1.dataf, data2.dataf, vals, flag);
      break;
    }
    default: {
      stringMap::iterator iter = indexStringMap.find(file_path);
      if (iter == indexStringMap.end())
        std::cout << "Fail to find the index." << std::endl;
      else
        iter->second->searchRange(data1.datas, data2.datas, vals, flag);
    }
  }
}

//计算B+树适合的degree
int IndexManager::getDegree(int type) {
  // 计算B+树一个节点能够容纳多少数据
  int ret = (PAGESIZE - sizeof(int)) / (getKeySize(type) + sizeof(int));

  if (ret % 2 == 0) ret--;

  return ret;
}

//计算不同类型Key的size
int IndexManager::getKeySize(int type) {
  int ret;

  switch (type) {
    case -1:
      ret = sizeof(int);
      break;
    case 0:
      ret = sizeof(float);
      break;
    default:
      ret = type;
      break;
  }

  // 如果输入的type小于-1，则输入不合法
  if (ret < 0) std::cout << "Invalid data type!" << std::endl;

  return ret;
}

#ifdef __TEST_INDM__

#include <iostream>
using namespace std;

BufferManager buffer_manager;

int main() {
  string table_name = "test1";

  IndexManager index_manager(table_name);

  return 0;
}

#endif
