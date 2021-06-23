#include "basic.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "tabulate.hpp"

Tuple::Tuple(const Tuple &tuple_in) {
  isDeleted_ = tuple_in.isDeleted_;
  for (int idx = 0; idx != tuple_in.data_.size(); idx++) {
    data_.push_back(tuple_in.data_[idx]);
  }
}

void Tuple::addData(Data data_in) { data_.push_back(data_in); }

std::vector<Data> Tuple::getData() const { return data_; }

int Tuple::getSize() { return data_.size(); }

bool Tuple::isDeleted() { return isDeleted_; }
void Tuple::setDeleted() { isDeleted_ = true; }

void Tuple::showTuple() {
  for (int idx = 0; idx < getSize(); idx++) {
    switch (data_[idx].type) {
      case -1:
        std::cout << data_[idx].datai << "\t";
        break;
      case 0:
        std::cout << data_[idx].dataf << "\t";
        break;
      default:
        std::cout << data_[idx].datas << "\t";
        break;
    }
  }
  std::cout << std::endl;
}

//构造函数
Table::Table(std::string title, Attribute attr) : title_(title) {
  title_ = title;
  index_.num = 0;
  attr_ = attr;
}
Table::Table(const Table &table_in) {
  title_ = table_in.title_;
  index_ = table_in.index_;
  attr_ = table_in.attr_;
  for (int i = 0; i < table_in.tuple_.size(); i++) tuple_.push_back(table_in.tuple_[i]);
}

void Table::addTuple(Tuple &t) { tuple_.push_back(t); }

int Table::setIndex(short index, std::string index_name) {
  /*
  三种情况插入失败：
      - 存在的索引数大于10；
      - 输入的index上已经存在索引
      - 输入的index_name已经被占用
  */
  if (index_.num >= 10) return 0;

  for (int idx = 0; idx < index_.num; idx++)
    if (index_.location[idx] == index) {
      std::cout << "Index on " << index << " already exists!" << std::endl;
      return 0;
    }

  for (int idx = 0; idx < index_.num; idx++)
    if (index_.indexname[idx] == index_name) {
      std::cout << "Index with name " << index_name << " already exists!" << std::endl;
      return 0;
    }

  index_.indexname[index_.num] = index_name;
  index_.location[index_.num] = index;
  index_.num++;
  attr_.has_index[index] = true;
  return 1;
}

int Table::dropIndex(std::string index_name) {
  /*
  一种情况删除索引失败：
      - 要删除的索引名不存在；
  */
  int idx;
  for (idx = 0; idx < index_.num; idx++)
    if (index_.indexname[idx] == index_name) break;

  if (idx == index_.num) {
    std::cout << "Index with name " << index_name << " does not exist!" << std::endl;
    return 0;
  }

  attr_.has_index[index_.location[idx]] = false;
  index_.indexname[idx] = index_.indexname[index_.num - 1];
  index_.location[idx] = index_.location[index_.num - 1];
  index_.num--;
  return 1;
}

// private的输出接口
std::string Table::getTitle() { return title_; }
Attribute Table::getAttr() { return attr_; }
std::vector<Tuple> &Table::getTuple() { return tuple_; }
Index Table::getIndex() { return index_; }

void addRow(tabulate::Table &tabu, std::vector<std::string> &row) {
  std::vector<variant<std::string, const char *, tabulate::Table>> mapped;
  for (const auto &it : row) {
    const auto varRow = variant<std::string, const char *, tabulate::Table>(it);
    mapped.push_back(varRow);
  }
  tabu.add_row(mapped);
}

void Table::showTable(int limit) {
  tabulate::Table tabu;
  std::vector<std::string> row;
  for (int i = 0; i < attr_.num; ++i) {
    row.push_back(attr_.name[i]);
  }

  addRow(tabu, row);

  for (int idx = 0; idx < std::min((int)tuple_.size(), limit); idx++) {
    row.clear();

    for (int k = 0; k < tuple_[idx].getData().size(); ++k) {
      switch (tuple_[idx].getData()[k].type) {
        case -1:
          row.push_back(std::to_string(tuple_[idx].getData()[k].datai));
          break;
        case 0:
          row.push_back(std::to_string(tuple_[idx].getData()[k].dataf));
          break;
        default:
          row.push_back(tuple_[idx].getData()[k].datas);
          break;
      }
    }
    addRow(tabu, row);
  }

  for (size_t i = 0; i < attr_.num; ++i) {
    tabu[0][i].format().font_color(tabulate::Color::yellow).font_align(tabulate::FontAlign::center).font_style({tabulate::FontStyle::bold});
  }

  std::cout << tabu << std::endl;
}

bool Data::operator<(const Data &rhs) const {
  assert(type == rhs.type);
  if (type == -1) {
    return datai < rhs.datai;
  } else if (type == 0) {
    return dataf < rhs.dataf;
  } else {
    return datas < rhs.datas;
  }
}

bool Tuple::operator<(const Tuple &rhs) const { return data_[0] < rhs.data_[0]; }

#ifdef __TEST_BS__

#include <iostream>
#include <string>
using namespace std;

int main() {
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
  a.name[0] = "int";
  a.name[1] = "str";
  a.type[0] = -1;
  a.type[1] = data2.type;

  Table table("MyFirstTable", a);
  table.addTuple(t);

  table.setIndex(0, "Index on int");
  table.setIndex(0, "Duplicate");
  table.setIndex(1, "Index on int");

  table.showTable();

  return 0;
}

#endif
