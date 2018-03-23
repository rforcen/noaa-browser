#ifndef TABLEAUXFILE_H
#define TABLEAUXFILE_H

#include <auxfile.h>
#include <config.h>
#include <QAbstractTableModel>
#include <QTableView>

class AuxFileTableModel : public QAbstractTableModel {
 public:
  AuxFile::FilesTypes ft;

  AuxFileTableModel(AuxFile::FilesTypes ft, QTableView *parent)
      : QAbstractTableModel(parent), ft(ft) {}

  int rowCount(const QModelIndex &) const { return auxFile.getRows(ft); }
  int columnCount(const QModelIndex &) const { return auxFile.getColumns(ft); }

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const {
    switch (role) {
      case Qt::DisplayRole:
        return auxFile.getData(ft, index);
      case Qt::TextAlignmentRole:
        return auxFile.isNumber(ft, index) ? Qt::AlignRight : Qt::AlignLeft;
    }
    return QVariant();
  }

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const {
    switch (orientation) {
      case Qt::Horizontal:
        if (role == Qt::DisplayRole) return auxFile.getFieldName(ft, section);
        break;
//      case Qt::Vertical:
//        if (role == Qt::DisplayRole) return Config::format(section + 1);
//        break;
    }
    return QVariant();
  }
};

class TableAuxFile : public QTableView {
 private:
  AuxFileTableModel *model = nullptr;

 public:
  TableAuxFile(QWidget *parent = nullptr) : QTableView(parent) {}

  void setFileType(AuxFile::FilesTypes ft) {
    if (model) delete model;
    model = new AuxFileTableModel(ft, this);
    setModel(model);
  }
};

#endif  // TABLEAUXFILE_H
