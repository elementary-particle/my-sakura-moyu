# -*- coding: utf-8 -*-

import csv, sys, time, zlib
from struct import pack, unpack

def main():
  rep = list()
  source = open(sys.argv[1], "r", encoding="utf-8-sig")
  reader = csv.reader(source)
  for row in reader:
    i = int(row[0])
    rep.append((i, int(row[1], 16), row[3]))
  source.close()
  with open("patch.bin.tmp", "wb") as tmp:
    tmp.write(pack("<L", len(rep)))
    for i, o, t in rep:
      if ((1 <= i and i < 53909) or i >= 57045):
        tmp.write(pack("<L", o))
        t = t.encode("gbk", errors="ignore") + b"\0"
        tmp.write(pack("<H", len(t)))
        tmp.write(t)
      else:
        tmp.write(pack("<L", o))
        t = b""
        tmp.write(pack("<H", len(t)))
        tmp.write(t)
  data = b""
  with open("patch.bin.tmp", "rb") as tmp:
    data = tmp.read()
  target = open("patch.dat", "wb")
  target.write(pack("<L", 0)) # compressed signature
  target.write(pack("<L", int(time.time())))
  target.write(pack("<L", len(data)))
  comp_data = zlib.compress(data)
  target.write(pack("<L", len(comp_data)))
  target.write(comp_data)
  target.close()

if __name__ == "__main__":
  main()
