// sqlCompiler.cpp

#include <sqlCompiler.h>

// constructors

SQLcompiler::SQLcompiler() {}
SQLcompiler::SQLcompiler(SQLcompiler &sql) {
  memcpy(this, &sql, sizeof(SQLcompiler));
}

SQLcompiler::~SQLcompiler() {}

// get next char from *s
char SQLcompiler::getch(void) {
  ch = 0;
  if (s) {
    ch = *s;
    if (ch) s++;
  }
  return ch;
}

void SQLcompiler::ungetch(void) { s--; }

bool SQLcompiler::isField() {  // update nFld, nFldStat, sym
  auto isf = true;
  sym = FIELD;
  if ((nFld = dailySt.fieldIndex(id)) != -1) {
    fType = ftDaily;
  } else if ((nFld = StatRow::fieldIndex(id)) != -1) {
    fType = ftStat;
  } else if ((nFld = auxFile.fieldIndex(AuxFile::fStations, id)) != -1) {
    fType = ftStation;
  } else
    isf = false;

  return isf;
}

// get next symbol
int SQLcompiler::getsym(void) {
  int i, ixf;

  sym = SNULL;

  // skip blanks
  while (ch && ch <= ' ') getch();

  // detect symbol
  id.clear();

  if (isalpha(ch)) {  // ident
    for (i = 0; isalnum(ch) || ch == '_'; i++) {
      id += ch;
      getch();
    }

    // field?
    if (!isField()) {  // reserved word ?
      ixf = resWords.indexOf(id.toLower());
      if (ixf != -1)
        sym = ixf + RANGE;
      else
        err = errNoIdent;
    }
  } else {
    if (isdigit(ch)) {  // number (double) take care of dddd.ddde-dd
      for (i = 0; isdigit(ch) || ch == '.' || ch == 'e' || ch == 'E'; i++) {
        id += ch;
        getch();
      }
      sym = NUMBER;
      nval = id.toDouble();
    } else {
      if (ch == '"' || ch == '\'') {
        auto ch_iniStr = ch;
        str.clear();
        while (getch() != ch_iniStr && ch != 0) str += ch;
        sym = STRING;
        nStr = strList.size();
        strList.append(str);
      } else {
        switch (ch) {
          case '+':
            sym = PLUS;
            break;
          case '-':
            sym = MINUS;
            break;
          case '*':
            sym = MULT;
            break;
          case '/':
            sym = DIV;
            break;
          case '(':
            sym = OPAREN;
            break;
          case ')':
            sym = CPAREN;
            break;
          case '.':
            sym = PERIOD;
            break;
          case ',':
            sym = COMMA;
            break;
          case '%':
            sym = MOD;
            break;

          case '=':
            sym = EQ;
            break;
          case '>':
            getch();
            if (ch == '=')
              sym = GE;
            else {
              ungetch();
              sym = GT;
            }
            break;
          case '!':
            getch();
            switch (ch) {
              case '=':
                sym = NE;
                break;
              default:
                ungetch();
                sym = NEG;
                break;
            }
            break;

          case '<':
            getch();
            switch (ch) {
              case '>':
                sym = NE;
                break;
              case '=':
                sym = LE;
                break;
              default:
                ungetch();
                sym = LT;
                break;
            }
            break;
          case 0:
            sym = SNULL;
            break;

          default:
            sym = SNULL;
            err = errNoSym;
            break;
        }
      }
      getch();
    }
  }

  return sym;
}

// Compile and execute SQL

// generate code
void SQLcompiler::gen(int token, double f) {
  code[pc++] = (char)token;
  *(double *)(code + pc) = f;
  pc += sizeof(double);
}

void SQLcompiler::gen(int token, int i) {
  code[pc++] = (char)token;
  *(int *)(code + pc) = i;
  pc += sizeof(int);
}

void SQLcompiler::gen(int token, FieldsType ft, int i) {
  code[pc++] = (char)token;
  *(int *)(code + pc) = (int)ft;
  pc += sizeof(int);
  *(int *)(code + pc) = i;
  pc += sizeof(int);
}

void SQLcompiler::gen(int token) { code[pc++] = (char)token; }

// Compile expression, return error=true

bool SQLcompiler::compile(const char *expr) {
  pc = 0;
  s = expr;
  getch();
  err = noError;
  getsym();

  exp0();
  codeSize = pc;
  ok = (err == noError);
  return ok;  // true = ok
}

// WHERE
void SQLcompiler::exp0(void) {
  if (err == noError) {
    exp1();
    do {
      switch (sym) {
        case PLUS:
          getsym();
          exp1();
          gen(PLUS);
          break;
        case MINUS:
          getsym();
          exp1();
          gen(MINUS);
          break;

        case AND:
          getsym();
          exp1();
          gen(AND);
          break;
        case OR:
          getsym();
          exp1();
          gen(OR);
          break;

        default:
          break;
      }
    } while (sym == PLUS || sym == MINUS || sym == AND || sym == OR);
  }
}

void SQLcompiler::exp1(void) {
  if (err == noError) {
    exp2();
    do {
      switch (sym) {
        case MULT:
          getsym();
          exp2();
          gen(MULT);
          break;
        case DIV:
          getsym();
          exp2();
          gen(DIV);
          break;
        case MOD:
          getsym();
          exp2();
          gen(MOD);
          break;

        case EQ:
          getsym();
          exp2();
          gen(EQ);
          break;
        case CONTAINS:
          getsym();
          exp2();
          gen(CONTAINS);
          break;

        case ANYOF: {  // expr anyof(expr,expr,....)
          int np = 0;
          getsym();
          if (sym == OPAREN) {
            do {
              getsym();  // ( | ,
              exp2();
              np++;
            } while (sym == COMMA);
            getsym();  // )
                       //          getsym();
            gen(ANYOF, np);
          } else
            err = errComma;
        } break;

        case NE:
          getsym();
          exp2();
          gen(NE);
          break;
        case LT:
          getsym();
          exp2();
          gen(LT);
          break;
        case LE:
          getsym();
          exp2();
          gen(LE);
          break;
        case GT:
          getsym();
          exp2();
          gen(GT);
          break;
        case GE:
          getsym();
          exp2();
          gen(GE);
          break;
      }
    } while (sym == MULT || sym == DIV || sym == MOD || sym == EQ ||
             sym == NE || sym == LT || sym == LE || sym == GT || sym == GE);
  }
}

void SQLcompiler::exp2(void) {
  if (err == noError) {
    switch (sym) {
      case OPAREN:
        getsym();
        exp0();
        getsym();
        break;
      case NUMBER:
        gen(PUSHCONST, nval);
        getsym();
        break;
      case STRING:
        gen(PUSHSTRING, nStr);
        getsym();
        break;
      case FIELD:
        gen(PUSHFIELD, fType, nFld);
        getsym();
        break;

      case MINUS:
        getsym();
        exp2();
        gen(NEG);
        break;
      case NOT:
        getsym();
        exp2();
        gen(NOT);
        break;

      case PLUS:
        getsym();
        exp2();
        break;

      case RANGE:  // range(exp, from,to)
        getsym();  // (
        getsym();
        exp0();    //  exp
        getsym();  // ,
        exp0();    // from
        getsym();  // ,
        exp0();    // to
        getsym();  // )
        gen(RANGE);
        break;

      case ROW:
        gen(ROW);
        getsym();
        break;

      case UPPER:
      case LOWER:
      case EMPTY:
      case INT:
      case ABS:
      case LIMIT:
      case SQRT: {
        int tsym = sym;
        getsym();
        exp2();
        gen(tsym);
      } break;

      case SNULL:
        break;
      default:
        err = errFuncNS;
        break;  // syntax error
    }
  }
}

bool SQLcompiler::execute(const QVector<Daily::Bytes> &Row, StatRow &stat,
                          int recNo) {
  err = noError;
  sp = 0;

  for (pc = 0; pc < codeSize && err == noError;) {
    switch (code[pc]) {
      case PUSHCONST:
        pc++;
        stack[sp] = *(double *)(code + pc);
        sp++;
        pc += sizeof(double);
        break;
      case PUSHFIELD:
        pc++;

        fType = (FieldsType)(*(int *)(code + pc));
        pc += sizeof(int);
        nFld = *(int *)(code + pc);

        switch (fType) {
          case ftDaily:
            switch (dailySt.fieldType(nFld)) {
              case Daily::ftInt:
                stack[sp] = Row[nFld].toInt();
                break;
              case Daily::ftChar:
                stack[sp] = Row[nFld];
                break;
            }
            break;
          case ftStat:
            stack[sp] = stat[nFld];
            break;
          case ftStation: {
            // get row and content -> convert to type
            auto r = auxFile
                         .findStation(
                             Row[dailySt.indexFlds.indexOf(Daily::fnStation)])
                         .second;
            auto d = auxFile.getData(AuxFile::fStations, r, nFld);

            switch (auxFile.fieldType(AuxFile::fStations, nFld)) {
              case AuxFile::tReal:
                stack[sp] = d.toDouble();
                break;
              case AuxFile::tChar:
                stack[sp] = d;
                break;
            }
          } break;
        }

        sp++;
        pc += sizeof(int);
        break;

      case PUSHSTRING:
        pc++;
        nStr = *(int *)(code + pc);
        stack[sp] = strList[nStr];
        sp++;
        pc += sizeof(int);
        break;

      case ROW:
        pc++;
        stack[sp] = recNo;
        sp++;
        break;

      case NOT:
        stack[sp - 1] = !stack[sp - 1].toBool();
        pc++;
        break;
      case AND:
        sp--;
        stack[sp - 1] = stack[sp - 1].toBool() && stack[sp].toBool();
        pc++;
        break;
      case OR:
        sp--;
        stack[sp - 1] = stack[sp - 1].toBool() || stack[sp].toBool();
        pc++;
        break;

      case PLUS:
        sp--;
        switch (stack[sp - 1].type()) {
          case QVariant::Double:
            stack[sp - 1] = stack[sp - 1].toDouble() + stack[sp].toDouble();
            break;
          case QVariant::String:
            stack[sp - 1] = stack[sp - 1].toString() + stack[sp].toString();
            break;
          default:
            break;
        }
        pc++;
        break;
      case MINUS:
        sp--;
        stack[sp - 1] = stack[sp - 1].toDouble() - stack[sp].toDouble();
        pc++;
        break;
      case EQ:
        sp--;
        stack[sp - 1] = stack[sp - 1] == stack[sp];
        pc++;
        break;
      case CONTAINS:
        sp--;
        stack[sp - 1] = stack[sp - 1].toString().contains(stack[sp].toString(),
                                                          Qt::CaseInsensitive);
        pc++;
        break;
      case ANYOF: {
        pc++;
        nFld = *(int *)(code + pc);
        sp -= nFld;
        bool found = false;
        for (int i = 0; i < nFld && !found; i++)
          found = (stack[sp - 1] == stack[i + sp]);
        stack[sp - 1] = found;
        pc += sizeof(int);
      } break;
      case NE:
        sp--;
        stack[sp - 1] = stack[sp - 1] != stack[sp];
        pc++;
        break;
      case LT:
        sp--;
        stack[sp - 1] = stack[sp - 1] < stack[sp];
        pc++;
        break;
      case LE:
        sp--;
        stack[sp - 1] = stack[sp - 1] <= stack[sp];
        pc++;
        break;
      case GT:
        sp--;
        stack[sp - 1] = stack[sp - 1] > stack[sp];
        pc++;
        break;
      case GE:
        sp--;
        stack[sp - 1] = stack[sp - 1] >= stack[sp];
        pc++;
        break;
      case MULT:
        sp--;
        stack[sp - 1] = stack[sp - 1].toDouble() * stack[sp].toDouble();
        pc++;
        break;
      case DIV:
        sp--;
        if (stack[sp].toDouble() != 0)
          stack[sp - 1] = stack[sp - 1].toDouble() / stack[sp].toDouble();
        pc++;
        break;
      case MOD:
        sp--;
        if (stack[sp].toDouble() != 0)
          stack[sp - 1] = fmod(stack[sp - 1].toDouble(), stack[sp].toDouble());
        pc++;
        break;

      case NEG:
        stack[sp - 1] = -stack[sp - 1].toDouble();
        pc++;
        break;

      case LIMIT:
        stack[sp - 1] = recNo < stack[sp - 1].toInt();
        pc++;
        break;

      case INT:
        stack[sp - 1] = floor(stack[sp - 1].toDouble());
        pc++;
        break;
      case ABS:
        stack[sp - 1] = fabs(stack[sp - 1].toDouble());
        pc++;
        break;
      case UPPER:
        stack[sp - 1] = stack[sp - 1].toString().toUpper();
        pc++;
      case LOWER:
        stack[sp - 1] = stack[sp - 1].toString().toUpper();
        pc++;
      case EMPTY:
        stack[sp - 1] = stack[sp - 1].toString().isEmpty();
        pc++;

      case SQRT:
        if (stack[sp - 1] >= 0)
          stack[sp - 1] = sqrt(stack[sp - 1].toDouble());
        else
          stack[sp - 1] = 0;
        pc++;
        break;

      case RANGE: {
        stack[sp - 3] = (stack[sp - 3] <= stack[sp - 1]) &&
                        (stack[sp - 3] >= stack[sp - 2]);
        pc++;
        sp -= 2;
      } break;

      default:
        err = errRunTime;
        break;
    }
  }

  return (err == noError) ? stack[sp - 1].toBool() : false;
}
