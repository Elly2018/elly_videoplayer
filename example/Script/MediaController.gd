class_name MediaController extends Control

signal ToPlay
signal ToPlayPause
signal ToPause
signal ToStop
signal ToLoad(p:String)
signal ToAudio(p:float)
signal SeekForward(m:float)
signal SeekBackward(m:float)

@export var input_uri: TextEdit
@export var Con: Control
@export var seek_time: float

func _input(event):
	if event is InputEventKey and event.pressed:
		if event.keycode == KEY_D:
			emit_signal("SeekForward", seek_time);
		if event.keycode == KEY_A:
			emit_signal("SeekBackward", seek_time);
		if event.keycode == KEY_F:
			emit_signal("ToPlayPause");
		
func OnPlay():
	emit_signal("ToPlay");
	
func OnPause():
	emit_signal("ToPause");
	
func OnStop():
	emit_signal("ToStop");
	
func OnLoad():
	emit_signal("ToLoad", input_uri.text);

func OnAudio(v:float):
	emit_signal("ToAudio", v);

func MenuTrigger():
	Con.visible = !Con.visible;
