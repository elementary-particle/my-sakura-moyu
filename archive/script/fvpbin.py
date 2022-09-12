# coding: utf-8
import struct, zlib, sys, os
import image

class FvpFile:
  def __init__(this, name_off, off, size):
    this.name_off = name_off
    this.off = off
    this.size = size
    this.name = None
  
  def __str__(this):
    return this.name
    
  def getname(this, names):
    this.name = names[this.name_off:].split(b"\0", 1)[0].decode("shift-jis")

def header(pack):
  files = list()
  file_count, names_size = struct.unpack("<LL", pack.read(8))
  for i in range(file_count):
    file_name_off, file_off, file_size = struct.unpack("<LLL", pack.read(12))
    file = FvpFile(file_name_off, file_off, file_size)
    files.append(file)
  names = pack.read(names_size)
  filed = dict()
  for file in files:
    file.getname(names)
    filed[file.name] = file
  return filed

def unpack(path, file):
  pack = open(path + ".bin", "rb")
  pack.seek(file.off)
  cont = pack.read(file.size)
  
  sign, = struct.unpack("<4s", cont[:4])
  if(sign == b"hzc1"):
    size, pad = struct.unpack("<LL", cont[4:12])
    cont = cont[12:]
    cont = cont[:pad] + zlib.decompress(cont[pad:])
    if(pad + size != len(cont)):
      print("WARN: size mismatch: %s: decompressed: %d, expected: %d" % (file.name, size, len(comp) - pad))
    sign, = struct.unpack("<4s", cont[:4])
  
  print("signature", sign)
  if(cont[:4] == b"NVSG"):
    image.save(path + os.path.sep + file.name + ".png", cont)
  else:
    file = open(path + os.path.sep + file.name, "wb")
    file.write(cont)
    file.close()
  
  pack.close()

def repack(path):
  pack = open(path + ".bin", "wb")
  off = 0
  name_list = sorted(os.listdir(path))
  file_count = len(name_list)
  name_data = b""
  file_name_list = list()
  for name in name_list:
    file_name_list.append(off)
    if (name.endswith(".png")):
      name = os.path.splitext(name)[0]
    name = name.encode("shift-jis") + b"\0"
    name_data += name
    off += len(name)
  off += 8 + file_count * 12
  pack.write(struct.pack("<LL", file_count, len(name_data)))
  pack.write((file_count * 12) * b"\0")
  pack.write(name_data)
  file_info_list = list()
  for name in name_list:
    file_path = os.path.join(path, name)
    if (name.endswith(".png")):
      data = image.load(file_path)
      pad = 0x20
      info = data[:pad]
      data = data[pad:]
      data = b"hzc1" + struct.pack("<LL", len(data), pad) + info + zlib.compress(data)
    else:
      file = open(file_path, "rb")
      data = file.read()
      file.close()
    file_info_list.append((off, len(data)))
    pack.write(data)
    off += len(data)
  pack.seek(8)
  for i in range(file_count):
    file_off, file_size = file_info_list[i]
    pack.write(struct.pack("<LLL", file_name_list[i], file_off, file_size))
  pack.close()

def main_unpack():
  path = input()
  pack = open(path + ".bin", "rb")
  files = header(pack)
  pack.close()
  if(not os.path.exists(path)):
    os.mkdir(path)
  unpack(path, files[input()])
  # mani = open(path + ".txt", "w")
  # for file in files.values():
    # mani.write(file.name + "\n")
    # print(file.name)
    # unpack(path, files[file.name])
  # mani.close()

def main_repack():
  path = input()
  repack(path)

if __name__ == '__main__':
  main_repack()
