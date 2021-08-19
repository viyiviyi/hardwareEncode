#include "dll.h"

qsvEncode* _qsvEncode = nullptr;

int API_QSV_Init()
{
	if(_qsvEncode == nullptr)
		_qsvEncode = new qsvEncode();
	return 0;
}

void API_QSV_Encode(BYTE* pData, bool bVPP_ENCODE)
{
	if(_qsvEncode)
		_qsvEncode->qsv_preproc_encode(pData, bVPP_ENCODE);
}

int API_QSV_Close()
{
	if (_qsvEncode)
		delete _qsvEncode;
	return 0;
}