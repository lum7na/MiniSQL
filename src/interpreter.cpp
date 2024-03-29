#include "interpreter.h"

#include <bits/stdc++.h>

using namespace std;
using namespace peg;

Interpreter::Interpreter() {
  parser.log = [](size_t line, size_t col, const string &msg) {
    cerr << "Please check the input format."
         << "\n";
    cerr << line << ":" << col << ": " << msg << endl;
  };
  parser.load_grammar(R"(
SQL <- SELECT / CREATE / DROP_TABLE / INDEX / DROP_INDEX / INSERT / DELETE / QUIT / EXEC 
SELECT <- 'select' '*' 'from' any_name ';' / 'select' '*' 'from' any_name 'where' any_name any_cond ('and' any_name any_cond)* ';'
DELETE <- 'delete' 'from' any_name ';' / 'delete' 'from' any_name 'where' any_name any_cond ('and' any_name any_cond)* ';'
CREATE <- 'create' 'table' any_name '(' CREATE_def*  'primary key' '(' any_name ')' ')'';'
CREATE_def <- any_name any_type 'unique' ',' /  any_name any_type ',' 
DROP_TABLE <- 'drop' 'table' any_name ';'
INDEX <- 'create' 'index' any_name 'on' any_name '(' any_name ')' ';'
DROP_INDEX <- 'drop' 'index' any_name ';'
INSERT <- 'insert' 'into' any_name 'values' '(' (any_val ','?)* ')'  ';'
QUIT <- 'quit;'
EXEC <- 'execfile' any_filename ';'
any_type <- 'int' / 'float' / 'char' '(' any_int ')'
any_name <- < [A-Za-z0-9_]+ >
any_cond <- any_op ' ' any_val
any_op <- [<=>]*
any_val <- any_string / any_float / any_int
any_string <- < '\'' [a-z0-9A-Z-_ ]+ '\'' > 
any_int <-  < '-'?[0-9]+ >
any_float <-  < '-'?[0-9]*'.'[0-9]+ >
any_filename <- < [A-Za-z0-9_.]+ >
%whitespace <- [ \t\n]*
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
    } else if (op == "<>") {
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
  parser["any_filename"] = [](const SemanticValues &vs) { return vs.token_to_string(); };
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
  parser["DELETE"] = [this](const SemanticValues &vs) { EXEC_DELETE(vs); };
  parser["DROP_TABLE"] = [this](const SemanticValues &vs) { EXEC_DROP_TABLE(vs); };
  parser["DROP_INDEX"] = [this](const SemanticValues &vs) { EXEC_DROP_INDEX(vs); };
  parser["EXEC"] = [this](const SemanticValues &vs) { EXEC_FILE(vs); };
  parser.enable_packrat_parsing();  // Enable packrat parsing.
}

bool Interpreter::getQuery(istream &is, int flag) {
  if (!flag) {
    cout << ">>> ";
  }
  string res;
  getline(is, res);
  if (is.eof()) {
    return false;
  }

  if (res == "quit;") {
    if (!flag) {
      API api;
      api.quit();
    }
    return false;
  }
  while (res.back() != ';') {
    string res_t;
    if (!flag) {
      cout << ">>> ";
    }
    getline(is, res_t);
    res += res_t;
  }
  if (!parser.parse(res)) {
    throw input_format_error();
  }
  return true;
}

void Interpreter::EXEC_SELECT(const SemanticValues &vs) {
  string table_name = any_cast<string>(vs[0]);
  vector<string> attr_name;
  vector<string> target_name;
  vector<Where> where_select;
  Table output_table;
  for (int i = 1; i < vs.size(); i += 2) {
    target_name.push_back(any_cast<string>(vs[i]));
    where_select.push_back(any_cast<Where>(vs[i + 1]));
  }
  API api;
  output_table = api.selectRecord(table_name, target_name, where_select);
  if (IO) {
    chrono::steady_clock::time_point time_start = chrono::steady_clock::now();
    output_table.showTable();
    chrono::steady_clock::time_point time_end = chrono::steady_clock::now();
    io_timer += chrono::duration_cast<chrono::duration<double>>(time_end - time_start);
  }
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
    auto def = any_cast<tuple<string, int, bool>>(vs[i + 1]);
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
  idx2table[index_name] = table_name;
  API api;
  api.createIndex(table_name, index_name, attr_name);
}

void Interpreter::EXEC_DELETE(const SemanticValues &vs) {
  string table_name = "";
  vector<string> attr_name;
  vector<Where> where;
  switch (vs.choice()) {
    case 0: {
      table_name = any_cast<string>(vs[0]);
      break;
    }
    case 1: {
      table_name = any_cast<string>(vs[0]);
      for (int i = 1; i < vs.size(); ++i) {
        attr_name.push_back(any_cast<string>(vs[i]));
        where.push_back(any_cast<Where>(vs[++i]));
      }
      break;
    }
  }
  API api;
  int res = api.deleteRecord(table_name, attr_name, where);
  cout << "delete " << res << " records." << endl;
}

void Interpreter::EXEC_DROP_TABLE(const SemanticValues &vs) {
  string table_name = any_cast<string>(vs[0]);
  API api;
  api.dropTable(table_name);
}

void Interpreter::EXEC_DROP_INDEX(const SemanticValues &vs) {
  string index_name = any_cast<string>(vs[0]);
  API api;
  api.dropIndex(idx2table[index_name], index_name);
}

void Interpreter::EXEC_FILE(const SemanticValues &vs) {
  chrono::steady_clock::time_point time_start = chrono::steady_clock::now();
  io_timer = chrono::duration<double>::zero();
  string filename = any_cast<string>(vs[0]);
  ifstream ifs(filename);
  if (!ifs.is_open()) {
    throw file_not_exist();
  }
  int num = 0;
  while (getQuery(ifs, 1)) {
    ++num;
  };
  chrono::steady_clock::time_point time_end = chrono::steady_clock::now();
  chrono::duration<double> time_used = chrono::duration_cast<chrono::duration<double>>(time_end - time_start) - io_timer;
  cout << ">>> " << num << " query finish in " << time_used.count() << "s" << endl;
  ifs.close();
}

void Interpreter::closeIO() { IO = false; }