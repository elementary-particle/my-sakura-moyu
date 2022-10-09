#include "overlay/i_d3d9.h"

ID3DApp GameApp;
PFUNC_Direct3DCreate9 pDirect3DCreate9;
IDirect3D9 *WINAPI OverlayD3DCreate(UINT SDKVersion) {
  IDirect3D9 *iD3D;
  iD3D = (*pDirect3DCreate9)(SDKVersion);
  if (iD3D) {
    auto *iNewD3D = new IOverlayD3D(iD3D);
    GameApp.AddD3D(iNewD3D);
    return iNewD3D;
  } else {
    return NULL;
  }
}

/** Hooked **/
HRESULT IOverlayD3D::QueryInterface(const IID &riid, void **ppvObj) {
  HRESULT hResult;
  hResult = iD3D->QueryInterface(riid, ppvObj);
  if (riid == IID_IDirect3D9 && SUCCEEDED(hResult)) {
    *ppvObj = this;
  }
  return hResult;
}
ULONG IOverlayD3D::AddRef() { return iD3D->AddRef(); }
ULONG IOverlayD3D::Release() {
  ULONG count;
  count = iD3D->Release();
  if (count == 0) {
    *piPrevD3D = iNextD3D;
    if (iNextD3D) {
      iNextD3D->piPrevD3D = piPrevD3D;
      iNextD3D = NULL;
    }
    delete this;
  }
  return count;
}

HRESULT IOverlayD3D::RegisterSoftwareDevice(void *pInitializeFunction) {
  return iD3D->RegisterSoftwareDevice(pInitializeFunction);
}
UINT IOverlayD3D::GetAdapterCount() { return iD3D->GetAdapterCount(); }
HRESULT IOverlayD3D::GetAdapterIdentifier(UINT Adapter, DWORD Flags,
                                          D3DADAPTER_IDENTIFIER9 *pIdentifier) {
  return iD3D->GetAdapterIdentifier(Adapter, Flags, pIdentifier);
}
UINT IOverlayD3D::GetAdapterModeCount(UINT Adapter, D3DFORMAT Format) {
  return iD3D->GetAdapterModeCount(Adapter, Format);
}
HRESULT IOverlayD3D::EnumAdapterModes(UINT Adapter, D3DFORMAT Format, UINT Mode,
                                      D3DDISPLAYMODE *pMode) {
  return iD3D->EnumAdapterModes(Adapter, Format, Mode, pMode);
}
HRESULT IOverlayD3D::GetAdapterDisplayMode(UINT Adapter,
                                           D3DDISPLAYMODE *pMode) {
  return iD3D->GetAdapterDisplayMode(Adapter, pMode);
}
HRESULT IOverlayD3D::CheckDeviceType(UINT Adapter, D3DDEVTYPE DevType,
                                     D3DFORMAT AdapterFormat,
                                     D3DFORMAT BackBufferFormat,
                                     BOOL bWindowed) {
  return iD3D->CheckDeviceType(Adapter, DevType, AdapterFormat,
                               BackBufferFormat, bWindowed);
}
HRESULT IOverlayD3D::CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType,
                                       D3DFORMAT AdapterFormat, DWORD Usage,
                                       D3DRESOURCETYPE RType,
                                       D3DFORMAT CheckFormat) {
  return iD3D->CheckDeviceFormat(Adapter, DeviceType, AdapterFormat, Usage,
                                 RType, CheckFormat);
}
HRESULT IOverlayD3D::CheckDeviceMultiSampleType(
    UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed,
    D3DMULTISAMPLE_TYPE MultiSampleType, DWORD *pQualityLevels) {
  return iD3D->CheckDeviceMultiSampleType(Adapter, DeviceType, SurfaceFormat,
                                          Windowed, MultiSampleType,
                                          pQualityLevels);
}
HRESULT IOverlayD3D::CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType,
                                            D3DFORMAT AdapterFormat,
                                            D3DFORMAT RenderTargetFormat,
                                            D3DFORMAT DepthStencilFormat) {
  return iD3D->CheckDepthStencilMatch(Adapter, DeviceType, AdapterFormat,
                                      RenderTargetFormat, DepthStencilFormat);
}
HRESULT IOverlayD3D::CheckDeviceFormatConversion(UINT Adapter,
                                                 D3DDEVTYPE DeviceType,
                                                 D3DFORMAT SourceFormat,
                                                 D3DFORMAT TargetFormat) {
  return iD3D->CheckDeviceFormatConversion(Adapter, DeviceType, SourceFormat,
                                           TargetFormat);
}
HRESULT IOverlayD3D::GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType,
                                   D3DCAPS9 *pCaps) {
  return iD3D->GetDeviceCaps(Adapter, DeviceType, pCaps);
}
HMONITOR IOverlayD3D::GetAdapterMonitor(UINT Adapter) {
  return iD3D->GetAdapterMonitor(Adapter);
}
void IOverlayD3D::AddDevice(IOverlayDevice *iDevice) {
  iDevice->piPrevDevice = &pDeviceList;
  iDevice->iNextDevice = pDeviceList;
  pDeviceList = iDevice;
}
/** Hooked **/
HRESULT IOverlayD3D::CreateDevice(
    UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS *pPresentationParameters,
    IDirect3DDevice9 **ppReturnedDeviceInterface) {
  HRESULT hResult;
  hResult = iD3D->CreateDevice(
      Adapter, DeviceType, hFocusWindow,
      D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
      pPresentationParameters, ppReturnedDeviceInterface);
  if (SUCCEEDED(hResult)) {
    auto *iNewDevice = new IOverlayDevice(
        this, *ppReturnedDeviceInterface, pPresentationParameters, TRUE);
    AddDevice(iNewDevice);
    *ppReturnedDeviceInterface = iNewDevice;
  } else {
    hResult =
        iD3D->CreateDevice(Adapter, DeviceType, hFocusWindow, BehaviorFlags,
                           pPresentationParameters, ppReturnedDeviceInterface);
    if (SUCCEEDED(hResult)) {
      auto *iNewDevice = new IOverlayDevice(
          this, *ppReturnedDeviceInterface, pPresentationParameters, TRUE);
      AddDevice(iNewDevice);
      *ppReturnedDeviceInterface = iNewDevice;
    }
  }
  return hResult;
}
