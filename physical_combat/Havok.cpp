#include "Havok.h"

hkVector4::hkVector4()
{
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
	w = 0.0f;
}

hkVector4 hkVector4::operator- () const
{
	return hkVector4(-x, -y, -z, -w);
}

hkVector4 hkVector4::operator+ (const hkVector4& pt) const
{
	return hkVector4(x + pt.x, y + pt.y, z + pt.z, w + pt.w);
}

hkVector4 hkVector4::operator- (const hkVector4& pt) const
{
	return hkVector4(x - pt.x, y - pt.y, z - pt.z, w - pt.w);
}

hkVector4& hkVector4::operator+= (const hkVector4& pt)
{
	x += pt.x;
	y += pt.y;
	z += pt.z;
	w += pt.w;
	return *this;
}
hkVector4& hkVector4::operator-= (const hkVector4& pt)
{
	x -= pt.x;
	y -= pt.y;
	z -= pt.z;
	z -= pt.w;
	return *this;
}

// Scalar operations
hkVector4 hkVector4::operator* (float scalar) const
{
	return hkVector4(scalar * x, scalar * y, scalar * z, scalar * w);
}
hkVector4 hkVector4::operator/ (float scalar) const
{
	float invScalar = 1.0f / scalar;
	return hkVector4(invScalar * x, invScalar * y, invScalar * z, invScalar * w);
}

hkVector4& hkVector4::operator*= (float scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
	w *= scalar;
	return *this;
}
hkVector4& hkVector4::operator/= (float scalar)
{
	float invScalar = 1.0f / scalar;
	x *= invScalar;
	y *= invScalar;
	z *= invScalar;
	w *= invScalar;
	return *this;
}
