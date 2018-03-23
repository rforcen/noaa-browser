#ifndef TABLEGROUP_H
#define TABLEGROUP_H

#include <daily.h>
#include <reader.h>
#include <QAbstractTableModel>
#include <QTableView>

// works w/Element/Year index

class GroupTableModel : public QAbstractTableModel {
 public:
  IndexStat ix;
  typedef QMap<QByteArray, int> IndexCount;  // unique row/col values
  IndexCount ix1, ix2;
  QVector<IndexCount::iterator> iter1, iter2;
  QStringList hdr1, hdr2;

  GroupTableModel(IndexStat ix, QTableView *parent)
      : QAbstractTableModel(parent), ix(ix) {
    for (auto i = ix.begin(); i != ix.end(); i++) {  // count unique items
      ix1[i.key().left(4)]++;
      ix2[i.key().right(4)]++;
    }
    for (auto i = ix1.begin(); i != ix1.end();
         i++) {  // create iterators & headers
      iter1 << i;
      hdr1 << i.key();
    }
    for (auto i = ix2.begin(); i != ix2.end(); i++) {
      iter2 << i;
      hdr2 << i.key();
    }
  }

  int rowCount(const QModelIndex &) const { return ix2.size(); }     // elements
  int columnCount(const QModelIndex &) const { return ix1.size(); }  // years

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const {
    auto i = iter1[index.column()].key() +
             iter2[index.row()].key();  // create group key
    auto ps = ix[i];                    // find pair value

    switch (role) {
      case Qt::DisplayRole:
        return QVariant(ps.toString());

      case Qt::TextAlignmentRole:
        return Qt::AlignCenter;
      case Qt::FontRole:
        return QFont("Courier New", 10);
      case Qt::ToolTipRole:
        return ps.toFormatted();
    }
    return QVariant();
  }

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const {
    switch (orientation) {
      case Qt::Horizontal:
        switch (role) {
          case Qt::DisplayRole:
            return QString("%1(%2)")
                .arg(hdr1[section])
                .arg(iter1[section].value());
          case Qt::ToolTipRole:
            return auxFile.find(AuxFile::fElements, "id", hdr1[section],
                                "name");
        }
        break;
      case Qt::Vertical:
        switch (role) {
          case Qt::DisplayRole:
            return QString("%1(%2)")
                .arg(hdr2[section])
                .arg(iter2[section].value());
        }
        break;
    }
    return QVariant();
  }
};

class TableGroup : public QTableView {
 private:
  GroupTableModel *model = nullptr;

 public:
  TableGroup(QWidget *parent = nullptr) : QTableView(parent) {}
  void setData(IndexStat ix) {
    if (model) delete model;
    model = new GroupTableModel(ix, this);
    setModel(model);
  }
};

#endif  // TABLEGROUP_H
