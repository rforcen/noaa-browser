/*
 * works w/: ghcnd_all.tar.gz, 3.11GB @Jan 2018
 */
#include "reader.h"

#include <archive.h>
#include <archive_entry.h>

Reader* reader;  // global reader

Reader::Reader() : TarGz() { init(); }

Reader::Reader(QString fnme) : TarGz() {
  this->fnme = fnme;
  init();
  TarGz::open(fnme);
}

Reader::~Reader() {
  abort();
  fnmeList.clear();
  dataList.clear();

  delete[] mtdata;
  TarGz::close();
}

void Reader::abort() {
  _abort = true;
  {
    if (thrdRead.joinable()) thrdRead.join();
    if (thrdFind.joinable()) thrdFind.join();
    if (thrdIndex.joinable()) thrdIndex.join();
  }
  _abort = false;
}

void Reader::prepareMT() {
  abort();  // prepare & go!
  time.start();
  nFound = recNo = 0;
  nRows = 0;
}

void Reader::prepareMTnoInit() {
  abort();  // prepare & go!
  time.start();
  nFound = recNo = 0;
}

void Reader::init() {
  re_stationFileName.setPattern(
      "[A-Z]{2}.{9}");  // two uppercase letters->country, 9 chars
  re_stationFileName.optimize();
  nFound = recNo = 0;
  nRows = 0;
}

void Reader::copyDataFileName(
    int n) {  // copy thread generated partial data to globals
  for (int i = 0; i < n; i++) {  // wait to complete -> append
    if (mtdata[i].th.joinable()) mtdata[i].th.join();

    fnmeList.append(mtdata[i].fnme);
    dataList.append(mtdata[i].data);
  }
}

int Reader::countTarLines() {
  int nl = 0;
  while (TarGz::nextHeader() != stEOF && !_abort) {
    auto ba = TarGz::read();
    nl += countLines(ba);
  }
  return nl;
}

void Reader::index() {
  prepareMT();

  thrdIndex = std::thread(&Reader::_index, this);
}

void Reader::_index() {
  int nth = 0, nfProcessed = 0;

  fnmeList.clear();
  dataList.clear();

  dailySt.beginIndex();  // load dicts from resources

  while (TarGz::nextHeader() != stEOF && !_abort) {
    auto fn =
        QFileInfo(TarGz::getFileName()).baseName();  // filename w/out extension
    if (re_stationFileName.match(fn).hasMatch()) {   // valid filename?
      mtdata[nth].fnme = fn;                         // copy to nth buffer
      mtdata[nth].data = TarGz::read();

      nth++;

      if (nth == nThreads) {                // read nThreads files?
        for (int i = 0; i < nThreads; i++)  // index files
          mtdata[i].th = std::thread([this, i]() {

            __index(i);

          });
        for (int i = 0; i < nThreads; i++)  // wait to complete
          if (mtdata[i].th.joinable()) mtdata[i].th.join();
        nth = 0;
      }
      if (!(++nfProcessed % 300))
        emit progress(QString("[reading]  # files: %1, rows: %2")
                          .arg(format(nfProcessed), 5)
                          .arg(format(dailySt.keyStat.lenght())));
    }
  }
  // complete pending items
  for (int i = 0; i < nth; i++) __index(i);

  emit progress(QString("[sorting]  # files: %1, rows: %2")
                    .arg(format(nfProcessed), 5)
                    .arg(format(dailySt.keyStat.lenght())));

  recNo = dailySt.keyStat.lenght();

  TarGz::close();

  for (int i = 0; i < nThreads; i++) mtdata[i].data.clear();

  dailySt.endIndex();

  auto msg = QString("[read end] total rows: %1 | lap:%2 | %3 ms/file")
                 .arg(format(recNo + 1), 8)
                 .arg(time.elapsed() / 1000., 7, 'f', 2)
                 .arg(1.0 * time.elapsed() / nfProcessed);
  emit progress(msg);

  emit finish(msg);
}

void Reader::__index(int i) {  // process 1 file in thread
  int idStation, idCountry;
  dailySt.getStationCountry(mtdata[i].fnme.toLocal8Bit(), idStation,
                            idCountry);  // for all file

  for (auto row : mtdata[i].data.split('\n'))
    if (!row.isEmpty()) {
      mutex.lock();

      dailySt.addRow(row, idStation, idCountry);

      mutex.unlock();
    }
}

void Reader::readAll() {
  prepareMT();

  thrdRead = std::thread(&Reader::readAll_MT, this);
}

void Reader::readAll_MT() {
  int nth = 0, nfAdded = 0;

  fnmeList.clear();
  dataList.clear();

  for (_abort = false; !_abort;) {
    switch (TarGz::nextHeader()) {
      case stOK: {
        auto fn = QFileInfo(TarGz::getFileName())
                      .baseName();  // filename w/out extension

        if (re_stationFileName.match(fn).hasMatch()) {  // valid filename?

          mtdata[nth].fnme = fn;  // copy to nth buffer
          mtdata[nth].data = TarGz::read();

          nth++;

          if (nth == nThreads) {                // read nThreads files?
            for (int i = 0; i < nThreads; i++)  // compress files MT
              mtdata[i].th = std::thread([this, i, nfAdded]() {

                nRows += countLines(mtdata[i].data);
                mtdata[i].data = qCompress(mtdata[i].data, compressionLevel);

              });

            copyDataFileName(nThreads);
            nth = 0;
          }

          if (!(nfAdded % 500))
            emit progress(
                QString(
                    "[reading] # files: %1 | station: %2 | total rows: %3 | "
                    "lap: %4 | ms/file: %5")
                    .arg(format(nfAdded), 7)
                    .arg(fn, 11)
                    .arg(format(nRows), 15)
                    .arg(time.elapsed() / 1000., 5, 'f', 0)
                    .arg(1.0 * time.elapsed() / nfAdded, 5, 'f', 2));

          nfAdded++;  // only count files added
        }
      } break;
      case stERROR:
      case stEOF:
        _abort = true;
        break;  // done
      case stSKIP:
        continue;
    }
  }

  // copy & compress 'nth' pending files
  for (int i = 0; i < nth; i++) {
    nRows += countLines(mtdata[i].data);
    fnmeList.append(mtdata[i].fnme);
    dataList.append(qCompress(mtdata[i].data));
  }

  emit finish(QString("read end, files %1  | rows %2 | lap %3\"")
                  .arg(format(fnmeList.length()))
                  .arg(format(nRows), 15)
                  .arg(time.elapsed() / 1000., 7, 'f', 2));
}

int Reader::countLines(QByteArray& ba) {
  return ba.count('\n') - 1;  // -1 : last line empty
}

// getters
QString Reader::getFileName() { return fnme; }
QStringList Reader::getNameList() { return fnmeList; }
QByteArray Reader::getDataList(int i) { return qUncompress(dataList[i]); }
QStringList Reader::getDataStringList(int i) {
  datasl.clear();

  if (i < dataList.length() && i >= 0) {
    QByteArray dl = qUncompress(dataList[i]);
    for (auto l : dl.split('\n'))
      if (!l.isEmpty()) datasl.append(l);
  }
  return datasl;
}

// find

bool Reader::findTest(QString expr) {  // thread scanData
  if (sql.compile(expr.toLocal8Bit().constData())) {
    prepareMT();
    for (int r = 0; r < dailySt.keyStat.lenght(); r++) {
      if (sql.execute(dailySt.getRow(r), dailySt.getStat(r), r)) nFound++;
      nRows++;
    }
  }
  return sql.ok;
}

// month=3 and year=1965 and country='RQ' and element='ACMH'
bool Reader::find(QString expr, int limit) {  // thread scanData
  if (sql.compile(expr.toLocal8Bit().constData())) {
    prepareMT();
    dailySt.keyStat.initFilter();
    dailySt.stGlobal.init();

    this->limit = limit;

    thrdFind = std::thread(&Reader::_find, this);
  }
  return sql.ok;
}

void Reader::_find() {
  for (int th = 0; th < nThreads; th++) {
    mtdata[th].sql = new SQLcompiler(sql);  // replicate to allow MT
    mtdata[th].th = std::thread(&Reader::__find_in_keyStat, this, th);
  }

  for (int th = 0; th < nThreads; th++) mtdata[th].th.join();

  auto msg = QString("[query end] found: %1 of %2 (%4%) | time: %3")
                 .arg(format(nFound))
                 .arg(format(dailySt.keyStat.lenght()))
                 .arg(time.elapsed() / 1000., 7, 'f', 2)
                 .arg(100.0 * nFound / dailySt.keyStat.lenght(), 6, 'f', 3);
  emit progress(msg);

  dailySt.keyStat.applyFilter();
  emit finish(msg);
}

void Reader::__find_in_keyStat(int nth) {
  int dl = dailySt.keyStat.lenght(), bpth = dl / nThreads;
  int from = nth * bpth,
      to = (nth == nThreads - 1) ? dl : (nth + 1) * bpth;  // last blocks to end

  for (int r = from; r < to && !_abort; r++) {
    StatRow st;
    if (mtdata[nth].sql->execute(dailySt.getRow(r), st = dailySt.getStat(r),
                                 r)) {
      mutex.lock();

      dailySt.keyStat.addFilterRecno(r);
      dailySt.stGlobal += st;
      nFound++;

      mutex.unlock();

      if (limit && nFound >= limit) _abort = true;
    }
    nRows++;
    if (nth == 0 && !((r - from) % 100000)) {  // progress only on first thread
      emit progress(QString("[query] read %1% | found %2  | lap:%3")
                        .arg(100.0 * nRows / dl, 3, 'f', 0)
                        .arg(format(nFound), 6)
                        .arg(time.elapsed() / 1000., 7, 'f', 2));
    }
  }
}

void Reader::__find_in_original_data(int nth) {
  Daily ds;

  int dl = dataList.length(), bpth = dl / nThreads;
  int from = nth * bpth,
      to = (nth == nThreads - 1) ? dl : (nth + 1) * bpth;  // last blocks to end

  for (int b = from; b < to && !_abort; b++) {
    for (auto r : qUncompress(dataList[b]).split('\n')) {
      if (!r.isEmpty()) {
        //        if (mtdata[nth].sql.execute(r)) nFound++;

        if (!((b - from) % 500)) {
          emit progress(QString("queried %1 | found %2 of %3 | lap:%4, '%5'")
                            .arg(format(recNo))
                            .arg(format(nFound), 6)
                            .arg(format(nRows))
                            .arg(time.elapsed() / 1000., 7, 'f', 2));
        }
      }
    }
  }
}

int Reader::getnFound() { return nFound; }
