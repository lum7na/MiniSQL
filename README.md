# MiniSQL

MiniSQL

header : [[https://github.com/Yusen1997/miniSQL]]

peglib: [[https://github.com/yhirose/cpp-peglib]]

tabulate: [[https://github.com/p-ranav/tabulate]]

编译：

```
mkdir build
cd build
cmake ..
make
```

- `execfile`会计算执行文件的时间，注意该时间不包括输出表格的时间。
- 可以添加参数运行：`./minisql without_io`，这时SELECT最终不会输出表格，方便测量时间

