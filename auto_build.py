# -*- coding: utf-8 -*-

import json, sched, struct, sys, time, urllib.request

class RequestFailure(IOError):
  def __init__(self, msg):
    super(IOError, self).__init__(msg)

def request(url):
  result = json.loads(urllib.request.urlopen(url).read().decode("utf-8"))
  if result["code"] != 0:
    raise RequestFailure(f"cannot access '{url}'.")
  else:
    return result["object"]

def build_patch():
  tc_url = "http://127.0.0.1:9092/api/sakura_moyu"
  span = request(tc_url + "/range")
  start, end = span["min"], span["max"]
  data = request(tc_url + f"/batch/{start}_{end}")
  target = open("patch.dat", "wb")
  target.write(struct.pack("<L", len(data)))
  data.sort(key=lambda unit: unit["id"])
  for unit in data:
    target.write(struct.pack("<L", int(unit["location"], 16)))
    t = unit["target"]
    t = t.encode("gbk", errors="ignore") + b"\0"
    target.write(struct.pack("<H", len(t)))
    target.write(t)
  target.close()

def main():
  while(1):
    try:
      build_patch()
    except:
      pass
    time.sleep(60 * 60)

if __name__ == "__main__":
  main()
