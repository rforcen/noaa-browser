#ifndef TABLEDAILY_H
#define TABLEDAILY_H
#include <config.h>
#include <daily.h>
#include <QAbstractTableModel>
#include <QTreeView>

class DailyKeyStatTableModel : public QAbstractTableModel {
  int fl = dailySt.indexFlds.length();

 public:
  DailyKeyStatTableModel(QTreeView *parent) : QAbstractTableModel(parent) {}

  int rowCount(const QModelIndex &) const { return dailySt.keyStat.lenght(); }
  int columnCount(const QModelIndex &) const { return fl + StatisticFields; }

  Daily::Bytes _getData(const QModelIndex &index) const {
    auto bs = dailySt.keyStat[index.row()].bitset;
    if (index.column() < fl)
      return dailySt.bitSet2Key(index.column(), bs);
    else
      return StatRow::format(
                 dailySt.keyStat[index.row()].stat[index.column() - fl])
          .toLocal8Bit();
  }

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const {
    Daily::Bytes desc;

    switch (role) {
      case Qt::DisplayRole:
        return _getData(index);

      case Qt::TextAlignmentRole:
        return Qt::AlignCenter;
      case Qt::FontRole:
        return QFont("Courier New", 10);
      case Qt::ToolTipRole: {
        if (index.column() < fl) {
          auto ft = dailySt.getAuxFileIndex(dailySt.indexFlds[index.column()]);

          if (ft != (AuxFile::FilesTypes)-1)
            return desc = auxFile.getAllFields(ft, _getData(index));
          else {
            if (dailySt.indexFlds[index.column()] == dailySt.fnYear)
              return QString("%1 years ago")
                  .arg(QDateTime::currentDateTime().date().year() -
                       _getData(index).toInt());
          }
        }
      } break;
    }
    return QVariant();
  }

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const {
    switch (orientation) {
      case Qt::Horizontal:
        if (role == Qt::DisplayRole)
          return (section < fl) ? dailySt.header[dailySt.indexFlds[section]]
                                : StatRow::getHeader(section - fl);
        break;
      case Qt::Vertical:
        if (role == Qt::DisplayRole) return Config::format(section + 1);
        break;
    }
    return QVariant();
  }
};

class DailyTableModel : public QAbstractTableModel {
 public:
  Daily ds;
  QStringList sl;

  DailyTableModel(QStringList sl, QTreeView *parent)
      : QAbstractTableModel(parent), sl(sl) {}

  int rowCount(const QModelIndex &) const { return sl.length(); }
  int columnCount(const QModelIndex &) const { return ds.getnColumns(); }

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const {
    auto line = sl[index.row()].toLocal8Bit();

    switch (role) {
      case Qt::DisplayRole:
        return QVariant(ds.line2field(line, index.column()));

      case Qt::TextAlignmentRole:
        return Qt::AlignCenter;
      case Qt::FontRole:
        return QFont("Courier New", 12);
    }
    return QVariant();
  }

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const {
    switch (orientation) {
      case Qt::Horizontal:
        if (role == Qt::DisplayRole) return ds.header[section];
        break;
      case Qt::Vertical:
        if (role == Qt::DisplayRole) return QString::number(section + 1);
        break;
    }
    return QVariant();
  }
};

class TableDaily : public QTreeView {
 private:
  DailyKeyStatTableModel *model = nullptr;

 public:
  TableDaily(QWidget *parent = nullptr) : QTreeView(parent) {
    setUniformRowHeights(true);
  }
  void setData() {
    if (model) delete model;
    model = new DailyKeyStatTableModel(this);

    setModel(model);
  }
};

#endif  // TABLEDAILY_H
