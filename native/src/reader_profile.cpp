#include "reader_profile.h"

#include <iomanip>
#include <sstream>
#include <utility>

namespace nfc {

ReaderProfile ReaderProfile::pcsc_page_addressed() {
	return ReaderProfile{};
}

std::vector<uint8_t> ReaderProfile::make_get_uid() const {
	return {0xFF, 0xCA, 0x00, 0x00, 0x00};
}

std::vector<uint8_t> ReaderProfile::make_read_page(uint8_t page) const {
	return {0xFF, 0xB0, 0x00, page, 0x04};
}

std::vector<uint8_t> ReaderProfile::make_write_page(uint8_t page, const std::array<uint8_t, 4> &bytes) const {
	return {0xFF, 0xD6, 0x00, page, 0x04, bytes[0], bytes[1], bytes[2], bytes[3]};
}

Result ReaderProfile::parse_response(Result response, size_t expected_data_size, ErrorCode operation_error) const {
	if (!response.ok()) {
		response.data.clear();
		return response;
	}
	if (response.data.size() != expected_data_size + 2) {
		return Result::failure(ErrorCode::INVALID_RESPONSE, "Reader returned an unexpected response length");
	}
	const size_t status_offset = response.data.size() - 2;
	const uint16_t status = static_cast<uint16_t>(response.data[status_offset] << 8U) | response.data[status_offset + 1];
	if (status != 0x9000) {
		std::ostringstream message;
		message << "Reader returned APDU status 0x" << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << status;
		const ErrorCode code = operation_error == ErrorCode::WRITE_FAILED && status == 0x6982 ? ErrorCode::TAG_PROTECTED : operation_error;
		return Result::failure(code, message.str(), 0, status);
	}
	response.data.resize(expected_data_size);
	response.apdu_status = status;
	return response;
}

} // namespace nfc
