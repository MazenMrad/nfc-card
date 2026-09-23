#pragma once

#include "nfc_controller.h"
#include "nfc_result.h"
#include "win_pcsc_transport.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

class NfcReader : public RefCounted {
	GDCLASS(NfcReader, RefCounted)

public:
	enum NfcError {
		OK = static_cast<int>(nfc::ErrorCode::OK),
		NO_READERS = static_cast<int>(nfc::ErrorCode::NO_READERS),
		READER_NOT_FOUND = static_cast<int>(nfc::ErrorCode::READER_NOT_FOUND),
		READER_SELECTION_REQUIRED = static_cast<int>(nfc::ErrorCode::READER_SELECTION_REQUIRED),
		UNSUPPORTED_READER = static_cast<int>(nfc::ErrorCode::UNSUPPORTED_READER),
		SERVICE_UNAVAILABLE = static_cast<int>(nfc::ErrorCode::SERVICE_UNAVAILABLE),
		NO_CARD = static_cast<int>(nfc::ErrorCode::NO_CARD),
		CARD_REMOVED = static_cast<int>(nfc::ErrorCode::CARD_REMOVED),
		CARD_RESET = static_cast<int>(nfc::ErrorCode::CARD_RESET),
		READER_REMOVED = static_cast<int>(nfc::ErrorCode::READER_REMOVED),
		WRONG_TAG = static_cast<int>(nfc::ErrorCode::WRONG_TAG),
		TAG_PROFILE_MISMATCH = static_cast<int>(nfc::ErrorCode::TAG_PROFILE_MISMATCH),
		TAG_PROTECTED = static_cast<int>(nfc::ErrorCode::TAG_PROTECTED),
		NOT_OPEN = static_cast<int>(nfc::ErrorCode::NOT_OPEN),
		BUSY = static_cast<int>(nfc::ErrorCode::BUSY),
		INVALID_LENGTH = static_cast<int>(nfc::ErrorCode::INVALID_LENGTH),
		INVALID_RESPONSE = static_cast<int>(nfc::ErrorCode::INVALID_RESPONSE),
		READ_FAILED = static_cast<int>(nfc::ErrorCode::READ_FAILED),
		WRITE_FAILED = static_cast<int>(nfc::ErrorCode::WRITE_FAILED),
		VERIFY_FAILED = static_cast<int>(nfc::ErrorCode::VERIFY_FAILED),
		PCSC_ERROR = static_cast<int>(nfc::ErrorCode::PCSC_ERROR),
	};

	NfcReader();
	~NfcReader() override = default;

	PackedStringArray list_readers();
	bool open(const String &reader_name = String());
	void close();
	[[nodiscard]] bool is_open() const;
	bool write_bytes(const PackedByteArray &data);
	PackedByteArray read_bytes(int64_t length);
	PackedByteArray get_uid();
	[[nodiscard]] int32_t get_last_error_code() const;
	[[nodiscard]] String get_last_error_message() const;

protected:
	static void _bind_methods();

private:
	nfc::WinPcscTransport transport_;
	nfc::NfcController controller_;
	int32_t last_error_code_ = OK;
	String last_error_message_;

	bool apply_result(const nfc::Result &result, bool emit_error = true);
	static PackedByteArray to_packed_bytes(const std::vector<uint8_t> &bytes);
};

} // namespace godot

VARIANT_ENUM_CAST(godot::NfcReader::NfcError)
