#ifndef _BPLUSTREE_H_
#define _BPLUSTREE_H_ 1

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "basic.h"
#include "buffer_manager.h"
#include "const.h"
#include "exception.h"
#include "template_function.h"

extern BufferManager buffer_manager;

using namespace std;

template <typename T>
class TreeNode {
 public:
  // int             num;                                 //num是节点数量，可是有了vector就不需要了
  bool isLeaf;    // false表示是非叶节点
  vector<T> key;  // key值
  int Degree;

  bool switchline;

 public:
  TreeNode(int Degree_, bool isLeaf_);
  ~TreeNode();
  virtual TreeNode<T>* Find(T ele, int& index) = 0;  //虚函数，需要重载，因为叶节点与非叶节点的查找方式不一样。
  virtual bool Insert(T ele, int val) = 0;
  virtual bool Delete(T ele) = 0;
  bool isFull();
  bool isHalfEmpty();
};

template <typename T>
class NonLeaf : public TreeNode<T> {
 public:
  vector<TreeNode<T>*> child;  // child值
 public:
  NonLeaf(int Degree_);
  TreeNode<T>* Find(T ele, int& index);  //返回值表示找到或者没找到，index用于存储找到时的key值下标值。注意是key值！！！！！！！！！
  bool Insert(T ele, int val);           //将ele插入节点中，并判断是否需要split，如果是，则split之
  bool Delete(T ele);

  void SplitLeaf(int i);            //当下标为i的子节点满时，分裂之，同时修改当前节点的key值与child值。
  void SplitNonLeaf(int i);         //当下标为i的子节点满时，分裂之，同时修改当前节点的key值与child值。
  void RedistributeLeaf(int i);     //当下标为i的叶子节点个数小于(n-1)/2时，分裂之，同时修改当前节点的key值与child值。
  void RedistributeNonLeaf(int i);  //当下标为i的非叶子节点个数小于(n-1)/2时，分裂之，同时修改当前节点的key值与child值。
};

template <typename T>
class Leaf : public TreeNode<T> {
 public:
  TreeNode<T>* NextLeaf;  //指向下一个叶节点
  vector<int> vals;       //对应的record的指针
 public:
  Leaf(int Degree_);
  TreeNode<T>* Find(T ele, int& index);
  bool Insert(T ele, int val);
  bool Delete(T ele);
};

template <typename T>
class BPlusTree {
 private:
  typedef TreeNode<T>* Tree;
  std::string file_name;
  Tree root;
  int Degree;
  // fileNode* file; // the filenode of this tree
  //每个key值的size，对于一颗树来说，所有key的size应是相同的
  int key_size;
  int size = 0;

 public:
  //构造函数
  //用于构造一颗新的树，确定m_name,key的size，树的度
  //同时调用其他函数为本树分配内存
  BPlusTree(std::string m_name, int key_size, int degree);
  //析构函数
  //释放相应的内存
  ~BPlusTree();
  //输入：树的根结点
  //功能：删除整棵树并释放内存空间，主要用在析构函数中
  void dropTree(Tree node);

  //根据key值返回对应的Value
  int searchVal(T& ele);
  //输入：key1，key2，返回vals的容器
  //功能：返回范围搜索结果，将value放入vals容器中
  void searchRange(T& key1, T& key2, std::vector<int>& vals, int flag);
  //输入：key值及其value
  //输出：bool
  //功能：在树中插入一个key值
  //返回是否插入成功
  bool insertKey(T key, int val);
  //输入：key值
  //输出：bool
  //功能：在树中删除一个key值
  //返回是否删除成功
  bool deleteKey(T key);

  void getFile(std::string file_path);
  int getBlockNum(std::string table_name);
  //从磁盘读取所有数据
  void readFromDiskAll();
  //将新数据写入磁盘
  void writtenbackToDiskAll();
  //在磁盘中读取某一块的数据
  void readFromDisk(char* p, char* end);

  void printleaf();
  void printTree();
};

/************************************/
/*          TreeNode函数            */
/************************************/
template <typename T>
TreeNode<T>::TreeNode(int Degree_, bool isLeaf_)
    : Degree(Degree_),
      isLeaf(isLeaf_),

      switchline(false)

{}

template <typename T>
TreeNode<T>::~TreeNode() {}

template <typename T>
bool TreeNode<T>::isFull() {
  if (isLeaf && key.size() == Degree) {
    return true;
  } else if (!isLeaf && key.size() == Degree - 1) {
    return true;
  } else
    return false;
}

template <typename T>
bool TreeNode<T>::isHalfEmpty() {
  if (isLeaf && key.size() < ceil((double)(Degree - 1) / 2)) {
    return true;
  } else if (!isLeaf && key.size() < ceil((double)(Degree - 1) / 2) - 1) {
    return true;
  } else
    return false;
}

/************************************/
/*          NonLeaf函数             */
/************************************/
template <typename T>
NonLeaf<T>::NonLeaf(int Degree_) : TreeNode<T>(Degree_, false) {}

//输入：待查找元素ele
//功能：向下一层中合适位置节点查找元素ele
//返回：ele的block number，即vals；若不存在，则返回-1
template <typename T>
TreeNode<T>* NonLeaf<T>::Find(T ele, int& index) {
  // num值较小，用线性查找
  if (this->key.size() <= 20) {
    for (int i = 0; i < this->key.size() - 1; i++) {
      if (this->key[i] > ele) {
        return child[0]->Find(ele, index);
      } else if (this->key[i] <= ele && this->key[i + 1] > ele) {
        return child[i + 1]->Find(ele, index);
      }
    }
    return child[child.size() - 1]->Find(ele, index);
  }
  // num值较大，用二分搜索
  else {
    int l = 0;
    int r = this->key.size() - 1;
    while (l < r) {
      int mid = (l + r + 1) / 2;
      if (this->key[mid] <= ele) {
        l = mid;
      } else {
        r = mid - 1;
      }
    }
    if (this->key[l] > ele)
      return child[0]->Find(ele, index);
    else
      return child[l + 1]->Find(ele, index);
  }
}

//输入：子节点(child)的序号i
//功能：将序号为i的子节点(child)分裂为两个节点，各自分配key值与record，并更新当前节点的key值与child值
template <typename T>
void NonLeaf<T>::SplitLeaf(int i) {
  //新节点的初始化
  TreeNode<T>* new_child = new Leaf<T>(this->Degree);

  int new_len = (this->Degree) / 2;

  // key的转移
  for (int j = new_len; j > 0; j--) {
    dynamic_cast<Leaf<T>*>(new_child)->vals.push_back(*(dynamic_cast<Leaf<T>*>(child[i])->vals.end() - j));
    new_child->key.push_back(*(child[i]->key.end() - j));
  }
  child[i]->key.erase(child[i]->key.end() - new_len, child[i]->key.end());
  dynamic_cast<Leaf<T>*>(child[i])->vals.erase(dynamic_cast<Leaf<T>*>(child[i])->vals.end() - new_len, dynamic_cast<Leaf<T>*>(child[i])->vals.end());

  dynamic_cast<Leaf<T>*>(new_child)->NextLeaf = dynamic_cast<Leaf<T>*>(child[i])->NextLeaf;
  dynamic_cast<Leaf<T>*>(child[i])->NextLeaf = new_child;

  T up_key = new_child->key[0];
  this->key.insert(this->key.begin() + i, up_key);
  child.insert(child.begin() + i + 1, new_child);
}

//输入：子节点(child)的序号i
//功能：将序号为i的子节点(child)分裂为两个节点，各自分配key值与record，并更新当前节点的key值与child值
template <typename T>
void NonLeaf<T>::SplitNonLeaf(int i) {
  //新节点的初始化
  //这个地方初始化有问题！！！！！
  TreeNode<T>* new_child = new NonLeaf<T>(this->Degree);

  int new_len = (this->Degree - 2) / 2;
  int old_len = this->Degree - new_len - 2;

  T up_key = child[i]->key[old_len];

  // key的转移
  for (int j = new_len; j > 0; j--) {
    new_child->key.push_back(*(child[i]->key.end() - j));
  }
  child[i]->key.erase(child[i]->key.end() - new_len - 1, child[i]->key.end());
  // child的转移
  new_len = this->Degree / 2;
  for (int j = new_len; j > 0; j--) {
    dynamic_cast<NonLeaf<T>*>(new_child)->child.push_back(*(dynamic_cast<NonLeaf<T>*>(child[i])->child.end() - j));
  }
  dynamic_cast<NonLeaf<T>*>(child[i])->child.erase(dynamic_cast<NonLeaf<T>*>(child[i])->child.end() - new_len, dynamic_cast<NonLeaf<T>*>(child[i])->child.end());

  //对子节点处理完后，处理当前节点
  this->key.insert(this->key.begin() + i, up_key);
  child.insert(child.begin() + i + 1, new_child);
}

//输入：子节点(child)的序号i
//功能：当序号为i的节点key数过小（小于(n-1)/2）时，将序号为i的子节点(child)与左右的孩子节点合并并重新分配；其中优先与左兄弟合并，否则与右兄弟合并
template <typename T>
void NonLeaf<T>::RedistributeLeaf(int i) {
  if (i == 0) {
    int total_num = this->child[i]->key.size() + this->child[i + 1]->key.size();

    if (total_num <= this->Degree - 1) {  //如果可以直接合并
      //合并key
      for (int j = 0; j < child[i + 1]->key.size(); j++) {
        child[i]->key.push_back(child[i + 1]->key[j]);
      }
      //合并record
      for (int j = 0; j < dynamic_cast<Leaf<T>*>(child[i + 1])->vals.size(); j++) {
        dynamic_cast<Leaf<T>*>(child[i])->vals.push_back(dynamic_cast<Leaf<T>*>(child[i + 1])->vals[j]);
      }
      //调整nextLeaf
      dynamic_cast<Leaf<T>*>(child[i])->NextLeaf = dynamic_cast<Leaf<T>*>(child[i + 1])->NextLeaf;
      //删除多余的key与record
      TreeNode<T>* tempnode = child[i + 1];
      this->key.erase(this->key.begin() + i, this->key.begin() + i + 1);
      child.erase(child.begin() + i + 1, child.begin() + i + 2);
      delete tempnode;
    } else {
      int new_len, left_len;
      new_len = total_num / 2;
      left_len = new_len - this->child[i]->key.size();
      //对key值的重分配
      for (int j = 0; j < left_len; j++) {
        child[i]->key.push_back(child[i + 1]->key[j]);
      }
      child[i + 1]->key.erase(child[i + 1]->key.begin(), child[i + 1]->key.begin() + left_len - 1);
      //对record的重分配
      for (int j = 0; j < left_len; j++) {
        dynamic_cast<Leaf<T>*>(child[i])->vals.push_back(dynamic_cast<Leaf<T>*>(child[i + 1])->vals[j]);
      }
      dynamic_cast<Leaf<T>*>(child[i + 1])->vals.erase(dynamic_cast<Leaf<T>*>(child[i + 1])->vals.begin(), dynamic_cast<Leaf<T>*>(child[i + 1])->vals.begin() + left_len - 1);
      //然后修改对应的key值
      this->key[i] = child[i + 1]->key[0];
    }
  }

  else {
    int total_num = this->child[i]->key.size() + this->child[i - 1]->key.size();
    if (total_num <= this->Degree - 1) {
      //合并key
      for (int j = 0; j < child[i]->key.size(); j++) {
        child[i - 1]->key.push_back(child[i]->key[j]);
      }
      //合并record
      for (int j = 0; j < dynamic_cast<Leaf<T>*>(child[i])->vals.size(); j++) {
        dynamic_cast<Leaf<T>*>(child[i - 1])->vals.push_back(dynamic_cast<Leaf<T>*>(child[i])->vals[j]);
      }
      //调整NextLeaf
      dynamic_cast<Leaf<T>*>(child[i - 1])->NextLeaf = dynamic_cast<Leaf<T>*>(child[i])->NextLeaf;
      //删除多余的key与child
      TreeNode<T>* tempnode = child[i];
      this->key.erase(this->key.begin() + i - 1, this->key.begin() + i);
      child.erase(child.begin() + i, child.begin() + i + 1);
      delete tempnode;
    } else {
      int new_len, left_len;
      new_len = total_num / 2;
      left_len = new_len - this->child[i]->key.size();
      //对key值的重分配
      for (int j = 0; j < left_len; j++) {
        child[i]->key.insert(child[i]->key.begin(), child[i - 1]->key[j]);
      }
      child[i - 1]->key.erase(child[i - 1]->key.end() - left_len, child[i - 1]->key.end());
      //对record的重分配
      for (int j = 0; j < left_len; j++) {
        dynamic_cast<Leaf<T>*>(child[i])->vals.insert(dynamic_cast<Leaf<T>*>(child[i])->vals.begin(), dynamic_cast<Leaf<T>*>(child[i - 1])->vals[j]);
      }
      dynamic_cast<Leaf<T>*>(child[i - 1])->vals.erase(dynamic_cast<Leaf<T>*>(child[i - 1])->vals.end() - left_len, dynamic_cast<Leaf<T>*>(child[i - 1])->vals.end());
      //然后修改对应的key值
      this->key[i - 1] = child[i]->key[0];
    }
  }
}

//输入：子节点(child)的序号i
//功能：当序号为i的节点key数过小（小于(n-1)/2）时，将序号为i的子节点(child)与左右的孩子节点合并并重新分配；其中优先与左兄弟合并，否则与右兄弟合并
template <typename T>
void NonLeaf<T>::RedistributeNonLeaf(int i) {
  if (i == 0) {
    int total_num = this->child[i]->key.size() + this->child[i + 1]->key.size();
    if (total_num <= this->Degree - 2) {
      //合并key
      child[i]->key.push_back(this->key[i]);
      for (int j = 0; j < child[i + 1]->key.size(); j++) {
        child[i]->key.push_back(child[i + 1]->key[j]);
      }
      //合并child
      for (int j = 0; j < dynamic_cast<NonLeaf<T>*>(child[i + 1])->child.size(); j++) {
        dynamic_cast<NonLeaf<T>*>(child[i])->child.push_back(dynamic_cast<NonLeaf<T>*>(child[i + 1])->child[j]);
      }
      //删除多余的key与child
      TreeNode<T>* tempnode = child[i + 1];
      this->key.erase(this->key.begin() + i, this->key.begin() + i + 1);
      child.erase(child.begin() + i + 1, child.begin() + i + 2);
      delete tempnode;
    } else {
      int new_len, left_len;
      new_len = total_num / 2;
      left_len = new_len - this->child[i]->key.size();
      //对key值的重分配
      child[i]->key.push_back(this->key[i]);  //首先把分割两个child的key值放进child[i]里
      for (int j = 0; j < left_len - 1; j++) {
        child[i]->key.push_back(child[i + 1]->key[j]);
      }
      child[i + 1]->key.erase(child[i + 1]->key.begin(), child[i + 1]->key.begin() + left_len - 1);
      //对child的重分配
      for (int j = 0; j < left_len; j++) {
        dynamic_cast<NonLeaf<T>*>(child[i])->child.push_back(dynamic_cast<NonLeaf<T>*>(child[i + 1])->child[j]);
      }
      dynamic_cast<NonLeaf<T>*>(child[i + 1])->child.erase(dynamic_cast<NonLeaf<T>*>(child[i + 1])->child.begin(), dynamic_cast<NonLeaf<T>*>(child[i + 1])->child.begin() + left_len);

      //然后修改对应的key值
      this->key[i] = child[i + 1]->key[0];
    }

  } else {
    int total_num = this->child[i]->key.size() + this->child[i - 1]->key.size();
    if (total_num <= this->Degree - 2) {
      //合并key
      child[i - 1]->key.push_back(this->key[i - 1]);
      for (int j = 0; j < child[i]->key.size(); j++) {
        child[i - 1]->key.push_back(child[i]->key[j]);
      }
      //合并child
      for (int j = 0; j < dynamic_cast<NonLeaf<T>*>(child[i])->child.size(); j++) {
        dynamic_cast<NonLeaf<T>*>(child[i - 1])->child.push_back(dynamic_cast<NonLeaf<T>*>(child[i])->child[j]);
      }
      //删除多余的key与child
      TreeNode<T>* tempnode = child[i];
      this->key.erase(this->key.begin() + i - 1, this->key.begin() + i);
      child.erase(child.begin() + i, child.begin() + i + 1);
      delete tempnode;
    } else {
      int new_len, left_len;
      new_len = total_num / 2;
      left_len = new_len - this->child[i]->key.size();
      //对key值的重分配
      child[i]->key.insert(child[i]->key.begin(), this->key[i - 1]);
      for (int j = 0; j < left_len - 1; j++) {
        child[i]->key.insert(child[i]->key.begin(), *(child[i - 1]->key.end() - 1 - j));
      }
      child[i - 1]->key.erase(child[i - 1]->key.end() - left_len - 1, child[i - 1]->key.end());
      //对child的重分配
      for (int j = 0; j < left_len; j++) {
        dynamic_cast<NonLeaf<T>*>(child[i])->child.insert(dynamic_cast<NonLeaf<T>*>(child[i])->child.begin(), *(dynamic_cast<NonLeaf<T>*>(child[i - 1])->child.end() - 1 - j));
      }
      //然后修改对应的key值
      this->key[i - 1] = *(dynamic_cast<NonLeaf<T>*>(child[i - 1])->key.end() - 1);
      child[i - 1]->key.pop_back();
    }
  }
}

//输入：待插入的key值ele，record在的块号val
//功能：向下层节点中插入对应的record，成功返回true，失败返回false
//输出：插入是否成功，成功返回true，失败返回false
template <typename T>
bool NonLeaf<T>::Insert(T ele, int val) {
  int i;
  //先找到小于等于ele的最大key值位置，i
  if (this->key[0] > ele) {
    i = -1;
  } else if (this->key.size() <= 20) {
    for (i = 0; i < this->key.size() - 1; i++) {
      if (this->key[i]<ele&& this->key[i + 1]> ele) break;
    }
  } else {
    int l = 0;
    int r = this->key.size() - 1;
    while (i < r) {
      int mid = (l + r + 1) / 2;
      if (this->key[mid] <= ele)
        l = mid;
      else
        r = mid - 1;
    }
    if (this->key[i] > ele)
      i = -1;
    else
      i = l;
  }
  bool insert_result;
  insert_result = child[i + 1]->Insert(ele, val);
  //若插入成功，还需要做分裂
  if (insert_result) {
    bool full_res = child[i + 1]->isFull();
    if (full_res == true && child[i + 1]->isLeaf) {
      SplitLeaf(i + 1);
    } else if (full_res == true) {
      SplitNonLeaf(i + 1);
    }
  }
  return insert_result;
}

template <typename T>
bool NonLeaf<T>::Delete(T ele) {
  int i;
  //先找到小于等于ele的最大key值位置，i
  if (this->key[0] > ele) {
    i = -1;
  } else if (this->key.size() <= 20) {
    for (i = 0; i < this->key.size() - 1; i++) {
      if (this->key[i]<ele&& this->key[i + 1]> ele) break;
    }
  } else {
    int l = 0;
    int r = this->key.size() - 1;
    while (i < r) {
      int mid = (l + r + 1) / 2;
      if (this->key[mid] <= ele)
        l = mid;
      else
        r = mid - 1;
    }
    if (this->key[i] > ele)
      i = -1;
    else
      i = l;
  }
  bool delete_result;
  delete_result = child[i + 1]->Delete(ele);
  //若删除成功，还需要做重分配
  if (delete_result) {
    if (this->key[i] == ele) {
      TreeNode<T>* tempnode;
      tempnode = child[i + 1];
      while (!tempnode->isLeaf) {
        tempnode = dynamic_cast<NonLeaf<T>*>(tempnode)->child[0];
      }
      this->key[i] = tempnode->key[0];
    }
    bool empty_res = child[i + 1]->isHalfEmpty();
    if (empty_res == true && child[i + 1]->isLeaf == true) {
      RedistributeLeaf(i + 1);
    } else if (empty_res == true) {
      RedistributeNonLeaf(i + 1);
    }
  }

  return delete_result;
}

/************************************/
/*             Leaf函数             */
/************************************/
//输入：待查找的元素ele，用于传递索引的引用index
//功能：在对应叶节点中查找ele，其中：
//      1.根节点是叶节点，可能出现key[0]>ele，此时返回NULL，index等于0；也可能根节点中没有key值，返回NULL，index=-1
//      2.根节点是非叶节点，一定满足key[0]<=ele
//对于所有key[0]<=ele的情况，都返回当前叶节点的指针；若ele在节点中，index为ele在节点中的下标；若不在节点中，index为小于等于ele的最大节点下标 - key.size()
template <typename T>
Leaf<T>::Leaf(int Degree_) : TreeNode<T>(Degree_, true), NextLeaf(NULL) {}

template <typename T>
TreeNode<T>* Leaf<T>::Find(T ele, int& index) {
  //排除一些极端情况
  if (this->key.size() == 0) {
    index = -1;
    return NULL;
  } else if (*(this->key.begin()) > ele) {
    index = 0;
    return NULL;  //这种情况只有可能是根节点为叶节点的情况时发生
  } else if (*(this->key.end() - 1) < ele) {
    index = -1;
    return this;
  }
  // num值较小，用线性查找
  else if (this->key.size() <= 20) {
    index = 0;
    for (int i = 0; i < this->key.size(); i++) {
      if (this->key[i] == ele) {
        index = i;
        return this;
      } else if (i < this->key.size() - 1 && this->key[i]<ele&& this->key[i + 1]> ele) {
        index = i - this->key.size();
        return this;
      }
    }
    return this;
  }
  // num  值较大，用二分搜索
  else {
    int l = 0;
    int r = this->key.size() - 1;
    while (l < r) {
      int mid = (l + r + 1) / 2;
      if (this->key[mid] <= ele)
        l = mid;
      else
        r = mid - 1;
    }
    index = l;
    if (this->key[l] < ele) index -= this->key.size();
    return this;
  }
}

//输入：待插入的key值ele，record在的块号val
//功能：向叶子节点中插入对应的record，成功返回true，失败返回false
//输出：插入是否成功，成功返回true，失败返回false
template <typename T>
bool Leaf<T>::Insert(T ele, int val) {
  int index;
  TreeNode<T>* find_result = Find(ele, index);
  if (find_result != NULL && index >= 0) {
    cout << "Key already in Bplus Tree! Insert failed." << endl;
    return false;
  } else if (find_result == NULL) {
    this->key.insert(this->key.begin(), ele);
    vals.insert(vals.begin(), val);
    return true;
  } else {
    index += this->key.size();
    this->key.insert(this->key.begin() + index + 1, ele);
    vals.insert(vals.begin() + index + 1, val);
    return true;
  }
}

template <typename T>
bool Leaf<T>::Delete(T ele) {
  int index;
  TreeNode<T>* find_result = Find(ele, index);
  if (find_result == NULL || index < 0) {
    cout << "Key not in Bplus tree! Delete failed!" << endl;
    return false;
  } else {
    this->key.erase(this->key.begin() + index, this->key.begin() + index + 1);
    vals.erase(vals.begin() + index, vals.begin() + index + 1);
    return true;
  }
}

/************************************/
/*          BPlusTree函数           */
/************************************/
//构造函数，初始化文件，初始化根节点
template <typename T>
BPlusTree<T>::BPlusTree(std::string m_name, int key_size, int degree) : file_name(m_name), root(NULL), key_size(key_size), Degree(degree) {
  //初始化分配内存并从磁盘读取数据
  //创建索引
  root = new Leaf<T>(degree);
  readFromDiskAll();
}

template <typename T>
BPlusTree<T>::~BPlusTree() {
  dropTree(root);
  root = NULL;
}

template <typename T>
int BPlusTree<T>::searchVal(T& ele) {
  int index;
  TreeNode<T>* find_result = root->Find(ele, index);
  if (find_result != NULL && index >= 0) {
    return dynamic_cast<Leaf<T>*>(find_result)->vals[index];
  } else {
    cout << "Key not in BPlus tree! Findkey failed." << endl;
    return -1;
  }
}

// flag有三种类型，0表示在data1与data2之间搜索，1表示从-INF搜索到data2，2表示从data1搜索到INF
template <typename T>
void BPlusTree<T>::searchRange(T& key1, T& key2, std::vector<int>& vals, int flag) {
  int index = 0;
  switch (flag) {
    case 0: {
      TreeNode<T>* fir;
      fir = root->Find(key1, index);
      if (fir == NULL && index == -1) {
        cout << "Bplus tree empty! Search failed." << endl;
      } else if (fir == NULL && index == 0) {
        fir = root;
        for (int i = 0; i < root->key.size() && root->key[i] <= key2; i++) {
          vals.push_back(dynamic_cast<Leaf<T>*>(root)->vals[i]);
        }
      } else {
        if (index < 0) {
          index += fir->key.size();
          index += 1;
          if (index == fir->key.size()) {
            fir = dynamic_cast<Leaf<T>*>(fir)->NextLeaf;
            index = 0;
          }
        }
        while (fir != NULL && fir->key[index] <= key2) {
          vals.push_back(dynamic_cast<Leaf<T>*>(fir)->vals[index]);
          index++;
          if (index == fir->key.size()) {
            index = 0;
            fir = dynamic_cast<Leaf<T>*>(fir)->NextLeaf;
          }
        }
      }
      break;
    }
    case 1: {
      TreeNode<T>* fir = root;
      while (!fir->isLeaf) {
        fir = dynamic_cast<NonLeaf<T>*>(fir)->child[0];
      }
      while (fir != NULL && fir->key[index] <= key2) {
        vals.push_back(dynamic_cast<Leaf<T>*>(fir)->vals[index]);
        index++;
        if (index == fir->key.size()) {
          index = 0;
          fir = dynamic_cast<Leaf<T>*>(fir)->NextLeaf;
        }
      }
      break;
    }
    case 2: {
      TreeNode<T>* fir;
      fir = root->Find(key1, index);
      if (fir == NULL && index == -1) {
        cout << "Bplus tree empty! Search failed." << endl;
      } else if (fir == NULL && index == 0) {
        fir = root;
        for (int i = 0; i < root->key.size(); i++) {
          vals.push_back(dynamic_cast<Leaf<T>*>(root)->vals[i]);
        }
      } else {
        if (index < 0) {
          index += fir->key.size();
          index += 1;
          if (index == fir->key.size()) {
            fir = dynamic_cast<Leaf<T>*>(fir)->NextLeaf;
            index = 0;
          }
        }
        while (fir != NULL) {
          vals.push_back(dynamic_cast<Leaf<T>*>(fir)->vals[index]);
          index++;
          if (index == fir->key.size()) {
            index = 0;
            fir = dynamic_cast<Leaf<T>*>(fir)->NextLeaf;
          }
        }
      }
      break;
    }
  }
  std::sort(vals.begin(), vals.end());
  vals.erase(unique(vals.begin(), vals.end()), vals.end());
}

template <typename T>
bool BPlusTree<T>::insertKey(T key, int val) {
  bool insert_result;
  insert_result = root->Insert(key, val);
  if (insert_result) {
    if (root->isFull()) {
      TreeNode<T>* new_root = new NonLeaf<T>(this->Degree);  //这个初始化有问题
      dynamic_cast<NonLeaf<T>*>(new_root)->child.push_back(root);
      if (root->isLeaf) {
        dynamic_cast<NonLeaf<T>*>(new_root)->SplitLeaf(0);
      } else {
        dynamic_cast<NonLeaf<T>*>(new_root)->SplitNonLeaf(0);
      }
      root = new_root;
    }
    ++size;
  }
  return insert_result;
}

template <typename T>
bool BPlusTree<T>::deleteKey(T key) {
  bool delete_res = root->Delete(key);
  if (delete_res) {
    --size;
  }
  if (delete_res && root->key.size() == 0 && !root->isLeaf) {
    TreeNode<T>* tempnode = root;
    root = dynamic_cast<NonLeaf<T>*>(root)->child[0];
    delete tempnode;
  }
  if (!size) {
    root = new Leaf<T>(Degree);
  }
  return delete_res;
}

template <typename T>
void BPlusTree<T>::dropTree(Tree node) {
  //空树
  if (!node) return;
  //非叶节点
  if (!node->isLeaf) {
    for (unsigned int i = 0; i < dynamic_cast<NonLeaf<T>*>(node)->child.size(); i++) {
      dropTree(dynamic_cast<NonLeaf<T>*>(node)->child[i]);
      dynamic_cast<NonLeaf<T>*>(node)->child[i] = NULL;
    }
  }
  delete node;
  return;
}
//获取文件大小
template <typename T>
void BPlusTree<T>::getFile(std::string fname) {
  FILE* f = fopen(fname.c_str(), "r");
  if (f == NULL) {
    f = fopen(fname.c_str(), "w+");
    fclose(f);
    f = fopen(fname.c_str(), "r");
  }
  fclose(f);
}

template <typename T>
int BPlusTree<T>::getBlockNum(std::string table_name) {
  char* p;
  int block_num = -1;
  do {
    p = buffer_manager.getPage(table_name, block_num + 1);
    block_num++;
  } while (p[0] != '\0');
  return block_num;
}

template <typename T>
void BPlusTree<T>::readFromDiskAll() {
  std::string fname = "./database/index/" + file_name;
  // std::string fname = file_name;
  getFile(fname);
  int block_num = getBlockNum(fname);

  if (block_num <= 0) block_num = 1;

  for (int i = 0; i < block_num; i++) {
    //获取当前块的句柄
    char* p = buffer_manager.getPage(fname, i);
    // char* t = p;
    //遍历块中所有记录
    readFromDisk(p, p + PAGESIZE);
  }
}

template <typename T>
void BPlusTree<T>::readFromDisk(char* p, char* end) {
  T key;
  int value;

  for (int i = 0; i < PAGESIZE; i++)
    if (p[i] != '#')
      return;
    else {
      i += 2;
      char tmp[100];
      int j;

      for (j = 0; i < PAGESIZE && p[i] != ' '; i++) tmp[j++] = p[i];
      tmp[j] = '\0';
      std::string s(tmp);
      std::stringstream stream(s);
      stream >> key;

      memset(tmp, 0, sizeof(tmp));

      i++;
      for (j = 0; i < PAGESIZE && p[i] != ' '; i++) tmp[j++] = p[i];
      tmp[j] = '\0';
      std::string s1(tmp);
      std::stringstream stream1(s1);
      stream1 >> value;

      insertKey(key, value);
    }
}

template <typename T>
void BPlusTree<T>::writtenbackToDiskAll() {
  std::string fname = "./database/index/" + file_name;
  // std::string fname = file_name;
  getFile(fname);
  int block_num = getBlockNum(fname);

  Tree ntmp = root;
  while (!ntmp->isLeaf) {
    ntmp = dynamic_cast<NonLeaf<T>*>(ntmp)->child[0];
  }
  int i, j;

  for (j = 0, i = 0; ntmp != NULL; j++) {
    char* p = buffer_manager.getPage(fname, j);
    int offset = 0;

    memset(p, 0, PAGESIZE);

    for (i = 0; i < ntmp->key.size(); i++) {
      p[offset++] = '#';
      p[offset++] = ' ';

      copyString(p, offset, ntmp->key[i]);
      p[offset++] = ' ';
      copyString(p, offset, dynamic_cast<Leaf<T>*>(ntmp)->vals[i]);
      p[offset++] = ' ';
    }

    p[offset] = '\0';

    int page_id = buffer_manager.getPageId(fname, j);
    buffer_manager.modifyPage(page_id);

    ntmp = dynamic_cast<Leaf<T>*>(ntmp)->NextLeaf;
  }

  while (j < block_num) {
    char* p = buffer_manager.getPage(fname, j);
    memset(p, 0, PAGESIZE);

    int page_id = buffer_manager.getPageId(fname, j);
    buffer_manager.modifyPage(page_id);

    j++;
  }

  return;
}

template <typename T>
void BPlusTree<T>::printleaf() {
  Tree p = root;
  while (!p->isLeaf) {
    p = dynamic_cast<NonLeaf<T>*>(p)->child[0];
  }
  while (p != NULL) {
    for (int i = 0; i < p->key.size(); i++) cout << " " << p->key[i];
    cout << endl;
    p = dynamic_cast<Leaf<T>*>(p)->NextLeaf;
  }

  return;
}

template <typename T>
void BPlusTree<T>::printTree() {
  queue<TreeNode<T>*> Q;
  root->switchline = true;
  Q.push(root);
  while (!Q.empty()) {
    TreeNode<T>* tempnode = Q.front();
    Q.pop();
    if (tempnode != NULL && !tempnode->isLeaf) {
      for (int i = 0; i < dynamic_cast<NonLeaf<T>*>(tempnode)->child.size(); i++) {
        if (i != dynamic_cast<NonLeaf<T>*>(tempnode)->child.size() - 1 || !tempnode->switchline)
          dynamic_cast<NonLeaf<T>*>(tempnode)->child[i]->switchline = false;
        else
          dynamic_cast<NonLeaf<T>*>(tempnode)->child[i]->switchline = true;
        Q.push(dynamic_cast<NonLeaf<T>*>(tempnode)->child[i]);
      }
    }
    cout << '[';
    for (int i = 0; i < tempnode->key.size(); i++) {
      if (i) cout << ',';
      cout << tempnode->key[i];
    }
    cout << ']';
    if (tempnode->switchline == true) cout << endl;
  }
}
#endif
