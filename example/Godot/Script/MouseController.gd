extends Node3D

@export var sensitivity:float = 1.0;
var pos: Vector2;
var delta: Vector2;
var view: bool = false;
var reset: bool = false;

func _input(event):
	if (event is InputEventMouseMotion  && view):
		if (reset):
			pos = event.position;
			delta = Vector2(0.0, 0.0);
			reset = false;
		else:
			var newpos:Vector2 = event.position;
			delta = (newpos - pos);
			pos = newpos;
			var rot:Vector3 = self.rotation;
			rot.y += delta.x * sensitivity * 0.001;
			rot.x += delta.y * sensitivity * 0.001;
			rot.z = 0;
			self.rotation = rot;
	if (event is InputEventMouseButton):
		if(event.button_index == MOUSE_BUTTON_LEFT):
			if (event.is_released()):
				view = false;
			elif (event.is_pressed()):
				view = true;
				reset = true;
