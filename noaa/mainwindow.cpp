#include "mainwindow.h"
#include "ui_mainwindow.h"

int extract(const char *fnme);
int read_targz(const char *fnme);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  ui->centralWidget->hide();

  loadSettings();

  for (auto cb : findChildren<AuxFileCombo *>())
    cb->setType();  // aux files set
  ui->stations->setFileType(AuxFile::fStations);

  ui->sortFields->addItems(
      dailySt.getFldsNameStat());  // populate sort fields filter
  ui->sortFields->setDragDropMode(QAbstractItemView::DragDrop);

  //    open("/Volumes/backup 1TB/noaa data/ghcnd_all.tar.gz");
  open("/Volumes/backup 1TB/noaa data/test.tar.gz");
}

MainWindow::~MainWindow() {
  saveSettings();
  if (reader) delete reader;
  delete ui;
}

// settings save/load
void MainWindow::saveSettings() {
  settings->setValue("geometry", saveGeometry());
  settings->setValue("windowState", saveState());

  settings->setValue("dailyFile", reader ? reader->getFileName() : "");
  delete settings;
}

void MainWindow::loadSettings() {
  auxFile.open();
  settings = new QSettings("voicesync", "noaa");  //  settings
  restoreState(settings->value("windowState").toByteArray());
  restoreGeometry(settings->value("geometry").toByteArray());

  fnme = settings->value("dailyFile").toString();
}

void MainWindow::open(QString fnme) {
  if (reader) delete reader;
  reader = new Reader(fnme);

  connect(reader, &Reader::progress, this, &MainWindow::onProgres);
  connect(reader, &Reader::finish, this, &MainWindow::onReadFinish);

  reader->index();
}

// progress / completetion slots
void MainWindow::onReadFinish(QString msg) {
  statusBar()->showMessage(msg);
  ui->stGlobal->setText(dailySt.stGlobal.toFormatted(" | "));
}

void MainWindow::onProgres(QString msg) { statusBar()->showMessage(msg); }

void MainWindow::onFindFinish(QString msg) {
  statusBar()->showMessage(msg);
  ui->stGlobal->setText(dailySt.stGlobal.toFormatted(" | "));
  sort();
}

void MainWindow::on_actionopen_triggered() {
  fnme = QFileDialog::getOpenFileName(this, tr("Open NOAA daily tar.gz"), fnme,
                                      tr("tar.gz (*.tar.gz)"));
  if (!fnme.isEmpty()) open(fnme);
}

void MainWindow::on_actionstop_triggered() {
  if (reader) reader->abort();
}

void MainWindow::on_query_returnPressed() {
  auto query = ui->query->text();
  if (!query.isEmpty()) {
    connect(reader, &Reader::progress, this, &MainWindow::onProgres);
    connect(reader, &Reader::finish, this, &MainWindow::onFindFinish);

    if (!reader->find(query, ui->limit->value()))
      onProgres(QString("syntax error"));
  }
}

void MainWindow::on_actionclear_find_triggered() {
  dailySt.keyStat.initFilter();
}

void MainWindow::on_queryStations_returnPressed() {
  auto query = ui->queryStations->text();
  if (!query.isEmpty()) {
    rowFind = auxFile.findAny(AuxFile::fStations, query,
                              ui->stations->currentIndex().row() + 1);
    rowFind = rowFind != -1 ? rowFind : 0;
    ui->stations->selectRow(rowFind);
  }
}

void MainWindow::graph() {
  lineCharter.graph(ui->layOutLine, tree, "");
  barCharter.graph(ui->layOutBar, tree, "");
  lineSeriesCharter.graph(ui->layOutLineSeries, tree,
                          ui->sortFields->selectedItems().count(), "");
}

void MainWindow::sort() {
  if (ui->sortFields->selectedItems().count()) {
    QStringList items;
    for (auto it : ui->sortFields->selectedItems()) items << it->text();
    dailySt.sortFilter(items, ui->sortOrder->currentIndex());
    ui->tree->setData(tree = dailySt.createTree(items));

    graph();
  }
}

void MainWindow::on_tree_clicked(const QModelIndex &index) {
  auto d = ui->tree->model->data(index);
  statusBar()->showMessage(d.toString());
}

void MainWindow::on_limit_editingFinished() { on_query_returnPressed(); }
void MainWindow::on_actionfind_triggered() { on_query_returnPressed(); }
void MainWindow::on_actionsort_triggered() { sort(); }
