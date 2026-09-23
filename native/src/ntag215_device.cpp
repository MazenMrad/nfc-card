#include "ntag215_device.h"

#include <algorithm>
#include <utility>

namespace nfc {

Ntag215Device::Ntag215Device(IPcscSession &session, ReaderProfile profile) : session_(session), profile_(std::move(profile)) {}

Result Ntag215Device::in_transaction(const std::function<Result()> &operation) {
	Result begin = session_.begin_transaction();
	if (!begin.ok()) {
		return begin;
	}
	Result primary = operation();
	Result end = session_.end_transaction();
	if (!primary.ok()) {
		primary.data.clear();
		return primary;
	}
	if (!end.ok()) {
		end.data.clear();
		return end;
	}
	return primary;
}

Result Ntag215Device::read_page(uint8_t page) {
	return profile_.parse_response(session_.transmit(profile_.make_read_page(page)), PAGE_SIZE, ErrorCode::READ_FAILED);
}

Result Ntag215Device::write_page(uint8_t page, const std::array<uint8_t, PAGE_SIZE> &bytes) {
	return profile_.parse_response(session_.transmit(profile_.make_write_page(page, bytes)), 0, ErrorCode::WRITE_FAILED);
}

Result Ntag215Device::validate_tag() {
	return in_transaction([this]() {
		Result cc = read_page(3);
		if (!cc.ok()) {
			return cc;
		}
		if (cc.data != std::vector<uint8_t>({0xE1, 0x10, 0x3E, 0x00})) {
			return Result::failure(ErrorCode::TAG_PROFILE_MISMATCH, "Card does not expose the expected writable NTAG215 capability container");
		}
		return Result::success();
	});
}

Result Ntag215Device::get_uid() {
	return in_transaction([this]() {
		return profile_.parse_response(session_.transmit(profile_.make_get_uid()), 7, ErrorCode::READ_FAILED);
	});
}

Result Ntag215Device::read_bytes(size_t length) {
	if (length == 0 || length > MAX_BYTES) {
		return Result::failure(ErrorCode::INVALID_LENGTH, "Read length must be between 1 and 100 bytes");
	}
	return in_transaction([this, length]() {
		std::vector<uint8_t> bytes;
		bytes.reserve(((length + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE);
		const size_t page_count = (length + PAGE_SIZE - 1) / PAGE_SIZE;
		for (size_t index = 0; index < page_count; ++index) {
			Result page = read_page(static_cast<uint8_t>(FIRST_USER_PAGE + index));
			if (!page.ok()) {
				return page;
			}
			bytes.insert(bytes.end(), page.data.begin(), page.data.end());
		}
		bytes.resize(length);
		return Result::success(std::move(bytes));
	});
}

Result Ntag215Device::write_bytes(const std::vector<uint8_t> &bytes) {
	if (bytes.empty() || bytes.size() > MAX_BYTES) {
		return Result::failure(ErrorCode::INVALID_LENGTH, "Write length must be between 1 and 100 bytes");
	}
	return in_transaction([this, &bytes]() {
		const size_t page_count = (bytes.size() + PAGE_SIZE - 1) / PAGE_SIZE;
		std::vector<std::array<uint8_t, PAGE_SIZE>> pages(page_count);
		const size_t remainder = bytes.size() % PAGE_SIZE;
		if (remainder != 0) {
			Result existing = read_page(static_cast<uint8_t>(FIRST_USER_PAGE + page_count - 1));
			if (!existing.ok()) {
				return existing;
			}
			std::copy(existing.data.begin(), existing.data.end(), pages.back().begin());
		}
		for (size_t index = 0; index < bytes.size(); ++index) {
			pages[index / PAGE_SIZE][index % PAGE_SIZE] = bytes[index];
		}
		for (size_t index = 0; index < page_count; ++index) {
			Result written = write_page(static_cast<uint8_t>(FIRST_USER_PAGE + index), pages[index]);
			if (!written.ok()) {
				return written;
			}
		}
		for (size_t index = 0; index < page_count; ++index) {
			Result verified = read_page(static_cast<uint8_t>(FIRST_USER_PAGE + index));
			if (!verified.ok()) {
				return verified;
			}
			if (!std::equal(verified.data.begin(), verified.data.end(), pages[index].begin())) {
				return Result::failure(ErrorCode::VERIFY_FAILED, "Card contents did not match after writing");
			}
		}
		return Result::success();
	});
}

} // namespace nfc
