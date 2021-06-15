#include "record_manager.h"

//输入：表名
//输出：void
//功能：建立表文件
//异常：无异常处理（由catalog manager处理）
void RecordManager::createTableFile(std::string table_name) {
  // 此函数仅创建文件，catalog中的记录需要另外调用catalog manager完成
  table_name = "./database/data/" + table_name;
  FILE *fp = fopen(table_name.c_str(), "w");
  fclose(fp);
}

//输入：表名
//输出：void
//功能：删除表文件
//异常：无异常处理（由catalog manager处理）
void RecordManager::dropTableFile(std::string table_name) {
  // 此函数仅移除文件，catalog中的记录需要另外调用catalog manager完成
  table_name = "./database/data/" + table_name;
  remove(table_name.c_str());
}

//输入：表名，一个元组
//输出：void
//功能：向对应表中插入一条记录
//异常：如果元组类型不匹配，抛出tuple_type_conflict异常。如果
//主键冲突，抛出primary_key_conflict异常。如果unique属性冲突，
//抛出unique_conflict异常。如果表不存在，抛出table_not_exist异常。
void RecordManager::insertRecord(std::string table_name, Tuple &tuple) {
  std::string tmp_name = table_name;
  // 索引文件的路径，便于之后buffer_manager操作
  table_name = "./database/data/" + table_name;

  // 建立catalog_manager检查表名等冲突
  CatalogManager catalog_manager;
  if (!catalog_manager.hasTable(tmp_name)) throw table_not_exist();

  Attribute attr = catalog_manager.getAttribute(tmp_name);
  std::vector<Data> data = tuple.getData();

  // 检查是否存在类型不匹配的情况
  for (int idx = 0; idx < attr.num; idx++) {
    if (data[idx].type != attr.type[idx]) throw tuple_type_conflict();
  }

  Table table = selectRecord(tmp_name);
  std::vector<Tuple> &tuples = table.getTuple();

  // 检查是否存在主键冲突
  for (int idx = 0; idx < attr.num; idx++) {
    if (isConflict(tuples, data, attr.primary_key)) throw primary_key_conflict();
  }

  // 检查是否满足唯一性条件
  for (int idx = 0; idx < attr.num; idx++) {
    if (attr.unique[idx] && isConflict(tuples, data, idx)) throw unique_conflict();
  }

  int block_offset;

  // 计算待插入的tuple的长度；
  int tuple_len = attr.num + 7;
  for (int idx = 0; idx < attr.num; idx++) {
    Data current_data = data[idx];
    switch (current_data.type) {
      case -1:
        tuple_len += getDataLength(current_data.datai);
        break;
      case 0:
        tuple_len += getDataLength(current_data.dataf);
        break;
      default:
        tuple_len += getDataLength(current_data.datas);
        break;
    }
  }

  // 找到拥有足够大剩余空间的页并完成插入
  int block_num = getBlockNum(table_name);
  block_num = (block_num == 0) ? 1 : block_num;
  char *p = buffer_manager.getPage(table_name, block_num - 1);

  int cnt = 0;
  while (p[cnt] != '\0' && cnt < PAGESIZE) cnt++;

  if (cnt + tuple_len < PAGESIZE) {
    // 空间足够大，则我们直接插入
    block_offset = block_num - 1;
    insertRecord1(p, cnt, tuple_len, data);
  } else {
    block_offset = block_num;
    char *p = buffer_manager.getPage(table_name, block_offset);
    insertRecord1(p, 0, tuple_len, data);
  }

  int page_id = buffer_manager.getPageId(table_name, block_offset);
  buffer_manager.modifyPage(page_id);

  //更新索引
  IndexManager index_manager(tmp_name);
  for (int i = 0; i < attr.num; i++) {
    if (attr.has_index[i] == true) {
      std::string attr_name = attr.name[i];
      std::string file_path = "INDEX_FILE_" + attr_name + "_" + tmp_name;
      std::vector<Data> d = tuple.getData();
      index_manager.insertIndex(file_path, d[i], block_offset);
    }
  }
}

//输入：表名
//输出：int(删除的记录数)
//功能：删除对应表中所有记录（不删除表文件）
//异常：如果表不存在，抛出table_not_exist异常
int RecordManager::deleteRecord(std::string table_name) {
  int ret = 0;  // ret储存删除的记录数

  std::string tmp_name = table_name;
  table_name = "./database/data/" + table_name;

  // 检查表格是否存在
  CatalogManager catalog_manager;
  if (!catalog_manager.hasTable(tmp_name)) throw table_not_exist();

  int block_num = getBlockNum(table_name);
  Attribute attr = catalog_manager.getAttribute(tmp_name);
  IndexManager index_manager(tmp_name);

  for (int idx = 0; idx < block_num; idx++) {
    char *p = buffer_manager.getPage(table_name, idx);
    int page_id = buffer_manager.getPageId(table_name, idx);
    char *original_p = p;

    while (*p != '\0' && p < original_p + PAGESIZE) {
      Tuple tuple = readTuple(p, attr);

      for (int idx = 0; idx < attr.num; idx++) {
        if (attr.has_index[idx]) {
          std::string attr_name = attr.name[idx];
          // 根据index_manager的设计，需要修改index_path
          std::string index_path = "INDEX_FILE_" + attr_name + "_" + tmp_name;
          std::vector<Data> data = tuple.getData();
          index_manager.deleteIndexByKey(index_path, data[idx]);
        }
      }

      p = deleteRecord1(p);
      ret++;
    }
    buffer_manager.modifyPage(page_id);
  }

  return ret;
}

//输入：表名，目标属性，一个Where类型的对象
//输出：int(删除的记录数)
//功能：删除对应表中所有目标属性值满足Where条件的记录
//异常：如果表不存在，抛出table_not_exist异常。如果属性不存在，抛出attribute_not_exist异常。
//如果Where条件中的两个数据类型不匹配，抛出data_type_conflict异常。
int RecordManager::deleteRecord(std::string table_name, std::string target_attr, Where where) {
  int ret = 0;  // ret储存删除的记录数

  std::string tmp_name = table_name;
  table_name = "./database/data/" + table_name;

  // 检查表格是否存在
  CatalogManager catalog_manager;
  if (!catalog_manager.hasTable(tmp_name)) throw table_not_exist();

  // 检查属性是否存在
  Attribute attr = catalog_manager.getAttribute(tmp_name);
  int index = -1;
  for (int idx = 0; idx < attr.num; idx++)
    if (attr.name[idx] == target_attr) {
      index = idx;
      break;
    }
  if (index == -1) throw attribute_not_exist();

  if (attr.type[index] != where.data.type) throw data_type_conflict();

  int block_num = getBlockNum(table_name);
  IndexManager index_manager(tmp_name);

  if (attr.has_index[index] && where.relation_character != NOT_EQUAL) {
    std::vector<int> block_ids;
    searchWithIndex(tmp_name, target_attr, where, block_ids);
    for (int idx = 0; idx < block_ids.size(); idx++) ret += conditionDeleteInBlock(tmp_name, block_ids[idx], attr, index, where);
  } else
    for (int idx = 0; idx < block_num; idx++) ret += conditionDeleteInBlock(tmp_name, idx, attr, index, where);

  return ret;
}

//输入：表名
//输出：Table类型对象
//功能：返回整张表
//异常：如果表不存在，抛出table_not_exist异常
Table RecordManager::selectRecord(std::string table_name, std::string result_table_name) {
  std::string tmp_name = table_name;
  table_name = "./database/data/" + table_name;

  // 检查表格是否存在
  CatalogManager catalog_manager;
  if (!catalog_manager.hasTable(tmp_name)) throw table_not_exist();

  Attribute attr = catalog_manager.getAttribute(tmp_name);
  Table ret(result_table_name, attr);

  int block_num = getBlockNum(table_name);
  for (int idx = 0; idx < block_num; idx++) {
    char *p = buffer_manager.getPage(table_name, idx);
    char *original_p = p;
    while (*p != '\0' && p < original_p + PAGESIZE) {
      Tuple tuple = readTuple(p, attr);
      if (!tuple.isDeleted()) ret.getTuple().push_back(tuple);
      int tuple_len = getTupleLength(p);
      p = p + tuple_len;
    }
  }

  return ret;
}

//输入：表名，目标属性，一个Where类型的对象
//输出：Table类型对象
//功能：返回包含所有目标属性满足Where条件的记录的表
//异常：如果表不存在，抛出table_not_exist异常。如果属性不存在，抛出attribute_not_exist异常。
//如果Where条件中的两个数据类型不匹配，抛出data_type_conflict异常。
Table RecordManager::selectRecord(std::string table_name, std::string target_attr, Where where, std::string result_table_name) {
  std::string tmp_name = table_name;
  table_name = "./database/data/" + table_name;

  // 检查表格是否存在
  CatalogManager catalog_manager;
  if (!catalog_manager.hasTable(tmp_name)) throw table_not_exist();

  // 检查属性是否存在
  Attribute attr = catalog_manager.getAttribute(tmp_name);
  int index = -1;
  for (int idx = 0; idx < attr.num; idx++)
    if (attr.name[idx] == target_attr) {
      index = idx;
      break;
    }
  if (index == -1) throw attribute_not_exist();

  if (attr.type[index] != where.data.type) throw data_type_conflict();

  Table ret(result_table_name, attr);
  std::vector<Tuple> &tuples = ret.getTuple();

  if (attr.has_index[index] && where.relation_character != NOT_EQUAL) {
    std::vector<int> block_ids;
    searchWithIndex(tmp_name, target_attr, where, block_ids);
    for (int idx = 0; idx < block_ids.size(); idx++) conditionSelectInBlock(tmp_name, block_ids[idx], attr, index, where, tuples);
  } else {
    int block_num = getBlockNum(table_name);
    for (int idx = 0; idx < block_num; idx++) conditionSelectInBlock(tmp_name, idx, attr, index, where, tuples);
  }

  return ret;
}

//输入：表名，目标属性名
//输出：void
//功能：对表中已经存在的记录建立索引
//异常：如果表不存在，抛出table_not_exist异常。如果属性不存在，抛出attribute_not_exist异常。
void RecordManager::createIndex(IndexManager &index_manager, std::string table_name, std::string target_attr) {
  std::string tmp_name = table_name;
  table_name = "./database/data/" + table_name;

  CatalogManager catalog_manager;
  if (!catalog_manager.hasTable(tmp_name)) throw table_not_exist();

  Attribute attr = catalog_manager.getAttribute(tmp_name);
  int index = -1;
  for (int idx = 0; idx < attr.num; idx++)
    if (attr.name[idx] == target_attr) {
      index = idx;
      break;
    }
  if (index == -1) throw attribute_not_exist();

  std::string index_path = "INDEX_FILE_" + target_attr + "_" + tmp_name;
  int block_num = getBlockNum(table_name);
  for (int idx = 0; idx < block_num; idx++) {
    char *p = buffer_manager.getPage(table_name, idx);
    char *original_p = p;

    while (*p != '\0' && p < original_p + PAGESIZE) {
      Tuple tuple = readTuple(p, attr);

      if (tuple.isDeleted()) {
        std::vector<Data> data = tuple.getData();
        index_manager.insertIndex(index_path, data[index], idx);
      }

      int tuple_len = getTupleLength(p);
      p = p + tuple_len;
    }
  }
}

//获取文件大小
int RecordManager::getBlockNum(std::string table_name) {
  char *p;
  int block_num = 0;
  do {
    p = buffer_manager.getPage(table_name, block_num++);
  } while (p[0] != '\0');

  return block_num - 1;
}

// insertRecord的辅助函数
void RecordManager::insertRecord1(char *p, int offset, int len, const std::vector<Data> &v) {
  std::string tmp_str = std::to_string(len);

  short bit_remain = 4 - tmp_str.length();
  while (bit_remain > 0) {
    tmp_str = "0" + tmp_str;
    bit_remain--;
  }

  // 先储存记录的长度
  for (int idx = 0; idx < 4; idx++) p[offset++] = tmp_str[idx];

  for (int idx = 0; idx < v.size(); idx++) {
    p[offset++] = ' ';  // 记录中两个值用“ ”分隔
    Data d = v[idx];
    switch (d.type) {
      case -1:
        copyString(p, offset, d.datai);
        break;
      case 0:
        copyString(p, offset, d.dataf);
        break;
      default:
        copyString(p, offset, d.datas);
        break;
    }
  }
  p[offset++] = ' ';
  // 倒数第二位置为0，表示记录存在；置为1，表示已删除
  p[offset++] = '0';
  p[offset++] = '\n';
}

// deleteRecord的辅助函数
char *RecordManager::deleteRecord1(char *p) {
  int len = getTupleLength(p);
  p = p + len;
  *(p - 2) = '1';

  // 返回下一条记录开头的地址
  return p;
}

//从内存中读取一个tuple
Tuple RecordManager::readTuple(const char *p, Attribute attr) {
  Tuple ret;

  p = p + 5;  // 将长度项略过

  for (int idx = 0; idx < attr.num; idx++) {
    Data data;
    char tmp[100];
    memset(tmp, 0, sizeof(tmp));
    int j;

    for (j = 0; *p != ' '; j++, p++) tmp[j] = *p;

    tmp[j] = '\0';
    p++;

    std::string tmp_str(tmp);

    data.type = attr.type[idx];

    switch (attr.type[idx]) {
      case -1: {
        std::stringstream str_stream(tmp_str);
        str_stream >> data.datai;
      } break;
      case 0: {
        std::stringstream str_stream(tmp_str);
        str_stream >> data.dataf;
      } break;
      default: {
        std::stringstream str_stream(tmp_str);
        str_stream >> data.datas;
      } break;
    }

    ret.addData(data);
  }

  if (*p == '1') ret.setDeleted();

  return ret;
}

//获取一个tuple的长度
int RecordManager::getTupleLength(char *p) {
  char tmp[10];
  memset(tmp, 0, sizeof(tmp));
  for (int idx = 0; p[idx] != ' '; idx++) tmp[idx] = p[idx];
  return atoi(tmp);
}

//判断插入的记录是否和其他记录冲突
bool RecordManager::isConflict(std::vector<Tuple> &tuples, std::vector<Data> &v, int index) {
  bool ret = false;

  for (int idx = 0; idx < tuples.size(); idx++) {
    // 若已经标记删除，则继续
    if (tuples[idx].isDeleted()) continue;

    std::vector<Data> tuple_data = tuples[idx].getData();
    switch (v[index].type) {
      case -1:
        if (v[index].datai == tuple_data[index].datai) ret = true;
        break;
      case 0:
        if (v[index].dataf == tuple_data[index].dataf) ret = true;
        break;
      default:
        if (v[index].datas == tuple_data[index].datas) ret = true;
        break;
    }
  }

  return ret;
}

//带索引查找
void RecordManager::searchWithIndex(std::string table_name, std::string target_attr, Where where, std::vector<int> &block_ids) {
  IndexManager index_manager(table_name);
  Data tmp_data;

  // 需要根据Index Manager的cpp进行修改
  std::string index_path = "INDEX_FILE_" + target_attr + "_" + table_name;

  // 根据where中不同的关系，我们需要调用不同的搜索范围
  if (where.relation_character == LESS || where.relation_character == LESS_OR_EQUAL) {
    // 第一种情况，小于，搜索范围-INF到where.data
    switch (where.data.type) {
      case -1:
        tmp_data.type = -1;
        tmp_data.datai = -INF;
        break;
      case 0:
        tmp_data.type = 0;
        tmp_data.dataf = -INF;
        break;
      default:
        tmp_data.type = 1;
        tmp_data.datas = "";
        break;
    }
    index_manager.searchRange(index_path, tmp_data, where.data, block_ids);
  } else if (where.relation_character == GREATER || where.relation_character == GREATER_OR_EQUAL) {
    // 第二种情况，大于，搜索范围where.data到INF
    switch (where.data.type) {
      case -1:
        tmp_data.type = -1;
        tmp_data.datai = INF;
        break;
      case 0:
        tmp_data.type = 0;
        tmp_data.dataf = INF;
        break;
      default:
        tmp_data.type = -2;
        tmp_data.datas = "";
        break;
    }
    index_manager.searchRange(index_path, where.data, tmp_data, block_ids);
  } else {
    // 第三种情况，等于或不等于，搜索范围where.data到where.data
    index_manager.searchRange(index_path, where.data, where.data, block_ids);
  }
}

//在块中进行条件删除
int RecordManager::conditionDeleteInBlock(std::string table_name, int block_id, Attribute attr, int index, Where where) {
  int ret = 0;  // 返回删除的记录条数
  table_name = "./database/data/" + table_name;
  char *p = buffer_manager.getPage(table_name, block_id);
  int page_id = buffer_manager.getPageId(table_name, block_id);
  char *original_p = p;

  while (*p != '\0' && p < original_p + PAGESIZE) {
    Tuple tuple = readTuple(p, attr);
    std::vector<Data> data = tuple.getData();

    switch (attr.type[index]) {
      case -1:
        if (isSatisfied(data[index].datai, where.data.datai, where.relation_character)) {
          p = deleteRecord1(p);
          ret++;
        } else {
          int len = getTupleLength(p);
          p = p + len;
        }
        break;
      case 0:
        if (isSatisfied(data[index].dataf, where.data.dataf, where.relation_character)) {
          p = deleteRecord1(p);
          ret++;
        } else {
          int len = getTupleLength(p);
          p = p + len;
        }
        break;
      default:
        if (isSatisfied(data[index].datas, where.data.datas, where.relation_character)) {
          p = deleteRecord1(p);
          ret++;
        } else {
          int len = getTupleLength(p);
          p = p + len;
        }
        break;
    }
  }

  buffer_manager.modifyPage(page_id);
  return ret;
}

//在块中进行条件查询
void RecordManager::conditionSelectInBlock(std::string table_name, int block_id, Attribute attr, int index, Where where, std::vector<Tuple> &v) {
  table_name = "./database/data/" + table_name;
  char *p = buffer_manager.getPage(table_name, block_id);
  char *original_p = p;

  while (*p != '\0' && p < original_p + PAGESIZE) {
    Tuple tuple = readTuple(p, attr);

    if (tuple.isDeleted()) continue;

    std::vector<Data> data = tuple.getData();

    switch (attr.type[index]) {
      case -1:
        if (isSatisfied(data[index].datai, where.data.datai, where.relation_character)) v.push_back(tuple);
        break;
      case 0:
        if (isSatisfied(data[index].dataf, where.data.dataf, where.relation_character)) v.push_back(tuple);
        break;
      default:
        if (isSatisfied(data[index].datas, where.data.datas, where.relation_character)) v.push_back(tuple);
        break;
    }
    int len = getTupleLength(p);
    p = p + len;
  }
}

#ifdef __TEST_RECM__

#include <iostream>
using namespace std;

BufferManager buffer_manager;

int main() {
  RecordManager rm;
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

  string tb_name;
  cin >> tb_name;

  Table table(tb_name, a);
  table.addTuple(t);

  table.setIndex(0, "Index_on_int");
  table.setIndex(0, "Duplicate");

  cat.createTable(table.getTitle(), table.attr_, table.attr_.primary_key, table.getIndex());

  rm.createTableFile(table.getTitle());
  rm.insertRecord(table.getTitle(), t);

  Table test_tb = rm.selectRecord(table.getTitle(), "test_tb");
  test_tb.showTable();

  // cat.dropTable(table.getTitle());

  return 0;
}

#endif
