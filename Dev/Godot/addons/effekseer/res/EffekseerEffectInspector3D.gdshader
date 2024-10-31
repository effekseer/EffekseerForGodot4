shader_type spatial;

const vec3 COLOR1 = vec3(0.1);
const vec3 COLOR2 = vec3(0.2);
varying vec3 v_world_pos;
varying vec3 v_world_nml;

void vertex() {
	VERTEX.y -= 0.0001;
	v_world_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
	v_world_nml = normalize((MODEL_MATRIX * vec4(NORMAL, 0.0)).xyz);
}

void fragment() {
	vec2 pos = v_world_pos.xy * v_world_nml.z +
		v_world_pos.xz * v_world_nml.y +
		v_world_pos.zy * vec2(-1.0, 1.0) * v_world_nml.x;
	pos *= 0.5;
	float ratio = abs(step(0.5, fract(pos.x)) - step(0.5, fract(pos.y)));
	ALBEDO = mix(COLOR1, COLOR2, ratio);
}
