shader_type canvas_item;
render_mode unshaded;

const vec3 COLOR1 = vec3(0.3);
const vec3 COLOR2 = vec3(0.4);
varying vec2 v_world_pos;

void vertex() {
	v_world_pos = (MODEL_MATRIX * vec4(VERTEX, 0.0, 1.0)).xy;
}

void fragment() {
	vec2 pos = v_world_pos.xy * 0.5;
	float ratio = abs(step(0.5, fract(pos.x)) - step(0.5, fract(pos.y)));
	COLOR = vec4(mix(COLOR1, COLOR2, ratio), 1.0);
}
