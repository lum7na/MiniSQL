#include "catalog_manager.h"

// 定义出来测试用，正式代码中需要删除
// BufferManager buffer_manager;

//输入：表名，属性对象，主键编号，索引对象
//输出：void
//功能：在catalog文件中插入一个表的元信息
//异常：如果已经有相同表名的表存在，则抛出table_exist异常
void CatalogManager::createTable(std::string table_name, Attribute attribute, int primary, Index index) {
  if (hasTable(table_name)) throw table_exist();  // 抛出同名表已经存在的异常
  attribute.unique[primary] = true;               // 主键应该为unique

  std::string tb_info = "";  // 将表格信息储存在tb_info中；

  // 表格信息储存的顺序为：表格名称、属性信息、主键编号、索引信息
  tb_info = tb_info + table_name;  // 储存表格名称

  tb_info = tb_info + " |" + num2str(primary, 2) + " " + num2str(attribute.num, 2);  // “|”符号之后开始储存属性信息——属性的数量，最大32，因此2位即可
  for (int idx = 0; idx < attribute.num; idx++)
    // 对于每一个属性，储存顺序为：属性名称、属性类型、属性是否唯一
    tb_info = tb_info + " " + attribute.name[idx] + " " + num2str(attribute.type[idx], 3) + " " + (attribute.unique[idx] ? "1" : "0");

  tb_info = tb_info + " ;" + num2str(index.num, 2);  // “;”符号之后开始储存索引信息——索引的数量，最大为10，因此2位即可
  for (int idx = 0; idx < index.num; idx++)
    // 对于每一个属性，储存顺序位：索引名称、索引位置
    tb_info = tb_info + " " + index.indexname[idx] + " " + num2str(index.location[idx], 2);

  tb_info = tb_info + "\n#";  // “#”符号标志当前块的结尾

  // 在所有信息开头储存当前信息所占的字节数
  std::string tb_info_len = num2str(tb_info.length() + 5, 4) + " ";

  int tb_info_len_int = tb_info.length() + 5;
  tb_info = tb_info_len + tb_info;

  int blockNum = getBlockNum(TABLE_MANAGER_PATH);
  if (blockNum != 0) {
    for (int blockNo = 0; blockNo < blockNum; blockNo++) {
      char *buffer = buffer_manager.getPage(TABLE_MANAGER_PATH, blockNo);
      int pageId = buffer_manager.getPageId(TABLE_MANAGER_PATH, blockNo);

      // 利用块末尾的标志位“#”计算当前块已存放信息的长度
      int length = 0;
      while (buffer[length] != '#') length++;

      if (length + tb_info_len_int >= PAGESIZE) continue;  // 当前块空间不足，继续循环

      // 将原本块尾的标志删除，插入新的表格信息
      buffer[length] = '\0';
      strcat(buffer, tb_info.c_str());
      buffer_manager.modifyPage(pageId);
      return;
    }
  }

  // 若之前没有块，或是所有当前块空间均不足，则新申请一块；
  char *buffer = buffer_manager.getPage(TABLE_MANAGER_PATH, blockNum);
  int pageId = buffer_manager.getPageId(TABLE_MANAGER_PATH, blockNum);
  strcat(buffer, tb_info.c_str());
  buffer_manager.modifyPage(pageId);
}

//输入：表名
//输出：void
//功能：在catalog文件中删除一个表的元信息
//异常：如果表不存在，抛出table_not_exist异常
void CatalogManager::dropTable(std::string table_name) {
  if (!hasTable(table_name)) throw table_not_exist();

  // 计算出待删除表格记录的开始下标与截止下标
  int blockNo;
  int start_index = getTablePlace(table_name, blockNo);
  char *buffer = buffer_manager.getPage(TABLE_MANAGER_PATH, blockNo);
  int pageId = buffer_manager.getPageId(TABLE_MANAGER_PATH, blockNo);

  // 转换为string类型便于后续处理，计算出截止下标
  std::string buffer_str(buffer);
  int end_index = start_index + str2num(buffer_str.substr(start_index, 4)) - 1;

  // 修改当前Block的信息，将buffer中end_index开始的字节向前移动到start_index处
  while (buffer[end_index] != '#') {
    buffer[start_index++] = buffer[end_index++];
  }

  // 添加块末尾标识符“#”，并用“\0”表示字符串结尾
  buffer[start_index++] = '#';
  buffer[start_index] = '\0';

  buffer_manager.modifyPage(pageId);
}

//输入：表名
//输出：bool
//功能：查找对应表是否存在，存在返回true，不存在返回false
//异常：无异常
bool CatalogManager::hasTable(std::string table_name) {
  bool ret = false;

  // 得到文件的块数，遍历每一块，查找是否存在该名字的表
  int blockNum = getBlockNum(TABLE_MANAGER_PATH);

  for (int blockNo = 0; blockNo < blockNum; blockNo++) {
    char *buffer = buffer_manager.getPage(TABLE_MANAGER_PATH, blockNo);
    std::string buffer_str(buffer);
    int currentIdx = 0;
    while (buffer_str[currentIdx] != '#') {
      int nameEnd = buffer_str.substr(currentIdx + 5).find(' ');
      std::string currentName = buffer_str.substr(currentIdx + 5, nameEnd);
      if (currentName == table_name) ret = true;
      currentIdx = currentIdx + str2num(buffer_str.substr(currentIdx, 4)) - 1;
    }
  }

  return ret;
}

//输入：表名，属性名
//输出：bool
//功能：查找对应表中是否有某一属性，如果有返回true，如果没有返回false
//异常：如果表不存在，抛出table_not_exist异常
bool CatalogManager::hasAttribute(std::string table_name, std::string attr_name) {
  bool ret = false;

  // 若不存在表格，则抛出异常
  if (!hasTable(table_name)) throw table_not_exist();

  Attribute attr = getAttribute(table_name);

  for (int idx = 0; idx < attr.num; idx++) {
    if (attr.name[idx] == attr_name) ret = true;
  }

  return ret;
}

//输入：表名
//输出：属性对象
//功能：获取一个表的属性
//异常：如果表不存在，抛出table_not_exist异常
Attribute CatalogManager::getAttribute(std::string table_name) {
  Attribute ret;

  // 若不存在表格，则抛出异常
  if (!hasTable(table_name)) throw table_not_exist();

  // 否则找到表格对应的块编号与对应表格开始的下标
  int blockNo;
  int start_index = getTablePlace(table_name, blockNo);

  // 得到当前页
  char *buffer = buffer_manager.getPage(TABLE_MANAGER_PATH, blockNo);
  int pageId = buffer_manager.getPageId(TABLE_MANAGER_PATH, blockNo);
  // 转化为string类型便于处理
  std::string buffer_str(buffer);

  // 因为属性信息从“|”开始记录
  start_index = start_index + buffer_str.find('|');
  start_index++;

  int primary = str2num(buffer_str.substr(start_index, 2));
  ret.primary_key = primary;
  start_index += 3;

  int attrNum = str2num(buffer_str.substr(start_index, 2));
  ret.num = attrNum;
  start_index += 3;

  for (int idx = 0; idx < attrNum; idx++) {
    int nameEnd = buffer_str.find(' ', start_index);
    std::string currentName = buffer_str.substr(start_index, nameEnd - start_index);
    ret.name[idx] = currentName;

    int typeEnd = buffer_str.find(' ', nameEnd + 1);
    int currentType = str2num(buffer_str.substr(nameEnd + 1, typeEnd - nameEnd - 1));
    ret.type[idx] = currentType;

    int unique = str2num(buffer_str.substr(typeEnd + 1, 1));
    ret.unique[idx] = unique;

    start_index = typeEnd + 3;
  }

  Index index_record = getIndex(table_name);
  for (int idx = 0; idx < 32; idx++) ret.has_index[idx] = false;
  for (int idx = 0; idx < index_record.num; idx++) ret.has_index[index_record.location[idx]] = true;

  return ret;
}

//输入：表名，属性名，索引名
//输出：void
//功能：在catalog文件中更新对应表的索引信息（在指定属性上建立一个索引）
//异常：如果表不存在，抛出table_not_exist异常。如果对应属性不存在，抛出attribute_not_exist异常。
//如果已经存在10个索引，抛出index_full异常。
//如果对应属性已经有了索引或者索引名重复，抛出index_exist异常。
void CatalogManager::createIndex(std::string table_name, std::string attr_name, std::string index_name) {
  // 表格不存在的异常
  if (!hasTable(table_name)) throw table_not_exist();

  // 属性不存在的异常
  if (!hasAttribute(table_name, attr_name)) throw attribute_not_exist();

  // 得到索引信息
  Index index = getIndex(table_name);
  Attribute attr = getAttribute(table_name);

  // 索引数目超过10的异常
  if (index.num == 10) throw index_full();

  for (int idx = 0; idx < index.num; idx++) {
    // 当前属性上已经有索引的异常
    if (attr.name[index.location[idx]] == attr_name) throw index_exist();

    // 当前索引名称已经存在的异常
    if (index.indexname[idx] == index_name) throw index_exist();
  }

  index.indexname[index.num] = index_name;
  for (int idx = 0; idx < attr.num; idx++) {
    if (attr.name[idx] == attr_name) index.location[index.num] = idx;
  }
  index.num++;

  // 更新表格信息的方法是删除表格信息后再重新插入
  dropTable(table_name);
  createTable(table_name, attr, attr.primary_key, index);
}

//输入：表名，索引名
//输出：索引所对应的属性名
//功能：通过索引名定位属性名
//异常：如果表不存在，抛出table_not_exist异常。如果对应索引不存在，抛出index_not_exist异常。
std::string CatalogManager::IndextoAttr(std::string table_name, std::string index_name) {
  if (!hasTable(table_name)) throw table_not_exist();

  Index index = getIndex(table_name);

  int idxpos = -1;

  for (int idx = 0; idx < index.num; idx++) {
    if (index.indexname[idx] == index_name) {
      idxpos = idx;
      break;
    }
  }

  if (idxpos == -1) throw index_not_exist();

  Attribute attr = getAttribute(table_name);
  return attr.name[index.location[idxpos]];
}

//输入：表名，索引名
//输出：void
//功能：删除对应表的对应属性上的索引
//异常：如果表不存在，抛出table_not_exist异常。如果对应属性不存在，抛出attribute_not_exist异常。
//如果对应属性没有索引，抛出index_not_exist异常。
void CatalogManager::dropIndex(std::string table_name, std::string index_name) {
  if (!hasTable(table_name)) throw table_not_exist();

  Index index = getIndex(table_name);
  Attribute attr = getAttribute(table_name);

  int idxpos = -1;
  for (int idx = 0; idx < index.num; idx++) {
    if (index.indexname[idx] == index_name) {
      idxpos = idx;
    }
  }

  if (idxpos == -1) throw index_not_exist();

  index.indexname[idxpos] = index.indexname[index.num - 1];
  index.location[idxpos] = index.location[index.num - 1];
  index.num--;

  dropTable(table_name);
  createTable(table_name, attr, attr.primary_key, index);
}

//输入：表名
//输出：void
//功能：显示表的信息
//异常：如果表不存在，抛出table_not_exist异常
void CatalogManager::showTable(std::string table_name) {
  if (!hasTable(table_name)) throw table_not_exist();

  Attribute attr = getAttribute(table_name);
  Index index = getIndex(table_name);
  // 输出表格名；
  std::cout << "Table Name: " << table_name << std::endl;

  // 输出属性信息
  std::cout << "Attribute: " << std::endl;
  std::cout << "Primary key: " << attr.primary_key << std::endl;

  for (int idx = 0; idx < attr.num; idx++) {
    std::cout << attr.name[idx] << " " << attr.type[idx] << " " << attr.unique[idx] << std::endl;
  }

  // 输出索引信息
  std::cout << "Index: " << std::endl;

  for (int idx = 0; idx < index.num; idx++) {
    std::cout << index.indexname[idx] << " " << index.location[idx] << std::endl;
  }
}

//数字转字符串，bit为数的位数
std::string CatalogManager::num2str(int num, short bit) {
  std::string ret = std::to_string(num);

  short bit_remain = bit - ret.length();

  while (bit_remain > 0) {
    ret = "0" + ret;
    bit_remain--;
  }

  return ret;
}

//字符串转数字
int CatalogManager::str2num(std::string str) {
  int idx = 0;
  if (str[idx] == '0') idx++;
  return atoi(str.substr(idx).c_str());
}

//得到该行的表的名字
std::string CatalogManager::getTableName(std::string buffer, int start, int &rear) {
  std::string ret;

  int nameEnd = buffer.substr(start + 5).find(' ');
  ret = buffer.substr(start + 5, nameEnd);

  rear = nameEnd;

  return ret;
}

//返回表在文件中的位置，返回具体位置，引用传出数据所在的块信息，未找到则返回-1
int CatalogManager::getTablePlace(std::string table_name, int &suitable_block) {
  int ret = -1;

  int blockNum = getBlockNum(TABLE_MANAGER_PATH);

  for (suitable_block = 0; suitable_block < blockNum; suitable_block++) {
    int start_index = 0;
    char *buffer = buffer_manager.getPage(TABLE_MANAGER_PATH, suitable_block);
    std::string buffer_str(buffer);

    while (buffer_str[start_index] != '#') {
      int nameEnd = 0;
      std::string currentName = getTableName(buffer_str, start_index, nameEnd);
      if (currentName == table_name) {
        ret = start_index;
        break;
      }
      start_index = start_index + str2num(buffer_str.substr(start_index, 4)) - 1;
    }
    if (ret != -1) break;
  }

  return ret;
}

//返回该表的index
Index CatalogManager::getIndex(std::string table_name) {
  Index ret;

  // 若不存在表格，则抛出异常
  if (!hasTable(table_name)) throw table_not_exist();

  // 否则找到表格对应的块编号与对应表格开始的下标
  int blockNo;
  int start_index = getTablePlace(table_name, blockNo);

  // 得到当前页
  char *buffer = buffer_manager.getPage(TABLE_MANAGER_PATH, blockNo);
  int pageId = buffer_manager.getPageId(TABLE_MANAGER_PATH, blockNo);
  // 转化为string类型便于处理
  std::string buffer_str(buffer);

  // 因为属性信息从“|”开始记录
  start_index = start_index + buffer_str.find(';');
  start_index++;

  int indexNum = str2num(buffer_str.substr(start_index, 2));
  ret.num = indexNum;
  start_index += 3;

  // tb_info = tb_info + " " + index.indexname[idx] + " " + num2str(index.location[idx], 2);
  for (int idx = 0; idx < indexNum; idx++) {
    int nameEnd = buffer_str.find(' ', start_index);
    std::string currentName = buffer_str.substr(start_index, nameEnd - start_index);
    ret.indexname[idx] = currentName;

    int location = str2num(buffer_str.substr(nameEnd + 1, 2));
    ret.location[idx] = location;

    start_index = nameEnd + 4;
  }

  return ret;
}

//获取文件大小
int CatalogManager::getBlockNum(std::string table_name) {
  char *p;
  int block_num = 0;
  do {
    p = buffer_manager.getPage(table_name, block_num++);
  } while (p[0] != '\0');
  return block_num - 1;
}

#ifdef __TEST_CATM__

#include <iostream>
using namespace std;

int main() {
  CatalogManager cat;

  Data data1, data2;
  data1.type = -1;
  data1.datai = 123;
  string str = "chen";
  data2.type = str.length();
  data2.datas = str;

  Tuple t;

  t.addData(data1);
  t.addData(data2);

  t.showTuple();

  Attribute a;
  a.num = 2;
  a.primary_key = 1;
  a.name[0] = "int";
  a.name[1] = "str";
  a.type[0] = -1;
  a.type[1] = data2.type;
  a.unique[0] = 0;
  a.unique[1] = 1;

  Table table("MyFirstTable", a);
  table.addTuple(t);

  table.setIndex(0, "Index_on_int");
  table.setIndex(0, "Duplicate");

  cat.createTable(table.getTitle(), table.attr_, table.attr_.primary_key, table.getIndex());
  cat.dropIndex(table.getTitle(), "Index_on_int");
  cat.createIndex(table.getTitle(), "str", "Index_on_str");
  cat.showTable(table.getTitle());
  // cat.dropTable(table.getTitle());

  return 0;
}

#endif