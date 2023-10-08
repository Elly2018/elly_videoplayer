extends Node

signal ToPlay
signal ToPause
signal ToStop
signal ToLoad(p:String)
signal ToAudio(p:float)

@export var input_uri: TextEdit
@export var Con: Control

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
