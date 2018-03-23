#ifndef TREEITEM_H
#define TREEITEM_H

#include <statistic.h>
#include <QtCore>

class Tree {
 public:
  explicit Tree(const QList<QVariant> &_data, Tree *parentItem = 0)
      : _data(_data), _parentItem(parentItem) {}
  ~Tree() {}

  Tree *appendChild(Tree *child) {
    childItems.append(child);
    return this;
  }

  Tree *child(int row) { return childItems.value(row); }
  QList<Tree *> children() { return childItems; }
  int childCount() const { return childItems.count(); }
  int columnCount() const { return _data.count(); }
  QVariant data(int column) const { return _data.value(column); }
  QVariant data() const { return _data; }
  int row() const {
    if (_parentItem)
      return _parentItem->childItems.indexOf(const_cast<Tree *>(this));

    return 0;
  }
  Tree *parentItem() { return _parentItem; }
  Tree *setData(const QList<QVariant> &_data) {
    this->_data = _data;
    return this;
  }
  Tree *appendData(QVariant v) {
    _data << v;
    return this;
  }
  Tree *appendData(QList<QVariant> lv) {
    _data << lv;
    return this;
  }
  Tree *operator+=(int n) {
    this->_data += QVariant(n);
    return this;
  }
  Tree *operator[](int row) { return childItems[row]; }

  void clear() {
    for (auto child : childItems) child->clear();
    qDeleteAll(childItems);
  }

  Tree *findChild(QVariant key) {  //  find key in first data item of all child
    for (auto c : childItems)
      if (c->_data[0] == key) return c;
    return nullptr;
  }

  Statistic getStat() { return stat; }
  void operator++() { stat.n++; }
  void addStat(StatRow &statRow) { stat += statRow; }

  static void delTree(Tree *node) {
    if (node == nullptr) return;
    for (auto child : node->childItems) delTree(child);
    qDeleteAll(node->childItems);
  }

  void genStats() { genStats(this); }

  void genStats(Tree *node) {
    if (node == nullptr) return;
    for (auto child : node->childItems) {
      for (auto i : child->stat.toString().split(' ', QString::SkipEmptyParts))
        child->_data << i;
      genStats(child);
    }
  }

 private:
  QList<Tree *> childItems;
  QList<QVariant> _data;
  Tree *_parentItem;
  Statistic stat;
};

#endif  // TREEITEM_H
