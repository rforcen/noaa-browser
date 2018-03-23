#ifndef STATISTIC_H
#define STATISTIC_H

#include <QLocale>
#include <QVector>
#include <QtCore>
#include <algorithm>
#include <climits>

static const int StatisticFields = 5;  // n, min, max, sum, avg

class StatRow {
 public:
  static QStringList header;
  StatRow() { init(); }
  StatRow& operator+=(StatRow& s) {
    n += s.n;
    sum += s.sum;
    max = std::max<short>(max, s.max);
    min = std::min<short>(min, s.min);
    return *this;
  }
  StatRow& operator-=(StatRow& s) {  // only min, max
    max = std::max<short>(max, s.max);
    min = std::min<short>(min, s.min);
    return *this;
  }
  void init() {
    n = 0;
    sum = 0;
    max = SHRT_MIN, min = SHRT_MAX;
  }

  int operator[](int ix) {
    switch (ix) {
      case 0:
        return (int)n;
      case 1:
        return (int)n ? min : 0;
      case 2:
        return (int)n ? max : 0;
      case 3:
        return sum;
      case 4:
        return (int)mean();
    }
    return 0;
  }
  void zero() { n = sum = max = min = 0; }

  double mean() { return n ? 1.0 * sum / n : 0.; }

  static QString format(int i) { return QLocale(QLocale::English).toString(i); }
  static QString getHeader(int col) { return header[col]; }
  static int fieldIndex(QString fld) { return header.indexOf(fld.toLower()); }

 public:
  uchar n = 0;
  short max = SHRT_MIN, min = SHRT_MAX;
  int sum = 0;
};

class Statistic {
 public:
  long n = 0;
  long sum = 0;
  int max = INT_MIN, min = INT_MAX;
  double stddev = 0;

 public:
  Statistic() {}

  Statistic& operator+=(Statistic& s) {
    n += s.n;
    sum += s.sum;
    max = std::max<int>(max, s.max);
    min = std::min<int>(min, s.min);
    return *this;
  }
  Statistic& operator-=(Statistic& s) {  // only min, max
    max = std::max<int>(max, s.max);
    min = std::min<int>(min, s.min);
    return *this;
  }

  Statistic& operator+=(StatRow& s) {
    n += s.n;
    sum += s.sum;
    max = std::max<int>(max, s.max);
    min = std::min<int>(min, s.min);
    return *this;
  }
  Statistic& operator-=(StatRow& s) {  // only min, max
    max = std::max<int>(max, s.max);
    min = std::min<int>(min, s.min);
    return *this;
  }
  void init() {
    n = 0;
    sum = 0;
    max = INT_MIN, min = INT_MAX;
  }

  void zero() { n = sum = max = min = 0; }

  double mean() { return n ? 1.0 * sum / n : 0.; }

  QString toString() {
    return n ? QString("%1 %2 %3 %4")
                   .arg(format(n))
                   .arg(min)
                   .arg(max)
                   .arg(mean(), 5, 'f', 1)
             : "";
  }
  const char* toChar() { return toString().toLocal8Bit().constData(); }
  static QString format(int i) { return QLocale(QLocale::English).toString(i); }
  static QString format(long i) {
    return QLocale(QLocale::English).toString((qlonglong)i);
  }
  static QString format(double d) {
    return QLocale(QLocale::English).toString(d);
  }
  QString toFormatted(QString sep = "\n") {
    return n ? QString("count: %1\nmin:%2\nmax:%3\nsum:%4\nmean:%5")
                   .replace('\n', sep)
                   .arg(format(n))
                   .arg(min)
                   .arg(max)
                   .arg(format((double)sum))
                   .arg(mean(), 5, 'f', 1)
             : "";
  }
};

#endif  // STATISTIC_H
