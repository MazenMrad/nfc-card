#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace nfc {

enum class ErrorCode : int32_t {
	OK = 0,
	NO_READERS,
	READER_NOT_FOUND,
	READER_SELECTION_REQUIRED,
	UNSUPPORTED_READER,
	SERVICE_UNAVAILABLE,
	NO_CARD,
	CARD_REMOVED,
	CARD_RESET,
	READER_REMOVED,
	WRONG_TAG,
	TAG_PROFILE_MISMATCH,
	TAG_PROTECTED,
	NOT_OPEN,
	BUSY,
	INVALID_LENGTH,
	INVALID_RESPONSE,
	READ_FAILED,
	WRITE_FAILED,
	VERIFY_FAILED,
	PCSC_ERROR,
};

struct Result {
	ErrorCode code = ErrorCode::OK;
	std::string message;
	std::vector<uint8_t> data;
	long native_status = 0;
	uint16_t apdu_status = 0;

	[[nodiscard]] bool ok() const { return code == ErrorCode::OK; }

	static Result success(std::vector<uint8_t> value = {}) {
		Result result;
		result.data = std::move(value);
		return result;
	}

	static Result failure(ErrorCode error, std::string detail, long native = 0, uint16_t apdu = 0) {
		Result result;
		result.code = error;
		result.message = std::move(detail);
		result.native_status = native;
		result.apdu_status = apdu;
		return result;
	}
};

const char *error_code_name(ErrorCode code);

} // namespace nfc
