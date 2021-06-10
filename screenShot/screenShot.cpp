#include "screenShot.h"
//#include <gdiplus.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#define RESET_OBJECT(obj) { if(obj) obj->Release(); obj = NULL; }
static BOOL g_bAttach = FALSE;

__declspec(naked) //__declspec(align(16))

void ARGBBlendRow_SSE2(const uint8_t* src_argb0, const uint8_t* src_argb1, uint8_t* dst_argb, int width)
{
	__asm {
		push       esi
		mov        eax, [esp + 4 + 4]   // src_argb0
		mov        esi, [esp + 4 + 8]   // src_argb1
		mov        edx, [esp + 4 + 12]  // dst_argb
		mov        ecx, [esp + 4 + 16]  // width
		pcmpeqb    xmm7, xmm7       // generate constant 1
		psrlw      xmm7, 15
		pcmpeqb    xmm6, xmm6       // generate mask 0x00ff00ff
		psrlw      xmm6, 8
		pcmpeqb    xmm5, xmm5       // generate mask 0xff00ff00
		psllw      xmm5, 8
		pcmpeqb    xmm4, xmm4       // generate mask 0xff000000
		pslld      xmm4, 24

		sub        ecx, 1
		je         convertloop1     // only 1 pixel?
		jl         convertloop1b

		// 1 pixel loop until destination pointer is aligned.
		alignloop1 :
		test       edx, 15          // aligned
			je         alignloop1b
			movd       xmm3, [eax]
			lea        eax, [eax + 4]
			movdqa     xmm0, xmm3       // src argb
			pxor       xmm3, xmm4       // ~alpha
			movd       xmm2, [esi]      // _r_b
			psrlw      xmm3, 8          // alpha
			pshufhw    xmm3, xmm3, 0F5h  // 8 alpha words
			pshuflw    xmm3, xmm3, 0F5h
			pand       xmm2, xmm6       // _r_b
			paddw      xmm3, xmm7       // 256 - alpha
			pmullw     xmm2, xmm3       // _r_b * alpha
			movd       xmm1, [esi]      // _a_g
			lea        esi, [esi + 4]
			psrlw      xmm1, 8          // _a_g
			por        xmm0, xmm4       // set alpha to 255
			pmullw     xmm1, xmm3       // _a_g * alpha
			psrlw      xmm2, 8          // _r_b convert to 8 bits again
			paddusb    xmm0, xmm2       // + src argb
			pand       xmm1, xmm5       // a_g_ convert to 8 bits again
			paddusb    xmm0, xmm1       // + src argb
			sub        ecx, 1
			movd[edx], xmm0
			lea        edx, [edx + 4]
			jge        alignloop1

			alignloop1b :
		add        ecx, 1 - 4
			jl         convertloop4b

			// 4 pixel loop.
			convertloop4 :
		movdqu     xmm3, [eax]      // src argb
			lea        eax, [eax + 16]
			movdqa     xmm0, xmm3       // src argb
			pxor       xmm3, xmm4       // ~alpha
			movdqu     xmm2, [esi]      // _r_b
			psrlw      xmm3, 8          // alpha
			pshufhw    xmm3, xmm3, 0F5h  // 8 alpha words
			pshuflw    xmm3, xmm3, 0F5h
			pand       xmm2, xmm6       // _r_b
			paddw      xmm3, xmm7       // 256 - alpha
			pmullw     xmm2, xmm3       // _r_b * alpha
			movdqu     xmm1, [esi]      // _a_g
			lea        esi, [esi + 16]
			psrlw      xmm1, 8          // _a_g
			por        xmm0, xmm4       // set alpha to 255
			pmullw     xmm1, xmm3       // _a_g * alpha
			psrlw      xmm2, 8          // _r_b convert to 8 bits again
			paddusb    xmm0, xmm2       // + src argb
			pand       xmm1, xmm5       // a_g_ convert to 8 bits again
			paddusb    xmm0, xmm1       // + src argb
			sub        ecx, 4
			movdqa[edx], xmm0
			lea        edx, [edx + 16]
			jge        convertloop4

			convertloop4b :
		add        ecx, 4 - 1
			jl         convertloop1b

			// 1 pixel loop.
			convertloop1 :
		movd       xmm3, [eax]      // src argb
			lea        eax, [eax + 4]
			movdqa     xmm0, xmm3       // src argb
			pxor       xmm3, xmm4       // ~alpha
			movd       xmm2, [esi]      // _r_b
			psrlw      xmm3, 8          // alpha
			pshufhw    xmm3, xmm3, 0F5h  // 8 alpha words
			pshuflw    xmm3, xmm3, 0F5h
			pand       xmm2, xmm6       // _r_b
			paddw      xmm3, xmm7       // 256 - alpha
			pmullw     xmm2, xmm3       // _r_b * alpha
			movd       xmm1, [esi]      // _a_g
			lea        esi, [esi + 4]
			psrlw      xmm1, 8          // _a_g
			por        xmm0, xmm4       // set alpha to 255
			pmullw     xmm1, xmm3       // _a_g * alpha
			psrlw      xmm2, 8          // _r_b convert to 8 bits again
			paddusb    xmm0, xmm2       // + src argb
			pand       xmm1, xmm5       // a_g_ convert to 8 bits again
			paddusb    xmm0, xmm1       // + src argb
			sub        ecx, 1
			movd[edx], xmm0
			lea        edx, [edx + 4]
			jge        convertloop1

			convertloop1b :
		pop        esi
			ret
	}
}

VideoDXGICaptor::VideoDXGICaptor()
{
	m_nDisPlay = 0;
	m_bHaveCursor = 1;
	m_pBuf = 0;
	m_bInit = FALSE;
	m_hDevice = NULL;
	m_hContext = NULL;
	m_hDeskDupl = NULL;
	m_iWidth = m_iHeight = 0;
	ZeroMemory(&m_dxgiOutDesc, sizeof(m_dxgiOutDesc));
}
VideoDXGICaptor::~VideoDXGICaptor()
{
	Deinit();
}

void VideoDXGICaptor::SetDisPlay(int nDisPlay)
{
	m_nDisPlay = nDisPlay;
}

BOOL VideoDXGICaptor::Init()
{
	HRESULT hr = S_OK;
	if (m_bInit)
	{
		printf("LINE: %d %s err return \n", __LINE__, __func__);
		return FALSE;
	}

	int nCount = Enum();
	if (nCount <= 0)
	{
		printf("LINE: %d %s err return \n", __LINE__, __func__);
		return FALSE;
	}

	m_iWidth = m_iHeight = 0;

	// Driver types supported
	D3D_DRIVER_TYPE DriverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT NumDriverTypes = ARRAYSIZE(DriverTypes);
	// Feature levels supported
	D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_1
	};

	//
	// Get output
	//
	INT nOutput = 0;
	DxgiData data;
	IDXGIAdapter* pAdapter = NULL;
	IDXGIOutput* hDxgiOutput = NULL;
	int nSize = m_vOutputs.size();
	if (m_nDisPlay < 0 || m_nDisPlay >= nSize)
	{
		m_nDisPlay = 0;
	}
	data = m_vOutputs.at(m_nDisPlay);
	hDxgiOutput = data.pOutput;
	pAdapter = data.pAdapter;
	hDxgiOutput->GetDesc(&m_dxgiOutDesc);

	UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
	for (int i = 0; i < NumFeatureLevels; i++)
	{
		D3D_FEATURE_LEVEL FeatureLevel = FeatureLevels[i];
		hr = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, 0, 0, D3D11_SDK_VERSION, &m_hDevice, &FeatureLevel, &m_hContext);
		if (SUCCEEDED(hr))
		{
			break;
		}
	}

	//
	// Get DXGI device
	//
	IDXGIDevice* hDxgiDevice = NULL;
	hr = m_hDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&hDxgiDevice));
	if (FAILED(hr))
	{
		printf("LINE: %d %s err return \n", __LINE__, __func__);
		return FALSE;
	}
	//
	// Get DXGI adapter
	//
	IDXGIAdapter* hDxgiAdapter = NULL;
	hr = hDxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&hDxgiAdapter));
	RESET_OBJECT(hDxgiDevice);
	if (FAILED(hr))
	{
		printf("LINE: %d %s err return \n", __LINE__, __func__);
		return FALSE;
	}

	IDXGIOutput1* hDxgiOutput1 = NULL;
	hr = hDxgiOutput->QueryInterface(__uuidof(hDxgiOutput1), reinterpret_cast<void**>(&hDxgiOutput1));
	RESET_OBJECT(hDxgiOutput);
	if (FAILED(hr))
	{
		printf("LINE: %d %s err return \n", __LINE__, __func__);
		return FALSE;
	}
	//
	// Create desktop duplication
	//
	hr = hDxgiOutput1->DuplicateOutput(m_hDevice, &m_hDeskDupl);
	RESET_OBJECT(hDxgiOutput1);
	if (FAILED(hr))
	{
		printf("LINE: %d %s err return \n", __LINE__, __func__);
		return FALSE;
	}

	m_Subresource = D3D11CalcSubresource(0, 0, 0);

	AttatchToThread();

	// 初始化成功
	m_bInit = TRUE;
	return TRUE;
	// #else
	// 小于vs2012,此功能不能实现
	//return FALSE;
	// #endif
}

//BOOL VideoDXGICaptor::Init()
//{
//	HRESULT hr = S_OK;
//	if (m_bInit)
//	{
//		return FALSE;
//	}
//	m_iWidth = m_iHeight = 0;
//	// Driver types supported
//	D3D_DRIVER_TYPE DriverTypes[] =
//	{
//		D3D_DRIVER_TYPE_HARDWARE,
//		D3D_DRIVER_TYPE_WARP,
//		D3D_DRIVER_TYPE_REFERENCE,
//	};
//	UINT NumDriverTypes = ARRAYSIZE(DriverTypes);
//	// Feature levels supported
//	D3D_FEATURE_LEVEL FeatureLevels[] =
//	{
//		D3D_FEATURE_LEVEL_11_0,
//		D3D_FEATURE_LEVEL_10_1,
//		D3D_FEATURE_LEVEL_10_0,
//		D3D_FEATURE_LEVEL_9_1
//	};
//	UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
//	D3D_FEATURE_LEVEL FeatureLevel;
//	//
//	// Create D3D device
//	//
//
//	for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
//	{
//		hr = D3D11CreateDevice(NULL, DriverTypes[DriverTypeIndex], NULL, 0, FeatureLevels, NumFeatureLevels, D3D11_SDK_VERSION, &m_hDevice, &FeatureLevel, &m_hContext);
//		if (SUCCEEDED(hr))
//		{
//			break;
//		}
//	}
//	if (FAILED(hr))
//	{
//		return FALSE;
//	}
//	//
//	// Get DXGI device
//	//
//	IDXGIDevice *hDxgiDevice = NULL;
//	hr = m_hDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&hDxgiDevice));
//	if (FAILED(hr))
//	{
//		return FALSE;
//	}
//	//
//	// Get DXGI adapter
//	//
//	IDXGIAdapter *hDxgiAdapter = NULL;
//	hr = hDxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&hDxgiAdapter));
//	RESET_OBJECT(hDxgiDevice);
//	if (FAILED(hr))
//	{
//		return FALSE;
//	}
//	//
//	// Get output
//	//
//
//	INT nOutput = 0;
//	IDXGIOutput *hDxgiOutput = NULL;
//	hr = hDxgiAdapter->EnumOutputs(nOutput, &hDxgiOutput);
//	RESET_OBJECT(hDxgiAdapter);
//	if (FAILED(hr))
//	{
//		return FALSE;
//	}
//
//	//
//	// get output description struct
//	//
//	hDxgiOutput->GetDesc(&m_dxgiOutDesc);
//
//	//
//	// QI for Output 1
//	//
//	IDXGIOutput1 *hDxgiOutput1 = NULL;
//	hr = hDxgiOutput->QueryInterface(__uuidof(hDxgiOutput1), reinterpret_cast<void**>(&hDxgiOutput1));
//	RESET_OBJECT(hDxgiOutput);
//	if (FAILED(hr))
//	{
//		return FALSE;
//	}
//	//
//	// Create desktop duplication
//	//
//	hr = hDxgiOutput1->DuplicateOutput(m_hDevice, &m_hDeskDupl);
//	RESET_OBJECT(hDxgiOutput1);
//	if (FAILED(hr))
//	{
//		return FALSE;
//	}
//
//	m_Subresource = D3D11CalcSubresource(0, 0, 0);
//
//	AttatchToThread();
//
//	
//
//	// 初始化成功
//	m_bInit = TRUE;
//	return TRUE;
//	// #else
//	// 小于vs2012,此功能不能实现
//	return FALSE;
//	// #endif
//}
VOID VideoDXGICaptor::Deinit()
{
	if (!m_bInit)
	{
		return;
	}
	m_bInit = FALSE;
	if (m_hDeskDupl)
	{
		m_hDeskDupl->Release();
		m_hDeskDupl = NULL;
	}
	if (m_hDevice)
	{
		m_hDevice->Release();
		m_hDevice = NULL;
	}
	if (m_hContext)
	{
		m_hContext->Release();
		m_hContext = NULL;
	}
	if (m_pBuf)
	{
		delete[] m_pBuf;
		m_pBuf = 0;
	}
	if (m_CursorData)
	{
		free(m_CursorData);
		m_CursorData = NULL;
	}
	m_vOutputs.clear();
	// #endif
}
BOOL VideoDXGICaptor::AttatchToThread(VOID)
{
	if (g_bAttach)
	{
		return TRUE;
	}

	HDESK hCurrentDesktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);
	if (!hCurrentDesktop)
	{
		return FALSE;
	}
	// Attach desktop to this thread
	BOOL bDesktopAttached = SetThreadDesktop(hCurrentDesktop);
	CloseDesktop(hCurrentDesktop);
	hCurrentDesktop = NULL;
	g_bAttach = TRUE;
	return bDesktopAttached;
}
BOOL VideoDXGICaptor::CaptureImage(RECT& rect, void* pData, INT& nLen)
{
	return QueryFrame(pData, nLen);
}

BOOL VideoDXGICaptor::CaptureImage(void* pData, INT& nLen)
{
	return QueryFrame(pData, nLen);
}


BOOL VideoDXGICaptor::ResetDevice()
{
	Deinit();
	return Init();
}

BOOL VideoDXGICaptor::QueryFrame(void* pImgData, INT& nImgSize)
{
	if (!m_bInit || !AttatchToThread())
	{
		return FALSE;
	}
	nImgSize = 0;
	IDXGIResource* hDesktopResource = NULL;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;
	HRESULT hr = m_hDeskDupl->AcquireNextFrame(0, &FrameInfo, &hDesktopResource);
	if (FAILED(hr))
	{
		//
		// 在一些win10的系统上,如果桌面没有变化的情况下，;
		// 这里会发生超时现象，但是这并不是发生了错误，而是系统优化了刷新动作导致的。;
		// 所以，这里没必要返回FALSE，返回不带任何数据的TRUE即可;
		//
		return TRUE;
	}
	//
	// query next frame staging buffer
	//
	ID3D11Texture2D* hAcquiredDesktopImage = NULL;
	hr = hDesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&hAcquiredDesktopImage));
	RESET_OBJECT(hDesktopResource);
	if (FAILED(hr))
	{
		return FALSE;
	}
	//
	// copy old description
	//
	D3D11_TEXTURE2D_DESC frameDescriptor;
	hAcquiredDesktopImage->GetDesc(&frameDescriptor);
	//
	// create a new staging buffer for fill frame image
	//
	ID3D11Texture2D* hNewDesktopImage = NULL;
	frameDescriptor.Usage = D3D11_USAGE_STAGING;
	frameDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	frameDescriptor.BindFlags = 0;
	frameDescriptor.MiscFlags = 0;
	frameDescriptor.MipLevels = 1;
	frameDescriptor.ArraySize = 1;
	frameDescriptor.SampleDesc.Count = 1;
	hr = m_hDevice->CreateTexture2D(&frameDescriptor, NULL, &hNewDesktopImage);
	if (FAILED(hr))
	{
		RESET_OBJECT(hAcquiredDesktopImage);
		m_hDeskDupl->ReleaseFrame();
		return FALSE;
	}
	//
	// copy next staging buffer to new staging buffer
	//
	m_hContext->CopyResource(hNewDesktopImage, hAcquiredDesktopImage);
	RESET_OBJECT(hAcquiredDesktopImage);
	m_hDeskDupl->ReleaseFrame();
	//
	// create staging buffer for map bits
	//
	IDXGISurface* hStagingSurf = NULL;
	hr = hNewDesktopImage->QueryInterface(__uuidof(IDXGISurface), (void**)(&hStagingSurf));
	RESET_OBJECT(hNewDesktopImage);
	if (FAILED(hr))
	{
		return FALSE;
	}
	//
	// copy bits to user space
	//
	DXGI_MAPPED_RECT mappedRect;
	hr = hStagingSurf->Map(&mappedRect, DXGI_MAP_READ);
	if (SUCCEEDED(hr))
	{
		m_iWidth = m_dxgiOutDesc.DesktopCoordinates.right - m_dxgiOutDesc.DesktopCoordinates.left;
		m_iHeight = m_dxgiOutDesc.DesktopCoordinates.bottom - m_dxgiOutDesc.DesktopCoordinates.top;
		nImgSize = m_iWidth * m_iHeight * 4;
		if (!m_pBuf)
		{
			m_pBuf = new BYTE[nImgSize];
			pImgData = m_pBuf;
		}
		// PrepareBGR24From32(mappedRect.pBits, (BYTE*)pImgData, m_dxgiOutDesc.DesktopCoordinates);
		// mappedRect.pBits;
		// am_dxgiOutDesc.DesktopCoordinates;
		memcpy((BYTE*)pImgData, mappedRect.pBits, m_iWidth * m_iHeight * 4);

		hStagingSurf->Unmap();
	}
	RESET_OBJECT(hStagingSurf);
	return SUCCEEDED(hr);
}

BYTE* VideoDXGICaptor::QueryFrame2(int& nImgSize)
{
	if (!m_bInit || !AttatchToThread())
	{

		printf("LINE: %d  err return| m_bInit = %d , AttatchToThread = %d  \n", __LINE__, m_bInit, AttatchToThread());
		return 0;
	}
	nImgSize = 0;
	IDXGIResource* hDesktopResource = NULL;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;
	HRESULT hr = m_hDeskDupl->AcquireNextFrame(0, &FrameInfo, &hDesktopResource);
	if (FAILED(hr))
	{
		//
		// 在一些win10的系统上,如果桌面没有变化的情况下，;
		// 这里会发生超时现象，但是这并不是发生了错误，而是系统优化了刷新动作导致的;
		// 所以，这里没必要返回FALSE，返回不带任何数据的TRUE即可;
		//
		return m_pBuf;
	}
	//
	// query next frame staging buffer
	//
	ID3D11Texture2D* hAcquiredDesktopImage = NULL;
	hr = hDesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&hAcquiredDesktopImage));
	RESET_OBJECT(hDesktopResource);
	if (FAILED(hr))
	{
		printf("LINE: %d  err return \n", __LINE__);
		return m_pBuf;
	}
	//
	// copy old description
	//
	D3D11_TEXTURE2D_DESC frameDescriptor;
	hAcquiredDesktopImage->GetDesc(&frameDescriptor);
	//
	// create a new staging buffer for fill frame image
	//
	ID3D11Texture2D* hNewDesktopImage = NULL;
	frameDescriptor.Usage = D3D11_USAGE_STAGING;
	frameDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
	frameDescriptor.BindFlags = 0;
	frameDescriptor.MiscFlags = 0;
	frameDescriptor.MipLevels = 1;
	frameDescriptor.ArraySize = 1;
	frameDescriptor.SampleDesc.Count = 1;
	hr = m_hDevice->CreateTexture2D(&frameDescriptor, NULL, &hNewDesktopImage);
	if (FAILED(hr))
	{
		RESET_OBJECT(hAcquiredDesktopImage);
		m_hDeskDupl->ReleaseFrame();
		printf("LINE: %d  err return \n", __LINE__);
		return m_pBuf;
	}
	//
	// copy next staging buffer to new staging buffer
	//
	m_hContext->CopyResource(hNewDesktopImage, hAcquiredDesktopImage);


	D3D11_MAPPED_SUBRESOURCE mapdesc;
	HRESULT hRes = m_hContext->Map((ID3D11Texture2D*)hNewDesktopImage, m_Subresource, D3D11_MAP_READ_WRITE, 0, &mapdesc);
	if (FAILED(hRes))
	{
		printf("LINE: %d  err return \n", __LINE__);
		return m_pBuf;
	}
	if (m_bHaveCursor)//画鼠标
		hRes = DrawCursor2(mapdesc, frameDescriptor, FrameInfo);
	m_hContext->Unmap(hAcquiredDesktopImage, m_Subresource);


	RESET_OBJECT(hAcquiredDesktopImage);
	m_hDeskDupl->ReleaseFrame();
	//
	// create staging buffer for map bits
	//
	IDXGISurface* hStagingSurf = NULL;
	hr = hNewDesktopImage->QueryInterface(__uuidof(IDXGISurface), (void**)(&hStagingSurf));
	RESET_OBJECT(hNewDesktopImage);
	if (FAILED(hr))
	{
		printf("LINE: %d  err return \n", __LINE__);
		return FALSE;
	}
	//
	// copy bits to user space
	//
	DXGI_MAPPED_RECT mappedRect;
	hr = hStagingSurf->Map(&mappedRect, DXGI_MAP_READ);
	if (SUCCEEDED(hr))
	{
		m_iWidth = m_dxgiOutDesc.DesktopCoordinates.right - m_dxgiOutDesc.DesktopCoordinates.left;
		m_iHeight = m_dxgiOutDesc.DesktopCoordinates.bottom - m_dxgiOutDesc.DesktopCoordinates.top;

		nImgSize = m_iWidth * m_iHeight * 4;
		if (!m_pBuf)
		{
			m_pBuf = new BYTE[nImgSize];
		}
		// PrepareBGR24From32(mappedRect.pBits, (BYTE*)pImgData, m_dxgiOutDesc.DesktopCoordinates);
		// mappedRect.pBits;
		// am_dxgiOutDesc.DesktopCoordinates;
		//memcpy((BYTE*)m_pBuf, mappedRect.pBits, m_dxgiOutDesc.DesktopCoordinates.right * m_dxgiOutDesc.DesktopCoordinates.bottom * 4);
		int nImagePitch = m_iWidth * 4;

		for (int i = 0; i < m_iHeight; i++)
		{
			memcpy(m_pBuf + i * nImagePitch, mappedRect.pBits + i * mapdesc.RowPitch, mapdesc.RowPitch);
			//memcpy(m_pBuf + i * nImagePitch, mappedRect.pBits + i * mapdesc.RowPitch, nImagePitch);
		}
		hStagingSurf->Unmap();
	}
	RESET_OBJECT(hStagingSurf);
	return m_pBuf;
}

static LPBYTE GetBitmapData(HBITMAP hBmp, BITMAP& bmp)
{
	if (!hBmp)
		return NULL;

	if (GetObject(hBmp, sizeof(bmp), &bmp) != 0) {
		UINT bitmapDataSize = bmp.bmHeight * bmp.bmWidth * bmp.bmBitsPixel;
		bitmapDataSize >>= 3;

		LPBYTE lpBitmapData = (LPBYTE)malloc(bitmapDataSize);
		GetBitmapBits(hBmp, bitmapDataSize, lpBitmapData);

		return lpBitmapData;
	}

	return NULL;
}

static inline BYTE BitToAlpha(LPBYTE lp1BitTex, int pixel, bool bInvert)
{
	BYTE pixelByte = lp1BitTex[pixel / 8];
	BYTE pixelVal = pixelByte >> (7 - (pixel % 8)) & 1;

	if (bInvert)
		return pixelVal ? 0xFF : 0;
	else
		return pixelVal ? 0 : 0xFF;
}

LPBYTE GetCursorData(HICON hIcon, ICONINFO& ii, uint32_t& width, uint32_t& height, uint32_t& pitch)
{
	BITMAP bmpColor, bmpMask;
	LPBYTE lpBitmapData = NULL, lpMaskData = NULL;

	if (lpBitmapData = GetBitmapData(ii.hbmColor, bmpColor))
	{
		if (bmpColor.bmBitsPixel < 32)
		{
			free(lpBitmapData);
			return NULL;
		}

		if (lpMaskData = GetBitmapData(ii.hbmMask, bmpMask))
		{
			int pixels = bmpColor.bmHeight * bmpColor.bmWidth;
			bool bHasAlpha = false;

			//god-awful horrible hack to detect 24bit cursor
			for (int i = 0; i < pixels; i++)
			{
				if (lpBitmapData[i * 4 + 3] != 0) {
					bHasAlpha = true;
					break;
				}
			}

			if (!bHasAlpha) {
				for (int i = 0; i < pixels; i++) {
					lpBitmapData[i * 4 + 3] = BitToAlpha(lpMaskData, i, false);
				}
			}

			free(lpMaskData);
		}

		width = bmpColor.bmWidth;
		height = bmpColor.bmHeight;
		pitch = bmpColor.bmWidthBytes;
	}
	else if (lpMaskData = GetBitmapData(ii.hbmMask, bmpMask))
	{
		bmpMask.bmHeight /= 2;

		int pixels = bmpMask.bmHeight * bmpMask.bmWidth;
		lpBitmapData = (LPBYTE)malloc(pixels * 4);
		memset(lpBitmapData, 0, pixels * 4);

		UINT bottom = bmpMask.bmWidthBytes * bmpMask.bmHeight;

		for (int i = 0; i < pixels; i++) {
			BYTE transparentVal = BitToAlpha(lpMaskData, i, false);
			BYTE colorVal = BitToAlpha(lpMaskData + bottom, i, true);

			if (!transparentVal)
				lpBitmapData[i * 4 + 3] = colorVal; //as an alternative to xoring, shows inverted as black
			else
				*(LPDWORD)(lpBitmapData + (i * 4)) = colorVal ? 0xFFFFFFFF : 0xFF000000;
		}

		free(lpMaskData);

		width = bmpMask.bmWidth;
		height = bmpMask.bmHeight;
		//pitch = bmpMask.bmWidthBytes;
		//char test[128];
		//sprintf_s(test, "hahah - %d", bmpMask.bmWidthBytes);
		//OutputDebugStringA(test);

	}

	return lpBitmapData;
}

HRESULT VideoDXGICaptor::DrawCursor2(D3D11_MAPPED_SUBRESOURCE mapdesc, D3D11_TEXTURE2D_DESC desc, DXGI_OUTDUPL_FRAME_INFO frameInfo)
{
	HRESULT hRes = S_OK;
	bool bShowCursor = true;
	if (mapdesc.pData)
	{

		CURSORINFO ci;
		memset(&ci, 0, sizeof(ci));
		ci.cbSize = sizeof(ci);

		if (GetCursorInfo(&ci))
		{
			memcpy(&m_CursorPos, &ci.ptScreenPos, sizeof(m_CursorPos));

			if (ci.flags & CURSOR_SHOWING)
			{
				if (ci.hCursor != m_hCurrentCursor) // re-get cursor data
				{
					HICON hIcon = CopyIcon(ci.hCursor);
					m_hCurrentCursor = ci.hCursor;

					free(m_CursorData);
					m_CursorData = NULL;

					if (hIcon)
					{
						ICONINFO ii;
						if (GetIconInfo(hIcon, &ii))
						{
							xHotspot = int(ii.xHotspot);
							yHotspot = int(ii.yHotspot);

							m_CursorData = GetCursorData(hIcon, ii, m_CursorWidth, m_CursorHeight, m_CursorPitch);

							DeleteObject(ii.hbmColor);
							DeleteObject(ii.hbmMask);
						}

						DestroyIcon(hIcon);
					}
				}
			}
			else
			{
				bShowCursor = false;
			}
		}


		if (m_CursorData && bShowCursor)
		{
			// Not supporting mono and masked pointers at the moment
			//printf("Drawing pointer at %d %d\n", data->PointerPosition.Position.x, data->PointerPosition.Position.y);
			const int ptrx = m_CursorPos.x - xHotspot;
			const int ptry = m_CursorPos.y - yHotspot;
			uint8_t* ptr = m_CursorData;
			uint8_t* dst;
			// ### Should really do the blending on the GPU (Using DirectX) rather than using SSE2 on the CPU
			const int ptrw = min(m_CursorWidth, desc.Width - ptrx);
			for (unsigned int y = 0; y < m_CursorHeight; ++y)
			{
				if (y + ptry >= desc.Height)
					break;
				dst = static_cast<uint8_t*>(mapdesc.pData) + (((y + ptry) * mapdesc.RowPitch) + (ptrx * 4));
				//memcpy(dst, ptr, data->PointerShape.Width * 4);
				ARGBBlendRow_SSE2(ptr, dst, dst, ptrw);
				ptr += m_CursorPitch;
			}
		}
	}

	return hRes;
}




