#pragma once

#include "nfc_result.h"

#include <cstdint>
#include <string>
#include <vector>

namespace nfc {

class IPcscSession {
public:
	virtual ~IPcscSession() = default;
	virtual Result begin_transaction() = 0;
	virtual Result end_transaction() = 0;
	virtual Result transmit(const std::vector<uint8_t> &command) = 0;
};

struct ReaderListResult {
	Result status;
	std::vector<std::wstring> readers;
};

class IPcscTransport : public IPcscSession {
public:
	~IPcscTransport() override = default;
	virtual ReaderListResult list_readers() = 0;
	virtual Result connect_reader(const std::wstring &reader_name) = 0;
	virtual Result disconnect_reader() = 0;
	[[nodiscard]] virtual bool is_connected() const = 0;
};

} // namespace nfc
