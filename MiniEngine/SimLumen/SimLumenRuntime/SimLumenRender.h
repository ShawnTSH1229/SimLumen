#pragma once
#include "../SimLumenCommon/SimLumenCommon.h"

class CSimLumenRender
{
public:
	void Init();

	inline ByteAddressBuffer* GetFullScreenPosBuffer() { return &m_full_screen_pos_buffer; }
	inline ByteAddressBuffer* GetFullScreenUVBuffer() { return &m_full_screen_uv_buffer; }
private:
	ByteAddressBuffer m_full_screen_pos_buffer;
	ByteAddressBuffer m_full_screen_uv_buffer;
};

CSimLumenRender* GetGlobalRender();