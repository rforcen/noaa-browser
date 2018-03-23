#include "targz.h"

TarGz::TarGz() { init(); }

void TarGz::init() {
  // create archive
  this->arch = archive_read_new();
  archive_read_support_format_all(arch);
  archive_read_support_filter_all(arch);
  memset(buff, 0, sizeof(buff));
}

TarGz::~TarGz() { close(); }

int TarGz::open(QString fnme) {
  // open file
  this->fnme = fnme;
  return err = (archive_read_open_filename(arch, fnme.toLatin1().data(),
                                           buffSize) == ARCHIVE_OK)
                   ? 0
                   : -1;
}

void TarGz::close() {
  // close & free archive
  if (arch != nullptr) {
    archive_read_close(arch);
    archive_read_free(arch);
    arch = nullptr;
  }
}

int TarGz::reopen() {
  int rv = -1;
  close();
  init();
  rv = open(fnme);

  return rv;
}

int TarGz::getSize() { return archive_entry_size(entry); }

TarGz::StatusHeader TarGz::nextHeader() {
  StatusHeader st;

  int r = archive_read_next_header(arch, &entry);

  if (r == ARCHIVE_EOF)
    st = stEOF;
  else {
    if (r < ARCHIVE_OK || r < ARCHIVE_WARN)
      st = stERROR;
    else {
      if (!S_ISREG(archive_entry_mode(entry)))
        st = stSKIP;  // skip this
      else {
        st = stOK;
      }
    }
  }

  return st;
}

QString TarGz::getFileName() { return QString(archive_entry_pathname(entry)); }

QByteArray TarGz::read() {
  QByteArray ba;
  int len;

  if (archive_entry_size(entry) > 0) {
    do {
      memset(buff, 0, sizeof(buff));
      len = archive_read_data(arch, buff, buffSize);
      if (len > 0) ba.append(QByteArray(buff));
    } while (len > 0);

    err = (len < 0) ? -1 : 0;
  }
  return ba;
}
