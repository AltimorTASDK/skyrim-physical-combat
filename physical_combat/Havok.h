#pragma once

#include <skse64/NiTypes.h>

constexpr unsigned int HK_INVALID_SHAPE_KEY = -1;

class hkpWorldRayCaster;
class hkpCollidable;
class hkpCdBody;

#pragma pack(push, 4)

// 10
class hkVector4 {
public:
	float	x;	// 0
	float	y;	// 4
	float	z;	// 8
	float	w;	// C

	hkVector4();
	hkVector4(float X, float Y, float Z) : x(X), y(Y), z(Z), w(0) { };
	hkVector4(float X, float Y, float Z, float W) : x(X), y(Y), z(Z), w(W) { };
	hkVector4(const NiPoint3 &p) : x(p.x), y(p.y), z(p.z), w(0) { };

	// Negative
	hkVector4 operator- () const;

	// Basic operations
	hkVector4 operator+ (const hkVector4& pt) const;
	hkVector4 operator- (const hkVector4& pt) const;

	hkVector4& operator+= (const hkVector4& pt);
	hkVector4& operator-= (const hkVector4& pt);

	// Scalar operations
	hkVector4 operator* (float fScalar) const;
	hkVector4 operator/ (float fScalar) const;

	hkVector4& operator*= (float fScalar);
	hkVector4& operator/= (float fScalar);
};

// 30
struct hkpWorldRayCastInput {
	hkVector4	m_from;							// 00
	hkVector4	m_to;							// 10
	bool		m_enableShapeCollectionFilter;	// 20
	UInt32		m_filterInfo;					// 24
	UInt64		m_userData;						// 28
};
STATIC_ASSERT(sizeof(hkpWorldRayCastInput) == 0x30);

// 20
struct hkpShapeRayCastCollectorOutput {
	hkpShapeRayCastCollectorOutput() : m_hitFraction(1.f), m_extraInfo(-1) { }

	hkVector4	m_normal;		// 00
	float		m_hitFraction;	// 10
	SInt64		m_extraInfo;	// 14
	SInt32		m_pad;			// 1C
};
STATIC_ASSERT(sizeof(hkpShapeRayCastCollectorOutput) == 0x20);

// 50
struct hkpShapeRayCastOutput : public hkpShapeRayCastCollectorOutput {
	hkpShapeRayCastOutput() : m_shapeKeys { HK_INVALID_SHAPE_KEY }, m_shapeKeyIndex(0) { }

	static constexpr auto MAX_HIERARCHY_DEPTH = 8;

	UInt32		m_shapeKeys[MAX_HIERARCHY_DEPTH];	// 20
	SInt32		m_shapeKeyIndex;					// 40
	SInt32		m_pad[3];							// 44
};
STATIC_ASSERT(sizeof(hkpShapeRayCastOutput) == 0x50);

// 60
struct hkpWorldRayCastOutput : public hkpShapeRayCastOutput {
	hkpWorldRayCastOutput() : m_rootCollidable(nullptr) { }

	hkpCollidable	*m_rootCollidable;	// 50
	SInt32			m_pad[2];			// 58
};
STATIC_ASSERT(sizeof(hkpWorldRayCastOutput) == 0x60);

// 10
struct hkpRayHitCollector {
	virtual ~hkpRayHitCollector();
	virtual void addRayHit(
		const hkpCdBody &cdBody,
		const hkpShapeRayCastCollectorOutput &hitInfo) = 0;

	float	m_earlyOutHitFraction;	// 08
	SInt32	m_pad;					// 0C
};
STATIC_ASSERT(sizeof(hkpRayHitCollector) == 0x10);

// 70
struct hkpClosestRayHitCollector : public hkpRayHitCollector {
	hkpWorldRayCastOutput	m_rayHit;	// 10
};
STATIC_ASSERT(sizeof(hkpClosestRayHitCollector) == 0x70);

// 10
template<typename T>
struct hkArray {
	hkArray() : m_data(nullptr), m_size(0), m_capacityAndFlags(0) { }

	T	*m_data;			// 00
	int	m_size;				// 08
	int	m_capacityAndFlags;	// 0C
};
STATIC_ASSERT(sizeof(hkArray<void>) == 0x10);

template<typename T, int N>
struct hkInplaceArray : public hkArray<T> {
	hkInplaceArray() : hkArray(), m_data(m_storage) {}

	T	m_storage[N];	// 10
};

// 320
struct hkpAllRayHitCollector : public hkpRayHitCollector {
	hkInplaceArray<hkpWorldRayCastOutput, 8>	m_hits;	// 10
};
STATIC_ASSERT(sizeof(hkpAllRayHitCollector) == 0x320);

#pragma pack(pop)
