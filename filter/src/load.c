#include <windows.h>

EXTERN_C int CDECL StartExecutable();

INT APIENTRY WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nShowCmd
) {
  return StartExecutable();
}
