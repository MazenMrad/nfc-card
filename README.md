# NFC Windows for Godot

This GDExtension reads and writes NTAG215 cards on Windows x86-64.

The addon uses a PC/SC NFC reader. It reads or writes a maximum of 100 raw bytes.

The project contains these items:

- C++ source files.
- Debug and release DLL files.
- A demo for Godot 4.6.
- Native protocol tests.
- Build instructions.

The software tests use a simulated PC/SC transport. Godot 4.6.3 also loads the addon and the demo correctly.

Physical tests are not complete. A reader driver controls the PC/SC command map.

## Safety information

CAUTION: Use a card that contains no important data. A write operation replaces data from NTAG page 4.

The addon does not change the UID, lock bytes, password, or configuration pages.

## Use the demo

1. Open this project in Godot 4.6.
2. Start the project.
3. Connect a PC/SC NFC reader to the computer.
4. Select **Refresh Readers**.
5. Select a reader.
6. Put an NTAG215 card on the reader.
7. Select **Open Card**.
8. Enter a payload of 1 through 100 UTF-8 bytes.
9. Select **Write and Verify**.
10. Use **Read** to read the specified number of bytes.

The demo shows the UID, payload size, hexadecimal data, elapsed time, and error information.

## Install the addon

1. Copy `addons/nfc_windows` to the `addons` directory of the Godot project.
2. Keep the descriptor and DLL files in their original relative paths.
3. Use an official single-precision Godot 4.6 build on Windows x86-64.

The supplied DLL files require a reader driver that supports these PC/SC commands:

- `FF CA` for the UID.
- `FF B0` for a page read.
- `FF D6` for a page write.

this command profile is provisional

## GDScript example

```gdscript
var nfc: NfcReader = NfcReader.new()

func _ready() -> void:
	nfc.card_error.connect(_on_card_error)

	var readers: PackedStringArray = nfc.list_readers()
	if readers.is_empty():
		return

	if not nfc.open(readers[0]):
		return

	var payload: PackedByteArray = "weapon=laser;up=W".to_utf8_buffer()
	if nfc.write_bytes(payload):
		var saved: PackedByteArray = nfc.read_bytes(payload.size())
		print(saved.get_string_from_utf8())

	print("UID: ", nfc.get_uid().hex_encode())
	nfc.close()

func _on_card_error(reason: String) -> void:
	push_error(reason)
```

## API

| Method | Function |
|---|---|
| `list_readers() -> PackedStringArray` | Gets the installed PC/SC reader names. |
| `open(reader_name := "") -> bool` | Opens the selected reader and validates the card profile. |
| `close() -> void` | Closes the card handle. Repeated calls are permitted. |
| `is_open() -> bool` | Reports whether the object has a PC/SC card handle. |
| `write_bytes(data) -> bool` | Writes and verifies 1 through 100 bytes from page 4. |
| `read_bytes(length) -> PackedByteArray` | Reads 1 through 100 bytes from page 4. |
| `get_uid() -> PackedByteArray` | Gets the seven-byte UID of the open card. |
| `get_last_error_code() -> int` | Gets the current `NfcReader` error code. |
| `get_last_error_message() -> String` | Gets the current error message. |

The `card_error(reason: String)` signal occurs one time after each failed public operation.

Use the numeric error code for program logic. Do not analyze the error message in program logic.

The proposed `connect()` name conflicts with `Object.connect()`. For this reason, the addon uses `open()`.

## Data format

The addon stores the supplied bytes without a header. It does not add a length, version, checksum, or schema.

The game must define the payload format. The game must also know the number of bytes to read.

A short write does not erase bytes after the new payload. It preserves unused bytes in the last partial page.

CAUTION: A write operation is not atomic. Card removal can cause a mix of old and new data.

After an interrupted write, open the card again. Then, write the complete payload again.

## Source and tests

Read [native/BUILD.md](native/BUILD.md) for build and test instructions.

Read [docs/reader-profile.md](docs/reader-profile.md) for the reader assumptions.

## License

The project-owned code uses the MIT License. The `godot-cpp` project keeps its separate MIT License.
