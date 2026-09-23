#include "nfc_reader.h"

#include <godot_cpp/core/class_db.hpp>

#include <string>
#include <vector>

namespace godot {

NfcReader::NfcReader() : controller_(transport_, nfc::ReaderProfile::pcsc_page_addressed()) {}

void NfcReader::_bind_methods() {
	ClassDB::bind_method(D_METHOD("list_readers"), &NfcReader::list_readers);
	ClassDB::bind_method(D_METHOD("open", "reader_name"), &NfcReader::open, DEFVAL(String()));
	ClassDB::bind_method(D_METHOD("close"), &NfcReader::close);
	ClassDB::bind_method(D_METHOD("is_open"), &NfcReader::is_open);
	ClassDB::bind_method(D_METHOD("write_bytes", "data"), &NfcReader::write_bytes);
	ClassDB::bind_method(D_METHOD("read_bytes", "length"), &NfcReader::read_bytes);
	ClassDB::bind_method(D_METHOD("get_uid"), &NfcReader::get_uid);
	ClassDB::bind_method(D_METHOD("get_last_error_code"), &NfcReader::get_last_error_code);
	ClassDB::bind_method(D_METHOD("get_last_error_message"), &NfcReader::get_last_error_message);

	ADD_SIGNAL(MethodInfo("card_error", PropertyInfo(Variant::STRING, "reason")));

	BIND_ENUM_CONSTANT(OK);
	BIND_ENUM_CONSTANT(NO_READERS);
	BIND_ENUM_CONSTANT(READER_NOT_FOUND);
	BIND_ENUM_CONSTANT(READER_SELECTION_REQUIRED);
	BIND_ENUM_CONSTANT(UNSUPPORTED_READER);
	BIND_ENUM_CONSTANT(SERVICE_UNAVAILABLE);
	BIND_ENUM_CONSTANT(NO_CARD);
	BIND_ENUM_CONSTANT(CARD_REMOVED);
	BIND_ENUM_CONSTANT(CARD_RESET);
	BIND_ENUM_CONSTANT(READER_REMOVED);
	BIND_ENUM_CONSTANT(WRONG_TAG);
	BIND_ENUM_CONSTANT(TAG_PROFILE_MISMATCH);
	BIND_ENUM_CONSTANT(TAG_PROTECTED);
	BIND_ENUM_CONSTANT(NOT_OPEN);
	BIND_ENUM_CONSTANT(BUSY);
	BIND_ENUM_CONSTANT(INVALID_LENGTH);
	BIND_ENUM_CONSTANT(INVALID_RESPONSE);
	BIND_ENUM_CONSTANT(READ_FAILED);
	BIND_ENUM_CONSTANT(WRITE_FAILED);
	BIND_ENUM_CONSTANT(VERIFY_FAILED);
	BIND_ENUM_CONSTANT(PCSC_ERROR);
}

bool NfcReader::apply_result(const nfc::Result &result, bool emit_error) {
	last_error_code_ = static_cast<int32_t>(result.code);
	if (result.ok()) {
		last_error_message_ = String();
		return true;
	}
	last_error_message_ = String(nfc::error_code_name(result.code));
	if (!result.message.empty()) {
		last_error_message_ += ": ";
		last_error_message_ += String::utf8(result.message.c_str());
	}
	if (emit_error) {
		emit_signal("card_error", last_error_message_);
	}
	return false;
}

PackedStringArray NfcReader::list_readers() {
	nfc::ReaderListResult result = controller_.list_readers();
	PackedStringArray names;
	if (!apply_result(result.status)) {
		return names;
	}
	for (const std::wstring &name : result.readers) {
		names.append(String(name.c_str()));
	}
	return names;
}

bool NfcReader::open(const String &reader_name) {
	CharWideString wide_name = reader_name.wide_string();
	return apply_result(controller_.open(std::wstring(wide_name.get_data())));
}

void NfcReader::close() {
	apply_result(controller_.close());
}

bool NfcReader::is_open() const {
	return controller_.is_open();
}

bool NfcReader::write_bytes(const PackedByteArray &data) {
	std::vector<uint8_t> bytes;
	if (!data.is_empty()) {
		bytes.assign(data.ptr(), data.ptr() + data.size());
	}
	return apply_result(controller_.write_bytes(bytes));
}

PackedByteArray NfcReader::read_bytes(int64_t length) {
	if (length <= 0 || length > static_cast<int64_t>(nfc::Ntag215Device::MAX_BYTES)) {
		apply_result(nfc::Result::failure(nfc::ErrorCode::INVALID_LENGTH, "Read length must be between 1 and 100 bytes"));
		return {};
	}
	nfc::Result result = controller_.read_bytes(static_cast<size_t>(length));
	if (!apply_result(result)) {
		return {};
	}
	return to_packed_bytes(result.data);
}

PackedByteArray NfcReader::get_uid() {
	nfc::Result result = controller_.get_uid();
	if (!apply_result(result)) {
		return {};
	}
	return to_packed_bytes(result.data);
}

int32_t NfcReader::get_last_error_code() const {
	return last_error_code_;
}

String NfcReader::get_last_error_message() const {
	return last_error_message_;
}

PackedByteArray NfcReader::to_packed_bytes(const std::vector<uint8_t> &bytes) {
	PackedByteArray result;
	result.resize(static_cast<int64_t>(bytes.size()));
	if (!bytes.empty()) {
		std::copy(bytes.begin(), bytes.end(), result.ptrw());
	}
	return result;
}

} // namespace godot
