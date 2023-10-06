shader_type spatial;

uniform bool flip;
uniform int mode;
uniform sampler2D tex;

void fragment() {
	if (mode == 0){ // No stereo
		ALBEDO = texture(tex, vec2(-UV.x, UV.y)).rgb;
	}
	else{
		vec2 r = vec2(0,0);
		if(mode == 1){ // Top down
			bool left = UV.y > 0.5;
			left = flip ? !left : left;
			if(left){
				r = vec2(UV.x, UV.y * 0.5 + 0.5);
			}else{
				r = vec2(UV.x, UV.y * 0.5);
			}
		}else if (mode == 2){ // Side by side
			bool left = UV.x < 0.5;
			left = flip ? !left : left;
			if(left){
				r = vec2(UV.x * 0.5, UV.y);
			}else{
				r = vec2(UV.x * 0.5 + 0.5, UV.y);
			}
		}
		ALBEDO = texture(tex, r).rgb;
	}
}
