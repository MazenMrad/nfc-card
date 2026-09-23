extends SceneTree

func _init() -> void:
	call_deferred("_run")

func _run() -> void:
	var scene: PackedScene = load("res://demo/nfc_demo.tscn")
	assert(scene != null)
	var demo: Control = scene.instantiate()
	root.add_child(demo)
	await process_frame
	assert(demo.get_node("Margin/Layout/ReaderRow/Readers") is OptionButton)
	assert(demo.get_node("Margin/Layout/Payload") is TextEdit)
	assert(demo.get_node("Margin/Layout/Status") is Label)
	print("DEMO_SMOKE_OK")
	quit()
