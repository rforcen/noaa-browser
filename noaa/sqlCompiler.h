#ifndef sqlCompilerH
#define sqlCompilerH

#include <daily.h>
#include <math.h>
#include <QString>
#include <QStringList>

class SQLcompiler {
  enum {
    SNULL,
    NUMBER,
    STRING,
    FIELD,
    FIELDSTAT,
    PLUS,
    MINUS,
    MULT,
    DIV,
    OPAREN,
    CPAREN,
    PERIOD,
    COMMA,
    MOD,

    EQ,
    GT,
    GE,
    LT,
    LE,
    NE,
    NEG
  };

  // reserved words
  enum {  // symbol start from 90
    RANGE = 90,
    ABS,
    INT,
    SQRT,
    EMPTY,
    UPPER,
    LOWER,
    NOT,
    AND,
    OR,
    CONTAINS,
    ANYOF,
    ROW,
    LIMIT,

    PUSHCONST,
    PUSHSTRING,
    PUSHFIELD
  };
  QStringList resWords = {"range",    "abs",   "int", "sqrt", "empty",
                          "upper",    "lower", "not", "and",  "or",
                          "contains", "anyof", "row", "limit"};

  typedef enum {
    noError,
    errFieldNotFound,
    errComma,
    errNoIdent,
    errNoSym,
    errFuncNS,
    errRunTime,
  } ErrorCodes;

  typedef enum { ftDaily, ftStat, ftStation } FieldsType;

 private:
  const char *s;  // expression to evaluate;
  int sym;        // actual sym
  char ch;        // actual ch
  double nval;    // actual numerical value
  QString id;     // actual id,
  QString str;
  int nFld;  // actual field #
  FieldsType fType;
  int nStr;  // actual string #

  static const int maxCode = 1024;

  // compiler
  char code[maxCode];
  int pc, codeSize;

  // run time
  QVariant stack[50];
  QStringList strList;
  int sp;

 public:
  double res;  // result
  bool ok;     // error condition
  ErrorCodes err;

 public:
  SQLcompiler();
  SQLcompiler(SQLcompiler &sql);
  ~SQLcompiler();

  SQLcompiler &operator=(SQLcompiler &sql) {
    memcpy(this, &sql, sizeof(SQLcompiler));
    return *this;
  }

 private:
  char getch(void);  // get next char from *s
  void ungetch(void);
  int getsym(void);  // get next symbol

  void exp0(void);  // compiler +-
  void exp1(void);  // /*
  void exp2(void);  // #,id,func, ()
  void gen(int token, double f);
  void gen(int token, int i);
  void gen(int token);
  void gen(int token, FieldsType ft, int i);

  bool isField();

 public:
  bool compile(const char *expr);
  bool execute(const QVector<Daily::Bytes> &Rows, StatRow &stat, int recNo);
};

#endif
