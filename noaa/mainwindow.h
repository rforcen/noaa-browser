#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <charter.h>
#include <reader.h>

#include <QFileDialog>
#include <QMainWindow>
#include <QSettings>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

 private slots:
  void onReadFinish(QString msg);
  void onProgres(QString msg);
  void onFindFinish(QString msg);

  void on_actionopen_triggered();
  void on_actionstop_triggered();
  void on_query_returnPressed();
  void on_actionclear_find_triggered();
  void on_queryStations_returnPressed();
  void on_tree_clicked(const QModelIndex &index);
  void on_limit_editingFinished();
  void on_actionfind_triggered();
  void on_actionsort_triggered();

 private:
  void saveSettings(), loadSettings();
  void open(QString fnme);
  void graph(), initGraph();
  void sort();

 private:
  Ui::MainWindow *ui;

  QSettings *settings = nullptr;

  QString fnme;
  int rowFind = 0;

  Tree *tree;
  BarCharter barCharter;
  LineCharter lineCharter;
  LineSeriesCharter lineSeriesCharter;
};

#endif  // MAINWINDOW_H
