# coding: utf-8

import os, struct, numpy
import matplotlib.pyplot as plt
import matplotlib.image as mpimg

def decode(raw):
  if (not (raw[0:4] == b"NVSG")):
    print("image: no fvp nvsg signature.")
    return None
  cat, = struct.unpack("<H", raw[6:8])
  w, h = struct.unpack("<HH", raw[8:12])
  if (cat == 0): # rgb
    print("dimensions: (%d x %d x 3B)" % (h, w))
    img = numpy.ndarray((h, w, 3), numpy.dtype("<u1"), raw[32:])
    img_swap = img.copy()
    img_swap[:, :, 0], img_swap[:, :, 2] = img[:, :, 2], img[:, :, 0]
    return img_swap
  elif (cat == 1 or cat == 2): # rgba
    print("dimensions: (%d x %d x 4B)" % (h, w))
    img = numpy.ndarray((h, w, 4), numpy.dtype("<u1"), raw[32:])
    img_swap = img.copy()
    img_swap[:, :, 0], img_swap[:, :, 2] = img[:, :, 2], img[:, :, 0]
    return img_swap
  elif (cat == 3): # grayscale
    print("dimensions: (%d x %d x 1B)" % (h, w))
    img = numpy.ndarray((h, w), numpy.dtype("<u1"), raw[32:])
    img = numpy.expand_dims(img, 2).repeat(3, axis=2)
    return img
  elif (cat == 4): # binary mask
    print("dimensions: (%d x %d x 1b)" % (h, w))
    img = numpy.ndarray((h, w), numpy.dtype("<u1"), raw[32:]) * 0xff
    img = numpy.expand_dims(img, 2).repeat(3, axis=2)
    return img
  else:
    print("image: unrecognized format %d." % cat)
    return None

def encode(img, meta=20 * b"\0"):
  if (img.shape[2] == 3):
    cat = 0
  elif (img.shape[2] == 4):
    cat = 1
  else:
    raise NotImplementedError("unsupported image format")
  raw = b"NVSG\0\0" + struct.pack("<HHH", cat, img.shape[1], img.shape[0]) + meta
  # we assume all images are encoded into rgba formats
  img_swap = img.copy()
  img_swap[:, :, 0], img_swap[:, :, 2] = img[:, :, 2], img[:, :, 0]
  raw += img_swap.tobytes()
  return raw

def plot(raw):
  img = decode(raw)
  if (img is None):
    return None
  plt.imshow(img)
  plt.show()

def save(path, raw):
  img = decode(raw)
  if (img is None):
    return None
  mpimg.imsave(path, img)

def load(path):
  print(path)
  img = mpimg.imread(path)
  img = (img * 0xff).astype(numpy.dtype("<u1"))
  # depth = img[:, :, 0] + img[:, :, 1] + img[:, :, 2] + img[:, :, 3]
  # img[depth == 0] = 0xff
  name = os.path.splitext(os.path.split(path)[-1])[0]
  sample_name = os.path.join("sample", name)
  if (name.startswith("menu_") and name.endswith("_mes")):
    print(640 - img.shape[1] // 2, 309)
    raw = encode(img, struct.pack("<HH", 640 - img.shape[1] // 2, 309) + 16 * b"\0")
  elif (os.path.exists(sample_name)):
    sample = open(sample_name, "rb")
    meta = sample.read(32)[12:32]
    print(struct.unpack("<HH", meta[:4]))
    sample.close()
    raw = encode(img, meta)
  else:
    raw = encode(img)
  return raw
