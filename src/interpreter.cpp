#include "interpreter.h"

#include <bits/stdc++.h>

using namespace std;
using namespace peg;

Interpreter::Interpreter() {
  parser.load_grammar(R"(
SQL <- SELECT / CREATE / DROP_TABLE / INDEX / DROP_INDEX / INSERT / DELETE / QUIT / EXEC 
SELECT <- 'select' '*' 'from' any_name ';' / 'select' '*' 'from' any_name 'where' any_name any_cond (any_logic any_name any_cond)* ';'
DELETE <- 'delete' 'from' any_name ';' / 'delete' 'from' any_name 'where' any_name any_cond (any_logic any_name any_cond)* ';'
CREATE <- 'create' 'table' any_name '(' CREATE_def*  'primary key' '(' any_name ')' ')'';'
CREATE_def <- any_name any_type 'unique' ',' /  any_name any_type ',' 
DROP_TABLE <- 'drop' 'table' any_name ';'
INDEX <- 'create' 'index' any_name 'on' any_name '(' any_name ')' ';'
DROP_INDEX <- 'drop' 'index' any_name ';'
INSERT <- 'insert' 'into' any_name 'values' '(' (any_val ','?)* ')'  ';'
QUIT <- 'quit;'
EXEC <- 'execfile' any_name ';'
any_type <- 'int' / 'float' / 'char' '(' any_int ')'
any_name <- < [a-z0-9]+ >
any_cond <- any_op ' ' any_val
any_logic <- w_and / w_or
any_op <- [<=>]*
any_val <- any_string / any_float / any_int
any_string <- < '\'' [a-z0-9A-Z-_]+ '\'' > 
any_int <-  < '-'?[0-9]+ >
any_float <-  < '-'?[0-9]*'.'[0-9]+ >
w_and <- 'and'
w_or <- 'or'
%whitespace <- [ \t]*
  )");
  assert(static_cast<bool>(parser) == true);

  // Setup actions
  parser["any_string"] = [](const SemanticValues &vs) {
    string s = vs.token_to_string();
    return s.substr(1, s.length() - 2);
  };
  parser["any_float"] = [](const SemanticValues &vs) { return vs.token_to_number<float>(); };
  parser["any_int"] = [](const SemanticValues &vs) { return vs.token_to_number<int>(); };
  parser["any_val"] = [](const SemanticValues &vs) {
    Data res;
    switch (vs.choice()) {
      case 0:
        res.datas = any_cast<string>(vs[0]);
        res.type = res.datas.length() + 1;
        break;
      case 1:
        res.dataf = any_cast<float>(vs[0]);
        res.type = 0;
        break;
      case 2:
        res.datai = any_cast<int>(vs[0]);
        res.type = -1;
        break;
    }
    return res;
  };
  parser["any_op"] = [](const SemanticValues &vs) {
    string op = vs.token_to_string();
    if (op == "<") {
      return LESS;
    } else if (op == "<=") {
      return LESS_OR_EQUAL;
    } else if (op == ">") {
      return GREATER;
    } else if (op == ">=") {
      return GREATER_OR_EQUAL;
    } else if (op == "!=") {
      return NOT_EQUAL;
    } else if (op == "=") {
      return EQUAL;
    }
    assert(0);
  };
  parser["any_logic"] = [](const SemanticValues &vs) {
    switch (vs.choice()) {
      case 0:
        return AND;
      case 1:
        return OR;
    }
    assert(0);
  };
  parser["any_cond"] = [](const SemanticValues &vs) {
    Where res;
    res.data = any_cast<Data>(vs[1]);
    res.relation_character = any_cast<WHERE>(vs[0]);
    return res;
  };
  parser["any_name"] = [](const SemanticValues &vs) { return vs.token_to_string(); };
  parser["any_type"] = [](const SemanticValues &vs) {
    switch (vs.choice()) {
      case 0:
        return -1;
      case 1:
        return 0;
      case 2:
        return any_cast<int>(vs[0]) + 1;
    }
    assert(0);
  };
  parser["CREATE_def"] = [](const SemanticValues &vs) {
    switch (vs.choice()) {
      case 0:
        return make_tuple(any_cast<string>(vs[0]), any_cast<int>(vs[1]), true);
      case 1:
        return make_tuple(any_cast<string>(vs[0]), any_cast<int>(vs[1]), false);
    }
    assert(0);
  };
  parser["SELECT"] = [this](const SemanticValues &vs) { EXEC_SELECT(vs); };
  parser["CREATE"] = [this](const SemanticValues &vs) { EXEC_CREATE_TABLE(vs); };
  parser["INSERT"] = [this](const SemanticValues &vs) { EXEC_INSERT(vs); };
  parser["INDEX"] = [this](const SemanticValues &vs) { EXEC_CREATE_INDEX(vs); };
  parser.enable_packrat_parsing();  // Enable packrat parsing.
}

bool Interpreter::getQuery() {
  string res;
  getline(cin, res);
  if (res == "quit;") {
    return false;
  }
  while (res.back() != ';') {
    string res_t;
    getline(cin, res_t);
    res += res_t;
  }
  parser.parse(res);
  return true;
}

void Interpreter::EXEC_SELECT(const SemanticValues &vs) {
  string table_name = any_cast<string>(vs[0]);
  vector<string> attr_name;
  vector<string> target_name;
  vector<Where> where_select;
  Table output_table;
  char op = 0;
  for (int i = 1; i < vs.size(); i += 3) {
    target_name.push_back(any_cast<string>(vs[i]));
    where_select.push_back(any_cast<Where>(vs[i + 1]));
    if (i + 2 < vs.size()) {
      op = (any_cast<LOGIC>(vs[i + 2]) == AND);
    }
  }
  API api;
  output_table = api.selectRecord(table_name, target_name, where_select, op);
  output_table.showTable();
  std::cerr << output_table.getTuple().size() << std::endl;
}

void Interpreter::EXEC_CREATE_TABLE(const SemanticValues &vs) {
  string table_name = any_cast<string>(vs[0]);
  Index index;
  index.num = 0;
  Attribute attr;
  int attr_cnt = vs.size() - 2;
  attr.num = attr_cnt;
  attr.primary_key = -1;
  string primary_key = any_cast<string>(vs.back());
  for (int i = 0; i < attr_cnt; ++i) {
    auto def = any_cast<tuple<string, int, bool> >(vs[i + 1]);
    attr.name[i] = get<0>(def);
    attr.type[i] = get<1>(def);
    attr.unique[i] = get<2>(def);
    attr.has_index[i] = false;
    if (attr.name[i] == primary_key) {
      attr.primary_key = i;
    }
  }
  API api;
  api.createTable(table_name, attr, attr.primary_key, index);
}

void Interpreter::EXEC_INSERT(const SemanticValues &vs) {
  string table_name = any_cast<string>(vs[0]);
  Tuple tuple;
  for (int i = 1; i < vs.size(); ++i) {
    tuple.addData(any_cast<Data>(vs[i]));
  }
  API api;
  api.insertRecord(table_name, tuple);
}

void Interpreter::EXEC_CREATE_INDEX(const SemanticValues &vs) {
  string index_name = any_cast<string>(vs[0]);
  string table_name = any_cast<string>(vs[1]);
  string attr_name = any_cast<string>(vs[2]);
  API api;
  api.createIndex(table_name, index_name, attr_name);
}