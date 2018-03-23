#include "dimension.h"

// convert index 2 tree
Tree* btIndex2Tree(
    QString fnme) {  // "/Volumes/backup 1TB/noaa data/elemyear.idx"
  BtreeDX idx;
  Tree *root = new Tree({"title", "#"}), *t = 0;
  int pc = 1;
  if (idx.open(fnme.toStdString())) {
    char _k[80];
    int recNo;
    Bytes k, ka, k1, k2;

    while (idx.next(_k, recNo)) {
      k = _k;
      if (!k.isEmpty()) {
        k1 = k.left(4);
        k2 = k.right(4);
        if (k1 != ka) {
          if (t) root->appendChild(t->setData({ka, pc}));
          t = new Tree({k1}, root);
          t->appendChild(new Tree({k2}, t));
          pc = 1;
        } else {
          t->appendChild(new Tree({k2}, t));
          pc++;
        }
        ka = k1;
      }
    }
    if (t) {
      root->appendChild(t->setData({ka, pc}));
    }

    idx.close();
  }

  return root;
}
