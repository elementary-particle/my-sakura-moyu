# -*- coding: utf-8 -*-
# subtitle generator for fvp

import re, struct, sys

time_re = re.compile(r"(\d\d):(\d\d):(\d\d),(\d\d\d)")
def time_text_to_ms(text):
  match = time_re.match(text)
  t = int(match.group(1))
  t = t * 60 + int(match.group(2))
  t = t * 60 + int(match.group(3))
  t = t * 1000 + int(match.group(4))
  return t

def load_b(n):
  return b"\x0c" + struct.pack("<b", n)

def load_h(n):
  return b"\x0b" + struct.pack("<h", n)

def load_l(n):
  return b"\x0a" + struct.pack("<l", n)

def load_x(x):
  x = x.encode("gbk") + b"\0"
  return b"\x0e" + struct.pack("<b", len(x)) + x

def func(n):
  return b"\x03" + struct.pack("<h", n)

def main():
  srt = open(sys.argv[1], "r", encoding="utf-8-sig")
  
  load_prim = load_h(300)     # lh [prim]
  load_text = load_b(14)      # lh [text]
  code = load_prim            #
  code += load_text           #
  code += load_h(160)         #
  code += load_h(570)         #
  code += func(0x56)          # func [PrimSetText]
  code += load_text           #
  code += load_h(960)         #
  code += load_h(60)          #
  code += func(0x6e)          # func [TextBuff]
  code += load_text           #
  code += func(0x73)          # func [TextFontGet]
  code += b"\x14"             # lt
  code += b"\x08"             # lc0
  code += func(0x71)          # func [TextFont]
  code += load_text           #
  code += load_h(10)          #
  code += load_h(224)         #
  code += load_h(100)         #
  code += func(0x70)          # func [TextColor]
  code += load_text           #
  code += load_b(48)          #
  code += load_b(16)          #
  code += func(0x7e)          # func [TextSize]
  code += load_text           #
  code += load_b(8)           #
  code += load_b(8)           #
  code += func(0x78)          # func [TextOutSize]
  code += load_text           #
  code += load_b(1)           #
  code += func(0x81)          # func [TextSpeed]
  code += load_text           #
  code += load_b(5)           #
  code += func(0x7d)          # func [TextShadowDist]
  code += load_text           #
  code += load_b(0)           #
  code += load_b(2)           #
  code += load_b(2)           #
  code += func(0x77)          # func [TextFunction]
  code += load_text           #
  code += load_b(-5)          #
  code += load_b(-5)          #
  code += load_b(6)          #
  code += load_b(6)          #
  code += load_b(-2)          #
  code += load_b(-2)          #
  code += func(0x76)          # func [TextFormat]
  code += load_text           #
  code += load_b(-1)           #
  code += func(0x7f)          # func [TextSkip]
  code += load_text           #
  code += func(0x6f)          # func [TextClear]
  code += load_prim           #
  code += load_h(20)          #
  code += func(0x49)          # func [PrimGroupIn]
  code += load_text
  code += load_x("测试字幕")
  code += func(0x7b)      # func [TextPrint]
  
  state = 0
  index = 0
  timed = 0
  time0 = 0
  time1 = 0
  text = ""
  for line in srt.readlines():
    if len(line) <= 1:
      continue
    line = line[:-1]
    if state == 0:
      index = int(line)
      state = 1
    elif state == 1:
      next_time0, next_time1 = tuple([time_text_to_ms(t) for t in line.split(" --> ")])
      timed = next_time0 - time1
      time0, time1 = next_time0, next_time1
      state = 2
    elif state == 2:
      text = line
      print(time0, time1, line)
      code += load_l(timed - 20)
      code += func(0x89)      # func [ThreadWait]
      code += load_text
      code += load_x(text)
      code += func(0x7b)      # func [TextPrint]
      code += load_l(time1 - time0 - 20)
      code += func(0x89)      # func [ThreadWait]
      code += load_text       #
      code += func(0x6f)      # func [TextClear]
      state = 0
  
  code += load_prim           #
  code += func(0x50)          # func [PrimSetNull]
  with open("filter_vm.dat", "wb") as dat:
    dat.write(code)
  
  srt.close()

if __name__ == "__main__":
  main()
