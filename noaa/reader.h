#ifndef READER_H
#define READER_H

#include <QByteArray>
#include <QFileInfo>
#include <QLocale>
#include <QMap>
#include <QMutex>
#include <QPair>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QTime>
#include <QVector>
#include <future>
#include <thread>

#include <auxfile.h>
#include <config.h>
#include <daily.h>
#include <sqlCompiler.h>
#include <statistic.h>
#include <targz.h>

class Reader : public QObject, public TarGz {
  Q_OBJECT

 public:
  Reader();
  Reader(QString fnme);
  ~Reader();
  void init();
  void readAll_MT();
  void readAll();
  void abort();
  int countTarLines();

  // find
  void _find(), __find_in_original_data(int nth), __find_in_keyStat(int nth);
  bool find(QString expr, int limit), findTest(QString expr);
  int limit = 0;

  void prepareMT(), prepareMTnoInit();
  int getnFound();
  SQLcompiler sql;

  // getters
  QStringList getNameList();
  QString getFileName();
  QByteArray getDataList(int i);
  QStringList getDataStringList(int i);

 signals:
  void progress(QString msg);
  void finish(QString msg);

 private:
  QString fnme;
  QStringList fnmeList, datasl;
  QVector<QByteArray> dataList;  // compressed file content to memory
  const int compressionLevel = 3;

  bool _abort = false;
  QRegularExpression re_stationFileName;

  QTime time;
  QString format(int i) { return QLocale(QLocale::English).toString(i); }
  QString formatHR(int n) {
    const char *suffix = " kmgtph";
    for (int d = 1000, ix = 0, d1 = 1; ix < (int)strlen(suffix);
         d1 = d, d *= 1000, ix++)
      if (n / d == 0) return QString::number(n / d1) + suffix[ix];
    return QString::number(n);
  }

  // MT read support

  class MTData {
   public:
    std::thread th;
    QString fnme;
    QByteArray data;
    QStringList rows;
    int rowOffset;
    SQLcompiler *sql = nullptr;
  };
  const int nThreads = std::thread::hardware_concurrency();
  MTData *mtdata = new MTData[nThreads];

  std::thread thrdRead, thrdFind, thrdIndex;  // threads
  QMutex mutex;

  void copyDataFileName(int n);
  int countLines(QByteArray &ba);

  // found
  int nFound = 0, nRows = 0, recNo = 0;

 public:
  void index(), _index(), __index(int i);
};

extern Reader *reader;

#endif  // TARGZ_H
