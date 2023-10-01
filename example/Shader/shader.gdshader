shader_type spatial;

uniform sampler2D tex;

void fragment() {
	ALBEDO = texture(tex, vec2(-UV.x, UV.y)).rgb;
}
