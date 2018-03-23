#ifndef LEASTSQUAREFIT_H
#define LEASTSQUAREFIT_H

#include <math.h>
#include <QPoint>
#include <QVector>

class LeastSquareFit {
 public:
  typedef QVector<double> DoubleV;
  LeastSquareFit(DoubleV x, DoubleV y) : x(x), y(y) {
    line = calc(x, y);
    corr = correlation(x, y);
  }
  QPointF getAY() { return line; }  // slope, y_inter -> A*slope +y_in
  double getSlope() { return line.rx(); }
  double getCorrelation() { return corr; }
  QPointF first() {
    return (!x.isEmpty())
               ? QPointF(x.first(), line.rx() * x.first() + line.ry())
               : QPointF();
  }
  QPointF last() {
    return (!x.isEmpty()) ? QPointF(x.last(), line.rx() * x.last() + line.ry())
                          : QPointF();
  }
  QList<QPointF> getLine() {
    if (!x.isEmpty())
      return {first(), last()};
    else
      return {};
  }
  double eval(double x) { return line.rx() * x + line.ry(); }

 private:
  QPointF line;
  double corr = 0;
  DoubleV x, y;

  double correlation(DoubleV x, DoubleV y) {
    double sx = 0, sy = 0, sxy = 0, sxx = 0, syy = 0, mx, my, sdx, sdy, cxy, r,
           vx, vy;

    int n = x.length();

    for (int i = 0; i < n; ++i) {
      sx += x[i];
      sxx += (x[i] * x[i]);
      sy += y[i];
      syy += (y[i] * y[i]);
      sxy += (x[i] * y[i]);
    }

    mx = sx / n;
    my = sy / n;
    vx = (sxx / n) - (mx * mx);
    vy = (syy / n) - (my * my);
    sdx = sqrt(vx);
    sdy = sqrt(vy);
    cxy = (sxy / n) - (mx * my);
    r = cxy / (sdx * sdy);

    return r;
  }

  QPointF calc(DoubleV x, DoubleV y) {
    int n = x.length();
    double sumx = 0, sumy = 0, sumxy = 0, sumxx = 0, slope, y_intercept;
    for (int i = 0; i < n; i++) {
      sumx += x[i];
      sumy += y[i];
      sumxy += x[i] * y[i];
      sumxx += x[i] * x[i];
    }
    slope = (sumx * sumy - n * sumxy) / (sumx * sumx - n * sumxx);
    y_intercept = (sumy - slope * sumx) / n;
    return QPointF(slope, y_intercept);  // y=slope * X + y_intercept, y=ax+b
  }

  // paint->drawLine(0, sh - line.rx(), sw, sh - (line.rx() * sw + line.ry()));
  double eval(QPointF &line, double x) { return line.rx() * x + line.ry(); }
  QPointF evalFirst(QPointF &line, DoubleV x) {
    return QPointF(x.first(), line.rx() * x.first() + line.ry());
  }
  QPointF evalLast(QPointF &line, DoubleV x) {
    return QPointF(x.last(), line.rx() * x.last() + line.ry());
  }
};

#endif  // LEASTSQUAREFIT_H
