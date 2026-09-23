extends SceneTree

func _init() -> void:
	var reader: NfcReader = NfcReader.new()
	var emitted_errors: Array[String] = []
	reader.card_error.connect(func(reason: String) -> void: emitted_errors.append(reason))
	var readers: PackedStringArray = reader.list_readers()
	assert(readers is PackedStringArray)
	assert(reader.get_last_error_code() >= NfcReader.OK)
	assert(reader.get_last_error_code() <= NfcReader.PCSC_ERROR)
	assert(not reader.is_open())
	assert(reader.read_bytes(0).is_empty())
	assert(reader.get_last_error_code() == NfcReader.INVALID_LENGTH)
	assert(emitted_errors.size() >= 1)
	reader.close()
	assert(reader.get_last_error_code() == NfcReader.OK)
	print("GODOT_SMOKE_OK")
	quit()
