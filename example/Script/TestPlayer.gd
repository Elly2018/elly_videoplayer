extends Node

@export var enable: bool;
@export var geo: GeometryInstance3D;
@export var player: FFmpegNode;
@export var audio_stream: AudioStreamPlayer;
var audio_stream_Gen: AudioStreamGenerator;
var audio_stream_Gen_playback: AudioStreamPlayback;

# const uri:String = "https://vrvod-funique.cdn.hinet.net/FuniqueDemo/edu/0001/HD/HD.m3u8";
# const uri:String = "C:/Users/chuel/Videos/Desktop/amatista-studio_30-art.wmv";
const uri:String = "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4";

var mat: Material
var phase = 0.0

func _ready():
	if (!enable):
		return
	mat = geo.material_override
	audio_stream_Gen = audio_stream.stream
	player.set_player(audio_stream)
	player.set_gen_streamer(audio_stream_Gen)
	player.set_loop(true)
	player.load_path(uri)
	player.play()
	audio_stream.play()
	audio_stream_Gen_playback = audio_stream.get_stream_playback()
	player.set_gen_streamer_playback(audio_stream_Gen_playback)
	fill_buffer()
	
func _process(_delta):
	# fill_buffer();
	if (!enable):
		return
	var tex = player.get_video_texture();
	mat.set_deferred("shader_parameter/tex", tex);
	

func fill_buffer():
	var increment = 100.0 / 44100;
	var c = audio_stream_Gen_playback.get_frames_available()
	if(c != 0):
		print(c)
	while c > 0:
		var f = sin(phase * TAU);
		audio_stream_Gen_playback.push_frame(Vector2(1.0,1.0) * f);
		phase = fmod(phase + increment, 1.0)
		c -= 1
		
func _load_finish(result):
	print("Loading result: ", result)
	player.play()
	
func message_feedback(m):
	print(m)

func error_feedback(m):
	printerr(m)
	
func push_audio(m):
	# print(m)
	audio_stream_Gen_playback.push_frame(m);
	
