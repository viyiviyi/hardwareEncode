#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#ifdef WIN32
#include "stdint.h"
#endif
#include <vector>

struct DxgiData
{
	IDXGIAdapter* pAdapter;
	IDXGIOutput* pOutput;
};
class VideoDXGICaptor
{
public:
	VideoDXGICaptor();
	~VideoDXGICaptor();

public:
	int m_bHaveCursor;
	int m_iWidth, m_iHeight;
	int m_nDisPlay;

	void SetDisPlay(int nDisPlay);
	BOOL Init();
	VOID Deinit();
	BYTE* CapImage();

public:
	void SetHaveCursor(int nHave)
	{
		m_bHaveCursor = nHave;
	}

	virtual BOOL CaptureImage(RECT& rect, void* pData, INT& nLen);
	virtual BOOL CaptureImage(void* pData, INT& nLen);

	BYTE* QueryFrame2(int& nImgSize);

	virtual BOOL ResetDevice();

private:
	BYTE* m_pBuf;
	BOOL  AttatchToThread(VOID);
	BOOL  QueryFrame(void* pImgData, INT& nImgSize);

	BOOL  QueryFrame(void* pImgData, INT& nImgSize, int z);

private:
	IDXGIResource* zhDesktopResource;
	DXGI_OUTDUPL_FRAME_INFO zFrameInfo;
	ID3D11Texture2D* zhAcquiredDesktopImage;
	IDXGISurface* zhStagingSurf;

private:
	BOOL                    m_bInit;

	ID3D11Device* m_hDevice;
	ID3D11DeviceContext* m_hContext;
	IDXGIOutputDuplication* m_hDeskDupl;
	DXGI_OUTPUT_DESC        m_dxgiOutDesc;

	int m_Subresource;
	POINT    m_CursorPos;
	HCURSOR  m_hCurrentCursor;
	uint8_t* m_CursorData;
	int      xHotspot, yHotspot;
	uint32_t m_CursorWidth, m_CursorHeight, m_CursorPitch;
	HRESULT DrawCursor2(D3D11_MAPPED_SUBRESOURCE mapdesc, D3D11_TEXTURE2D_DESC desc, DXGI_OUTDUPL_FRAME_INFO frameInfo);

	std::vector<DxgiData>	m_vOutputs;
	int Enum()
	{
		IDXGIFactory1* pFactory1;

		HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&pFactory1));

		if (FAILED(hr))
		{
			return 0;
		}

		for (UINT i = 0;; i++)
		{
			IDXGIAdapter1* pAdapter1 = nullptr;

			hr = pFactory1->EnumAdapters1(i, &pAdapter1);

			if (hr == DXGI_ERROR_NOT_FOUND)
			{
				// no more adapters
				break;
			}

			if (FAILED(hr))
			{
				return 0;
			}

			DXGI_ADAPTER_DESC1 desc;

			hr = pAdapter1->GetDesc1(&desc);

			if (FAILED(hr))
			{
				return 0;
			}

			desc.Description;

			for (UINT j = 0;; j++)
			{
				IDXGIOutput* pOutput = nullptr;

				HRESULT hr = pAdapter1->EnumOutputs(j, &pOutput);

				if (hr == DXGI_ERROR_NOT_FOUND)
				{
					// no more outputs
					break;
				}

				if (FAILED(hr))
				{
					return 0;
				}

				DXGI_OUTPUT_DESC desc;

				hr = pOutput->GetDesc(&desc);

				if (FAILED(hr))
				{
					return 0;
				}

				desc.DeviceName;
				desc.DesktopCoordinates.left;
				desc.DesktopCoordinates.top;
				int width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
				int height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
				DxgiData data;
				data.pAdapter = pAdapter1;
				data.pOutput = pOutput;
				m_vOutputs.push_back(data);
			}
		}
		return m_vOutputs.size();
	}

};

