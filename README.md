# MiniSQL

MiniSQL

header : [[https://github.com/Yusen1997/miniSQL]]

peglib: [[https://github.com/yhirose/cpp-peglib]]

tabulate: [[https://github.com/p-ranav/tabulate]]


注意：对于尚未完成的文件，暂时使用Yusen1997的文件作为替换。在完成后，请将ext或src文件夹内的文件替换。

编译：

```
mkdir build
cd build
cmake ..
make
```


PEG Grammer:

```bash
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
```