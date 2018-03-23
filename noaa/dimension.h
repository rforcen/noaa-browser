#ifndef DIMENSION_H
#define DIMENSION_H

#include <btreedx.h>
#include <daily.h>
#include <statistic.h>
#include <treeitem.h>

#include <QMap>
#include <QtCore>

typedef QByteArray Bytes;

typedef QMap<Bytes, Statistic> IndexStat;  // simple Statistic Index
typedef QSet<int> Recs;

class RecItem {
 public:
  Bytes key;
  Statistic stat;
};

class KeyItem {
 public:
  Recs recs;
  Statistic stat;

  void add(int rec, DailyStruct::Daily* daily) {
    recs << rec;
    stat << daily;
  }
};

class Index {
 public:
  typedef QMap<Bytes, KeyItem> KeyRecsIndex;
  typedef QMap<Bytes, IndexStat> TreeIndex;

  KeyRecsIndex index;

  DailyStruct ds;
  Statistic st;
  int fld, sz;

  Index(int fld) : fld(fld), sz(ds.getSize(fld)) {}
  ~Index() { clear(); }

  void clear() { index.clear(); }

  void add(Bytes line, DailyStruct::Daily* daily, int recNo) {
    auto kk = ds.line2field(line, fld);  // get key
    index[kk].add(recNo, daily);         // create / acc. statis
  }

  Tree* genTree(QList<Index*> ixl, Tree* t, int i, Recs recs) {
    if (i < ixl.size() - 1) {
      for (auto k : ixl[i]->index.keys())
        t->appendChild(
            genTree(ixl, new Tree({k}, t), i + 1, ixl[i]->index[k].recs));
    } else {
      auto accSt = Statistic();
      for (auto k : ixl[i]->index.keys()) {
        auto item = ixl[i]->index[k];
        if (recs.intersects(item.recs)) {
          auto _st = item.stat;
          accSt += _st;
          t->appendChild(
              new Tree({k, "", st.format(_st.n), _st.min, _st.max,
                        st.format(_st.mean()), st.format((float)_st.sum)},
                       t));
        }
      }
      t->setData({t->data(0), st.format(t->childCount()), st.format(accSt.n),
                  accSt.min, accSt.max, st.format(accSt.mean()),
                  st.format((float)accSt.sum)});
    }
    return t;
  }

  Tree* operator[](Index& ix) {
    return genTree({this, &ix},
                   new Tree({ds.header[fld] + "/" + ds.header[ix.fld], "#",
                             "# obs.", "min", "max", "avg", "sum"}),
                   0, {});  // index[index.keys().first()].recs
  }
};

// manages a double index by key and by recno

class Dimension {
  typedef QList<int> RecList;
  typedef QList<Bytes> BytesList;

  typedef QMap<Bytes, KeyItem> Key;  // key index[key] = rec
  typedef QMap<int, RecItem> Rec;    // rec index[recNo] = key

 public:
  Dimension(int fk) : fld(fk) {}
  ~Dimension() { clear(); }

  void add(Bytes line, DailyStruct::Daily* daily, int recNo) {  // and stats
    // get fk key
    auto kk = ds.line2field(line, fld);

    key[kk].recs << recNo;  // key index [key]
    key[kk].stat << daily;

    rec[recNo].key = kk;  // rec index rec[recNo]
    rec[recNo].stat << daily;
  }

  Tree* operator[](Dimension& d) {
    Tree* root = new Tree({ds.header[fld] + "/" + ds.header[d.fld], "#",
                           "# obs.", "min", "max", "avg", "sum"});
    for (auto k : key.keys()) {
      auto st = key[k].stat;
      IndexStat rm = genRecMap(k, d);

      Tree* t =
          new Tree({k, st.format(rm.size()), st.format(st.n), st.min, st.max,
                    st.format(st.mean()), st.format((float)st.sum)},
                   root);

      for (auto i : rm.keys()) {
        auto st = rm[i];
        t->appendChild(
            new Tree({i, "", st.format(st.n), st.min, st.max,
                      st.format(st.mean()), st.format((float)st.sum)},
                     t));
      }

      root->appendChild(t);
    }
    return root;
  }
  IndexStat genRecMap(Bytes k, Dimension& d) {
    IndexStat rm;
    for (auto r : key[k].recs) {
      auto kr = d.rec[r].key;
      rm[kr] += d.rec[r].stat;
    }
    return rm;
  }

  Tree* genTree(Dimension& d1, Dimension& d2) { return d1[d2]; }

  void clear() {
    for (auto k : key) k.recs.clear();
    key.clear();
    rec.clear();
  }

  void print() {
    for (auto k : key.keys()) {
      printf("%s - %s\n", k.constData(),
             key[k].stat.toString().toLatin1().constData());
    }
  }

 private:
  DailyStruct ds;
  Key key;
  Rec rec;
  int fld;  // field key
};

Tree* btIndex2Tree(QString fnme);

#endif  // DIMENSION_H
