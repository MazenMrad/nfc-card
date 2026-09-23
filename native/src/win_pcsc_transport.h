#pragma once

#include "pcsc_session.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winscard.h>

#include <string>
#include <vector>

namespace nfc {

enum class PcscOperation {
	CONTEXT,
	LIST_READERS,
	CONNECT,
	DISCONNECT,
	TRANSACTION,
	TRANSMIT,
};

Result map_pcsc_error(long status, PcscOperation operation);

class WinPcscTransport final : public IPcscTransport {
public:
	WinPcscTransport() = default;
	~WinPcscTransport() override;

	ReaderListResult list_readers() override;
	Result connect_reader(const std::wstring &reader_name) override;
	Result disconnect_reader() override;
	[[nodiscard]] bool is_connected() const override;
	Result begin_transaction() override;
	Result end_transaction() override;
	Result transmit(const std::vector<uint8_t> &command) override;

private:
	SCARDCONTEXT context_ = 0;
	SCARDHANDLE card_ = 0;
	DWORD active_protocol_ = 0;

	Result ensure_context();
	void release_context();
	Result handle_status(long status, PcscOperation operation);
};

} // namespace nfc
