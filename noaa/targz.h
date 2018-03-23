#ifndef TARGZ_H
#define TARGZ_H

#include <archive.h>
#include <archive_entry.h>

#include <QByteArray>
#include <QString>

class TarGz {
 public:
  typedef enum {
    stOK,
    stEOF,
    stSKIP,
    stERROR
  } StatusHeader;  // status from nextheader

  struct archive *arch = nullptr;
  struct archive_entry *entry = nullptr;

  int err = 0;
  static const int buffSize = 1024 * 16;
  char buff[buffSize + 1];  // zero terminated
  QString fnme;

  TarGz();
  TarGz(QString fnme);
  ~TarGz();

  int open(QString fnme);
  int reopen();
  void close();
  StatusHeader nextHeader();
  QByteArray read();

  // getters
  int getSize();
  QString getFileName();

 private:
  void init();
};

#endif  // TARGZ_H
