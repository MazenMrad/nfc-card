#include "win_pcsc_transport.h"

#include <array>
#include <iomanip>
#include <sstream>

namespace nfc {

namespace {

std::string pcsc_message(long status) {
	std::ostringstream message;
	message << "WinSCard status 0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << static_cast<unsigned long>(status);
	return message.str();
}

} // namespace

Result map_pcsc_error(long status, PcscOperation operation) {
	if (status == SCARD_S_SUCCESS || (operation == PcscOperation::LIST_READERS && status == SCARD_E_NO_READERS_AVAILABLE)) {
		return Result::success();
	}
	ErrorCode code = ErrorCode::PCSC_ERROR;
	switch (status) {
		case SCARD_E_NO_SERVICE:
		case SCARD_E_SERVICE_STOPPED:
			code = ErrorCode::SERVICE_UNAVAILABLE;
			break;
		case SCARD_E_NO_SMARTCARD:
		case SCARD_W_UNPOWERED_CARD:
			code = ErrorCode::NO_CARD;
			break;
		case SCARD_W_REMOVED_CARD:
			code = ErrorCode::CARD_REMOVED;
			break;
		case SCARD_W_RESET_CARD:
			code = ErrorCode::CARD_RESET;
			break;
		case SCARD_E_READER_UNAVAILABLE:
			code = ErrorCode::READER_REMOVED;
			break;
		case SCARD_E_UNKNOWN_READER:
			code = operation == PcscOperation::CONNECT ? ErrorCode::READER_NOT_FOUND : ErrorCode::READER_REMOVED;
			break;
		case SCARD_E_SHARING_VIOLATION:
			code = ErrorCode::BUSY;
			break;
		case SCARD_E_INVALID_HANDLE:
			code = ErrorCode::NOT_OPEN;
			break;
		case SCARD_E_PROTO_MISMATCH:
		case SCARD_E_UNSUPPORTED_FEATURE:
			code = ErrorCode::UNSUPPORTED_READER;
			break;
		default:
			break;
	}
	return Result::failure(code, pcsc_message(status), status);
}

WinPcscTransport::~WinPcscTransport() {
	disconnect_reader();
	release_context();
}

Result WinPcscTransport::ensure_context() {
	if (context_ != 0) {
		return Result::success();
	}
	const long status = SCardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, &context_);
	return handle_status(status, PcscOperation::CONTEXT);
}

void WinPcscTransport::release_context() {
	if (context_ != 0) {
		SCardReleaseContext(context_);
		context_ = 0;
	}
}

Result WinPcscTransport::handle_status(long status, PcscOperation operation) {
	Result result = map_pcsc_error(status, operation);
	if (result.code == ErrorCode::SERVICE_UNAVAILABLE) {
		card_ = 0;
		active_protocol_ = 0;
		context_ = 0;
	}
	return result;
}

ReaderListResult WinPcscTransport::list_readers() {
	Result ready = ensure_context();
	if (!ready.ok()) {
		return {ready, {}};
	}

	DWORD character_count = 0;
	long status = SCardListReadersW(context_, nullptr, nullptr, &character_count);
	if (status == SCARD_E_NO_READERS_AVAILABLE) {
		return {Result::success(), {}};
	}
	Result sized = handle_status(status, PcscOperation::LIST_READERS);
	if (!sized.ok()) {
		return {sized, {}};
	}

	for (int attempt = 0; attempt < 2; ++attempt) {
		std::vector<wchar_t> buffer(character_count);
		DWORD requested = character_count;
		status = SCardListReadersW(context_, nullptr, buffer.data(), &requested);
		if (status == SCARD_E_INSUFFICIENT_BUFFER && attempt == 0) {
			character_count = requested;
			continue;
		}
		if (status == SCARD_E_NO_READERS_AVAILABLE) {
			return {Result::success(), {}};
		}
		Result filled = handle_status(status, PcscOperation::LIST_READERS);
		if (!filled.ok()) {
			return {filled, {}};
		}

		std::vector<std::wstring> readers;
		const wchar_t *cursor = buffer.data();
		while (*cursor != L'\0') {
			std::wstring reader(cursor);
			readers.push_back(reader);
			cursor += reader.size() + 1;
		}
		return {Result::success(), std::move(readers)};
	}
	return {Result::failure(ErrorCode::PCSC_ERROR, "Reader list changed repeatedly while it was being queried"), {}};
}

Result WinPcscTransport::connect_reader(const std::wstring &reader_name) {
	Result disconnected = disconnect_reader();
	if (!disconnected.ok()) {
		return disconnected;
	}
	Result ready = ensure_context();
	if (!ready.ok()) {
		return ready;
	}
	const long status = SCardConnectW(context_, reader_name.c_str(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &card_, &active_protocol_);
	return handle_status(status, PcscOperation::CONNECT);
}

Result WinPcscTransport::disconnect_reader() {
	if (card_ == 0) {
		return Result::success();
	}
	const SCARDHANDLE card = card_;
	card_ = 0;
	active_protocol_ = 0;
	return handle_status(SCardDisconnect(card, SCARD_LEAVE_CARD), PcscOperation::DISCONNECT);
}

bool WinPcscTransport::is_connected() const {
	return card_ != 0;
}

Result WinPcscTransport::begin_transaction() {
	if (card_ == 0) {
		return Result::failure(ErrorCode::NOT_OPEN, "No PC/SC card handle is open");
	}
	return handle_status(SCardBeginTransaction(card_), PcscOperation::TRANSACTION);
}

Result WinPcscTransport::end_transaction() {
	if (card_ == 0) {
		return Result::failure(ErrorCode::NOT_OPEN, "No PC/SC card handle is open");
	}
	return handle_status(SCardEndTransaction(card_, SCARD_LEAVE_CARD), PcscOperation::TRANSACTION);
}

Result WinPcscTransport::transmit(const std::vector<uint8_t> &command) {
	if (card_ == 0) {
		return Result::failure(ErrorCode::NOT_OPEN, "No PC/SC card handle is open");
	}
	if (command.empty()) {
		return Result::failure(ErrorCode::INVALID_LENGTH, "Cannot transmit an empty command");
	}
	SCARD_IO_REQUEST request{active_protocol_, sizeof(SCARD_IO_REQUEST)};
	std::array<uint8_t, 512> response{};
	DWORD response_size = static_cast<DWORD>(response.size());
	const long status = SCardTransmit(card_, &request, command.data(), static_cast<DWORD>(command.size()), nullptr, response.data(), &response_size);
	Result sent = handle_status(status, PcscOperation::TRANSMIT);
	if (!sent.ok()) {
		return sent;
	}
	return Result::success(std::vector<uint8_t>(response.begin(), response.begin() + response_size));
}

} // namespace nfc
