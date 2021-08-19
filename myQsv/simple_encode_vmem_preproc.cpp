#include "encode_vmem_preproc.h"

qsvEncode::qsvEncode()
{
    initCreate();
}

qsvEncode::~qsvEncode()
{
    if (screenShot)
        delete screenShot;
    qsv_close();
}

int qsvEncode::initCreate()
{
    int ret;

    screenShot = new VideoDXGICaptor();

    screenShot->SetDisPlay(1);

    ret = screenShot->Init();

    if (ret < 0)
    {
        printf("dxgi init err \n");
        return -1;
    }

    screenShot->QueryFrame2(bCaptureOff);

    CmdOptions options;

    memset(&options, 0, sizeof(CmdOptions));
    options.ctx.options = OPTIONS_ENCODE;
    //options.values.impl = MFX_IMPL_HARDWARE;

    options.values.Bitrate = 1000;
    options.values.FrameRateD = 1;
    options.values.FrameRateN = 25;
    options.values.Width =  screenShot->m_iWidth;
    options.values.Height = screenShot->m_iHeight;

    if (!options.values.Width || !options.values.Height) {
        printf("error: input video geometry not set (mandatory)\n");
        return -1;
    }
    if (!options.values.Bitrate) {
        printf("error: bitrate not set (mandatory)\n");
        return -1;
    }
    if (!options.values.FrameRateN || !options.values.FrameRateD) {
        printf("error: framerate not set (mandatory)\n");
        return -1;
    }

    sts = Initialize(impl, ver, &session, &mfxAllocator);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Initialize encoder parameters
    memset(&mfxEncParams, 0, sizeof(mfxEncParams));
    mfxEncParams.mfx.CodecId = MFX_CODEC_AVC;//MFX_CODEC_JPEG;
    mfxEncParams.mfx.CodecProfile = MFX_PROFILE_JPEG_BASELINE;
    mfxEncParams.mfx.TargetUsage = MFX_TARGETUSAGE_BALANCED;
    mfxEncParams.mfx.TargetKbps = options.values.Bitrate;
    mfxEncParams.mfx.RateControlMethod = MFX_RATECONTROL_VBR;
    mfxEncParams.mfx.FrameInfo.FrameRateExtN = options.values.FrameRateN;
    mfxEncParams.mfx.FrameInfo.FrameRateExtD = options.values.FrameRateD;
    mfxEncParams.mfx.FrameInfo.FourCC = MFX_FOURCC_NV12;
    mfxEncParams.mfx.FrameInfo.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    mfxEncParams.mfx.FrameInfo.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    mfxEncParams.mfx.FrameInfo.CropX = 0;
    mfxEncParams.mfx.FrameInfo.CropY = 0;
    mfxEncParams.mfx.FrameInfo.CropW = options.values.Width; // *96 / horizontalDPI;
    mfxEncParams.mfx.FrameInfo.CropH = options.values.Height; // *96 / verticalDPI;
    
    //JPEG需要的参数
    //mfxEncParams.mfx.Interleaved = MFX_SCANTYPE_INTERLEAVED;
    //mfxEncParams.mfx.Quality = 50;  //图像质量
    //mfxEncParams.mfx.RestartInterval = 2;

    // Width must be a multiple of 16
    // Height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    mfxEncParams.mfx.FrameInfo.Width = MSDK_ALIGN16(options.values.Width);
    mfxEncParams.mfx.FrameInfo.Height =
        (MFX_PICSTRUCT_PROGRESSIVE == mfxEncParams.mfx.FrameInfo.PicStruct) ?
        MSDK_ALIGN16(options.values.Height) :
        MSDK_ALIGN32(options.values.Height);

    mfxEncParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY;

    // Initialize VPP parameters
    memset(&VPPParams, 0, sizeof(VPPParams));
    // Input data
    VPPParams.vpp.In.FourCC = MFX_FOURCC_RGB4;
    VPPParams.vpp.In.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    VPPParams.vpp.In.CropX = 0;
    VPPParams.vpp.In.CropY = 0;
    VPPParams.vpp.In.CropW = options.values.Width;
    VPPParams.vpp.In.CropH = options.values.Height;
    VPPParams.vpp.In.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    VPPParams.vpp.In.FrameRateExtN = options.values.FrameRateN;
    VPPParams.vpp.In.FrameRateExtD = options.values.FrameRateD;
    // width must be a multiple of 16
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    VPPParams.vpp.In.Width = MSDK_ALIGN16(options.values.Width);
    VPPParams.vpp.In.Height =
        (MFX_PICSTRUCT_PROGRESSIVE == VPPParams.vpp.In.PicStruct) ?
        MSDK_ALIGN16(options.values.Height) :
        MSDK_ALIGN32(options.values.Height);

    // Output data
    VPPParams.vpp.Out.FourCC = MFX_FOURCC_NV12;
    VPPParams.vpp.Out.ChromaFormat = MFX_CHROMAFORMAT_YUV420;
    VPPParams.vpp.Out.CropX = 0;
    VPPParams.vpp.Out.CropY = 0;
    VPPParams.vpp.Out.CropW = options.values.Width;
    VPPParams.vpp.Out.CropH = options.values.Height;
    VPPParams.vpp.Out.PicStruct = MFX_PICSTRUCT_PROGRESSIVE;
    VPPParams.vpp.Out.FrameRateExtN = options.values.FrameRateN;
    VPPParams.vpp.Out.FrameRateExtD = options.values.FrameRateD;
    // width must be a multiple of 16
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
    VPPParams.vpp.Out.Width = MSDK_ALIGN16(VPPParams.vpp.Out.CropW);
    VPPParams.vpp.Out.Height =
        (MFX_PICSTRUCT_PROGRESSIVE == VPPParams.vpp.Out.PicStruct) ?
        MSDK_ALIGN16(VPPParams.vpp.Out.CropH) :
        MSDK_ALIGN32(VPPParams.vpp.Out.CropH);

    VPPParams.IOPattern = MFX_IOPATTERN_IN_VIDEO_MEMORY | MFX_IOPATTERN_OUT_VIDEO_MEMORY;

    mfxENC = MFXVideoENCODE(session);

    memset(&EncRequest, 0, sizeof(EncRequest));
    sts = mfxENC.QueryIOSurf(&mfxEncParams, &EncRequest);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    mfxVPP = MFXVideoVPP(session);
    // Query number of required surfaces for VPP

    memset(&VPPRequest, 0, sizeof(mfxFrameAllocRequest) * 2);
    sts = mfxVPP.QueryIOSurf(&VPPParams, VPPRequest);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    EncRequest.Type |= MFX_MEMTYPE_FROM_VPPOUT;     // surfaces are shared between VPP output and encode input

    // Determine the required number of surfaces for VPP input and for VPP output (encoder input)
    nSurfNumVPPIn = VPPRequest[0].NumFrameSuggested;
    nSurfNumVPPOutEnc = EncRequest.NumFrameSuggested + VPPRequest[1].NumFrameSuggested;

    EncRequest.NumFrameSuggested = nSurfNumVPPOutEnc;

    VPPRequest[0].Type |= WILL_WRITE; // This line is only required for Windows DirectX11 to ensure that surfaces can be written to by the application

    // Allocate required surfaces
    sts = mfxAllocator.Alloc(mfxAllocator.pthis, &VPPRequest[0], &mfxResponseVPPIn);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    sts = mfxAllocator.Alloc(mfxAllocator.pthis, &EncRequest, &mfxResponseVPPOutEnc);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Allocate surface headers (mfxFrameSurface1) for VPPIn
    pmfxSurfacesVPPIn.resize(nSurfNumVPPIn);
    for (int i = 0; i < nSurfNumVPPIn; i++) {
        memset(&pmfxSurfacesVPPIn[i], 0, sizeof(mfxFrameSurface1));
        pmfxSurfacesVPPIn[i].Info = VPPParams.vpp.In;
        pmfxSurfacesVPPIn[i].Data.MemId = mfxResponseVPPIn.mids[i];
        //ClearRGBSurfaceVMem(pmfxSurfacesVPPIn[i].Data.MemId);
    }

    pVPPSurfacesVPPOutEnc.resize(nSurfNumVPPOutEnc);
    for (int i = 0; i < nSurfNumVPPOutEnc; i++) {
        memset(&pVPPSurfacesVPPOutEnc[i], 0, sizeof(mfxFrameSurface1));
        pVPPSurfacesVPPOutEnc[i].Info = VPPParams.vpp.Out;
        pVPPSurfacesVPPOutEnc[i].Data.MemId = mfxResponseVPPOutEnc.mids[i];
    }

    // Disable default VPP operations
    memset(&extDoNotUse, 0, sizeof(mfxExtVPPDoNotUse));
    extDoNotUse.Header.BufferId = MFX_EXTBUFF_VPP_DONOTUSE;
    extDoNotUse.Header.BufferSz = sizeof(mfxExtVPPDoNotUse);
    extDoNotUse.NumAlg = 4;
    extDoNotUse.AlgList = tabDoNotUseAlgo;
    MSDK_CHECK_POINTER(extDoNotUse.AlgList, MFX_ERR_MEMORY_ALLOC);
    extDoNotUse.AlgList[0] = MFX_EXTBUFF_VPP_DENOISE;       // turn off denoising (on by default)
    extDoNotUse.AlgList[1] = MFX_EXTBUFF_VPP_SCENE_ANALYSIS;        // turn off scene analysis (on by default)
    extDoNotUse.AlgList[2] = MFX_EXTBUFF_VPP_DETAIL;        // turn off detail enhancement (on by default)
    extDoNotUse.AlgList[3] = MFX_EXTBUFF_VPP_PROCAMP;       // turn off processing amplified (on by default)

    // Add extended VPP buffers

    extBuffers[0] = (mfxExtBuffer*)&extDoNotUse;
    VPPParams.ExtParam = extBuffers;
    VPPParams.NumExtParam = 1;

    // Initialize the Media SDK encoder
    sts = mfxENC.Init(&mfxEncParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Initialize Media SDK VPP
    sts = mfxVPP.Init(&VPPParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Retrieve video parameters selected by encoder.
    // - BufferSizeInKB parameter is required to set bit stream buffer size

    memset(&par, 0, sizeof(par));
    sts = mfxENC.GetVideoParam(&par);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Prepare Media SDK bit stream buffer
    int BytesPerPx = 1.5; //NV12

    memset(&mfxBS, 0, sizeof(mfxBS));
    //JPEG 需要的缓存区
    //par.mfx.BufferSizeInKB = 4 + (options.values.Width * options.values.Height * BytesPerPx + 1023) / 1024;
    mfxBS.MaxLength = par.mfx.BufferSizeInKB * 1000;
    bstData.resize(mfxBS.MaxLength);
    mfxBS.Data = bstData.data();

    //sts = MFXVideoVPP_Query(session, &VPPParams, &VPPParams);
    //MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
}

int qsvEncode::qsv_preproc_encode(BYTE* pData, bool bVPP_ENCODE)
{
    // ===================================
    // Start processing frames
    //

    //bVPP_ENCODE = true;
    //
    // Stage 1: Main VPP/encoding loop
    //
    while ( (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts) && bVPP_ENCODE) {

        nVPPSurfIdx = GetFreeSurfaceIndex(pmfxSurfacesVPPIn);    // Find free input frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nVPPSurfIdx, MFX_ERR_MEMORY_ALLOC);

        // Surface locking required when read/write video surfaces
        sts = mfxAllocator.Lock(mfxAllocator.pthis, pmfxSurfacesVPPIn[nVPPSurfIdx].Data.MemId, &(pmfxSurfacesVPPIn[nVPPSurfIdx].Data));
        MSDK_BREAK_ON_ERROR(sts); 

        //sts = LoadRawRGBFrame(&pmfxSurfacesVPPIn[nVPPSurfIdx], fSource.get());  // Load frame from file into surface
		sts = LoadRawRGBFrame(&pmfxSurfacesVPPIn[nVPPSurfIdx], screenShot->QueryFrame2(bCaptureOff), screenShot->mapdesc);
        MSDK_BREAK_ON_ERROR(sts);
        //Sleep(30);
        bVPP_ENCODE = false;
        sts = mfxAllocator.Unlock(mfxAllocator.pthis, pmfxSurfacesVPPIn[nVPPSurfIdx].Data.MemId, &(pmfxSurfacesVPPIn[nVPPSurfIdx].Data));
        MSDK_BREAK_ON_ERROR(sts);

        nEncSurfIdx = GetFreeSurfaceIndex(pVPPSurfacesVPPOutEnc);    // Find free output frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nEncSurfIdx, MFX_ERR_MEMORY_ALLOC);

        for (;;) {
            // Process a frame asychronously (returns immediately)
            sts = mfxVPP.RunFrameVPPAsync(&pmfxSurfacesVPPIn[nVPPSurfIdx], &pVPPSurfacesVPPOutEnc[nEncSurfIdx], NULL, &syncpVPP);
            if (MFX_WRN_DEVICE_BUSY == sts) {
                MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            } else
                break;
        }

        if (MFX_ERR_MORE_DATA == sts)
            continue;


        // MFX_ERR_MORE_SURFACE means output is ready but need more surface (example: Frame Rate Conversion 30->60)
        // * Not handled in this example!

        MSDK_BREAK_ON_ERROR(sts);

        for (;;) {
            // Encode a frame asychronously (returns immediately)
            sts = mfxENC.EncodeFrameAsync(NULL, &pVPPSurfacesVPPOutEnc[nEncSurfIdx], &mfxBS, &syncpEnc);

            if (MFX_ERR_NONE < sts && !syncpEnc) {  // Repeat the call if warning and no output
                if (MFX_WRN_DEVICE_BUSY == sts)
                    MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            } else if (MFX_ERR_NONE < sts && syncpEnc) {
                sts = MFX_ERR_NONE;     // Ignore warnings if output is available
                break;
            } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
                // Allocate more bitstream buffer memory here if needed...
                break;
            } else
                break;
        }

        if (MFX_ERR_NONE == sts) {
            sts = session.SyncOperation(syncpEnc, 60000);   // Synchronize. Wait until encoded frame is ready                       
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            sts = WriteBitStreamFrame(&mfxBS, pData);
            MSDK_BREAK_ON_ERROR(sts);

           // ++nFrame;
           // if (bEnableOutput) {
           //     printf("Frame number: %d\r", nFrame);
           // }
        }
    }

    // MFX_ERR_MORE_DATA means that the input file has ended, need to go to buffering loop, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    //
    // Stage 2: Retrieve the buffered VPP frames
    //
    while (MFX_ERR_NONE <= sts) {
        nEncSurfIdx = GetFreeSurfaceIndex(pVPPSurfacesVPPOutEnc);    // Find free output frame surface
        MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nEncSurfIdx, MFX_ERR_MEMORY_ALLOC);

        for (;;) {
            // Process a frame asychronously (returns immediately)
            sts = mfxVPP.RunFrameVPPAsync(NULL, &pVPPSurfacesVPPOutEnc[nEncSurfIdx], NULL, &syncpVPP);
            if (MFX_WRN_DEVICE_BUSY == sts) {
                MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            } else
                break;
        }

        MSDK_BREAK_ON_ERROR(sts);

        for (;;) {
            // Encode a frame asychronously (returns immediately)
            sts = mfxENC.EncodeFrameAsync(NULL, &pVPPSurfacesVPPOutEnc[nEncSurfIdx], &mfxBS, &syncpEnc);

            if (MFX_ERR_NONE < sts && !syncpEnc) {  // Repeat the call if warning and no output
                if (MFX_WRN_DEVICE_BUSY == sts)
                    MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            } else if (MFX_ERR_NONE < sts && syncpEnc) {
                sts = MFX_ERR_NONE;     // Ignore warnings if output is available
                break;
            } else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts) {
                // Allocate more bitstream buffer memory here if needed...
                break;
            } else
                break;
        }

        if (MFX_ERR_NONE == sts) {
            sts = session.SyncOperation(syncpEnc, 60000);   // Synchronize. Wait until encoded frame is ready
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            sts = WriteBitStreamFrame(&mfxBS, pData);
            MSDK_BREAK_ON_ERROR(sts);

           // ++nFrame;
           // if (bEnableOutput) {
            //    printf("Frame number: %d\r", nFrame);
            //}
        }
    }

    // MFX_ERR_MORE_DATA indicates that there are no more buffered frames, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    //
    // Stage 3: Retrieve the buffered encoder frames
    //
    while (MFX_ERR_NONE <= sts) {
        for (;;) {
            // Encode a frame asychronously (returns immediately)
            sts = mfxENC.EncodeFrameAsync(NULL, NULL, &mfxBS, &syncpEnc);

            if (MFX_ERR_NONE < sts && !syncpEnc) {  // Repeat the call if warning and no output
                if (MFX_WRN_DEVICE_BUSY == sts)
                    MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call
            } else if (MFX_ERR_NONE < sts && syncpEnc) {
                sts = MFX_ERR_NONE;     // Ignore warnings if output is available
                break;
            } else
                break;
        }

        if (MFX_ERR_NONE == sts) {
            sts = session.SyncOperation(syncpEnc, 60000);   // Synchronize. Wait until encoded frame is ready
            MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

            sts = WriteBitStreamFrame(&mfxBS, pData);
            MSDK_BREAK_ON_ERROR(sts);

           // ++nFrame;
            //if (bEnableOutput) {
            //    printf("Frame number: %d\r", nFrame);
            //}
        }
    }

    // MFX_ERR_MORE_DATA indicates that there are no more buffered frames, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    //mfxGetTime(&tEnd);
    //double elapsed = TimeDiffMsec(tEnd, tStart) / 1000;
    //double fps = ((double)nFrame / elapsed);
    //printf("\nExecution time: %3.2f s (%3.2f fps)\n", elapsed, fps);

}

int qsvEncode::qsv_close()
{
    // ===================================================================
 // Clean up resources
 //  - It is recommended to close Media SDK components first, before releasing allocated surfaces, since
 //    some surfaces may still be locked by internal Media SDK resources.

    mfxENC.Close();
    mfxVPP.Close();
    // session closed automatically on destruction

    mfxAllocator.Free(mfxAllocator.pthis, &mfxResponseVPPIn);
    mfxAllocator.Free(mfxAllocator.pthis, &mfxResponseVPPOutEnc);
    Release();
    return 0;
}

