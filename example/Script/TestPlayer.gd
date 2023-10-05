extends Node

@export var play_on_start: bool;
@export var loop: bool;
@export var geo: GeometryInstance3D;
@export var v: SubViewport;
@export var plane: TextureRect;
@export var player: FFmpegMediaPlayer;
@export var audio_stream: AudioStreamPlayer;


@export var uri: String;
# const uri:String = "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4";

var mat: Material
var phase = 0.0

func _ready():
	mat = geo.material_override
	player.set_player(audio_stream);
	player.set_loop(loop);
	if (play_on_start):
		player.load_path(uri);
		player.play();
	player.audio_init();
	
func texture_update(tex, size):
	v.size = size;
	mat.set_deferred("shader_parameter/tex", tex);
	plane.set_deferred("texture", tex);
		
func audio_update(data:PackedFloat32Array, size:int, channel:int):
	pass
		
func async_load_finish(result):
	print("Loading result: ", result)
	player.play()
	
func message_feedback(m):
	print(m)

func error_feedback(m):
	printerr(m)
	
