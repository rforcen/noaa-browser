#ifndef COMBOBOXMODEL_H
#define COMBOBOXMODEL_H

#include <auxfile.h>
#include <QAbstractListModel>
#include <QComboBox>

class ComboBoxModel : public QAbstractListModel {
 public:
  AuxFile::FilesTypes ft;

  ComboBoxModel(QWidget *parent, AuxFile::FilesTypes ft)
      : QAbstractListModel(parent), ft(ft) {}

  int rowCount(const QModelIndex &) const override {
    return auxFile.getRows(ft);
  }
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override {
    switch (role) {
      case Qt::DisplayRole:
        return QVariant(auxFile.getIdName(ft, index.row()));
    }
    return QVariant();
  }
};

class AuxFileCombo : public QComboBox {
 private:
  ComboBoxModel *model = nullptr;

 public:
  AuxFileCombo(QWidget *parent = nullptr) : QComboBox(parent) {
    setStyleSheet("combobox-popup: 0;");
  }

  void setType(AuxFile::FilesTypes ft) {
    if (ft != (AuxFile::FilesTypes)-1) {
      if (model) delete model;
      model = new ComboBoxModel(this, ft);
      setModel(model);
    }
  }

  void setType() { setType(auxFile.getType(accessibleName())); }
};

#endif  // COMBOBOXMODEL_H
