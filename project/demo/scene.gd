extends MyNode

func _ready() -> void:
	print("Hello GDScript!")
	MySingleton.hello_singleton();
