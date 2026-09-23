#pragma once

#include "ntag215_device.h"
#include "pcsc_session.h"
#include "reader_profile.h"

#include <memory>
#include <string>
#include <vector>

namespace nfc {

class NfcController {
public:
	NfcController(IPcscTransport &transport, ReaderProfile profile);
	~NfcController();

	ReaderListResult list_readers();
	Result open(const std::wstring &reader_name = L"");
	Result close();
	[[nodiscard]] bool is_open() const;
	Result write_bytes(const std::vector<uint8_t> &bytes);
	Result read_bytes(size_t length);
	Result get_uid();

private:
	IPcscTransport &transport_;
	ReaderProfile profile_;
	std::unique_ptr<Ntag215Device> device_;
	bool busy_ = false;

	Result require_device() const;
	Result finish_device_operation(Result result);
	[[nodiscard]] static bool invalidates_session(ErrorCode code);
};

} // namespace nfc
