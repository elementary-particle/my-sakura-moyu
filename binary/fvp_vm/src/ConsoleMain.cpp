#include <cstdio>

#include "VmEnv.h"

int main(int argc, char *argv[]) {
  FILE *scriptFile, *dumpFile;
  if (argc < 3) {
    printf("Too few arguments.\n");
    printf("Usage: %s [hcbFile] [dasDump]\n", argv[0]);
    return 1;
  }
  scriptFile = fopen(argv[1], "rb");
  dumpFile = stdout; // fopen(argv[2], "w");

  if (scriptFile == nullptr) {
    fprintf(stderr, "ERROR: Cannot open '%s'", argv[1]);
    return -1;
  }

  uint32_t scriptLen;
  fseek(scriptFile, 0, SEEK_END);
  scriptLen = ftell(scriptFile);
  rewind(scriptFile);
  uint8_t *buffer = new uint8_t[scriptLen];
  fread(buffer, scriptLen, 1, scriptFile);

  VmEnv *env = new VmEnv(buffer);

  env->getSubs();
  // env->printAsm(dumpFile);
  // env->printFlow(dumpFile);
  env->subMap.get(0x00055659)->printFlow(dumpFile);

  delete env;

  delete[] buffer;

  fclose(dumpFile);
  fclose(scriptFile);
  return 0;
}
