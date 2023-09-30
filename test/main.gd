extends TextureRect


const FILE_PATH =\
		"http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4"
const TIME_SKIP = 5

@onready var ffmpeg = $FFmpegNode


func _ready():
	texture = ffmpeg.get_video_texture()

	ffmpeg.load_path(FILE_PATH)
	ffmpeg.play()
	ffmpeg.set_loop(true)


func _input(event):
	if event.is_action_pressed("ui_left"):
		ffmpeg.seek(ffmpeg.get_playback_position() - TIME_SKIP)
	elif event.is_action_pressed("ui_right"):
		ffmpeg.seek(ffmpeg.get_playback_position() + TIME_SKIP)
	elif event.is_action_pressed("ui_accept"):
		ffmpeg.set_paused(not ffmpeg.is_paused())
