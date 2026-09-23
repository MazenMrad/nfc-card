# NFC Windows addon

This addon reads and writes NTAG215 cards on Windows x86-64.

## Installation

1. Copy this directory to `res://addons/nfc_windows/`.
2. Use an official single-precision Godot 4.6 build.
3. Connect a compatible PC/SC NFC reader.

## Operation

1. Create an `NfcReader` object.
2. Connect a function to the `card_error` signal.
3. Call `list_readers()`.
4. Call `open(reader_name)`.
5. Call `read_bytes()` or `write_bytes()`.
6. Call `close()`.

The addon uses 1 through 100 raw bytes. The data starts at NTAG215 page 4.

The `write_bytes()` method reads the new data and verifies it after the write operation.

CAUTION: Use a card that contains no important data. Raw writes replace NDEF data in the same memory area.

Reader support is provisional. The reader driver must support the `FF CA`, `FF B0`, and `FF D6` commands.

Read the project README and `docs/reader-profile.md` before physical acceptance tests.

This addon uses the MIT License. Read `LICENSE` for the license text.
