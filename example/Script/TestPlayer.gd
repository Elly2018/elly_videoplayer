extends Node

@export var enable: bool;
@export var geo: GeometryInstance3D;
@export var player: FFmpegNode;
@export var audio_stream: AudioStreamPlayer;

const uri:String = "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4";

var mat: Material
var phase = 0.0

func _ready():
	if (!enable):
		return
	mat = geo.material_override
	player.set_player(audio_stream);
	player.load_path(uri)
	player.play()
	
func texture_update(tex):
	mat.set_deferred("shader_parameter/tex", tex);
		
func audio_update(data:PackedFloat32Array, size:int, channel:int):
	pass
		
func async_load_finish(result):
	print("Loading result: ", result)
	player.play()
	
func message_feedback(m):
	print(m)

func error_feedback(m):
	printerr(m)
	
