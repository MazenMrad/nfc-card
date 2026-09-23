extends Control

const MAX_BYTES: int = 100

var _nfc: NfcReader = NfcReader.new()

@onready var _readers: OptionButton = %Readers
@onready var _uid: Label = %Uid
@onready var _payload: TextEdit = %Payload
@onready var _byte_count: Label = %ByteCount
@onready var _read_length: SpinBox = %ReadLength
@onready var _output: TextEdit = %Output
@onready var _status: Label = %Status

func _ready() -> void:
	_nfc.card_error.connect(_on_card_error)
	_payload.text_changed.connect(_on_payload_changed)
	%Refresh.pressed.connect(_refresh_readers)
	%Open.pressed.connect(_open_card)
	%Close.pressed.connect(_close_card)
	%Write.pressed.connect(_write_payload)
	%Read.pressed.connect(_read_payload)
	_on_payload_changed()
	_refresh_readers()

func _exit_tree() -> void:
	_nfc.close()

func _refresh_readers() -> void:
	_readers.clear()
	var names: PackedStringArray = _nfc.list_readers()
	for reader_name: String in names:
		_readers.add_item(reader_name)
	if names.is_empty():
		if _nfc.get_last_error_code() == NfcReader.OK:
			_set_status("No PC/SC readers found.", false)
	else:
		_readers.select(0)
		_set_status("Found %d reader(s). Present an NTAG215, then open it." % names.size(), true)

func _open_card() -> void:
	if _readers.selected < 0:
		_set_status("Choose a reader first.", false)
		return
	var started_ms: int = Time.get_ticks_msec()
	if not _nfc.open(_readers.get_item_text(_readers.selected)):
		return
	var card_uid: PackedByteArray = _nfc.get_uid()
	if card_uid.is_empty():
		return
	_uid.text = "UID: %s" % card_uid.hex_encode().to_upper()
	_set_status("Card opened in %d ms." % (Time.get_ticks_msec() - started_ms), true)

func _close_card() -> void:
	_nfc.close()
	_uid.text = "UID: —"
	if _nfc.get_last_error_code() == NfcReader.OK:
		_set_status("Card closed.", true)

func _write_payload() -> void:
	var bytes: PackedByteArray = _payload.text.to_utf8_buffer()
	if bytes.is_empty() or bytes.size() > MAX_BYTES:
		_set_status("Payload must contain 1–100 UTF-8 bytes.", false)
		return
	var started_ms: int = Time.get_ticks_msec()
	if _nfc.write_bytes(bytes):
		_read_length.value = bytes.size()
		_set_status("Wrote and verified %d bytes in %d ms." % [bytes.size(), Time.get_ticks_msec() - started_ms], true)

func _read_payload() -> void:
	var length: int = int(_read_length.value)
	var started_ms: int = Time.get_ticks_msec()
	var bytes: PackedByteArray = _nfc.read_bytes(length)
	if bytes.is_empty():
		return
	_output.text = "Hex: %s\n\nUTF-8: %s" % [bytes.hex_encode().to_upper(), bytes.get_string_from_utf8()]
	_set_status("Read %d bytes in %d ms." % [bytes.size(), Time.get_ticks_msec() - started_ms], true)

func _on_payload_changed() -> void:
	var count: int = _payload.text.to_utf8_buffer().size()
	_byte_count.text = "%d / %d bytes" % [count, MAX_BYTES]
	_byte_count.modulate = Color(0.95, 0.4, 0.35) if count > MAX_BYTES else Color(0.65, 0.75, 0.9)

func _on_card_error(reason: String) -> void:
	_set_status(reason, false)
	if not _nfc.is_open():
		_uid.text = "UID: —"

func _set_status(message: String, success: bool) -> void:
	_status.text = message
	_status.modulate = Color(0.45, 0.9, 0.62) if success else Color(0.95, 0.42, 0.38)
