#pragma once

#include "nfc_result.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace nfc {

class ReaderProfile {
public:
	static ReaderProfile pcsc_page_addressed();

	[[nodiscard]] std::vector<uint8_t> make_get_uid() const;
	[[nodiscard]] std::vector<uint8_t> make_read_page(uint8_t page) const;
	[[nodiscard]] std::vector<uint8_t> make_write_page(uint8_t page, const std::array<uint8_t, 4> &bytes) const;
	[[nodiscard]] Result parse_response(Result response, size_t expected_data_size, ErrorCode operation_error) const;
};

} // namespace nfc
