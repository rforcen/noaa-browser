#ifndef TREEGROUP_H
#define TREEGROUP_H

#include <auxfile.h>
#include <reader.h>
#include <treeitem.h>

#include <QAbstractItemModel>
#include <QTreeView>

class TreeModel : public QAbstractItemModel {
  Q_OBJECT

  Tree *tree = nullptr;

 public:
  explicit TreeModel(Tree *tree, QObject *parent = 0)
      : QAbstractItemModel(parent), tree(tree) {}

  ~TreeModel() { deleteTree(tree); }

  void deleteTree(Tree *root) {
    if (root) {
      Tree::delTree(root);
      root = nullptr;
    }
  }

  QVariant data(const QModelIndex &index, int role=Qt::DisplayRole) const {
    if (!index.isValid()) return QVariant();
    switch (role) {
      case Qt::TextAlignmentRole:
        return Qt::AlignRight;
      case Qt::DisplayRole: {
        Tree *item = static_cast<Tree *>(index.internalPointer());
        return item->data(index.column());
      }
      case Qt::FontRole:
        return QFont("Courier New", 10);
    }
    return QVariant();
  }

  Qt::ItemFlags flags(const QModelIndex &index) const {
    if (!index.isValid()) return 0;

    return QAbstractItemModel::flags(index);
  }

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const {
    if (role == Qt::TextAlignmentRole) return Qt::AlignRight;
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
      return tree->data(section);

    return QVariant();
  }

  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const {
    if (!hasIndex(row, column, parent)) return QModelIndex();

    Tree *parentItem;

    if (!parent.isValid())
      parentItem = tree;
    else
      parentItem = static_cast<Tree *>(parent.internalPointer());

    Tree *childItem = parentItem->child(row);
    if (childItem)
      return createIndex(row, column, childItem);
    else
      return QModelIndex();
  }

  QModelIndex parent(const QModelIndex &index) const {
    if (!index.isValid()) return QModelIndex();

    Tree *childItem = static_cast<Tree *>(index.internalPointer());
    Tree *parentItem = childItem->parentItem();

    if (parentItem == tree) return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
  }

  int rowCount(const QModelIndex &parent = QModelIndex()) const {
    Tree *parentItem;
    if (parent.column() > 0) return 0;

    if (!parent.isValid())
      parentItem = tree;
    else
      parentItem = static_cast<Tree *>(parent.internalPointer());

    return parentItem ? parentItem->childCount() : 0;
  }

  int columnCount(const QModelIndex &parent = QModelIndex()) const {
    if (parent.isValid())
      return static_cast<Tree *>(parent.internalPointer())->columnCount();
    else
      return tree ? tree->columnCount() : 0;
  }
};

class TreeGroup : public QTreeView {
 public:
  TreeModel *model = nullptr;

  explicit TreeGroup(QWidget *parent) : QTreeView(parent) {}

  void setData(Tree *tree) {
    if (model) delete model;
    model = new TreeModel(tree, this);
    setModel(model);
  }
};

#endif  // TREEGROUP_H
