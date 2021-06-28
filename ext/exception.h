#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_ 1

#include <exception>

class table_exist : public std::exception {
  const char* what() const throw() { return "Table exist!"; }
};

class table_not_exist : public std::exception {
  const char* what() const throw() { return "Table not exist!"; }
};

class attribute_not_exist : public std::exception {
  const char* what() const throw() { return "Attribute not exist!"; }
};

class index_exist : public std::exception {
  const char* what() const throw() { return "Index exist!"; }
};

class index_not_exist : public std::exception {
  const char* what() const throw() { return "Index not exist!"; }
};

class tuple_type_conflict : public std::exception {
  const char* what() const throw() { return "Tuple type conflict!"; }
};

class primary_key_conflict : public std::exception {
  const char* what() const throw() { return "Primary key conflict!"; }
};

class data_type_conflict : public std::exception {
  const char* what() const throw() { return "Data type conflict!"; }
};

//增加
class index_full : public std::exception {};

class input_format_error : public std::exception {
  const char* what() const throw() { return "Input format error!"; }
};

class exit_command : public std::exception {};

class unique_conflict : public std::exception {
  const char* what() const throw() { return "Unique conflict!"; }
};

// Buffer_manager

class buffer_is_full : public std::exception {};

#endif
