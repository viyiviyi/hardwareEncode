#pragma once
#include "common_utils.h"
#include "cmd_options.h"
#include "mfxvideo.h"
#include "screenShot.h"
#include  <conio.h>

class qsvEncode
{
public:
	qsvEncode();
	~qsvEncode();

public:
	int initCreate();

	int qsv_preproc_encode(BYTE* pData, bool bVPP_ENCODE);

	int qsv_close();	

private:
	//dxgi
	bool bCaptureOff = false;
	VideoDXGICaptor* screenShot = nullptr;

	//intel qsv
	mfxStatus sts = MFX_ERR_NONE;
	mfxFrameAllocator mfxAllocator;
	mfxIMPL impl = MFX_IMPL_HARDWARE;
	mfxVersion ver = { {0, 1} };
	mfxVideoParam mfxEncParams;

	mfxBitstream mfxBS;
	std::vector<mfxU8> bstData;
	mfxU16 nSurfNumVPPIn = 0;
	mfxU16 nSurfNumVPPOutEnc = 0;
	mfxVideoParam VPPParams;
	mfxFrameAllocRequest VPPRequest[2];     // [0] - in, [1] - out
	mfxExtVPPDoNotUse extDoNotUse;
	mfxU32 tabDoNotUseAlgo[4];
	mfxExtBuffer* extBuffers[1];
	mfxVideoParam par;
	// Query number of required surfaces for encoder
	mfxFrameAllocRequest EncRequest;

	mfxFrameAllocResponse mfxResponseVPPIn;
	mfxFrameAllocResponse mfxResponseVPPOutEnc;

	std::vector<mfxFrameSurface1> pmfxSurfacesVPPIn;
	std::vector<mfxFrameSurface1> pVPPSurfacesVPPOutEnc;

	int nEncSurfIdx = 0;
	int nVPPSurfIdx = 0;
	mfxSyncPoint syncpVPP = NULL, syncpEnc = NULL;

	MFXVideoSession session;
	// Create Media SDK encoder
	MFXVideoENCODE mfxENC = NULL;
	// Create Media SDK VPP component
	MFXVideoVPP mfxVPP = NULL;	
};


