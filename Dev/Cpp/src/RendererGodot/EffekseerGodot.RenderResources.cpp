
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
	using namespace godot;

	int32_t vertexCount = GetVertexCount();
	const Vertex* vertexData = GetVertexes();
	int32_t faceCount = GetFaceCount();
	const Face* faceData = GetFaces();

	PackedVector3Array positions; positions.resize(vertexCount);
	PackedVector3Array normals; normals.resize(vertexCount);
	PackedFloat32Array tangents; tangents.resize(vertexCount * 4);
	PackedColorArray colors; colors.resize(vertexCount);
	PackedVector2Array texUVs; texUVs.resize(vertexCount);
	PackedInt32Array indeces; indeces.resize(faceCount * 3);

	for (int32_t i = 0; i < vertexCount; i++)
	{
		positions.set(i, ToGdVector3(vertexData[i].Position));
		normals.set(i, ToGdVector3(vertexData[i].Normal));
		tangents.set(i * 4 + 0, vertexData[i].Tangent.X);
		tangents.set(i * 4 + 1, vertexData[i].Tangent.Y);
		tangents.set(i * 4 + 2, vertexData[i].Tangent.Z);
		tangents.set(i * 4 + 3, 1.0f);
		colors.set(i, ToGdColor(vertexData[i].VColor));
		texUVs.set(i, ToGdVector2(vertexData[i].UV));
	}
	for (int32_t i = 0; i < faceCount; i++)
	{
		indeces.set(i * 3 + 0, faceData[i].Indexes[0]);
		indeces.set(i * 3 + 1, faceData[i].Indexes[1]);
		indeces.set(i * 3 + 2, faceData[i].Indexes[2]);
	}

	Array arrays;
	arrays.resize(RenderingServer::ARRAY_MAX);
	arrays[RenderingServer::ARRAY_VERTEX] = positions;
	arrays[RenderingServer::ARRAY_NORMAL] = normals;
	arrays[RenderingServer::ARRAY_TANGENT] = tangents;
	arrays[RenderingServer::ARRAY_COLOR] = colors;
	arrays[RenderingServer::ARRAY_TEX_UV] = texUVs;
	arrays[RenderingServer::ARRAY_INDEX] = indeces;

	auto vs = RenderingServer::get_singleton();
	meshRid_ = vs->mesh_create();
	vs->mesh_add_surface_from_arrays(meshRid_, RenderingServer::PRIMITIVE_TRIANGLES, arrays);
}

} // namespace EffekseerGodot
