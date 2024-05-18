#include "SimLumenCommon.h"

static SLumenConfig* config = nullptr;


SLumenConfig GetLumenConfig()
{
	if (config)
	{
		return *config;
	}
	
	config = new SLumenConfig();
	config->m_LightDirection = Normalize(Math::Vector3(-1, 1, 1));
	config->m_atlas_size = Math::XMINT2(4096, 4096);
	config->m_atlas_num_xy = Math::XMINT2(4096 / 128, 4096 / 128);
	return *config;
}
