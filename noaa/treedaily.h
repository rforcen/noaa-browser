#ifndef TREEDAILY_H
#define TREEDAILY_H

#include <auxfile.h>
#include <daily.h>
#include <reader.h>

#include <QAbstractItemModel>
#include <QTreeView>

class TreeDailyModel : public QAbstractItemModel {
  DailyStruct ds;
  QStringList header = {"country", "station", "element", "year",
                        "month",   "values",  "flags"};

 public:
  explicit TreeDailyModel(QObject *parent = 0) : QAbstractItemModel(parent) {}

  QVariant data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();
    if (role == Qt::TextAlignmentRole) return Qt::AlignRight;
    if (role == Qt::DisplayRole) {
      switch (index.column()) {
        case 0:
          return auxFile.getData(AuxFile::fCountries, 0, index.row());
      }
    }
    return QVariant();
  }

  QModelIndex parent(const QModelIndex &index) const { return QModelIndex(); }

  int rowCount(const QModelIndex &parent = QModelIndex()) const {
    if (parent.column() > 0) return 0;
    return 1;
  }

  int columnCount(const QModelIndex &parent = QModelIndex()) const {
    if (parent.isValid())
      return 1;
    else
      return header.length();
  }

  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const {
    if (!hasIndex(row, column, parent)) return QModelIndex();
    return QModelIndex();
  }

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
      return header[section];  // header of root item

    return QVariant();
  }

  Qt::ItemFlags flags(const QModelIndex &index) const {
    if (!index.isValid()) return 0;

    return QAbstractItemModel::flags(index);
  }

  ~TreeDailyModel() {}
};

class TreeDaily : public QTreeView {
 public:
  TreeDailyModel *model = nullptr;

  explicit TreeDaily(QWidget *parent) : QTreeView(parent) {}

  void setTreeModel() {
    if (model) delete model;
    model = new TreeDailyModel(this);
    setModel(model);
  }
};

#endif  // TREEDAILY_H
