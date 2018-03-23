#ifndef CHARTER_H
#define CHARTER_H

#include <float.h>
#include <leastsquarefit.h>
#include <treeitem.h>

#include <Q3DSurface>
#include <QBarSet>
#include <QStackedBarSeries>
#include <QtCharts>
#include <QtCore>
#include <QtDataVisualization>

using namespace QtDataVisualization;
using namespace QtCharts;

class Charter {
 public:
  Charter() {}

  void init(QLayout *layout) {
    this->layout = layout;

    if (chartView) {
      layout->removeWidget(chartView);
      delete chartView;  //  & chart
      chartView = 0;
    }
    chart = new QChart();

    _isInit = true;
  }

  void graph() {
    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    layout->addWidget(chartView);
  }

  void remove() {
    if (layout && chartView) {
      layout->removeWidget(chartView);

      delete chartView;
      chartView = 0;
    }
    _isInit = false;
  }

  bool isInit() { return _isInit; }

  QChartView *chartView = new QChartView();
  QChart *chart = 0;
  QLayout *layout = 0;

 private:
  bool _isInit = false;
};

class BarCharter : public Charter {
 public:
  BarCharter() : Charter() {}

  void clear() {
    if (isInit()) {
      delete minSet;
      delete maxSet;
      delete barSeries;
      delete barAxis;
    }
  }

  void remove() {
    clear();
    Charter::remove();
  }

  void init(QLayout *layout) {
    clear();

    minSet = new QBarSet("min");  // min/max
    maxSet = new QBarSet("max");

    barSeries = new QStackedBarSeries();
    barAxis = new QBarCategoryAxis();
    Charter::init(layout);
  }

  void graph(QLayout *layout, Tree *tree, QString title) {
    if (tree) {
      init(layout);

      Statistic stat;
      double min = DBL_MAX, max = DBL_MIN, x = 0, xInc = 0;

      for (auto n : tree->children()) {  // scan first tree level
        auto st = n->getStat();
        auto m = st.mean();
        bool ok;

        x = n->data(0).toDouble(&ok);
        x = ok ? x : xInc++;

        min = std::min<double>(min, m);
        max = std::max<double>(max, m);

        stat += st;

        *minSet << st.min;
        *maxSet << st.max;

        barAxis->append(n->data(0).toString());
      }

      if (minSet->count()) {
        barSeries->append(minSet);
        barSeries->append(maxSet);

        chart->addSeries(barSeries);
        chart->setAnimationOptions(QChart::SeriesAnimations);

        barAxis->setTitleText(title);
        barAxis->setLabelsFont(QFont("Courier New", 9));

        chart->createDefaultAxes();
        chart->setAxisX(barAxis, barSeries);
        chart->axisY(barSeries)->setRange(stat.min, stat.max);

        Charter::graph();
      }
    }
  }

  QBarSet *minSet = 0, *maxSet = 0;
  QStackedBarSeries *barSeries = 0;
  QBarCategoryAxis *barAxis = 0;
};

class LineCharter : public Charter {
 public:
  LineCharter() : Charter() {}

  QLineSeries *lineSeries, *interSeries = 0;
  QValueAxis *lineAxis = 0;
  //  QCategoryAxis *axisX = new QCategoryAxis;

  void init(QLayout *layout) {
    clear();

    lineSeries = new QLineSeries();
    interSeries = new QLineSeries();
    lineAxis = new QValueAxis();

    Charter::init(layout);
  }

  void clear() {
    if (isInit()) {
      delete lineSeries;
      delete interSeries;
      delete lineAxis;
    }
  }

  void remove() {
    clear();
    Charter::remove();
  }

  void graph(QLayout *layout, Tree *tree, QString title) {
    if (tree) {
      init(layout);

      double x = 0, xInc = 0;
      LeastSquareFit::DoubleV meanv, xv;

      for (auto n : tree->children()) {  // scan first tree level
        auto st = n->getStat();
        auto m = st.mean();
        bool ok;

        x = n->data(0).toDouble(&ok);
        x = ok ? x : xInc++;

        meanv << m;
        xv << x;

        lineSeries->append(x, st.mean());
      }

      if (!xv.isEmpty()) {
        LeastSquareFit lsf(xv, meanv);
        interSeries->append(lsf.getLine());
        interSeries->setName(QString("interpolation, corr=%1, slope=%2")
                                 .arg(lsf.getCorrelation())
                                 .arg(lsf.getSlope()));

        lineAxis->setTickCount(tree->childCount());
        lineAxis->setRange(tree->children().first()->data(0).toDouble(),
                           tree->children().last()->data(0).toDouble());
        lineAxis->setLabelFormat("%.0f");
        lineAxis->setTitleText(title);

        chart->addSeries(lineSeries);
        chart->addSeries(interSeries);

        chart->setAnimationOptions(QChart::SeriesAnimations);

        chart->createDefaultAxes();
        chart->setAxisX(lineAxis, lineSeries);
        chart->setAxisX(lineAxis, interSeries);
        //        chart->axisY(lineSeries)->setRange(min, max);
        chart->axisX(lineSeries)->setLabelsFont(QFont("Courier New", 9));

        Charter::graph();
      }
    }
  }
};

class LineSeriesCharter : public Charter {
 public:
  LineSeriesCharter() : Charter() {}

  QVector<QLineSeries *> lineSeries;
  QValueAxis *axis = 0;

  void init(QLayout *layout, int nSeries) {
    clear();

    for (int i = 0; i < nSeries; i++) lineSeries << new QLineSeries();
    axis = new QValueAxis();

    Charter::init(layout);
  }

  void clear() {
    if (isInit()) {
      for (auto l : lineSeries) delete l;
      lineSeries.clear();
      delete axis;
    }
  }

  void remove() {
    clear();
    Charter::remove();
  }

  void graph(QLayout *layout, Tree *tree, int nVars, QString title) {
    if (tree && nVars > 1) {
      int nSeries = tree->childCount();

      if (nSeries > 1) {
        init(layout, nSeries);

        QVector<LeastSquareFit::DoubleV> meanv(nSeries), xv(nSeries);

        for (int i = 0; i < nSeries; i++) {
          double x = 0, xInc = 0;

          for (auto n : tree->child(i)->children()) {  // scan 'i' tree level
            auto st = n->getStat();
            auto m = st.mean();
            bool ok;

            x = n->data(0).toDouble(&ok);
            x = ok ? x : xInc++;

            meanv[i] << m;
            xv[i] << x;

            lineSeries[i]->append(x, st.mean());
          }
        }

        for (int i = 0; i < nSeries; i++) {
          if (!xv[i].isEmpty()) {
            if (i == 0) {
              axis->setTickCount(tree->child(i)->childCount());
              axis->setRange(
                  tree->child(i)->children().first()->data(0).toDouble(),
                  tree->child(i)->children().last()->data(0).toDouble());
              axis->setLabelFormat("%.0f");
              axis->setTitleText(title);
              chart->setAnimationOptions(QChart::SeriesAnimations);
            }

            lineSeries[i]->setName(tree->child(i)->data(0).toString());
            chart->addSeries(lineSeries[i]);

            if (i == 0) {
              chart->createDefaultAxes();
              chart->setAxisX(axis, lineSeries[i]);
              chart->axisX(lineSeries[i])
                  ->setLabelsFont(QFont("Courier New", 9));
            }
          }
        }
        Charter::graph();
      }
    }
  }
};

class Charter3D {
 public:
  Charter3D() { surface.setFlags(surface.flags() ^ Qt::FramelessWindowHint); }

  Q3DSurface surface;
  QSurfaceDataArray *data = new QSurfaceDataArray;
  QSurfaceDataRow *dataRow1 = new QSurfaceDataRow;
  QSurfaceDataRow *dataRow2 = new QSurfaceDataRow;
  QSurface3DSeries *series = new QSurface3DSeries;
  QWidget *container = 0;

  void graph(QLayout *layout, Tree *tree) {
    dataRow1->clear();
    dataRow2->clear();
    data->clear();

    *dataRow1 << QVector3D(0.0f, 0.1f, 0.5f) << QVector3D(1.0f, 0.5f, 0.5f);
    *dataRow2 << QVector3D(0.0f, 1.8f, 1.0f) << QVector3D(1.0f, 1.2f, 1.0f);
    *data << dataRow1 << dataRow2;

    series->dataProxy()->resetArray(data);
    surface.addSeries(series);

    if (container) {
      layout->removeWidget(container);
      delete container;  //  & chart
    }
    container = QWidget::createWindowContainer(&surface);
    layout->addWidget(container);
  }
};

#endif  // CHARTER_H
