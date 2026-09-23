#include "nfc_result.h"

namespace nfc {

const char *error_code_name(ErrorCode code) {
	switch (code) {
		case ErrorCode::OK: return "OK";
		case ErrorCode::NO_READERS: return "NO_READERS";
		case ErrorCode::READER_NOT_FOUND: return "READER_NOT_FOUND";
		case ErrorCode::READER_SELECTION_REQUIRED: return "READER_SELECTION_REQUIRED";
		case ErrorCode::UNSUPPORTED_READER: return "UNSUPPORTED_READER";
		case ErrorCode::SERVICE_UNAVAILABLE: return "SERVICE_UNAVAILABLE";
		case ErrorCode::NO_CARD: return "NO_CARD";
		case ErrorCode::CARD_REMOVED: return "CARD_REMOVED";
		case ErrorCode::CARD_RESET: return "CARD_RESET";
		case ErrorCode::READER_REMOVED: return "READER_REMOVED";
		case ErrorCode::WRONG_TAG: return "WRONG_TAG";
		case ErrorCode::TAG_PROFILE_MISMATCH: return "TAG_PROFILE_MISMATCH";
		case ErrorCode::TAG_PROTECTED: return "TAG_PROTECTED";
		case ErrorCode::NOT_OPEN: return "NOT_OPEN";
		case ErrorCode::BUSY: return "BUSY";
		case ErrorCode::INVALID_LENGTH: return "INVALID_LENGTH";
		case ErrorCode::INVALID_RESPONSE: return "INVALID_RESPONSE";
		case ErrorCode::READ_FAILED: return "READ_FAILED";
		case ErrorCode::WRITE_FAILED: return "WRITE_FAILED";
		case ErrorCode::VERIFY_FAILED: return "VERIFY_FAILED";
		case ErrorCode::PCSC_ERROR: return "PCSC_ERROR";
	}
	return "UNKNOWN_ERROR";
}

} // namespace nfc
