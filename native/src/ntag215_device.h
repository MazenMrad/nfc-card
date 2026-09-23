#pragma once

#include "pcsc_session.h"
#include "reader_profile.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace nfc {

class Ntag215Device {
public:
	static constexpr uint8_t FIRST_USER_PAGE = 4;
	static constexpr size_t PAGE_SIZE = 4;
	static constexpr size_t MAX_BYTES = 100;

	Ntag215Device(IPcscSession &session, ReaderProfile profile);

	Result validate_tag();
	Result get_uid();
	Result read_bytes(size_t length);
	Result write_bytes(const std::vector<uint8_t> &bytes);

private:
	IPcscSession &session_;
	ReaderProfile profile_;

	Result in_transaction(const std::function<Result()> &operation);
	Result read_page(uint8_t page);
	Result write_page(uint8_t page, const std::array<uint8_t, PAGE_SIZE> &bytes);
};

} // namespace nfc
