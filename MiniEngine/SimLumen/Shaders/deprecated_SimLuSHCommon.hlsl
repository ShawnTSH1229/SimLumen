struct SThreeBandSH
{
	half4 V0; // 1 + 3
	half4 V1; // 5
	half V2;
};


SThreeBandSH SHBasisFunction3(half3 InputVector)
{
	SThreeBandSH Result;
	// These are derived from simplifying SHBasisFunction in C++
	Result.V0.x = 0.282095f; 
	Result.V0.y = -0.488603f * InputVector.y;
	Result.V0.z = 0.488603f * InputVector.z;
	Result.V0.w = -0.488603f * InputVector.x;

	half3 VectorSquared = InputVector * InputVector;
	Result.V1.x = 1.092548f * InputVector.x * InputVector.y;
	Result.V1.y = -1.092548f * InputVector.y * InputVector.z;
	Result.V1.z = 0.315392f * (3.0f * VectorSquared.z - 1.0f);
	Result.V1.w = -1.092548f * InputVector.x * InputVector.z;
	Result.V2 = 0.546274f * (VectorSquared.x - VectorSquared.y);

	return Result;
}
SThreeBandSH CalcDiffuseTransferSH3(half3 Normal,half Exponent)
{
	SThreeBandSH Result = SHBasisFunction3(Normal);

	// These formula are scaling factors for each SH band that convolve a SH with the circularly symmetric function
	// max(0,cos(theta))^Exponent
	half L0 =					2 * PI / (1 + 1 * Exponent						);
	half L1 =					2 * PI / (2 + 1 * Exponent						);
	half L2 = Exponent *		2 * PI / (3 + 4 * Exponent + Exponent * Exponent);
	half L3 = (Exponent - 1) *	2 * PI / (8 + 6 * Exponent + Exponent * Exponent);

	// Multiply the coefficients in each band with the appropriate band scaling factor.
	Result.V0.x *= L0;
	Result.V0.yzw *= L1;
	Result.V1.xyzw *= L2;
	Result.V2 *= L2;
	return Result;
}

half DotSH3(SThreeBandSH A,SThreeBandSH B)
{
	half Result = dot(A.V0, B.V0);
	Result += dot(A.V1, B.V1);
	Result += A.V2 * B.V2;
	return Result;
}

SThreeBandSH AddSH(SThreeBandSH A, SThreeBandSH B)
{
	SThreeBandSH Result = A;
	Result.V0 += B.V0;
	Result.V1 += B.V1;
	Result.V2 += B.V2;
	return Result;
}