extends Node

@export var play_on_start: bool;
@export var loop: bool;
@export var uri: String = "https://test-streams.mux.dev/x36xhzz/url_6/193039199_mp4_h264_aac_hq_7.m3u8";
@export var geo: GeometryInstance3D;
@export var texture_rect: TextureRect;

@export_group("Media Source")
@export var player: FFmpegMediaPlayer;
@export var audio_stream: AudioStreamPlayer;

# Big Buck mp4
# http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4
# Big Buck 480p
# https://test-streams.mux.dev/x36xhzz/url_6/193039199_mp4_h264_aac_hq_7.m3u8
# Big Buck ABR
# https://test-streams.mux.dev/x36xhzz/x36xhzz.m3u8
# Elephants Dream
# http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ElephantsDream.mp4
# Tear of steel (audio channel test)
# https://storage.googleapis.com/gtv-videos-bucket/sample/TearsOfSteel.mp4
# Bitbop ball for sync test
# http://devimages.apple.com/iphone/samples/bipbop/bipbopall.m3u8
var mat: Material
var aspect: float
var rootInterface:Control
var phase = 0.0

var current_size: Vector2i = Vector2i(1, 1)

func _ready():
	if(geo != null):
		mat = geo.material_override
	if(texture_rect != null):
		rootInterface = texture_rect.get_parent_control()
	player.set_player(audio_stream);
	player.set_loop(loop);
	if (play_on_start):
		player.load_path(uri);
		player.play()

func _process(delta):
	_update_size();

func _update_size():
	aspect = float(current_size.x) / float(current_size.y);
	var root_size = rootInterface.get_rect().size
	var root_aspect = root_size.x / root_size.y
	if current_size == Vector2i(1, 1):
		texture_rect.size = Vector2(root_size.x, root_size.y)
		texture_rect.position = Vector2(0,0)
		return	
	if root_aspect > aspect:
		# Fit height
		texture_rect.size = Vector2(root_size.y * aspect, root_size.y)
		texture_rect.position = Vector2((root_size.x - (root_size.y * aspect)) / 2.0, 0.0)
	else:
		# Fit width
		texture_rect.size = Vector2(root_size.x, root_size.x / aspect)
		texture_rect.position = Vector2(0.0, (root_size.y - (root_size.x / aspect)) / 2.0)

func texture_update(tex:Texture2D, size:Vector2i):
	current_size = size;
	if (mat != null):
		mat.set_deferred("shader_parameter/tex", tex);
	if (texture_rect != null):
		texture_rect.set_deferred("texture", tex);
	
		
func audio_update(data:PackedFloat32Array, size:int, channel:int):
	pass
		
func audio_volumn(p:float):
	audio_stream.volume_db = p;

func play_pause_trigger():
	print("Play / Pause trigger")
	player.set_paused(!player.is_paused())
	
func pause_trigger():	
	print("Pause trigger")
	player.set_paused(true)
	
func play_trigger():
	print("Play trigger")
	player.set_paused(false)
	
func stop_trigger():
	print("Stop trigger")
	player.stop()
	
func load_trigger(p:String):
	print("Loading: ", p)
	player.load_path(p);
	player.play()
		
func async_load_finish(result):
	print("Loading result: ", result)
	if (result):
		player.play()
	
func message_feedback(m:String):
	print(m)

func error_feedback(m:String):
	printerr(m)
	
func quick_seek_forward(m:float):
	var currentTime = player.get_playback_position();
	var t = currentTime + m;
	player.seek(t);
	
func quick_seek_backward(m:float):
	var currentTime = player.get_playback_position();
	var t = currentTime - m;
	player.seek(t);
