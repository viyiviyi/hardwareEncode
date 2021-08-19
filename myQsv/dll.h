#pragma once
#include "encode_vmem_preproc.h"

extern "C" __declspec(dllexport) int API_QSV_Init();

extern "C" __declspec(dllexport) void API_QSV_Encode(BYTE * pData, bool bVPP_ENCODE);

extern "C" __declspec(dllexport) int API_QSV_Close();