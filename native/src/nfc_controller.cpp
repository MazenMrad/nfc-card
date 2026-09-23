#include "nfc_controller.h"

#include <algorithm>
#include <utility>

namespace nfc {

NfcController::NfcController(IPcscTransport &transport, ReaderProfile profile) : transport_(transport), profile_(std::move(profile)) {}

NfcController::~NfcController() {
	close();
}

ReaderListResult NfcController::list_readers() {
	return transport_.list_readers();
}

Result NfcController::open(const std::wstring &reader_name) {
	if (busy_) {
		return Result::failure(ErrorCode::BUSY, "Another NFC operation is already running");
	}
	busy_ = true;
	Result closed = close();
	if (!closed.ok()) {
		busy_ = false;
		return closed;
	}
	ReaderListResult available = transport_.list_readers();
	if (!available.status.ok()) {
		busy_ = false;
		return available.status;
	}
	if (available.readers.empty()) {
		busy_ = false;
		return Result::failure(ErrorCode::NO_READERS, "No PC/SC readers are installed");
	}

	std::wstring selected = reader_name;
	if (selected.empty()) {
		if (available.readers.size() != 1) {
			busy_ = false;
			return Result::failure(ErrorCode::READER_SELECTION_REQUIRED, "More than one reader is available; pass a reader name");
		}
		selected = available.readers.front();
	} else if (std::find(available.readers.begin(), available.readers.end(), selected) == available.readers.end()) {
		busy_ = false;
		return Result::failure(ErrorCode::READER_NOT_FOUND, "The requested reader was not found");
	}

	Result connected = transport_.connect_reader(selected);
	if (!connected.ok()) {
		busy_ = false;
		return connected;
	}
	device_ = std::make_unique<Ntag215Device>(transport_, profile_);
	Result validated = device_->validate_tag();
	if (!validated.ok()) {
		device_.reset();
		transport_.disconnect_reader();
		busy_ = false;
		return validated;
	}
	busy_ = false;
	return Result::success();
}

Result NfcController::close() {
	device_.reset();
	if (!transport_.is_connected()) {
		return Result::success();
	}
	return transport_.disconnect_reader();
}

bool NfcController::is_open() const {
	return device_ != nullptr && transport_.is_connected();
}

Result NfcController::require_device() const {
	if (!is_open()) {
		return Result::failure(ErrorCode::NOT_OPEN, "No NFC card session is open");
	}
	return Result::success();
}

Result NfcController::write_bytes(const std::vector<uint8_t> &bytes) {
	Result ready = require_device();
	if (!ready.ok()) {
		return ready;
	}
	return finish_device_operation(device_->write_bytes(bytes));
}

Result NfcController::read_bytes(size_t length) {
	Result ready = require_device();
	if (!ready.ok()) {
		return ready;
	}
	return finish_device_operation(device_->read_bytes(length));
}

Result NfcController::get_uid() {
	Result ready = require_device();
	if (!ready.ok()) {
		return ready;
	}
	return finish_device_operation(device_->get_uid());
}

Result NfcController::finish_device_operation(Result result) {
	if (invalidates_session(result.code)) {
		close();
	}
	return result;
}

bool NfcController::invalidates_session(ErrorCode code) {
	return code == ErrorCode::CARD_REMOVED || code == ErrorCode::CARD_RESET || code == ErrorCode::READER_REMOVED || code == ErrorCode::SERVICE_UNAVAILABLE;
}

} // namespace nfc
