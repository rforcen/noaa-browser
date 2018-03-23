#ifndef AUXFILE_H
#define AUXFILE_H

#include <QtCore>

class AuxFile {
 public:
  static const int nFiles = 4;

  typedef enum {
    fCountries,
    fStations,
    fElements,
    fStates,
    fMonths
  } FilesTypes;
  typedef enum { tChar, tReal } FieldsTypes;

  typedef QList<QByteArray> BytesList;
  typedef QPair<QByteArray, int> PairDescRow;
  typedef QMap<QByteArray, PairDescRow>
      IndexDesc;  // <description, row> index for large files as Stations
  typedef QMap<QByteArray, int> FileMap;  // key(id), nº
  QVector<FileMap> filesMap = QVector<FileMap>(nFiles);

  QStringList _resources = {":/data/ghcnd-countries.txt",
                            ":/data/ghcnd-stations.txt", ":/data/elements.txt",
                            ":/data/ghcnd-states.txt", ":/data/months.txt"};
  QStringList fileNames = {"countries", "stations", "elements", "states",
                           "months"};
  class Struct {
   public:
    Struct(QString fldName, int start, int size, FieldsTypes type)
        : fldName(fldName), start(start), size(size), type(type) {}

    QString fldName;
    int start;
    int size;
    FieldsTypes type;
  };
  /*
------------------------------
Variable   Columns   Type
------------------------------
0 ID            1-11   Character
1 LATITUDE     13-20   Real
2 LONGITUDE    22-30   Real
3 ELEVATION    32-37   Real
4 STATE        39-40   Character
5 NAME         42-71   Character
6 GSN FLAG     73-75   Character
7 HCN/CRN FLAG 77-79   Character
8 WMO ID       81-85   Character
------------------------------
  */
  QList<QList<Struct>> filesStruct = {
      {{"id", 0, 2, tChar}, {"name", 3, 62, tChar}},  // countries
      {{"country", 0, 2, tChar},                      // stations
       {"id", 0, 11, tChar},
       {"lat", 12, 8, tReal},
       {"long", 21, 9, tReal},
       {"elevation", 31, 6, tReal},
       {"state", 38, 2, tChar},
       {"name", 41, 30, tChar},
       {"gsn_flag", 72, 4, tChar},
       {"hcn_crn_flag", 76, 3, tChar},
       {"who_id", 80, 5, tChar}},
      {{"id", 0, 4, tChar}, {"name", 5, 90, tChar}},  // elements
      {{"id", 0, 2, tChar}, {"name", 3, 28, tChar}},  // states
      {{"id", 0, 2, tChar}, {"name", 2, 12, tChar}}   // months
  };

  class ElementHash {  // hash elements w/checksum
   public:
    ElementHash() {}
    ~ElementHash() {
      if (vHash) free(vHash);
    }

    uchar *vHash = 0;
    const int szKey = 4, fldId = 0;  // 4 bytes, key is fld=0
    const quint16 max = 65507, min = 146,
                  size = max - min;  // min: 146, max:65507, diff:65361

    void hash(QList<BytesList> fs) {  // generate hash array checksum-min
      if ((vHash = (uchar *)calloc(size, sizeof(*vHash))) != nullptr)
        for (int row = 0; row < fs.length(); row++)
          vHash[qChecksum(fs[row][fldId].constData(), szKey) - min] = row;
    }

    inline int operator[](char *key) const {
      return vHash[qChecksum(key, szKey) - min];
    }
    inline int operator[](QByteArray key) const {
      return vHash[qChecksum(key.constData(), szKey) - min];
    }
  };

  QList<QList<BytesList>> files;
  IndexDesc ixStations;
  ElementHash elemHash;
  bool isLoaded = false;

 public:
  AuxFile() {}

  int fieldIndex(FilesTypes ft, QString id) {
    int ix = 0;
    for (auto &f : filesStruct[ft]) {
      if (id == f.fldName) return ix;
      ix++;
    }
    return -1;
  }

  FieldsTypes fieldType(FilesTypes ft, int col) {
    return filesStruct[ft][col].type;
  }

  QStringList
  open() {  // load files flat sorted list, create filesMap for fast key access
    if (!isLoaded) {
      for (int f = fCountries; f <= fMonths; f++) {  // read all files

        QFile fh(_resources[f]);
        fh.open(QIODevice::ReadOnly);

        QList<BytesList> file;
        auto ba = fh.readAll();

        for (auto &line : ba.split('\n')) {
          if (!line.isEmpty()) {
            QList<QByteArray> fs;
            for (auto &fst : filesStruct[f])
              fs << line.mid(fst.start, fst.size).simplified();
            file << fs;
          }
        }

        std::sort(file.begin(), file.end());  // sort file by 1st field
        files << file;

        fh.close();
      }

      indexStations();
      elemHash.hash(files[fElements]);

      isLoaded = true;

      for (int f = fCountries; f <= fStates; f++) {  // generate filesMap
        auto fs = files[f];
        for (int row = 0; row < fs.length(); row++) filesMap[f][fs[row][0]]++;
      }
    }
    return fileNames;
  }

  void indexStations() {
    ixStations.clear();
    auto fs = files[fStations];
    for (int row = 0; row < fs.length(); row++)
      ixStations[fs[row][getFieldCol(fStations, "id")]] =
          PairDescRow(fs[row][getFieldCol(fStations, "name")], row);
  }

  FilesTypes getType(QString name) {
    return (FilesTypes)fileNames.indexOf(name);
  }

  QList<BytesList> getFile(FilesTypes ft) { return files[ft]; }

  int getRows(FilesTypes ft) {  // # lines
    return files[ft].length();
  }

  int getColumns(FilesTypes ft) { return filesStruct[ft].length(); }

  QString getFieldName(FilesTypes ft, int col) {
    return filesStruct[ft][col].fldName;
  }

  bool isNumber(FilesTypes ft, QModelIndex index) {
    return filesStruct[ft][index.column()].type == tReal;
  }

  bool isChar(FilesTypes ft, QModelIndex index) {
    return filesStruct[ft][index.column()].type == tChar;
  }

  QByteArray getData(FilesTypes ft, int row, int col) {
    return files[ft][row][col];
  }
  QByteArray getIdName(FilesTypes ft, int row) {
    return files[ft][row][getFieldCol(ft, "id")] + " - " +
           files[ft][row][getFieldCol(ft, "name")];
  }
  QByteArray getId(FilesTypes ft, int row) {
    return files[ft][row][getFieldCol(ft, "id")];
  }

  QByteArray getName(FilesTypes ft, int row) {
    return files[ft][row][getFieldCol(ft, "name")];
  }
  QByteArray getName(FilesTypes ft, QByteArray id) {
    int row = indexOf(ft, id);
    return files[ft][row][getFieldCol(ft, "name")];
  }
  QByteArray getAllFields(FilesTypes ft, QByteArray id) {
    int row = indexOf(ft, id);
    QByteArray srow;

    for (auto &f : files[ft][row]) srow += f + " | ";
    return srow;
  }

  QByteArray getAllFields(FilesTypes ft, int row) {
    QByteArray srow;
    for (auto &f : files[ft][row]) srow += f + " | ";
    return srow;
  }

  QByteArray getData(FilesTypes ft, QModelIndex index) {
    return files[ft][index.row()][index.column()];
  }

  QVariant getDataType(FilesTypes ft, int row, int col) {
    auto d = getData(ft, row, col);
    QVariant v;
    if (filesStruct[ft][col].type == tReal)
      v = QVariant(d.toDouble());
    else
      v = QVariant(d);
    return v;
  }

  int getFieldCol(FilesTypes ft, QString fldName) {
    int col = 0;
    for (int f = 0; f < filesStruct[ft].length(); f++)
      if (filesStruct[ft][f].fldName == fldName) {
        col = f;
        break;
      }
    return col;
  }

  QByteArray getData(FilesTypes ft, int row, QString fldName) {
    return files[ft][row][getFieldCol(ft, fldName)];
  }

  int find(FilesTypes ft, QString fldName, QString _text) {
    QByteArray text = _text.toLocal8Bit();
    int col = getFieldCol(ft, fldName);
    for (int row = 0; row < getRows(ft); row++)
      if (getData(ft, row, col) == text) return row;
    return -1;
  }

  int findAny(FilesTypes ft, QString _text, int from = 0) {
    auto text = _text.toLocal8Bit().constData();
    for (int row = from < 0 ? 0 : from; row < getRows(ft); row++)
      for (int col = 0; col < getColumns(ft); col++)
        if (strcasestr(getData(ft, row, col).constData(), text)) return row;
    return -1;
  }

  QByteArray find(FilesTypes ft, QString fldName, QString _text,
                  QString retFld) {
    QByteArray text = _text.toLocal8Bit();
    int col = getFieldCol(ft, fldName);
    for (int row = 0; row < getRows(ft); row++)
      if (getData(ft, row, col) == text)
        return getData(ft, row, getFieldCol(ft, retFld));
    return "";
  }

  QByteArray findDescription(FilesTypes ft, QByteArray id, int descFld = 1) {
    auto fc = files[ft];
    auto idl = {id, QByteArray()};

    return (*std::lower_bound(fc.begin(), fc.end(), idl,
                              [](QList<QByteArray> p1, QList<QByteArray> p2)
                                  -> bool { return p1[0] < p2[0]; }))[descFld];
  }

  QByteArray findCountry(QByteArray id) {
    return findDescription(fCountries, id);
  }
  QByteArray findElements(QByteArray id) {
    return findDescription(fElements, id);
  }
  QByteArray findState(QByteArray id) { return findDescription(fStates, id); }
  PairDescRow findStation(QByteArray id) { return ixStations[id]; }
  PairDescRow findStation(QString id) { return ixStations[id.toLocal8Bit()]; }

  int indexOf(FilesTypes ft, QString id) {
    auto fc = files[ft];
    auto idl = {id.toLocal8Bit(), id.toLocal8Bit()};  // x2 as station[1]='id'
    auto ixId = getFieldCol(ft, "id");

    auto i = std::lower_bound(
        fc.begin(), fc.end(), idl,
        [ixId](QList<QByteArray> p1, QList<QByteArray> p2) -> bool {
          return p1[ixId] < p2[ixId];
        });
    return std::distance(fc.begin(), i);
  }
};

extern AuxFile auxFile;  // aux files reader

#endif  // AUXFILE_H
