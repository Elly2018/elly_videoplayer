extends Node

@export var player: AudioStreamPlayer

var wav: AudioStreamWAV
var thread: Thread

const uri:String = "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4";

# Called when the node enters the scene tree for the first time.
func _ready():
	thread = Thread.new()
	thread.start(call_ffmpeg)

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	pass
	
func call_ffmpeg():
	var output = OS.execute("ffmpeg", ["-i", uri, "-f", "f32be", "pipe:1"], [], true, true);
	pass
