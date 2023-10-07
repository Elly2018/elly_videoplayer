extends Node

@export var play_on_start: bool;
@export var loop: bool;
@export var uri: String = "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4";
@export var geo: GeometryInstance3D;
@export var te: TextureRect;

@export_group("Media Source")
@export var player: FFmpegMediaPlayer;
@export var audio_stream: AudioStreamPlayer;

# Big Buck mp4
# http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4
# Big Buck 480p
# https://test-streams.mux.dev/x36xhzz/url_6/193039199_mp4_h264_aac_hq_7.m3u8
# Big Buck ABR
# https://test-streams.mux.dev/x36xhzz/x36xhzz.m3u8

var mat: Material
var phase = 0.0

func _ready():
	if(geo != null):
		mat = geo.material_override
	player.set_player(audio_stream);
	player.set_loop(loop);
	if (play_on_start):
		player.load_path(uri);
		player.play();
		player.audio_init();
	
func texture_update(tex:ImageTexture, size:Vector2i):
	# vieport.size = size;
	if (mat != null):
		mat.set_deferred("shader_parameter/tex", tex);
	if (te != null):
		te.set_deferred("texture", tex);
		
func audio_update(data:PackedFloat32Array, size:int, channel:int):
	pass
		
func audio_volumn(p:float):
	audio_stream.volume_db = p;
	
func pause_trigger():
	print("Pause trigger")
	player.set_paused(!player.is_paused())
	
func play_trigger():
	print("Play trigger")
	player.play()
	
func stop_trigger():
	print("Stop trigger")
	player.stop()
	
func load_trigger(p:String):
	print("Loading: ", p)
	player.load_path(p);
	player.play();
	player.audio_init();
		
func async_load_finish(result):
	print("Loading result: ", result)
	player.play()
	
func message_feedback(m:String):
	print(m)

func error_feedback(m:String):
	printerr(m)
	
