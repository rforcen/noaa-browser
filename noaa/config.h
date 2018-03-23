#ifndef CONFIG_H
#define CONFIG_H

#include <auxfile.h>
#include <daily.h>
#include <QLocale>
#include <QString>

class Config {
 public:
  Config() {}

  static QString format(int i) { return QLocale(QLocale::English).toString(i); }
  static QString formatHR(int n) {
    const char *suffix = " kmgtph";
    for (int d = 1000, ix = 0, d1 = 1; ix < (int)strlen(suffix);
         d1 = d, d *= 1000, ix++)
      if (n / d == 0) return QString::number(n / d1) + suffix[ix];
    return QString::number(n);
  }
};

extern Config conf;

#endif  // CONFIG_H
