@tool
extends PrimitiveMesh
class_name BrushMesh

@export var size: Vector3 = Vector3.ONE:
	get:
		return size
	set(value):
		size = value
		flip_faces = flip_faces

@export var uv_offset: Vector2 = Vector2.ZERO:
	get:
		return uv_offset
	set(value):
		uv_offset = value
		flip_faces = flip_faces

@export var uv_scale: Vector2 = Vector2.ONE:
	get:
		return uv_scale
	set(value):
		uv_scale = value
		flip_faces = flip_faces


func _create_mesh_array() -> Array:
	var points = [
		Vector3(-0.5,  0.5, -0.5), Vector3(-0.5,  0.5,  0.5),
		Vector3( 0.5,  0.5, -0.5), Vector3( 0.5,  0.5,  0.5),
		Vector3(-0.5, -0.5, -0.5), Vector3(-0.5, -0.5,  0.5),
		Vector3( 0.5, -0.5, -0.5), Vector3( 0.5, -0.5,  0.5),
	]
	var point_indices = [
		[1, 5, 3, 7], [2, 6, 0, 4],
		[3, 7, 2, 6], [0, 4, 1, 5],
		[0, 1, 2, 3], [5, 4, 7, 6],
	]
	var src_normals = [
		Vector3( 0,  0,  1), Vector3( 0,  0, -1), 
		Vector3( 1,  0,  0), Vector3(-1,  0,  0),
		Vector3( 0,  1,  0), Vector3( 0, -1,  0), 
	]
	var src_uvs = [
		Vector2(0, 0),
		Vector2(0, 1),
		Vector2(1, 0),
		Vector2(1, 1),
	]
	var uv_indices = [
		[0, 1], [0, 1], [2, 1], [2, 1], [0, 2], [0, 2]
	]
	var src_indices = [
		0, 2, 1, 3, 1, 2
	]
	
	var verts = PackedVector3Array()
	var normals = PackedVector3Array()
	var uvs = PackedVector2Array()
	var indices = PackedInt32Array()

	for f in range(6):
		var uv_index = uv_indices[f]
		for v in range(4):
			var point: Vector3 = points[point_indices[f][v]] * size
			var uv_pos = Vector2(point[uv_index[0]], point[uv_index[1]])
			verts.append(point)
			normals.append(src_normals[f])
			uvs.append(src_uvs[v] * uv_pos * uv_scale + uv_offset)
		for i in range(6):
			indices.append(f * 4 + src_indices[i])

	var vertex_array = []
	vertex_array.resize(Mesh.ARRAY_MAX)
	vertex_array[Mesh.ARRAY_VERTEX] = verts
	vertex_array[Mesh.ARRAY_NORMAL] = normals
	vertex_array[Mesh.ARRAY_TEX_UV] = uvs
	vertex_array[Mesh.ARRAY_INDEX] = indices
	
	return vertex_array
