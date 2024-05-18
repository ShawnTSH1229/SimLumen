#define GLOBAL_SDF_SIZE_X 256
#define GLOBAL_SDF_SIZE_Y 256
#define GLOBAL_SDF_SIZE_Z 128

#define SCENE_VOXEL_SIZE_X 256
#define SCENE_VOXEL_SIZE_Y 256
#define SCENE_VOXEL_SIZE_Z 128

#define VOXEL_TEXTURE_SIZE_X (SCENE_VOXEL_SIZE_X * 16) //4096
#define VOXEL_TEXTURE_SIZE_Y (SCENE_VOXEL_SIZE_Y *  8) //2048
#define VOXEL_TEXTURE_SIZE_Z 6 //2048

#define VOXEL_SLICE_SIZE_X (VOXEL_TEXTURE_SIZE_X / SCENE_VOXEL_SIZE_X) // 16
#define VOXEL_SLICE_SIZE_Y (VOXEL_TEXTURE_SIZE_Y / SCENE_VOXEL_SIZE_Y) //  8

struct SSimLuDirectionalLight
{
    float3 SunDirection;
    float3 SunIntensity;
};

struct SSimLuMeshCard
{
    uint card_start_index;

    float3 world_origin;
    float3x3 w2l_rotation;
};

struct SSimLuCard
{
    float3x3 meshcard_to_card_space_rotation;
    float3 mesh_card_space_orgin;
 
    uint2 atlas_card_index; // 0 -> 4096 / 128 (32)

    // local space, not world space or mesh space
    float2 card_extent_xy;
    float card_extent_z;
};

struct SSimLuSurfaceCacheData
{
    bool valid;
    float3 albedo;

    float3 world_position;
    float3 world_normal;
};

//todo:!!!!set volume center to (0,0,0)
struct SMeshSDFInfo
{
    float4x4 volume_to_world;
    float4x4 world_to_volume;

    // unsed variable
    // the center of the aabb, not the volume, aabb is smaller than volume
    //float3 mesh_sdf_aabb_center; // volume space, equal to volume position center
    //float3 mesh_sdf_aabb_extent;    // volume space

    float3 volume_position_center; // volume space
    float3 volume_position_extent; // volume space

    uint3 volume_brick_num_xyz;

    float volume_brick_size;
    uint volume_brick_start_idx;

    float2 sdf_distance_scale; // x : 2 * max distance , y : - max distance

    uint mesh_card_index;
};

struct SDFTraceResult
{
    bool bHit;
    uint hit_mesh_index;
    uint hit_mesh_card_idx;
    float hit_distance;
};


float Luminance( float3 color )
{
	return dot( color, float3( 0.3, 0.59, 0.11 ) );
}

uint PackRayInfo(uint2 TexelCoord, uint Level)
{
	// Pack in 16 bits
	return (TexelCoord.x & 0x3F) | ((TexelCoord.y & 0x3F) << 6) | ((Level & 0xF) << 12);
}

void UnpackRayInfo(uint RayInfo, out uint2 TexelCoord, out uint Level)
{
	TexelCoord.x = RayInfo & 0x3F;
	TexelCoord.y = (RayInfo >> 6) & 0x3F;
	Level = (RayInfo >> 12) & 0xF;
}

void GetProbeTracingUV(uint ray_info,out float2 probe_uv)
{
    uint2 ray_tex_coord;
    uint ray_level;
    UnpackRayInfo(ray_info, ray_tex_coord, ray_level);

    uint mip_size = 8 >> ray_level;
    probe_uv = ray_tex_coord / float2(mip_size,mip_size);
}

float3 EquiAreaSphericalMapping(float2 UV)
{
	UV = 2 * UV - 1;
	float D = 1 - (abs(UV.x) + abs(UV.y));
	float R = 1 - abs(D);
	float Phi = R == 0 ? 0 : (PI / 4) * ((abs(UV.y) - abs(UV.x)) / R + 1);
	float F = R * sqrt(2 - R * R);
	return float3(
		F * sign(UV.x) * abs(cos(Phi)),
		F * sign(UV.y) * abs(sin(Phi)),
		sign(D) * (1 - R * R)
	);
}

