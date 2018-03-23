#ifndef DAILY_H
#define DAILY_H
#include <auxfile.h>
#include <math.h>
#include <statistic.h>
#include <treeitem.h>
#include <QList>
#include <QString>
#include <future>

/* Global values: (-3045,32588) (-3045,32588) (-94395,1010228)
 * qlist vs, c[] 4.1/2.5GB

  ID            1-11   Character
  YEAR         12-15   Integer
  MONTH        16-17   Integer
  ELEMENT      18-21   Character
  '''
      values = '''
  VALUE%       22-26   Integer
  MFLAG%       27-27   Character
  QFLAG%       28-28   Character
  SFLAG%       29-29   Character
*/
class Daily {
 public:
  typedef enum {
    fnCountry,
    fnStation,
    fnYear,
    fnMonth,
    fnElement,
    fnValue,
    fnMflag,
    fnQflag,
    fnSflag
  } Fields;
  typedef enum { ftChar, ftInt } FieldTypes;

  typedef quint64 BitSet;  // base dict bitset based key

  typedef QByteArray Bytes;  // key definition
  typedef QVector<Fields> VFields;
  typedef QVector<Bytes> VBytes;

  static const short nonValid = -9999;  // non valid valus
  static const int baseYear = 1763, endYear = 2100;
  static const int maxKeyStat = 105000000;  // 105e6

  QVector<FieldTypes> fieldTypes = {ftChar, ftChar, ftInt,  ftInt, ftChar,
                                    ftInt,  ftChar, ftChar, ftChar};
  VFields indexFlds = {fnCountry, fnStation, fnElement, fnYear,
                       fnMonth};  // all index fields
  int nFlds = indexFlds.length();

  const QString _header =
      "country,station,year,month,element,value,mflag,qflag,sflag";
  QStringList header;
  int nCols;  // header cols

  static const int szCountry = 2, szStation = 11, szYear = 4, szMonth = 2,
                   szElement = 4, szValue = 5;
  typedef struct { int from, size; } PosSize;
  const QList<PosSize> pos = {{0, szCountry}, {0, szStation},  {11, szYear},
                              {15, szMonth},  {17, szElement}, {21, szValue},
                              {26, 1},        {27, 1},         {28, 1}};

  typedef QMap<Bytes, int> Dictionary;  // key, # -> size
  typedef QMap<int, int> SizeBits;      // FieldsType, n- bits
  bool loaded = false;

  struct _FieldsLink {  // fields - auxfile link
    Fields f;
    AuxFile::FilesTypes ft;
  };
  QList<struct _FieldsLink> auxFieldLink = {{fnCountry, AuxFile::fCountries},
                                            {fnStation, AuxFile::fStations},
                                            {fnElement, AuxFile::fElements},
                                            {fnMonth, AuxFile::fMonths}};

  AuxFile::FilesTypes getAuxFileIndex(Fields f) {
    for (auto afl : auxFieldLink)
      if (afl.f == f) return afl.ft;
    return (AuxFile::FilesTypes)-1;
  }

  // field support
  int fieldIndex(QString id) {
    return indexFlds.indexOf((Fields)header.indexOf(id.toLower()));
  }
  FieldTypes fieldType(int ixf) { return fieldTypes[indexFlds[ixf]]; }

  // 1's mask &
  class Mask {
   public:
    BitSet _mask1[sizeof(long) * 8];  // 1, 11, 111, 1111...0xffffffffffffffff
    Mask() {
      BitSet b = 0;  // generate mask of 1's-> 0,1,11,111,...
      for (size_t i = 0; i < sizeof(long) * 8; i++) {
        _mask1[i] = b;
        b <<= 1;
        b |= 1;
      }
    }
    inline BitSet operator[](int i) { return _mask1[i]; }
  };
  Mask mask1;

  class BitKeyStat {  // binary key, stat
   public:
    BitKeyStat(int size) : size(size) {
      data = (Data *)calloc(size, sizeof(Data));
      if (data == nullptr) size = 0;
    }
    ~BitKeyStat() {
      if (data) delete[] data;
    }

    typedef struct _Data {  // data struct -> bitset key, stat info
      BitSet bitset;
      StatRow stat;

      inline bool operator<(const struct _Data &bk) const {
        return (bitset < bk.bitset);
      }
      inline bool operator<(const BitSet &bs) const {
        return (this->bitset < bs);
      }
      inline void set(BitSet bitset, StatRow stat) {
        this->bitset = bitset;
        this->stat = stat;
      }
    } Data;
    Data *data = nullptr;
    Data zeroData;
    int n = 0, size = 0;

    void add(const BitSet &bs, const StatRow &stat) {
      if (n < size) {
        data[n].bitset = bs;
        data[n].stat = stat;
        n++;
      }
    }
    void add(Data rec) {
      if (n < size) {
        data[n] = rec;
        n++;
      }
    }

    void sort() { std::sort(data, data + n); }

    Data *find(BitSet bs) { return std::lower_bound(data, data + n, bs); }
    // indexing r/w
    inline Data &operator[](int rn) {
      return data[filter ? filterRecs[rn] : rn];
    }
    inline const Data &operator[](int rn) const {
      return (rn < n) ? data[rn] : zeroData;
    }

    inline void operator<<(Data rec) { data[n++] = rec; }

    Data *begin() { return data; }
    Data *end() { return data + n; }

    int lenght() { return filter ? filterRecs.length() : n; }
    void fixSize() { data = (Data *)realloc(data, n * sizeof(Data)); }
    void init() {
      memset(data, 0, n * sizeof(Data));
      n = 0;
      filter = false;
      filterRecs.clear();
    }

    // filter
    bool filter = false;
    QVector<int> filterRecs;
    void addFilterRecno(int rn) { filterRecs << rn; }
    int nFilter() { return filterRecs.length(); }
    void initFilter() {
      filterRecs.clear();
      filter = false;
    }
    void applyFilter() { filter = true; }
  };
  BitKeyStat keyStat =
      BitKeyStat(maxKeyStat);  // main index struct Binary Key, Stat

  QMap<int, Dictionary> dict;  // one per index field. dict[field][key]->#
  QMap<int, QList<Bytes>>
      dictIndex;  // to access key from index. dictIndex[f][#]->key
  SizeBits bitsKey;

  Statistic stGlobal;  // global stat for find

  int size2Bits(double sz) {  // bits to accomodate sz values
    auto l = log2(sz);
    return (int)(l + (floor(l) == l ? 0. : 1.));
  }

  int nBitsDict = 0;

  void beginIndex() {  // load dicts from resource
    keyStat.init();
    stGlobal.init();
    loadDictsFromAuxFiles();
  }

  void endIndex() {  // sort keyStat
    keyStat.sort();
  }

  void sortFilter(QStringList flds, int sortOrder) {
    QVector<int> iflds;  // create index fields
    for (auto f : flds) iflds << getFldsNameStat().indexOf(f);
    auto fl = indexFlds.length();

    if (keyStat.filter) {  // sort filterRecs
      std::sort(keyStat.filterRecs.begin(), keyStat.filterRecs.end(),
                [this, iflds, fl, sortOrder](int i1, int i2) {
                  QVector<int> b1,
                      b2;  // create comparing vectors from data & stat
                  auto dbs1 = bitSet2Ints(keyStat.data[i1].bitset),
                       dbs2 = bitSet2Ints(keyStat.data[i2].bitset);
                  auto s1 = keyStat.data[i1].stat, s2 = keyStat.data[i2].stat;
                  for (auto f : iflds)
                    if (f < fl) {  // data
                      b1 << dbs1[f];
                      b2 << dbs2[f];
                    } else {  // stat
                      b1 << s1[f - fl];
                      b2 << s2[f - fl];
                    }

                  return (sortOrder) ? b1 > b2 : b1 < b2;
                });
    } else {
      std::sort(  // sort data
          keyStat.begin(), keyStat.end(),
          [this, iflds, fl, sortOrder](BitKeyStat::Data &d1,
                                       BitKeyStat::Data &d2) {
            QVector<int> b1, b2;  // create comparing vectors from data & stat
            auto dbs1 = bitSet2Ints(d1.bitset), dbs2 = bitSet2Ints(d2.bitset);
            auto s1 = d1.stat, s2 = d2.stat;
            for (auto f : iflds)
              if (f < fl) {  // data
                b1 << dbs1[f];
                b2 << dbs2[f];
              } else {  // stat
                b1 << s1[f - fl];
                b2 << s2[f - fl];
              }

            return (sortOrder) ? b1 > b2 : b1 < b2;
          });
    }
  }

  Tree *createTree(QStringList flds) {
    QString sl;
    QVector<int> iflds;  // create index fields & f1/f2/../
    for (auto f : flds) {
      auto ixf = header.indexOf(f);  // non stat fields
      if (ixf != -1) {
        iflds << ixf;
        sl += f + (f == flds.last() ? "" : "/");
      }
    }

    auto root = new Tree({sl, "n", "min", "max", "avg"});

    if (iflds.length())
      for (int r = 0; r < keyStat.lenght(); r++) {  // for all data
        auto d = keyStat[r];
        VBytes bks = bitSet2Keys(d.bitset), ks;  // required key
        for (auto f : iflds) ks << bks[indexFlds.indexOf((Fields)f)];
        insert(root, ks, d.stat);
      }

    root->genStats();
    return root;
  }

  Tree *insert(Tree *t, VBytes keys, StatRow &stat) {
    Tree *_t = t, *__t;
    for (auto k : keys) {
      if ((__t = _t->findChild(k)) == nullptr)
        _t->appendChild(__t = new Tree({k}, _t));
      else
        __t->addStat(stat);
      _t = __t;
    }
    return t;
  }

  QStringList getFldsNameStat() {  // fields names and stat fields names
    QStringList sl;
    for (auto f : indexFlds) sl << header[f];
    sl << StatRow::header;
    return sl;
  }

  void loadDictsFromAuxFiles() {
    if (!loaded) {
      // load dict
      for (auto fr : auxFieldLink)  // countries, stations, elements
        for (auto nrow = 0; nrow < auxFile.getRows(fr.ft); nrow++)
          dict[fr.f][auxFile.getId(fr.ft, nrow)] = nrow;
      // years 1750..2100 range

      for (int y = baseYear; y < endYear; y++)
        dict[fnYear][Bytes::number(y)] = y - baseYear;

      nBitsDict = bits2CodeDict();
      if (nBitsDict <= 64) {  // should be < 64 to accomodate in long int
        for (int f = fnCountry; f <= fnElement; f++) {  // in indexFlds order
          bitsKey[f] = size2Bits(dict[f].size());
          dictIndex[f] = dict[f].keys();
        }
      }

      loaded = true;
    }
  }

  void getStationCountry(Bytes station, int &idStation, int &idCountry) {
    idStation = dict[fnStation][station];
    idCountry = dict[fnCountry][station.left(2)];
  }

  void addRow(Bytes row, int idStation,
              int idCountry) {  // with dict & stats create bitIndex
    StatRow st;
    keyStat.add({row2BitSet(row, idStation, idCountry), st = row2Stat(row)});
    stGlobal += st;
  }

  VBytes getRow(int row) { return bitSet2Keys(keyStat[row].bitset); }
  StatRow &getStat(int row) { return keyStat[row].stat; }

  Bytes line2field(Bytes &line, int nf)
      const {  // first cols to fnValue are scalars, 5,6,7,8 are [31] arrays
    return (nf < fnValue ? line.mid(pos[nf].from, pos[nf].size)
                         : array31(line, nf));
  }

  // generate opt. hash code from row keys
  BitSet row2BitSet(Bytes &row, int idStation, int idCountry) {
    BitSet bs = 0;
    for (auto f : indexFlds) {
      auto key = line2field(row, f);

      if (f != indexFlds.first()) bs <<= bitsKey[f];
      switch (f) {
        case fnStation:
          bs |= idStation;
          break;
        case fnCountry:
          bs |= idCountry;
          break;
        case fnYear:
          bs |= key.toInt() - baseYear;
          break;
        case fnMonth:
          bs |= key.toInt() - 1;  // 0-jan..11-dec
          break;
        case fnElement:
          bs |= auxFile.elemHash[key];
          break;
        default:
          bs |= dict[f][key];
          break;
      }
    }
    return bs;
  }

  BitSet __row2BitSet(Bytes &row, int idStation, int idCountry) {
    VBytes k = VBytes(nFlds);  // generate key in file order
    for (auto f = 0; f < nFlds; f++) k[f] = line2field(row, f);
    return keys2BitSet(k, idStation, idCountry);
  }

  BitSet keys2BitSet(const VBytes &key, int idStation,
                     int idCountry) {  // convert key to BitSet
    BitSet bs = 0;
    for (auto f : indexFlds) {
      if (f != indexFlds.first()) bs <<= bitsKey[f];
      switch (f) {
        case fnStation:
          bs |= idStation;
          break;
        case fnCountry:
          bs |= idCountry;
          break;
        case fnYear:
          bs |= key[f].toInt() - baseYear;
          break;
        case fnMonth:
          bs |= key[f].toInt() - 1;  // 0-jan..11-dec
          break;
        default:  // element
          bs |= dict[f][key[f]];
          break;
      }
    }
    return bs;
  }

  BitSet ints2BitSet(const QVector<int> &ints) {  // convert ints vect to BitSet
    BitSet bs = 0;
    for (int f = 0; f < nFlds; f++) {
      BitSet b = ints[f];
      bs |= b;
      if (f < nFlds - 1) bs <<= bitsKey[indexFlds[f + 1]];
    }
    return bs;
  }

  BitSet ints2BitSet(const QVector<int> &ints,
                     VFields &newFields) {  // convert ints vect to BitSet
    BitSet bs = 0;
    for (int f = 0; f < nFlds; f++) {
      BitSet b = ints[f];
      bs |= b;
      if (f < nFlds - 1) bs <<= newFields[indexFlds[f + 1]];
    }
    return bs;
  }

  BitSet key2BitSet(int f, Bytes &key) {  // convert key to BitSet
    return (BitSet)dict[indexFlds[f]][key];
  }

  VBytes bitSet2Keys(BitSet bs) {  // in indexFields
    VBytes key = VBytes(nFlds);
    for (int f = nFlds - 1; f >= 0; f--) {
      int ixf = indexFlds[f];
      int b = bs & mask1[bitsKey[ixf]];
      key[f] = dictIndex[ixf][b];  // key of index 'b'

      bs >>= bitsKey[ixf];
    }

    return key;
  }

  Bytes bitSet2Key(int nf, BitSet bs) {  // conv. BitSet2Key
    for (int f = nFlds - 1; f >= 0; f--) {
      int ixf = indexFlds[f];
      int b = bs & mask1[bitsKey[ixf]];
      if (f == nf) {
        auto k = dictIndex[ixf][b];
        return k;
      }
      bs >>= bitsKey[ixf];
    }
    return Bytes();
  }

  int bitSet2int(int nf, BitSet bs) {  //
    for (int f = nFlds - 1; f >= 0; f--) {
      int ixf = indexFlds[f];
      if (f == nf) return bs & mask1[bitsKey[ixf]];
      bs >>= bitsKey[ixf];
    }
    return 0;
  }

  QVector<int> bitSet2Ints(BitSet bs) {
    QVector<int> vi = QVector<int>(nFlds);
    for (int f = nFlds - 1; f >= 0; f--) {
      int ixf = indexFlds[f];
      int b = bs & mask1[bitsKey[ixf]];
      vi[f] = b;
      bs >>= bitsKey[ixf];
    }
    return vi;
  }

  int bits2CodeDict() {  // total bits required to code 'dict[]'
    double p = 1;
    for (auto d : dict) p *= d.size();
    return size2Bits(p);
  }

  void sortBy(VFields newFlds) {
    struct {  // swaps holder, nswaps
      int from, to;
    } swPos[nFlds];
    int nsw = 0;

    for (int i = 0; i < nFlds; i++)  // complete newFlds w/missing flds
      if (newFlds.indexOf(indexFlds[i]) == -1) newFlds << indexFlds[i];

    for (int i = 0; i < nFlds; i++) {  // swap dictVect, bkOld
      auto pn = indexFlds.indexOf(newFlds[i]);
      swPos[nsw++] = {i, pn};
    }

    for (auto &ks : keyStat) {           // re-arrange keyStat
      auto iv = bitSet2Ints(ks.bitset);  // w/ actual bitsKey
      auto _iv = iv;
      for (int i = 0; i < nsw; i++) _iv[swPos[i].from] = iv[swPos[i].to];
      ks.bitset = ints2BitSet(_iv, newFlds);
    }

    indexFlds = newFlds;

    keyStat.sort();
  }

  class DailyStr {
   public:
    DailyStr() {}
    DailyStr(DailyStr *d) { memcpy(d, this, sizeof(*this)); }

    char station[11];
    short year, month;
    char element[4];
    short value[31];
    char mflag[31], sflag[31], qflag[31];
  };
  DailyStr daily;

  Daily() {
    header = _header.split(',');
    nCols = header.length();
    nFlds = indexFlds.length();
  }

  QByteArray array31(QByteArray &line, int nf) const {
    QByteArray sr;

    for (int d = 0; d < 31; d++)
      sr += line.mid(pos[nf].from + d * 8, pos[nf].size) + ' ';
    return sr;
  }

  bool testStationEQ(DailyStr *daily, char *station) {
    return strncmp(station, daily->station, szStation) == 0;
  }

  bool testYearEQ(DailyStr *daily, short year) { return daily->year == year; }

  DailyStr *line2daily(QByteArray &line) {
    char t[10];
    auto l = line.constData();

    memcpy(daily.station, l + pos[0].from, pos[0].size);  // station
    memcpy(t, l + pos[1].from, pos[1].size);              // year
    t[pos[1].size] = 0;
    daily.year = atoi(t);
    memcpy(t, l + pos[2].from, pos[2].size);  // month
    t[pos[2].size] = 0;
    daily.month = atoi(t);
    memcpy(daily.element, l + pos[3].from, pos[3].size);  // element
    for (int d = 0; d < 31; d++) {                        // array[31]
      memcpy(t, l + pos[4].from + d * 8, pos[4].size);
      t[pos[4].size] = 0;
      daily.value[d] = atoi(t);
      daily.mflag[d] = l[pos[5].from + d * 8];
      daily.qflag[d] = l[pos[6].from + d * 8];
      daily.sflag[d] = l[pos[7].from + d * 8];
    }
    return &daily;
  }

  QVector<int> line2values(
      QByteArray &line) {  // vector of valid values in line
    QVector<int> v(31);
    char _v[6];
    auto _line = line.constData();

    for (int d = 0, i, p; d < 31; d++) {
      for (i = 0, p = pos[fnValue].from + d * 8; i < szValue; i++)
        _v[i] = _line[p + i];
      _v[i] = 0;
      auto value = atoi(_v);

      char qflag = _line[pos[fnQflag].from];
      if (value != Daily::nonValid && qflag == ' ') v[d] = value;
    }
    return v;
  }

  StatRow row2Stat(const QByteArray &line) {  // vector of valid values in line
    StatRow st;
    char _v[szValue + 1];
    auto _line = line.constData();
    auto posV = pos[fnValue].from;
    auto posQ = pos[fnQflag].from;

    for (int d = 0, i = 0, p = posV + d * 8; d < 31; d++) {
      while (i < szValue) _v[i++] = _line[p++];
      _v[i] = 0;
      auto value = atoi(_v);

      if (value != Daily::nonValid && _line[posQ] == ' ') {
        st.n++;
        st.sum += value;
        if (value > st.max) st.max = value;
        if (value < st.min) st.min = value;
      }
    }
    return st;
  }

  int getnColumns() const { return nCols; }
  int getSize(int fld) { return pos[fld].size; }

  void printLimits() {
    int minmin = INT_MAX, maxmin = INT_MIN, minmax = INT_MAX, maxmax = INT_MIN,
        minsum = INT_MAX, maxsum = INT_MIN;
    for (auto ks : keyStat) {
      StatRow st = ks.stat;
      if (st.n == 0) st.zero();

      minmin = std::min<int>(minmin, st.min);
      maxmin = std::max<int>(maxmin, st.min);

      minmax = std::min<int>(minmax, st.max);
      maxmax = std::max<int>(maxmax, st.max);

      minsum = std::min<int>(minsum, st.sum);
      maxsum = std::max<int>(maxsum, st.sum);
    }
    printf("Global values: (%d,%d) (%d,%d) (%d,%d)\n", minmin, maxmin, minmax,
           maxmax, minsum, maxsum);
  }

  void report(const VFields &flds) {
    int nf = flds.size();
    VBytes keys(nf);
    bool brk = false;
    Statistic st[nf + 1];

    sortBy(flds);

    for (auto ks : keyStat) {
      brk = false;
      for (int i = 0; i < nf; i++) {
        auto ki = bitSet2Key(i, ks.bitset);
        if (keys[i] != ki || brk) {
          printf("%*c%s - %s\n", i * 3, ' ', ki.constData(),
                 st[i + 1].toChar());
          brk = true;  // propagate break

          st[i] += st[i + 1];
          st[i + 1].zero();
        }
        st[i + 1] += ks.stat;
      }

      for (int i = 0; i < nf; i++) keys[i] = bitSet2Key(i, ks.bitset);
    }
  }

  Bytes genFieldsHeader(const VFields &flds) {
    Bytes h;
    for (auto f : flds) h += header[f] + "/";
    return h.left(h.length() - 1);
  }

  bool findPartial(const QVector<int> &ivkey, StatRow &sr) {  //
    auto kf =
        keyStat.find(ints2BitSet(ivkey + QVector<int>(nFlds - ivkey.size())));
    auto _resiv = bitSet2Ints(kf->bitset);

    for (int i = 0; i < ivkey.size(); i++)  // partial match?
      if (ivkey[i] != _resiv[i]) return false;

    sr = kf->stat;
    return true;
  }

  Tree *genTree(const VFields &flds) {
    sortBy(flds);
    return genTree(flds, 0, new Tree({genFieldsHeader(flds)}), {});
  }

  Tree *genTree(const VFields &flds, int fn, Tree *t, QVector<int> accKey) {
    int count = 0;
    Statistic st;
    StatRow sr;

    if (fn < flds.size() - 1) {
      for (auto k = 0; k < dict[fn].size(); k++) {
        auto sk = accKey + QVector<int>{k};
        if (findPartial(sk, sr)) {
          t->appendChild(
              genTree(flds, fn + 1, new Tree({dictIndex[fn][k]}, t), sk));
          count++;
          st += sr;
        }
      }
    } else {
      for (auto k = 0; k < dict[fn].size(); k++)
        if (findPartial(accKey + QVector<int>{k}, sr)) {
          t->appendChild(new Tree(
              {dictIndex[fn][k], "", sr.format(sr.n), sr.format(sr.min),
               sr.format(sr.max), sr.format(sr.sum)},
              t));
          count++;
          st += sr;
        }
    }
    return t->appendData({count, st.format(st.n), st.format(st.min),
                          st.format(st.max), st.format(st.sum)});
  }
};

extern Daily dailySt;

#endif  // DAILY_H
