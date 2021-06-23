#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_ 1

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "api.h"
#include "basic.h"
#include "catalog_manager.h"
#include "peglib.h"
using namespace peg;

class Interpreter {
 public:
  Interpreter();

  //功能：获取一行输入的信息，并将输入的格式规范化
  //异常：无异常
  bool getQuery(int flag = 0);
  //输入：select last_name,first_name,birth,state from president where t1<10 and t2>20 ;
  //输出：Success或者异常
  //功能：进行选择操作，支持单表多约束
  //异常：格式错误则抛出input_format_error
  //如果表不存在，抛出table_not_exist异常
  //如果属性不存在，抛出attribute_not_exist异常
  //如果Where条件中的两个数据类型不匹配，抛出data_type_conflict异常
  void EXEC_SELECT(const SemanticValues &vs);
  //输入：drop table t1;
  //输出：Success或者异常
  //功能：删除表t1
  //异常：格式错误则抛出input_format_error
  //如果表不存在，抛出table_not_exist异常
  void EXEC_DROP_TABLE(const SemanticValues &vs);
  //输入：drop index ID_index on t1;
  //输出：Success或者异常
  //功能：在表t1中删除一个名字叫ID_index的索引
  //异常：格式错误则抛出input_format_error异常
  //如果表不存在，抛出table_not_exist异常
  //如果对应属性不存在，抛出attribute_not_exist异常
  //如果对应属性没有索引，抛出index_not_exist异常
  void EXEC_DROP_INDEX(const SemanticValues &vs);
  //输入：create table T1(
  //            NAME char(32),
  //            ID int unique,
  //            SCORE float,
  //            primary key (ID));
  //输出：Success或者异常
  //功能：在数据库中插入一个表的元信息
  //异常：格式错误则抛出input_format_error异常
  //如果表不存在，抛出table_not_exist异常
  void EXEC_CREATE_TABLE(const SemanticValues &vs);
  //输入：create index ID_index on t1 (id);
  //输出：Success或者异常
  //功能：在表中插入一个名字叫ID_index的索引，其对应属性为ID
  //异常：格式错误则抛出input_format_error异常
  //如果表不存在，抛出table_not_exist异常
  //如果对应属性不存在，抛出attribute_not_exist异常
  //如果对应属性已经有了索引，抛出index_exist异常
  void EXEC_CREATE_INDEX(const SemanticValues &vs);
  //输入：insert into T1 values('WuZhaoHui',0001,99.99);
  //输出：Success或者异常
  //功能：向T1内插入值的信息
  //异常：
  void EXEC_INSERT(const SemanticValues &vs);
  //输入：delete from MyClass where id=1;
  //     delete * from MyClass;
  // where中只存在一条信息
  //输出：Success或者异常
  //功能：从Myclass中删除id=1的元组
  //异常：格式错误则抛出input_format_error异常
  //如果表不存在，抛出table_not_exist异常
  //如果属性不存在，抛出attribute_not_exist异常
  //如果Where条件中的两个数据类型不匹配，抛出data_type_conflict异常
  void EXEC_DELETE(const SemanticValues &vs);
  //输入：exit;
  //功能：退出数据库
  void EXEC_EXIT();
  //输入：execfile 文件路径
  //功能：根据文件路径读取文件信息，并用于数据库的操作
  void EXEC_FILE(const SemanticValues &vs);

 private:
  std::map<std::string, std::string> idx2table;
  peg::parser parser;
};

template <class Type>
Type stringToNum(const std::string &str) {
  std::istringstream iss(str);
  Type num;
  iss >> num;
  return num;
}

#endif
