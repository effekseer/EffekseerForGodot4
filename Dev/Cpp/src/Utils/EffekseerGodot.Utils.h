#pragma once

#include <stdint.h>
#include <Effekseer.h>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/classes/script.hpp>
#include <godot_cpp/classes/rendering_server.hpp>

namespace EffekseerGodot
{

union FP32
{
	float f;
	uint32_t u;
	struct
	{
		uint32_t Mantissa : 23;
		uint32_t Exponent : 8;
		uint32_t Sign : 1;
	};
};

union FP16
{
	uint16_t u;
	struct
	{
		uint32_t Mantissa : 10;
		uint32_t Exponent : 5;
		uint32_t Sign : 1;
	};
};

// from https://gist.github.com/rygorous/2156668
inline uint16_t ToHalfFloat(float value)
{
	FP32 f = { value };
	FP16 h = { 0 };

	if (f.Exponent == 0)
	{
		h.Exponent = 0;
	}
	else if (f.Exponent == 255)
	{
		h.Exponent = 31;
		h.Mantissa = f.Mantissa ? 0x200 : 0;
	}
	else
	{
		int newexp = f.Exponent - 127 + 15;
		if (newexp >= 31)
		{
			h.Exponent = 31;
		}
		else if (newexp <= 0)
		{
			if ((14 - newexp) <= 24)
			{
				uint32_t mant = f.Mantissa | 0x800000;
				h.Mantissa = mant >> (14 - newexp);
				if ((mant >> (13 - newexp)) & 1)
				{
					h.u++;
				}
			}
		}
		else
		{
			h.Exponent = newexp;
			h.Mantissa = f.Mantissa >> 13;
			if (f.Mantissa & 0x1000)
			{
				h.u++;
			}
		}
	}

	h.Sign = f.Sign;
	return h.u;
}

inline float ToHalfFloat(const float (&value)[2])
{
	FP32 bits;
	bits.u = ToHalfFloat(value[0]) | ToHalfFloat(value[1]) << 16;
	return bits.f;
}

inline void SafeReleaseRID(godot::RenderingServer* rs, godot::RID& rid)
{
	if (rid.is_valid())
	{
		rs->free_rid(rid);
		rid = godot::RID();
	}
}

inline int64_t RIDToInt64(godot::RID rid)
{
	int64_t val;
	memcpy(&val, &rid, sizeof(rid));
	return val;
}

inline godot::RID Int64ToRID(int64_t val)
{
	godot::RID rid;
	memcpy(&rid, &val, sizeof(rid));
	return rid;
}

inline Effekseer::Vector2D ToEfkVector2(godot::Vector2 v)
{
	return { v.x, v.y };
}

inline godot::Vector2 ToGdVector2(Effekseer::Vector2D v)
{
	return { v.X, v.Y };
}

inline godot::Vector2 ToGdVector2(const std::array<float, 2>& v)
{
	return godot::Vector2(v[0], v[1]);
}

inline Effekseer::Vector3D ToEfkVector3(godot::Vector3 v)
{
	return { v.x, v.y, v.z };
}

inline Effekseer::Vector3D ToEfkVector3(godot::Vector2 v)
{
	return { v.x, v.y, 0.0f };
}

inline godot::Vector3 ToGdVector3(Effekseer::Vector3D v)
{
	return { v.X, v.Y, v.Z };
}

struct SRT2D {
	godot::Vector2 scale;
	real_t rotation;
	godot::Vector2 translation;
};
inline SRT2D ToSRT(const godot::Transform2D& transform)
{
	SRT2D srt;

	srt.rotation = atan2(transform.columns[0].y, transform.columns[0].x);

	real_t cr = cos(-srt.rotation);
	real_t sr = sin(-srt.rotation);
	srt.scale.x = cr * transform.columns[0].x - sr * transform.columns[0].y;
	srt.scale.y = sr * transform.columns[1].x + cr * transform.columns[1].y;

	srt.translation.x = transform.columns[2].x;
	srt.translation.y = transform.columns[2].y;

	return srt;
}

inline Effekseer::Matrix44 ToEfkMatrix44(const godot::Transform3D& transform)
{
	Effekseer::Matrix44 matrix;
	matrix.Values[0][0] = transform.basis[0][0];
	matrix.Values[0][1] = transform.basis[1][0];
	matrix.Values[0][2] = transform.basis[2][0];
	matrix.Values[0][3] = 0.0f;
	matrix.Values[1][0] = transform.basis[0][1];
	matrix.Values[1][1] = transform.basis[1][1];
	matrix.Values[1][2] = transform.basis[2][1];
	matrix.Values[1][3] = 0.0f;
	matrix.Values[2][0] = transform.basis[0][2];
	matrix.Values[2][1] = transform.basis[1][2];
	matrix.Values[2][2] = transform.basis[2][2];
	matrix.Values[2][3] = 0.0f;
	matrix.Values[3][0] = transform.origin.x;
	matrix.Values[3][1] = transform.origin.y;
	matrix.Values[3][2] = transform.origin.z;
	matrix.Values[3][3] = 1.0f;
	return matrix;
}

inline Effekseer::Matrix44 ToEfkMatrix44(const godot::Transform2D& transform)
{
	Effekseer::Matrix44 matrix;
	matrix.Values[0][0] = transform.columns[0].x;
	matrix.Values[0][1] = transform.columns[0].y;
	matrix.Values[0][2] = 0.0f;
	matrix.Values[0][3] = 0.0f;
	matrix.Values[1][0] = transform.columns[1].x;
	matrix.Values[1][1] = transform.columns[1].y;
	matrix.Values[1][2] = 0.0f;
	matrix.Values[1][3] = 0.0f;
	matrix.Values[2][0] = 0.0f;
	matrix.Values[2][1] = 0.0f;
	matrix.Values[2][2] = 1.0f;
	matrix.Values[2][3] = 0.0f;
	matrix.Values[3][0] = transform.columns[2].x;
	matrix.Values[3][1] = transform.columns[2].y;
	matrix.Values[3][2] = 0.0f;
	matrix.Values[3][3] = 1.0f;
	return matrix;
}

inline Effekseer::Matrix43 ToEfkMatrix43(const godot::Transform3D& transform)
{
	Effekseer::Matrix43 matrix;
	matrix.Value[0][0] = transform.basis[0][0];
	matrix.Value[0][1] = transform.basis[1][0];
	matrix.Value[0][2] = transform.basis[2][0];
	matrix.Value[1][0] = transform.basis[0][1];
	matrix.Value[1][1] = transform.basis[1][1];
	matrix.Value[1][2] = transform.basis[2][1];
	matrix.Value[2][0] = transform.basis[0][2];
	matrix.Value[2][1] = transform.basis[1][2];
	matrix.Value[2][2] = transform.basis[2][2];
	matrix.Value[3][0] = transform.origin.x;
	matrix.Value[3][1] = transform.origin.y;
	matrix.Value[3][2] = transform.origin.z;
	return matrix;
}

inline Effekseer::Matrix43 ToEfkMatrix43(const godot::Transform2D& transform, 
	const godot::Vector3& orientation, bool flipH, bool flipV)
{
	using namespace Effekseer::SIMD;

	auto srt = ToSRT(transform);

	// Invert XY by flip or negative scale
	float scaleX = (flipH ^ (srt.scale.x < 0.0f)) ? -1.0f : 1.0f;
	float scaleY = (flipV ^ (srt.scale.y < 0.0f)) ? -1.0f : 1.0f;
	
	// Invalidate scale (Apply scale at rendering)
	float translationX = srt.translation.x / abs(srt.scale.x);
	float translationY = srt.translation.y / abs(srt.scale.y);

	Mat43f transformMatrix = Mat43f::SRT({scaleX, scaleY, 1.0f},
		Mat43f::RotationZ(srt.rotation), 
		{translationX, translationY, 0.0f});

	Mat43f orientationMatrix = Mat43f::RotationZXY(orientation.z, orientation.x, orientation.y);
	
	// Multiply and Convert
	return ToStruct(orientationMatrix * transformMatrix);
}

inline godot::Transform3D ToGdTransform3D(Effekseer::Matrix44 matrix)
{
	godot::Transform3D transform;
	transform.basis[0][0] = matrix.Values[0][0];
	transform.basis[1][0] = matrix.Values[0][1];
	transform.basis[2][0] = matrix.Values[0][2];
	transform.basis[0][1] = matrix.Values[1][0];
	transform.basis[1][1] = matrix.Values[1][1];
	transform.basis[2][1] = matrix.Values[1][2];
	transform.basis[0][2] = matrix.Values[2][0];
	transform.basis[1][2] = matrix.Values[2][1];
	transform.basis[2][2] = matrix.Values[2][2];
	transform.origin.x = matrix.Values[3][0];
	transform.origin.y = matrix.Values[3][1];
	transform.origin.z = matrix.Values[3][2];
	return transform;
}

inline godot::Transform3D ToGdTransform3D(Effekseer::Matrix43 matrix)
{
	godot::Transform3D transform;
	transform.basis[0][0] = matrix.Value[0][0];
	transform.basis[1][0] = matrix.Value[0][1];
	transform.basis[2][0] = matrix.Value[0][2];
	transform.basis[0][1] = matrix.Value[1][0];
	transform.basis[1][1] = matrix.Value[1][1];
	transform.basis[2][1] = matrix.Value[1][2];
	transform.basis[0][2] = matrix.Value[2][0];
	transform.basis[1][2] = matrix.Value[2][1];
	transform.basis[2][2] = matrix.Value[2][2];
	transform.origin.x = matrix.Value[3][0];
	transform.origin.y = matrix.Value[3][1];
	transform.origin.z = matrix.Value[3][2];
	return transform;
}

inline void ToGdMatrix4(float* mem, const Effekseer::Matrix44& matrix)
{
	using namespace Effekseer::SIMD;
	Mat44f mat(matrix);
	memcpy(mem, &matrix, sizeof(matrix));
	//Float4::Store4(&mem[4 * 0], mat.X);
	//Float4::Store4(&mem[4 * 1], mat.Y);
	//Float4::Store4(&mem[4 * 2], mat.Z);
	//Float4::Store4(&mem[4 * 3], mat.W);
}

inline Effekseer::Color ToEfkColor(godot::Color c)
{
	return {
		(uint8_t)Effekseer::Clamp((int)(c.r * 255.0f), 255, 0),
		(uint8_t)Effekseer::Clamp((int)(c.g * 255.0f), 255, 0),
		(uint8_t)Effekseer::Clamp((int)(c.b * 255.0f), 255, 0),
		(uint8_t)Effekseer::Clamp((int)(c.a * 255.0f), 255, 0),
	};
}

inline godot::Color ToGdColor(Effekseer::Color c)
{
	return { c.R / 255.0f, c.G / 255.0f, c.B / 255.0f, c.A / 255.0f };
}

inline godot::Color ToGdColor(const float (&c)[4])
{
	return { c[0], c[1], c[2], c[3] };
}

inline godot::Color ToGdColor(uint32_t c)
{
	return { (uint8_t)(c >> 24) / 255.0f, (uint8_t)(c >> 16) / 255.0f, (uint8_t)(c >> 8) / 255.0f, (uint8_t)(c) / 255.0f };
}

inline void ToGdBuffer(float* dst, Effekseer::Matrix44 matrix)
{
	dst[0] = matrix.Values[0][0];
	dst[1] = matrix.Values[1][0];
	dst[2] = matrix.Values[2][0];
	dst[3] = matrix.Values[3][0];
	dst[4] = matrix.Values[0][1];
	dst[5] = matrix.Values[1][1];
	dst[6] = matrix.Values[2][1];
	dst[7] = matrix.Values[3][1];
	dst[8] = matrix.Values[0][2];
	dst[9] = matrix.Values[1][2];
	dst[10] = matrix.Values[2][2];
	dst[11] = matrix.Values[3][2];
}

inline void ToGdBuffer(float* dst, const float (&c)[4])
{
	dst[0] = c[0];
	dst[1] = c[1];
	dst[2] = c[2];
	dst[3] = c[3];
}

size_t ToEfkString(char16_t* to, const godot::String& from, size_t size);

godot::String ToGdString(const char16_t* from);

godot::Variant ScriptNew(godot::Ref<godot::Script> script);


//----------------------------
// for RenderingServer
//----------------------------

struct GdVertexPos2D {
	Effekseer::Vector2D pos;
};

struct GdVertexPos3D {
	Effekseer::Vector3D pos;
};

// For 3D
struct GdVertexNrmTan {
	uint32_t normal;
	uint32_t tangent;
};

struct GdAttributeColUV {
	Effekseer::Color color;
	Effekseer::Vector2D uv;
};

// For 2D
struct GdAttributeNrmTan {
	uint32_t normal;
	uint32_t tangent;
};

struct GdAttributeAdvance {
	std::array<uint16_t, 12> advance;
};

struct GdAttributeCustom {
	std::array<uint16_t, 8> custom;
};

struct GdMeshVertexPos {
	Effekseer::Vector3D pos;
};
struct GdMeshVertexNrmTan {
	uint32_t normal;
	uint32_t tangent;
};

struct GdMeshAttribute {
	Effekseer::Color color;
	Effekseer::Vector2D uv;

	float depth;      // For 2D
	uint32_t normal;  // For 2D 
	uint32_t tangent; // For 2D
};

uint32_t ToGdNormal(const Effekseer::Vector3D& v);

uint32_t ToGdTangent(const Effekseer::Vector3D& v);

uint32_t ToGdNormal(const Effekseer::Color& v);

uint32_t ToGdTangent(const Effekseer::Color& v);

}
