#include "overlay/i_d3d9.h"

union COLOR4 {
  BYTE F[4];
  DWORD S;
};

static inline BYTE Div8(WORD wColor) { return (BYTE)(wColor / 255); }

HRESULT IOverlayDevice::SoftBlend(FRAMEDATA *pFrame) {
  HRESULT hResult;
  ASS_Image *pImage, *pThis;
  IDirect3DTexture9 *iTexture;
  hResult = S_OK;
  EnterCriticalSection(&SubSection);
  if (pSub) {
    pImage = pSub->Render(GetTickCount() - llTickStart + pFrame->llInterval);
    LeaveCriticalSection(&SubSection);
    if (pImage) {
      hResult =
          iDevice->CreateTexture(uViewWidth, uViewHeight, 1, 0, D3DFMT_A8R8G8B8,
                                 D3DPOOL_MANAGED, &iTexture, NULL);
      if (SUCCEEDED(hResult)) {
        D3DLOCKED_RECT rcLock;
        hResult = iTexture->LockRect(0, &rcLock, NULL, 0);
        if (SUCCEEDED(hResult)) {
          int X, Y;

          for (pThis = pImage; pThis; pThis = pThis->next) {
            COLOR4 *pSrc;
            pSrc = (COLOR4 *)&pThis->color;

            for (Y = 0; Y < pThis->h; Y++) {
              for (X = 0; X < pThis->w; X++) {
                WORD A;
                COLOR4 *pDst;
                A = pThis->bitmap[Y * pThis->stride + X];
                A = Div8((0xff - pSrc->F[0]) * A);
                pDst = (COLOR4 *)((PBYTE)rcLock.pBits +
                                  (pThis->dst_y + Y) * rcLock.Pitch) +
                       (pThis->dst_x + X);

                pDst->F[0] = Div8(A * pSrc->F[1] + (0xff - A) * pDst->F[0]);
                pDst->F[1] = Div8(A * pSrc->F[2] + (0xff - A) * pDst->F[1]);
                pDst->F[2] = Div8(A * pSrc->F[3] + (0xff - A) * pDst->F[2]);
                pDst->F[3] = 0xff - Div8((0xff - A) * (0xff - pDst->F[3]));
              }
            }
          }
          iTexture->UnlockRect(0);
          pFrame->TextureList.push(iTexture);
          hResult = iDevice->CreateVertexBuffer(
              6 * sizeof(VERTEX), D3DUSAGE_WRITEONLY, dwOverlayFVF,
              D3DPOOL_MANAGED, &pFrame->iVertexBuffer, NULL);
          if (SUCCEEDED(hResult)) {
            VERTEX *pVertex;
            hResult = pFrame->iVertexBuffer->Lock(0, 6 * sizeof(VERTEX),
                                                  &(PVOID &)pVertex, 0);
            if (SUCCEEDED(hResult)) {
              int X0, Y0, X1, Y1;
              X0 = Y0 = 0;
              X1 = uViewWidth;
              Y1 = uViewHeight;
              pVertex[0] = VERTEX(X0, Y0, 0.0f, 0.0f, 0.0f);
              pVertex[1] = VERTEX(X1, Y0, 0.0f, 1.0f, 0.0f);
              pVertex[2] = VERTEX(X1, Y1, 0.0f, 1.0f, 1.0f);
              pVertex[3] = VERTEX(X1, Y1, 0.0f, 1.0f, 1.0f);
              pVertex[4] = VERTEX(X0, Y1, 0.0f, 0.0f, 1.0f);
              pVertex[5] = VERTEX(X0, Y0, 0.0f, 0.0f, 0.0f);
              pFrame->iVertexBuffer->Unlock();
            }
          }
        }
      }
      Subtitle::DeleteImage(pImage);
    }
  } else {
    LeaveCriticalSection(&SubSection);
  }
  return hResult;
}

HRESULT IOverlayDevice::RasterFrame(FRAMEDATA *pFrame) {
  HRESULT hResult;
  ASS_Image *pImage, *pThis;
  IDirect3DTexture9 *iTexture;
  EnterCriticalSection(&SubSection);
  hResult = S_OK;
  if (pSub) {
    pImage = pSub->Render(GetTickCount() - llTickStart + pFrame->llInterval);
    LeaveCriticalSection(&SubSection);
    if (pImage) {
      for (pThis = pImage; pThis; pThis = pThis->next) {
        hResult = iDevice->CreateTexture(pThis->w, pThis->h, 1, 0, D3DFMT_A8,
                                         D3DPOOL_MANAGED, &iTexture, NULL);
        if (FAILED(hResult)) {
          break;
        }
        pFrame->TextureList.push(iTexture);
      }
      if (SUCCEEDED(hResult)) {
        for (pThis = pImage; pThis; pThis = pThis->next) {
          DWORD Color;
          D3DLOCKED_RECT rcLock;
          int Y;
          Color = pThis->color;
          Color = (Color << 24) | (Color >> 8);  // RGBA -> ARGB
          pFrame->ColorList.push(Color);
          iTexture = pFrame->TextureList.front();
          hResult = iTexture->LockRect(0, &rcLock, NULL, 0);
          if (SUCCEEDED(hResult)) {
            for (Y = 0; Y < pThis->h; Y++) {
              memcpy((PBYTE)rcLock.pBits + Y * rcLock.Pitch,
                     pThis->bitmap + Y * pThis->stride, pThis->w);
            }
            iTexture->UnlockRect(0);
          }
          pFrame->TextureList.pop();
          pFrame->TextureList.push(iTexture);
        }
        UINT Length;
        Length = pFrame->TextureList.size() * 6 * sizeof(VERTEX);
        hResult = iDevice->CreateVertexBuffer(Length, D3DUSAGE_WRITEONLY, 0,
                                              D3DPOOL_MANAGED,
                                              &pFrame->iVertexBuffer, NULL);
        if (SUCCEEDED(hResult)) {
          VERTEX *pVertexBuffer;
          hResult = pFrame->iVertexBuffer->Lock(0, Length,
                                                &(PVOID &)pVertexBuffer, 0);
          if (SUCCEEDED(hResult)) {
            for (pThis = pImage; pThis; pThis = pThis->next) {
              int X0, Y0, X1, Y1;
              X0 = pThis->dst_x;
              Y0 = pThis->dst_y;
              X1 = X0 + pThis->w;
              Y1 = Y0 + pThis->h;
              pVertexBuffer[0] = VERTEX(X0, Y0, 0.0f, 0.0f, 0.0f);
              pVertexBuffer[1] = VERTEX(X1, Y0, 0.0f, 1.0f, 0.0f);
              pVertexBuffer[2] = VERTEX(X1, Y1, 0.0f, 1.0f, 1.0f);
              pVertexBuffer[3] = VERTEX(X1, Y1, 0.0f, 1.0f, 1.0f);
              pVertexBuffer[4] = VERTEX(X0, Y1, 0.0f, 0.0f, 1.0f);
              pVertexBuffer[5] = VERTEX(X0, Y0, 0.0f, 0.0f, 0.0f);
              pVertexBuffer += 6;
            }
            pFrame->iVertexBuffer->Unlock();
          }
        }
      }
      Subtitle::DeleteImage(pImage);
    }
  } else {
    LeaveCriticalSection(&SubSection);
  }
  return hResult;
}

HRESULT IOverlayDevice::RenderFrame(FRAMEDATA *pFrame) {
  HRESULT hResult;
  IDirect3DSurface9 *iBackBuffer;
  IDirect3DStateBlock9 *iState;
  IDirect3DTexture9 *iTexture;
  std::queue<IDirect3DTexture9 *> TextureList;
  UINT StartVertex;
  if (pFrame->iVertexBuffer) {
    hResult = iDevice->CreateStateBlock(D3DSBT_ALL, &iState);
    if (SUCCEEDED(hResult)) {
      hResult =
          iDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &iBackBuffer);
      if (SUCCEEDED(hResult)) {
        iDevice->SetRenderTarget(0, iBackBuffer);
        iState->Capture();
        iDevice->BeginScene();
        iOverlayState->Apply();
        iDevice->SetStreamSource(0, pFrame->iVertexBuffer, 0, sizeof(VERTEX));
        StartVertex = 0;
        if (bUseSoftBlend) {
          iTexture = pFrame->TextureList.front();
          iDevice->SetTexture(0, iTexture);
          iDevice->SetFVF(dwOverlayFVF);
          iDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);
          TextureList.push(iTexture);
        } else {
          while (!pFrame->TextureList.empty()) {
            DWORD Color;
            iTexture = pFrame->TextureList.front();
            Color = pFrame->ColorList.front();
            pFrame->ColorList.pop();
            pFrame->ColorList.push(Color);

            iDevice->SetTexture(0, iTexture);
            iDevice->SetTextureStageState(0, D3DTSS_CONSTANT, Color);
            iDevice->SetFVF(dwOverlayFVF);
            iDevice->DrawPrimitive(D3DPT_TRIANGLELIST, StartVertex, 2);

            pFrame->TextureList.pop();
            TextureList.push(iTexture);
            StartVertex += 6;
          }
        }
        iDevice->EndScene();
        iState->Apply();
        iBackBuffer->Release();
      }
      iState->Release();
    }
  } else {
    hResult = S_OK;
  }
  pFrame->TextureList = std::move(TextureList);
  return hResult;
}

void IOverlayDevice::ReleaseFrame(FRAMEDATA *pFrame) {
  IDirect3DTexture9 *iTexture;
  SafeRelease(&pFrame->iVertexBuffer);
  while (!pFrame->TextureList.empty()) {
    iTexture = pFrame->TextureList.front();
    pFrame->TextureList.pop();
    iTexture->Release();
  }
  while (!pFrame->ColorList.empty()) {
    pFrame->ColorList.pop();
  }
}

DWORD WINAPI DrawProc(LPVOID lpData) {
  IOverlayDevice::FRAMEDATA *pFrame;
  IOverlayDevice *iDevice;
  pFrame = (IOverlayDevice::FRAMEDATA *)lpData;
  iDevice = pFrame->iDevice;
  HANDLE hWait[2] = {pFrame->hRenderEvent, iDevice->hFinishEvent};
  DWORD dwWaited = WAIT_OBJECT_0;
  HRESULT (IOverlayDevice::*pRenderFunc)(IOverlayDevice::FRAMEDATA *);
  pRenderFunc = iDevice->bUseSoftBlend ? &IOverlayDevice::SoftBlend
                                       : &IOverlayDevice::RasterFrame;
  LONGLONG llPrevTick, llNextTick;
  while (dwWaited == WAIT_OBJECT_0) {
    llPrevTick = GetTickCount();
    (iDevice->*pRenderFunc)(pFrame);
    SetEvent(pFrame->hRasterEvent);
    dwWaited = WaitForMultipleObjects(2, hWait, FALSE, INFINITE);
    llNextTick = GetTickCount();
    pFrame->llInterval = llNextTick - llPrevTick;
    iDevice->ReleaseFrame(pFrame);
  }
  ResetEvent(pFrame->hRasterEvent);
  return 0;
}
void IOverlayDevice::AttachSub(Subtitle *pSub_) {
  SIZE_T I;
  EnterCriticalSection(&SubSection);
  delete pSub;
  pSub = pSub_;
  pSub->Prepare(uViewWidth, uViewHeight);
  llTickStart = GetTickCount();
  LeaveCriticalSection(&SubSection);
  EnterCriticalSection(&RenderSection);
  bRunRender = TRUE;
  RenderIndex = 0;
  LeaveCriticalSection(&RenderSection);
  for (I = 0; I < IOverlayDevice::CHAIN_SIZE; I++) {
    Frame[I].iDevice = this;
    Frame[I].hThread = CreateThread(NULL, 1 << 20, DrawProc, (PVOID)&Frame[I],
                                    CREATE_SUSPENDED, NULL);
    SetThreadPriority(Frame[I].hThread, THREAD_PRIORITY_ABOVE_NORMAL);
    ResumeThread(Frame[I].hThread);
  }
}

void IOverlayDevice::DetachSub() {
  SIZE_T I;
  HANDLE hWait[CHAIN_SIZE];
  for (I = 0; I < CHAIN_SIZE; I++) {
    hWait[I] = Frame[I].hThread;
  }
  EnterCriticalSection(&SubSection);
  llTickStart = 0;
  delete pSub;
  pSub = NULL;
  LeaveCriticalSection(&SubSection);
  EnterCriticalSection(&RenderSection);
  SetEvent(hFinishEvent);
  bRunRender = FALSE;
  WaitForMultipleObjects(CHAIN_SIZE, hWait, TRUE, INFINITE);
  ResetEvent(hFinishEvent);
  for (I = 0; I < IOverlayDevice::CHAIN_SIZE; I++) {
    CloseHandle(Frame[I].hThread);
    Frame[I].hThread = NULL;
  }
  LeaveCriticalSection(&RenderSection);
}

DWORD WINAPI FrameCounter(LPVOID lpDevice) {
  IOverlayDevice *iDevice;
  iDevice = (IOverlayDevice *)lpDevice;
  UINT bRun;
  bRun = InterlockedExchange(&iDevice->bRunCount, iDevice->bRunCount);
  while (bRun) {
    LONG uFrameRate;
    InterlockedExchange(&iDevice->uCount, 0);
    Sleep(1000);
    uFrameRate = InterlockedExchange(&iDevice->uFrameRate, iDevice->uCount);
    // DebugLog(0, "Frame Rate: %ld\n", uFrameRate);
    bRun = InterlockedExchange(&iDevice->bRunCount, iDevice->bRunCount);
  }
  return 0;
}

IOverlayDevice::IOverlayDevice(IOverlayD3D *_iOverlayD3D,
                               IDirect3DDevice9 *_iDevice,
                               D3DPRESENT_PARAMETERS *pPresentationParameters,
                               BOOL bSoft)
    : iOverlayD3D(_iOverlayD3D), iDevice(_iDevice), bUseSoftBlend(bSoft) {
  hFinishEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
  RenderIndex = 0;
  uFrameRate = uCount = 0;
  bRunCount = TRUE;
  bRunRender = FALSE;
  hFrameCountThread =
      CreateThread(NULL, 0x1000, FrameCounter, (PVOID)this, 0, NULL);
  InitializeCriticalSection(&SubSection);
  InitializeCriticalSection(&RenderSection);
  pSub = NULL;
  iOverlayState = NULL;
  uViewWidth = uViewHeight = 0;
  CreateDeviceResources(pPresentationParameters);
}

IOverlayDevice::~IOverlayDevice() {
  ReleaseDeviceResources();
  DeleteCriticalSection(&RenderSection);
  DeleteCriticalSection(&SubSection);
  InterlockedExchange(&bRunCount, FALSE);
  CloseHandle(hFrameCountThread);
  CloseHandle(hFinishEvent);
}

HRESULT IOverlayDevice::CreateDeviceResources(
    D3DPRESENT_PARAMETERS *pPresentationParameters) {
  HRESULT hResult;
  IDirect3DStateBlock9 *iCurrentState;
  iOverlayState = NULL;
  hResult = iDevice->CreateStateBlock(D3DSBT_ALL, &iCurrentState);
  if (SUCCEEDED(hResult)) {
    hResult = iCurrentState->Capture();
    if (SUCCEEDED(hResult)) {
      hResult = iDevice->CreateStateBlock(D3DSBT_ALL, &iOverlayState);
      if (SUCCEEDED(hResult)) {
        iDevice->SetVertexShader(NULL);
        iDevice->SetPixelShader(NULL);
        iDevice->SetFVF(dwOverlayFVF);
        iDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
        iDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
        iDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
        iDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        iDevice->SetRenderState(D3DRS_CLIPPING, TRUE);
        iDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, FALSE);
        iDevice->SetRenderState(D3DRS_VERTEXBLEND, FALSE);
        iDevice->SetRenderState(D3DRS_WRAP0, FALSE);

        iDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
        if (bUseSoftBlend) {
          iDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
          iDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        } else {
          iDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
          iDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        }

        iDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
        iDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);

        iDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        iDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        iDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
        iDevice->SetRenderState(D3DRS_COLORVERTEX, FALSE);

        if (bUseSoftBlend) {
          iDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
          iDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
          iDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
          iDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
        } else {
          iDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
          iDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CONSTANT);
          iDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
          iDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
          iDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2,
                                        D3DTA_CONSTANT | D3DTA_COMPLEMENT);
        }

        // iDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        // iDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        // iDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

        iDevice->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
        iDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS,
                                      D3DTTFF_DISABLE);

        iDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

        hResult = iOverlayState->Capture();
        if (FAILED(hResult)) {
          iOverlayState->Release();
          iOverlayState = NULL;
        }
        iCurrentState->Apply();
      }
    }
    iCurrentState->Release();
  }
  D3DVIEWPORT9 Viewport;
  iDevice->GetViewport(&Viewport);
  uViewWidth = Viewport.Width;
  uViewHeight = Viewport.Height;
  if (pSub) {
    pSub->SetFrameSize(uViewWidth, uViewHeight);
  }
  return hResult;
}
HRESULT IOverlayDevice::ReleaseDeviceResources() {
  SafeRelease(&iOverlayState);
  return S_OK;
}
/** Hooked **/
HRESULT IOverlayDevice::Reset(D3DPRESENT_PARAMETERS *pPresentationParameters) {
  /* OnReset */
  HRESULT hResult;
  SIZE_T I;
  HANDLE hWait[CHAIN_SIZE];
  for (I = 0; I < CHAIN_SIZE; I++) {
    hWait[I] = Frame[I].hThread;
  }
  EnterCriticalSection(&RenderSection);
  SetEvent(hFinishEvent);
  WaitForMultipleObjects(CHAIN_SIZE, hWait, TRUE, INFINITE);
  ResetEvent(hFinishEvent);
  for (I = 0; I < IOverlayDevice::CHAIN_SIZE; I++) {
    CloseHandle(Frame[I].hThread);
    Frame[I].hThread = NULL;
  }
  ReleaseDeviceResources();
  hResult = iDevice->Reset(pPresentationParameters);
  CreateDeviceResources(pPresentationParameters);

  for (I = 0; I < IOverlayDevice::CHAIN_SIZE; I++) {
    Frame[I].iDevice = this;
    Frame[I].hThread = CreateThread(NULL, 1 << 20, DrawProc, (PVOID)&Frame[I],
                                    CREATE_SUSPENDED, NULL);
    SetThreadPriority(Frame[I].hThread, THREAD_PRIORITY_ABOVE_NORMAL);
    ResumeThread(Frame[I].hThread);
  }
  LeaveCriticalSection(&RenderSection);
  return hResult;
}
/** Hooked **/
HRESULT IOverlayDevice::Present(const RECT *pSourceRect, const RECT *pDestRect,
                                HWND hDestWindowOverride,
                                const RGNDATA *pDirtyRegion) {
  HRESULT hResult;
  InterlockedIncrement(&uCount);
  EnterCriticalSection(&RenderSection);
  if (bRunRender) {
    FRAMEDATA *pFrame = &Frame[RenderIndex >> 1];
    if (RenderIndex & 1) {
      RenderFrame(pFrame);
      SetEvent(pFrame->hRenderEvent);
    } else {
      WaitForSingleObject(pFrame->hRasterEvent, INFINITE);
      RenderFrame(pFrame);
    }
    RenderIndex = (RenderIndex + 1) & ((CHAIN_SIZE << 1) - 1);
  }
  LeaveCriticalSection(&RenderSection);
  hResult = iDevice->Present(pSourceRect, pDestRect, hDestWindowOverride,
                             pDirtyRegion);
  return hResult;
}

/** Hooked **/
HRESULT IOverlayDevice::QueryInterface(const IID &riid, void **ppvObj) {
  HRESULT hResult;
  hResult = iDevice->QueryInterface(riid, ppvObj);
  if (riid == IID_IDirect3DDevice9 && SUCCEEDED(hResult)) {
    *ppvObj = this;
  }
  return hResult;
}
ULONG IOverlayDevice::AddRef() { return iDevice->AddRef(); }
ULONG IOverlayDevice::Release() {
  ULONG count;
  count = iDevice->Release();
  if (count == 0) {
    *piPrevDevice = iNextDevice;
    if (iNextDevice) {
      iNextDevice->piPrevDevice = piPrevDevice;
      iNextDevice = NULL;
    }
    delete this;
  }
  return count;
}

HRESULT IOverlayDevice::TestCooperativeLevel() {
  return iDevice->TestCooperativeLevel();
}
UINT IOverlayDevice::GetAvailableTextureMem() {
  return iDevice->GetAvailableTextureMem();
}
HRESULT IOverlayDevice::EvictManagedResources() {
  return iDevice->EvictManagedResources();
}
/** Hooked **/
HRESULT IOverlayDevice::GetDirect3D(IDirect3D9 **ppD3D9) {
  HRESULT hResult;
  hResult = iDevice->GetDirect3D(ppD3D9);
  if (SUCCEEDED(hResult)) {
    // (*ppD3D9 == iOverlayD3D->GetDirect3D());
    *ppD3D9 = iOverlayD3D;
  }
  return hResult;
}
HRESULT IOverlayDevice::GetDeviceCaps(D3DCAPS9 *pCaps) {
  return iDevice->GetDeviceCaps(pCaps);
}
HRESULT IOverlayDevice::GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE *pMode) {
  return iDevice->GetDisplayMode(iSwapChain, pMode);
}
HRESULT IOverlayDevice::GetCreationParameters(
    D3DDEVICE_CREATION_PARAMETERS *pParameters) {
  return iDevice->GetCreationParameters(pParameters);
}
HRESULT IOverlayDevice::SetCursorProperties(UINT XHotSpot, UINT YHotSpot,
                                            IDirect3DSurface9 *pCursorBitmap) {
  return iDevice->SetCursorProperties(XHotSpot, YHotSpot, pCursorBitmap);
}
void IOverlayDevice::SetCursorPosition(int X, int Y, DWORD Flags) {
  iDevice->SetCursorPosition(X, Y, Flags);
}
BOOL IOverlayDevice::ShowCursor(BOOL bShow) {
  return iDevice->ShowCursor(bShow);
}
HRESULT IOverlayDevice::CreateAdditionalSwapChain(
    D3DPRESENT_PARAMETERS *pPresentationParameters,
    IDirect3DSwapChain9 **pSwapChain) {
  return iDevice->CreateAdditionalSwapChain(pPresentationParameters,
                                            pSwapChain);
}
HRESULT IOverlayDevice::GetSwapChain(UINT iSwapChain,
                                     IDirect3DSwapChain9 **pSwapChain) {
  return iDevice->GetSwapChain(iSwapChain, pSwapChain);
}
UINT IOverlayDevice::GetNumberOfSwapChains() {
  return iDevice->GetNumberOfSwapChains();
}
HRESULT IOverlayDevice::GetBackBuffer(UINT iSwapChain, UINT iBackBuffer,
                                      D3DBACKBUFFER_TYPE Type,
                                      IDirect3DSurface9 **ppBackBuffer) {
  return iDevice->GetBackBuffer(iSwapChain, iBackBuffer, Type, ppBackBuffer);
}
HRESULT IOverlayDevice::GetRasterStatus(UINT iSwapChain,
                                        D3DRASTER_STATUS *pRasterStatus) {
  return iDevice->GetRasterStatus(iSwapChain, pRasterStatus);
}
HRESULT IOverlayDevice::SetDialogBoxMode(BOOL bEnableDialogs) {
  return iDevice->SetDialogBoxMode(bEnableDialogs);
}
void IOverlayDevice::SetGammaRamp(UINT iSwapChain, DWORD Flags,
                                  const D3DGAMMARAMP *pRamp) {
  iDevice->SetGammaRamp(iSwapChain, Flags, pRamp);
}
void IOverlayDevice::GetGammaRamp(UINT iSwapChain, D3DGAMMARAMP *pRamp) {
  iDevice->GetGammaRamp(iSwapChain, pRamp);
}
HRESULT IOverlayDevice::CreateTexture(UINT Width, UINT Height, UINT Levels,
                                      DWORD Usage, D3DFORMAT Format,
                                      D3DPOOL Pool,
                                      IDirect3DTexture9 **ppTexture,
                                      HANDLE *pSharedHandle) {
  return iDevice->CreateTexture(Width, Height, Levels, Usage, Format, Pool,
                                ppTexture, pSharedHandle);
}
HRESULT IOverlayDevice::CreateVolumeTexture(
    UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage,
    D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9 **ppVolumeTexture,
    HANDLE *pSharedHandle) {
  return iDevice->CreateVolumeTexture(Width, Height, Depth, Levels, Usage,
                                      Format, Pool, ppVolumeTexture,
                                      pSharedHandle);
}
HRESULT IOverlayDevice::CreateCubeTexture(UINT EdgeLength, UINT Levels,
                                          DWORD Usage, D3DFORMAT Format,
                                          D3DPOOL Pool,
                                          IDirect3DCubeTexture9 **ppCubeTexture,
                                          HANDLE *pSharedHandle) {
  return iDevice->CreateCubeTexture(EdgeLength, Levels, Usage, Format, Pool,
                                    ppCubeTexture, pSharedHandle);
}
HRESULT IOverlayDevice::CreateVertexBuffer(
    UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool,
    IDirect3DVertexBuffer9 **ppVertexBuffer, HANDLE *pSharedHandle) {
  return iDevice->CreateVertexBuffer(Length, Usage, FVF, Pool, ppVertexBuffer,
                                     pSharedHandle);
}
HRESULT IOverlayDevice::CreateIndexBuffer(UINT Length, DWORD Usage,
                                          D3DFORMAT Format, D3DPOOL Pool,
                                          IDirect3DIndexBuffer9 **ppIndexBuffer,
                                          HANDLE *pSharedHandle) {
  return iDevice->CreateIndexBuffer(Length, Usage, Format, Pool, ppIndexBuffer,
                                    pSharedHandle);
}
HRESULT IOverlayDevice::CreateRenderTarget(
    UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample,
    DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9 **ppSurface,
    HANDLE *pSharedHandle) {
  return iDevice->CreateRenderTarget(Width, Height, Format, MultiSample,
                                     MultisampleQuality, Lockable, ppSurface,
                                     pSharedHandle);
}
HRESULT IOverlayDevice::CreateDepthStencilSurface(
    UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample,
    DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9 **ppSurface,
    HANDLE *pSharedHandle) {
  return iDevice->CreateDepthStencilSurface(Width, Height, Format, MultiSample,
                                            MultisampleQuality, Discard,
                                            ppSurface, pSharedHandle);
}
HRESULT IOverlayDevice::UpdateSurface(IDirect3DSurface9 *pSourceSurface,
                                      const RECT *pSourceRect,
                                      IDirect3DSurface9 *pDestinationSurface,
                                      const POINT *pDestPoint) {
  return iDevice->UpdateSurface(pSourceSurface, pSourceRect,
                                pDestinationSurface, pDestPoint);
}
HRESULT IOverlayDevice::UpdateTexture(
    IDirect3DBaseTexture9 *pSourceTexture,
    IDirect3DBaseTexture9 *pDestinationTexture) {
  return iDevice->UpdateTexture(pSourceTexture, pDestinationTexture);
}
HRESULT IOverlayDevice::GetRenderTargetData(IDirect3DSurface9 *pRenderTarget,
                                            IDirect3DSurface9 *pDestSurface) {
  return iDevice->GetRenderTargetData(pRenderTarget, pDestSurface);
}
HRESULT IOverlayDevice::GetFrontBufferData(UINT iSwapChain,
                                           IDirect3DSurface9 *pDestSurface) {
  return iDevice->GetFrontBufferData(iSwapChain, pDestSurface);
}
HRESULT IOverlayDevice::StretchRect(IDirect3DSurface9 *pSourceSurface,
                                    const RECT *pSourceRect,
                                    IDirect3DSurface9 *pDestSurface,
                                    const RECT *pDestRect,
                                    D3DTEXTUREFILTERTYPE Filter) {
  return iDevice->StretchRect(pSourceSurface, pSourceRect, pDestSurface,
                              pDestRect, Filter);
}
HRESULT IOverlayDevice::ColorFill(IDirect3DSurface9 *pSurface,
                                  const RECT *pRect, D3DCOLOR color) {
  return iDevice->ColorFill(pSurface, pRect, color);
}
HRESULT IOverlayDevice::CreateOffscreenPlainSurface(
    UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool,
    IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle) {
  return iDevice->CreateOffscreenPlainSurface(Width, Height, Format, Pool,
                                              ppSurface, pSharedHandle);
}
HRESULT IOverlayDevice::SetRenderTarget(DWORD RenderTargetIndex,
                                        IDirect3DSurface9 *pRenderTarget) {
  return iDevice->SetRenderTarget(RenderTargetIndex, pRenderTarget);
}
HRESULT IOverlayDevice::GetRenderTarget(DWORD RenderTargetIndex,
                                        IDirect3DSurface9 **ppRenderTarget) {
  return iDevice->GetRenderTarget(RenderTargetIndex, ppRenderTarget);
}
HRESULT IOverlayDevice::SetDepthStencilSurface(
    IDirect3DSurface9 *pNewZStencil) {
  return iDevice->SetDepthStencilSurface(pNewZStencil);
}
HRESULT IOverlayDevice::GetDepthStencilSurface(
    IDirect3DSurface9 **ppZStencilSurface) {
  return iDevice->GetDepthStencilSurface(ppZStencilSurface);
}
HRESULT IOverlayDevice::BeginScene() { return iDevice->BeginScene(); }
HRESULT IOverlayDevice::EndScene() { return iDevice->EndScene(); }
HRESULT IOverlayDevice::Clear(DWORD Count, const D3DRECT *pRects, DWORD Flags,
                              D3DCOLOR Color, float Z, DWORD Stencil) {
  return iDevice->Clear(Count, pRects, Flags, Color, Z, Stencil);
}
HRESULT IOverlayDevice::SetTransform(D3DTRANSFORMSTATETYPE State,
                                     const D3DMATRIX *pMatrix) {
  return iDevice->SetTransform(State, pMatrix);
}
HRESULT IOverlayDevice::GetTransform(D3DTRANSFORMSTATETYPE State,
                                     D3DMATRIX *pMatrix) {
  return iDevice->GetTransform(State, pMatrix);
}
HRESULT IOverlayDevice::MultiplyTransform(D3DTRANSFORMSTATETYPE State,
                                          const D3DMATRIX *pMatrix) {
  return iDevice->MultiplyTransform(State, pMatrix);
}
HRESULT IOverlayDevice::SetViewport(const D3DVIEWPORT9 *pViewport) {
  return iDevice->SetViewport(pViewport);
}
HRESULT IOverlayDevice::GetViewport(D3DVIEWPORT9 *pViewport) {
  return iDevice->GetViewport(pViewport);
}
HRESULT IOverlayDevice::SetMaterial(const D3DMATERIAL9 *pMaterial) {
  return iDevice->SetMaterial(pMaterial);
}
HRESULT IOverlayDevice::GetMaterial(D3DMATERIAL9 *pMaterial) {
  return iDevice->GetMaterial(pMaterial);
}
HRESULT IOverlayDevice::SetLight(DWORD Index, const D3DLIGHT9 *pLight) {
  return iDevice->SetLight(Index, pLight);
}
HRESULT IOverlayDevice::GetLight(DWORD Index, D3DLIGHT9 *pLight) {
  return iDevice->GetLight(Index, pLight);
}
HRESULT IOverlayDevice::LightEnable(DWORD Index, BOOL Enable) {
  return iDevice->LightEnable(Index, Enable);
}
HRESULT IOverlayDevice::GetLightEnable(DWORD Index, BOOL *pEnable) {
  return iDevice->GetLightEnable(Index, pEnable);
}
HRESULT IOverlayDevice::SetClipPlane(DWORD Index, const float *pPlane) {
  return iDevice->SetClipPlane(Index, pPlane);
}
HRESULT IOverlayDevice::GetClipPlane(DWORD Index, float *pPlane) {
  return iDevice->GetClipPlane(Index, pPlane);
}
HRESULT IOverlayDevice::SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) {
  return iDevice->SetRenderState(State, Value);
}
HRESULT IOverlayDevice::GetRenderState(D3DRENDERSTATETYPE State,
                                       DWORD *pValue) {
  return iDevice->GetRenderState(State, pValue);
}
HRESULT IOverlayDevice::CreateStateBlock(D3DSTATEBLOCKTYPE Type,
                                         IDirect3DStateBlock9 **ppSB) {
  return iDevice->CreateStateBlock(Type, ppSB);
}
HRESULT IOverlayDevice::BeginStateBlock() { return iDevice->BeginStateBlock(); }
HRESULT IOverlayDevice::EndStateBlock(IDirect3DStateBlock9 **ppSB) {
  return iDevice->EndStateBlock(ppSB);
}
HRESULT IOverlayDevice::SetClipStatus(const D3DCLIPSTATUS9 *pClipStatus) {
  return iDevice->SetClipStatus(pClipStatus);
}
HRESULT IOverlayDevice::GetClipStatus(D3DCLIPSTATUS9 *pClipStatus) {
  return iDevice->GetClipStatus(pClipStatus);
}
HRESULT IOverlayDevice::GetTexture(DWORD Stage,
                                   IDirect3DBaseTexture9 **ppTexture) {
  return iDevice->GetTexture(Stage, ppTexture);
}
HRESULT IOverlayDevice::SetTexture(DWORD Stage,
                                   IDirect3DBaseTexture9 *pTexture) {
  return iDevice->SetTexture(Stage, pTexture);
}
HRESULT IOverlayDevice::GetTextureStageState(DWORD Stage,
                                             D3DTEXTURESTAGESTATETYPE Type,
                                             DWORD *pValue) {
  return iDevice->GetTextureStageState(Stage, Type, pValue);
}
HRESULT IOverlayDevice::SetTextureStageState(DWORD Stage,
                                             D3DTEXTURESTAGESTATETYPE Type,
                                             DWORD Value) {
  return iDevice->SetTextureStageState(Stage, Type, Value);
}
HRESULT IOverlayDevice::GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type,
                                        DWORD *pValue) {
  return iDevice->GetSamplerState(Sampler, Type, pValue);
}
HRESULT IOverlayDevice::SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type,
                                        DWORD Value) {
  return iDevice->SetSamplerState(Sampler, Type, Value);
}
HRESULT IOverlayDevice::ValidateDevice(DWORD *pNumPasses) {
  return iDevice->ValidateDevice(pNumPasses);
}
HRESULT IOverlayDevice::SetPaletteEntries(UINT PaletteNumber,
                                          const PALETTEENTRY *pEntries) {
  return iDevice->SetPaletteEntries(PaletteNumber, pEntries);
}
HRESULT IOverlayDevice::GetPaletteEntries(UINT PaletteNumber,
                                          PALETTEENTRY *pEntries) {
  return iDevice->GetPaletteEntries(PaletteNumber, pEntries);
}
HRESULT IOverlayDevice::SetCurrentTexturePalette(UINT PaletteNumber) {
  return iDevice->SetCurrentTexturePalette(PaletteNumber);
}
HRESULT IOverlayDevice::GetCurrentTexturePalette(UINT *pPaletteNumber) {
  return iDevice->GetCurrentTexturePalette(pPaletteNumber);
}
HRESULT IOverlayDevice::SetScissorRect(const RECT *pRect) {
  return iDevice->SetScissorRect(pRect);
}
HRESULT IOverlayDevice::GetScissorRect(RECT *pRect) {
  return iDevice->GetScissorRect(pRect);
}
HRESULT IOverlayDevice::SetSoftwareVertexProcessing(BOOL bSoftware) {
  return iDevice->SetSoftwareVertexProcessing(bSoftware);
}
BOOL IOverlayDevice::GetSoftwareVertexProcessing() {
  return iDevice->GetSoftwareVertexProcessing();
}
HRESULT IOverlayDevice::SetNPatchMode(float nSegments) {
  return iDevice->SetNPatchMode(nSegments);
}
float IOverlayDevice::GetNPatchMode() { return iDevice->GetNPatchMode(); }
HRESULT IOverlayDevice::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType,
                                      UINT StartVertex, UINT PrimitiveCount) {
  return iDevice->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
}
HRESULT IOverlayDevice::DrawIndexedPrimitive(D3DPRIMITIVETYPE PrimitiveType,
                                             INT BaseVertexIndex,
                                             UINT MinVertexIndex,
                                             UINT NumVertices, UINT StartIndex,
                                             UINT PrimitiveCount) {
  return iDevice->DrawIndexedPrimitive(PrimitiveType, BaseVertexIndex,
                                       MinVertexIndex, NumVertices, StartIndex,
                                       PrimitiveCount);
}
HRESULT IOverlayDevice::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,
                                        UINT PrimitiveCount,
                                        const void *pVertexStreamZeroData,
                                        UINT VertexStreamZeroStride) {
  return iDevice->DrawPrimitiveUP(PrimitiveType, PrimitiveCount,
                                  pVertexStreamZeroData,
                                  VertexStreamZeroStride);
}
HRESULT IOverlayDevice::DrawIndexedPrimitiveUP(
    D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices,
    UINT PrimitiveCount, const void *pIndexData, D3DFORMAT IndexDataFormat,
    const void *pVertexStreamZeroData, UINT VertexStreamZeroStride) {
  return iDevice->DrawIndexedPrimitiveUP(
      PrimitiveType, MinVertexIndex, NumVertices, PrimitiveCount, pIndexData,
      IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
}
HRESULT IOverlayDevice::ProcessVertices(
    UINT SrcStartIndex, UINT DestIndex, UINT VertexCount,
    IDirect3DVertexBuffer9 *pDestBuffer,
    IDirect3DVertexDeclaration9 *pVertexDecl, DWORD Flags) {
  return iDevice->ProcessVertices(SrcStartIndex, DestIndex, VertexCount,
                                  pDestBuffer, pVertexDecl, Flags);
}
HRESULT IOverlayDevice::CreateVertexDeclaration(
    const D3DVERTEXELEMENT9 *pVertexElements,
    IDirect3DVertexDeclaration9 **ppDecl) {
  return iDevice->CreateVertexDeclaration(pVertexElements, ppDecl);
}
HRESULT IOverlayDevice::SetVertexDeclaration(
    IDirect3DVertexDeclaration9 *pDecl) {
  return iDevice->SetVertexDeclaration(pDecl);
}
HRESULT IOverlayDevice::GetVertexDeclaration(
    IDirect3DVertexDeclaration9 **ppDecl) {
  return iDevice->GetVertexDeclaration(ppDecl);
}
HRESULT IOverlayDevice::SetFVF(DWORD FVF) { return iDevice->SetFVF(FVF); }
HRESULT IOverlayDevice::GetFVF(DWORD *pFVF) { return iDevice->GetFVF(pFVF); }
HRESULT IOverlayDevice::CreateVertexShader(const DWORD *pFunction,
                                           IDirect3DVertexShader9 **ppShader) {
  return iDevice->CreateVertexShader(pFunction, ppShader);
}
HRESULT IOverlayDevice::SetVertexShader(IDirect3DVertexShader9 *pShader) {
  return iDevice->SetVertexShader(pShader);
}
HRESULT IOverlayDevice::GetVertexShader(IDirect3DVertexShader9 **ppShader) {
  return iDevice->GetVertexShader(ppShader);
}
HRESULT IOverlayDevice::SetVertexShaderConstantF(UINT StartRegister,
                                                 const float *pConstantData,
                                                 UINT Vector4fCount) {
  return iDevice->SetVertexShaderConstantF(StartRegister, pConstantData,
                                           Vector4fCount);
}
HRESULT IOverlayDevice::GetVertexShaderConstantF(UINT StartRegister,
                                                 float *pConstantData,
                                                 UINT Vector4fCount) {
  return iDevice->GetVertexShaderConstantF(StartRegister, pConstantData,
                                           Vector4fCount);
}
HRESULT IOverlayDevice::SetVertexShaderConstantI(UINT StartRegister,
                                                 const int *pConstantData,
                                                 UINT Vector4iCount) {
  return iDevice->SetVertexShaderConstantI(StartRegister, pConstantData,
                                           Vector4iCount);
}
HRESULT IOverlayDevice::GetVertexShaderConstantI(UINT StartRegister,
                                                 int *pConstantData,
                                                 UINT Vector4iCount) {
  return iDevice->GetVertexShaderConstantI(StartRegister, pConstantData,
                                           Vector4iCount);
}
HRESULT IOverlayDevice::SetVertexShaderConstantB(UINT StartRegister,
                                                 const BOOL *pConstantData,
                                                 UINT BoolCount) {
  return iDevice->SetVertexShaderConstantB(StartRegister, pConstantData,
                                           BoolCount);
}
HRESULT IOverlayDevice::GetVertexShaderConstantB(UINT StartRegister,
                                                 BOOL *pConstantData,
                                                 UINT BoolCount) {
  return iDevice->GetVertexShaderConstantB(StartRegister, pConstantData,
                                           BoolCount);
}
HRESULT IOverlayDevice::SetStreamSource(UINT StreamNumber,
                                        IDirect3DVertexBuffer9 *pStreamData,
                                        UINT OffsetInBytes, UINT Stride) {
  return iDevice->SetStreamSource(StreamNumber, pStreamData, OffsetInBytes,
                                  Stride);
}
HRESULT IOverlayDevice::GetStreamSource(UINT StreamNumber,
                                        IDirect3DVertexBuffer9 **ppStreamData,
                                        UINT *pOffsetInBytes, UINT *pStride) {
  return iDevice->GetStreamSource(StreamNumber, ppStreamData, pOffsetInBytes,
                                  pStride);
}
HRESULT IOverlayDevice::SetStreamSourceFreq(UINT StreamNumber, UINT Setting) {
  return iDevice->SetStreamSourceFreq(StreamNumber, Setting);
}
HRESULT IOverlayDevice::GetStreamSourceFreq(UINT StreamNumber, UINT *pSetting) {
  return iDevice->GetStreamSourceFreq(StreamNumber, pSetting);
}
HRESULT IOverlayDevice::SetIndices(IDirect3DIndexBuffer9 *pIndexData) {
  return iDevice->SetIndices(pIndexData);
}
HRESULT IOverlayDevice::GetIndices(IDirect3DIndexBuffer9 **ppIndexData) {
  return iDevice->GetIndices(ppIndexData);
}
HRESULT IOverlayDevice::CreatePixelShader(const DWORD *pFunction,
                                          IDirect3DPixelShader9 **ppShader) {
  return iDevice->CreatePixelShader(pFunction, ppShader);
}
HRESULT IOverlayDevice::SetPixelShader(IDirect3DPixelShader9 *pShader) {
  return iDevice->SetPixelShader(pShader);
}
HRESULT IOverlayDevice::GetPixelShader(IDirect3DPixelShader9 **ppShader) {
  return iDevice->GetPixelShader(ppShader);
}
HRESULT IOverlayDevice::SetPixelShaderConstantF(UINT StartRegister,
                                                const float *pConstantData,
                                                UINT Vector4fCount) {
  return iDevice->SetPixelShaderConstantF(StartRegister, pConstantData,
                                          Vector4fCount);
}
HRESULT IOverlayDevice::GetPixelShaderConstantF(UINT StartRegister,
                                                float *pConstantData,
                                                UINT Vector4fCount) {
  return iDevice->GetPixelShaderConstantF(StartRegister, pConstantData,
                                          Vector4fCount);
}
HRESULT IOverlayDevice::SetPixelShaderConstantI(UINT StartRegister,
                                                const int *pConstantData,
                                                UINT Vector4iCount) {
  return iDevice->SetPixelShaderConstantI(StartRegister, pConstantData,
                                          Vector4iCount);
}
HRESULT IOverlayDevice::GetPixelShaderConstantI(UINT StartRegister,
                                                int *pConstantData,
                                                UINT Vector4iCount) {
  return iDevice->GetPixelShaderConstantI(StartRegister, pConstantData,
                                          Vector4iCount);
}
HRESULT IOverlayDevice::SetPixelShaderConstantB(UINT StartRegister,
                                                const BOOL *pConstantData,
                                                UINT BoolCount) {
  return iDevice->SetPixelShaderConstantB(StartRegister, pConstantData,
                                          BoolCount);
}
HRESULT IOverlayDevice::GetPixelShaderConstantB(UINT StartRegister,
                                                BOOL *pConstantData,
                                                UINT BoolCount) {
  return iDevice->GetPixelShaderConstantB(StartRegister, pConstantData,
                                          BoolCount);
}
HRESULT IOverlayDevice::DrawRectPatch(UINT Handle, const float *pNumSegs,
                                      const D3DRECTPATCH_INFO *pRectPatchInfo) {
  return iDevice->DrawRectPatch(Handle, pNumSegs, pRectPatchInfo);
}
HRESULT IOverlayDevice::DrawTriPatch(UINT Handle, const float *pNumSegs,
                                     const D3DTRIPATCH_INFO *pTriPatchInfo) {
  return iDevice->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo);
}
HRESULT IOverlayDevice::DeletePatch(UINT Handle) {
  return iDevice->DeletePatch(Handle);
}
HRESULT IOverlayDevice::CreateQuery(D3DQUERYTYPE Type,
                                    IDirect3DQuery9 **ppQuery) {
  return iDevice->CreateQuery(Type, ppQuery);
}
