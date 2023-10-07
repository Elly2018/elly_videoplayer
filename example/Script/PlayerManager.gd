extends Node

var _Sphere = load("res://Prefab/Sphere.tscn")
var _XRPlayer = load("res://Prefab/XRPlayer.tscn")

var _2DPlayer = load("res://Prefab/2DPlayer.tscn")
var _PlayerUI = load("res://Prefab/PlayUI.tscn")
var _Plane = load("res://Prefab/Plane.tscn")


# Called when the node enters the scene tree for the first time.
func _ready():
	var isXR:bool = ProjectSettings.get_setting("xr/openxr/enabled");
	if (isXR):
		add_child(_XRPlayer.instantiate());
		add_child(_Sphere.instantiate());
	else:
		add_child(_Plane.instantiate());
		add_child(_2DPlayer.instantiate());
		add_child(_PlayerUI.instantiate());


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	pass
