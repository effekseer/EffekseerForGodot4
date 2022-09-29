
//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
#include <godot_cpp/classes/rendering_server.hpp>
#include "../Utils/EffekseerGodot.Utils.h"
#include "EffekseerGodot.RenderResources.h"

//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
namespace EffekseerGodot
{
	
Model::Model(const Effekseer::CustomVector<Vertex>& vertecies, const Effekseer::CustomVector<Face>& faces)
	: Effekseer::Model(vertecies, faces)
{
	UploadToEngine();
}

Model::Model(const void* data, int32_t size)
	: Effekseer::Model(data, size)
{
	UploadToEngine();
}

Model::~Model()
{
	using namespace godot;

	if (meshRid_.is_valid())
	{
		auto rs = RenderingServer::get_singleton();
		rs->free_rid(meshRid_);
	}
}

void Model::UploadToEngine()
{
	using namespace Effekseer;
	using namespace Effekseer::SIMD;
	using namespace EffekseerGodot;
	using namespace godot;

	const int32_t vertexCount = GetVertexCount();
	const int32_t faceCount = GetFaceCount();
	const int32_t indexCount = faceCount * 3;

	Vec3f aabbMin{}, aabbMax{};

	uint32_t format = RenderingServer::ARRAY_FORMAT_VERTEX |
		RenderingServer::ARRAY_FORMAT_NORMAL |
		RenderingServer::ARRAY_FORMAT_TANGENT |
		RenderingServer::ARRAY_FORMAT_COLOR |
		RenderingServer::ARRAY_FORMAT_TEX_UV;

	PackedByteArray vertexData;
	PackedByteArray attributeData;

	vertexData.resize(vertexCount * sizeof(GdLitVertex));
	attributeData.resize(vertexCount * sizeof(GdAttribute));

	GdLitVertex* dstVertex = (GdLitVertex*)vertexData.ptrw();
	GdAttribute* dstAttribute = (GdAttribute*)attributeData.ptrw();

	const Vertex* srcVertex = GetVertexes();

	aabbMin = aabbMax = srcVertex[0].Position;
	for (int32_t i = 0; i < vertexCount; i++)
	{
		auto& v = *srcVertex++;
		*dstVertex++ = GdLitVertex{ v.Position, ToGdNormal(v.Normal), ToGdTangent(v.Tangent) };
		*dstAttribute++ = GdAttribute{ v.VColor, v.UV };

		aabbMin = Vec3f::Min(aabbMin, v.Position);
		aabbMax = Vec3f::Max(aabbMax, v.Position);
	}

	PackedByteArray indexData;
	indexData.resize(indexCount * sizeof(uint16_t));
	uint16_t* dstIndex = (uint16_t*)indexData.ptrw();

	const Face* srcFaces = GetFaces();
	for (size_t i = 0; i < faceCount; i++)
	{
		auto& f = srcFaces[i];
		dstIndex[0] = (uint16_t)(f.Indexes[0]);
		dstIndex[1] = (uint16_t)(f.Indexes[1]);
		dstIndex[2] = (uint16_t)(f.Indexes[2]);
		dstIndex += 3;
	}

	AABB aabb;
	Vec3f::Store(&aabb.position, aabbMin);
	Vec3f::Store(&aabb.size, aabbMax - aabbMin);

	Dictionary surface;
	surface["primitive"] = RenderingServer::PRIMITIVE_TRIANGLES;
	surface["format"] = format;
	surface["vertex_data"] = vertexData;
	surface["attribute_data"] = attributeData;
	surface["vertex_count"] = (int)vertexCount;
	surface["index_data"] = indexData;
	surface["index_count"] = (int)indexCount;
	surface["aabb"] = aabb;

	auto rs = RenderingServer::get_singleton();
	meshRid_ = rs->mesh_create();
	rs->mesh_add_surface(meshRid_, surface);
}

} // namespace EffekseerGodot
