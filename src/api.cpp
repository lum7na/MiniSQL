#include "api.h"

#include "basic.h"
#include "index_manager.h"

Table API::selectRecord(std::string table_name, std::vector<std::string> target_attr, std::vector<Where> where, char operation) {
  if (target_attr.size() == 0) {
    return record.selectRecord(table_name);
  } else if (target_attr.size() == 1) {
    return record.selectRecord(table_name, target_attr[0], where[0]);
  } else {
    Table table1 = record.selectRecord(table_name, target_attr[0], where[0]);
    Table table2 = record.selectRecord(table_name, target_attr[1], where[1]);
    if (!operation) {
      return joinTable(table1, table2);
    } else {
      return unionTable(table1, table2);
    }
  }
}

int API::deleteRecord(std::string table_name, std::string target_attr, Where where) {
  int result = -1;
  if (target_attr == "") {
    result = record.deleteRecord(table_name);
  } else {
    result = record.deleteRecord(table_name, target_attr, where);
  }
  return result;
}

void API::insertRecord(std::string table_name, Tuple &tuple) { record.insertRecord(table_name, tuple); }
 
bool API::createTable(std::string table_name, Attribute attribute, int primary, Index index) {
  record.createTableFile(table_name);
  catalog.createTable(table_name, attribute, primary, index);
  createIndex(table_name, table_name + "_primary", attribute.name[primary]);
  return true;
}

bool API::dropTable(std::string table_name) {
  Attribute attr = catalog.getAttribute(table_name);
  Index index = catalog.getIndex(table_name);
  for (int i = 0; i < index.num; i++) {
    dropIndex(table_name, index.indexname[i]);
  }
  record.dropTableFile(table_name);
  catalog.dropTable(table_name);
  return true;
}

bool API::createIndex(std::string table_name, std::string index_name, std::string attr_name) {
  IndexManager index(table_name);
  std::string file_path = "INDEX_FILE_" + attr_name + "_" + table_name;
  int type = -1;
  catalog.createIndex(table_name, attr_name, index_name);
  Attribute attr = catalog.getAttribute(table_name);
  for (int i = 0; i < attr.num; i++) {
    if (attr.name[i] == attr_name) {
      type = (int)attr.type[i];
      break;
    }
  }
  index.createIndex(file_path, type);
  record.createIndex(index, table_name, attr_name);
  return true;
}

bool API::dropIndex(std::string table_name, std::string index_name) {
  IndexManager index(table_name);
  std::string attr_name = catalog.IndextoAttr(table_name, index_name);
  std::string file_path = "INDEX_FILE_" + attr_name + "_" + table_name;
  int type = -1;
  Attribute attr = catalog.getAttribute(table_name);
  for (int i = 0; i < attr.num; i++) {
    if (attr.name[i] == attr_name) {
      type = (int)attr.type[i];
      break;
    }
  }
  index.dropIndex(file_path, type);
  catalog.dropIndex(table_name, index_name);
  file_path = "./database/index/" + file_path;
  remove(file_path.c_str());
  return true;
}

Table API::unionTable(Table &table1, Table &table2) {
  Table result_table(table1.getTitle(), table1.getAttr());
  std::vector<Tuple> tuple1 = table1.getTuple();
  std::vector<Tuple> tuple2 = table2.getTuple();

  int k = table1.getAttr().primary_key;
  int i = 0;
  int j = 0;
  while (i < tuple1.size() && j < tuple2.size()) {
    if (tuple1[i].getData()[k] < tuple2[j].getData()[k]) {
      i++;
    } else if (tuple2[j].getData()[k] < tuple1[i].getData()[k]) {
      j++;
    } else {
      result_table.getTuple().push_back(tuple1[i++]);
      j++;
    }
  }
  std::sort(result_table.getTuple().begin(), result_table.getTuple().end());
  return result_table;
}

void API::quit() {
  IndexManager index_manager("");
  index_manager.writeBack();
}

Table API::joinTable(Table &table1, Table &table2) {
  Table result_table(table1.getTitle(), table1.getAttr());
  std::vector<Tuple> tuple1 = table1.getTuple();
  std::vector<Tuple> tuple2 = table2.getTuple();

  int k = table1.getAttr().primary_key;
  int i = 0;
  int j = 0;
  while (i < tuple1.size() && j < tuple2.size()) {
    if (tuple1[i].getData()[k] < tuple2[j].getData()[k]) {
      result_table.getTuple().push_back(tuple1[i++]);
    } else if (tuple2[j].getData()[k] < tuple1[i].getData()[k]) {
      result_table.getTuple().push_back(tuple2[j++]);
    } else {
      result_table.getTuple().push_back(tuple1[i++]);
      j++;
    }
  }

  while (i < tuple1.size()) {
    result_table.getTuple().push_back(tuple1[i++]);
  }
  while (j < tuple2.size()) {
    result_table.getTuple().push_back(tuple2[j++]);
  }
  std::sort(result_table.getTuple().begin(), result_table.getTuple().end());
  return result_table;
}

bool isSatisfied(Tuple &tuple, int target_attr, Where where) {
  std::vector<Data> data = tuple.getData();

  switch (where.relation_character) {
    case LESS: {
      switch (where.data.type) {
        case -1:
          return (data[target_attr].datai < where.data.datai);
          break;
        case 0:
          return (data[target_attr].dataf < where.data.dataf);
          break;
        default:
          return (data[target_attr].datas < where.data.datas);
          break;
      }
    } break;
    case LESS_OR_EQUAL: {
      switch (where.data.type) {
        case -1:
          return (data[target_attr].datai <= where.data.datai);
          break;
        case 0:
          return (data[target_attr].dataf <= where.data.dataf);
          break;
        default:
          return (data[target_attr].datas <= where.data.datas);
          break;
      }
    } break;
    case EQUAL: {
      switch (where.data.type) {
        case -1:
          return (data[target_attr].datai == where.data.datai);
          break;
        case 0:
          return (data[target_attr].dataf == where.data.dataf);
          break;
        default:
          return (data[target_attr].datas == where.data.datas);
          break;
      }
    } break;
    case GREATER_OR_EQUAL: {
      switch (where.data.type) {
        case -1:
          return (data[target_attr].datai >= where.data.datai);
          break;
        case 0:
          return (data[target_attr].dataf >= where.data.dataf);
          break;
        default:
          return (data[target_attr].datas >= where.data.datas);
          break;
      }
    } break;
    case GREATER: {
      switch (where.data.type) {
        case -1:
          return (data[target_attr].datai > where.data.datai);
          break;
        case 0:
          return (data[target_attr].dataf > where.data.dataf);
          break;
        default:
          return (data[target_attr].datas > where.data.datas);
          break;
      }
    } break;
    case NOT_EQUAL: {
      switch (where.data.type) {
        case -1:
          return (data[target_attr].datai != where.data.datai);
          break;
        case 0:
          return (data[target_attr].dataf != where.data.dataf);
          break;
        default:
          return (data[target_attr].datas != where.data.datas);
          break;
      }
    } break;
    default:
      break;
  }

  return false;
}

