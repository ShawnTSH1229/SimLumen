# SimLumen[![License](https://img.shields.io/github/license/ShawnTSH1229/SimLumen.svg)](https://github.com/ShawnTSH1229/SimLumen/blob/master/LICENSE)
Simplified Unreal Lumen Implementation in MiniEngine

<p align="center">
    <img src="/Resources/logo.png" width="60%" height="60%">
</p>

[<u>**SimLumen Development Blog**</u>](https://shawntsh1229.github.io/2024/05/18/Simplified-Lumen-GI-In-MiniEngine/)

# Introduction

This is a simplified Unreal Lumen GI implementation (**SimLumen**) based on **Unreal's Lumen GI**. We have implemented **most** of Unreal Lumen's features.

To perform fast ray tracing, SimLumen builds the **mesh SDFs** offline using the **embree** library. We also precompute a **global low resolution SDF** of the whole scene, which is used in surface cache ray tracing and screen probe voxel ray tracing.

SimLumen builds **mesh cards** offline in order to capture **material attributes** (normal, albedo) at run time. Mesh cards store the capture direction as well as the camera capture frustum. Since our meshes in the test example are simple and most are boxes, we generate only 6 cards for each mesh. Each direction corresponds a mesh card. At run time, SimLumen captures the mesh attributes, and copies them into a **global surface cache material attributes atlas**.

The surface cache describes the **lighting of the scene**. It contains 5 parts: **surface cache material attributes**, **surface cache direct lighting**, **surface cache indirect lighting**, **surface cache combined final lighting** and **voxelized scene lighting**.

With the global surface cache material attributes (normal, albedo and depth), SimLumen computes the **direct lighting** for each pixel in the surface cache atlas.

What's more, we have implemented **infinity** bounce lighting similar to Unreal Lumen. At first, we **voxelize** the scene. Each voxel has 6 directions. For each direction, we perform a mesh SDF trace and store the **hit mesh index and hit distance** in the **voxel visibility buffer**. Then, we **inject** the surface cache final lighting into the voxel if the voxel hit a mesh.

With the voxelized lighting, we compute the **surface cache indirect lighting** in the surface cache Atlas space. Firstly, SimLumen places probes every **4x4 pixels** in the Atlas space. In the next step, we trace the ray to the voxelized scene via global SDF and sample the radiance within the voxel. In order to denoise the trace result, SimLumen **filters** the radiance atlas and converts them into **spherical harmonics**. By integrating the probes around the pixel, we obtained surface cache indirect lighting.

The surface cache final lighting is computed by **combining surface direct and indirect lighting**.

As we have SDF to trace the scene quickly as well as surface cache that describes the scene lighting, we are able to proform the **screen space probe trace**.

SimLumen uses **importance sampling** to reduce the trace noisy. The PDF of the sampling function contains two parts: **BRDF PDF** and **lighting PDF**. The BRDF PDF is stored in **spherical harmonic** form, and we project the pixel BRDF PDF around the probe into spherical harmonics if the PDF is **not rejected** by the plane depth weight.  We use the **previous frame's** screen radiance result to estimate the lighting PDF by **reprojecting** the probe into the previous screen space, since we do not have information about the lighting source in the current frame. To improve performance, SimLumen employs **structured importance sampling** by reassigning the unimportant samples to those with a higher PDF.

Each probe traces 64 rays to the scene. SimLumen implements a **hybrid GI** similar to Unreal Lumen. The probes whose distance from the camera is less than 100 meters trace the scene by **mesh SDF** and sample the radiance from the **surface cache atlas**. Other probes use **Global SDF** to trace the scene and sample the radiance from the **voxel lighting**.

After that, we perform two additional passes to denoise the results. In the first pass, we **filter** the radiance with a uniform weight. Then, in the second pass, we convert the radiance into **spherical harmonics** and transform the SH into boardered **octchedron form**. This is usefull in hardware **bilinear sampling** in the following pass.

We finally obtained the final indirect lighting by integrating the probes around the current screen pixel and sampling the **octchedron form SH** by a linear sampler.


# Getting Started

1.Cloning the repository with `git clone https://github.com/ShawnTSH1229/SimLumen.git`.

2.Decompress the file: MiniEngine\SimLumen\Assets\erato.rar

2.Compile and run the visual studio solution located in `SimLumen\MiniEngine\SimLumen\SimLumen.sln`

# Source Tree

<p align="left">
    <img src="/Resource/source_tree_shader.png" width="60%" height="60%">
</p>
