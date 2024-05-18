#include "SimLumenRender.h"

static CSimLumenRender* g_global_render = nullptr;

CSimLumenRender* GetGlobalRender()
{
	if (g_global_render)
	{
		return g_global_render;
	}
	
	g_global_render = new CSimLumenRender();
	return g_global_render;
}

void CSimLumenRender::Init()
{
	XMFLOAT3 vtx_pos[6] = {
		XMFLOAT3(0,0,0),XMFLOAT3(1,0,0), XMFLOAT3(1,1,0),
		XMFLOAT3(0,0,0), XMFLOAT3(1,1,0), XMFLOAT3(0,1,0) };
	m_full_screen_pos_buffer.Create(L"FullScreenPosBuffer",6,sizeof(XMFLOAT3), vtx_pos);

	XMFLOAT2 vtx_uv[6] = {
	XMFLOAT2(0,1),XMFLOAT2(1,1), XMFLOAT2(1,0),
	XMFLOAT2(0,1), XMFLOAT2(1,0), XMFLOAT2(0,0) };

	m_full_screen_uv_buffer.Create(L"FullScreenUVBuffer", 6, sizeof(XMFLOAT2), vtx_uv);
}
