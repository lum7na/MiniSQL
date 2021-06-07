#include "interpreter.h"

#include <bits/stdc++.h>

#include "basic.h"
#include "peglib.h"

using namespace std;
using namespace peg;

Interpreter::Interpreter() {
  parser.load_grammar(R"(
    SQL <- SELECT / CREATE / DROP_TABLE / INDEX / DROP_INDEX / INSERT / DELETE / QUIT / EXEC
    SELECT <- 'select' '*' 'from' any_name ';' / 'select' '*' 'from' any_name 'where' any_name any_cond (any_logic any_name any_cond)* ';'
    DELETE <- 'delete' 'from' any_name ';' / 'delete' 'from' any_name 'where' any_name any_cond (any_logic any_name any_cond)* ';'
    CREATE <- 'create' 'table' any_name '(' CREATE_def*  'primary key' any_name ')'';'
    CREATE_def <- any_name any_type 'unique' ',' /  any_name any_type ',' 
    DROP_TABLE <- 'drop' 'table' any_name ';'
    INDEX <- 'create' 'index' any_name 'on' any_name '(' any_name ')' ';'
    DROP_INDEX <- 'drop' 'index' any_name ';'
    INSERT <- 'insert' 'into' any_name 'values' '(' (any_val ','?)* ')'  ';'
    QUIT <- 'quit;'
    EXEC <- 'execfile' any_name ';'
    any_type <- 'int' / 'float' / 'char(' any_int ')'
    any_name <- < [a-z0-9]+ >
    any_cond <- any_op any_val
    any_logic <- 'and' | 'or'
    any_op <- '<' | '<=' | '>' | '>=' | '!=' | '='
    any_val <- any_string / any_float / any_int
    any_string <- < '\'' [a-z0-9A-Z-_]+ '\'' > 
    any_int <-  < '-'?[0-9]+ >
    any_float <-  < '-'?[0-9]*'.'[0-9]+ >
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
    switch (vs.choice()) {
      case 0:
        return LESS;
      case 1:
        return LESS_OR_EQUAL;
      case 2:
        return GREATER;
      case 3:
        return GREATER_OR_EQUAL;
      case 4:
        return NOT_EQUAL;
      case 5:
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
  parser["SELECT"] = [this](const SemanticValues &vs) { EXEC_SELECT(vs); };
  parser.enable_packrat_parsing();  // Enable packrat parsing.
}

void Interpreter::getQuery() {
  string res;
  getline(cin, res);
  while (res.back() != ';') {
    string res_t;
    getline(cin, res_t);
    res += res_t;
  }
  parser.parse(res);
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
    op = (any_cast<LOGIC>(vs[i + 2]) == AND);
  }
  API api;
  output_table = api.selectRecord(table_name, target_name, where_select, op);
}

int main() {
  // (2) Make a parser

  // string val;
  // parser.parse("select * from school where score = 12.3123 and name = 'wang' and building = '7';");
  // cout << val << endl;
}
// select * from school where score = 12.3123 and name = 'wang' and building = '7';