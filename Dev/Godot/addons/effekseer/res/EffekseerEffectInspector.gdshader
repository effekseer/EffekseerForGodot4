shader_type spatial;

void fragment() {
	vec2 pos = UV * 8.0;
	float lum = abs(step(0.5, fract(pos.x)) - step(0.5, fract(pos.y))) * 0.1 + 0.05;
	ALBEDO = vec3(lum);
}
