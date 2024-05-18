#pragma once
#include "../SimLumenCommon/SimLumenCommon.h"

struct SSimLuCard
{
	uint32_t m_card_idx_x;
	uint32_t m_card_idx_y;
};

struct SSimLuMeshCards
{
	Math::Matrix4 m_local_to_world;
	uint32_t m_first_card_index;
	uint32_t m_num_cards;
};
