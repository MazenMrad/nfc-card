#include "ntag215_device.h"
#include "nfc_controller.h"
#include "reader_profile.h"
#include "win_pcsc_transport.h"

#include <winscard.h>

#include <array>
#include <cstdlib>
#include <deque>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using nfc::ErrorCode;
using nfc::IPcscTransport;
using nfc::Ntag215Device;
using nfc::NfcController;
using nfc::ReaderListResult;
using nfc::Result;

class FakeSession final : public IPcscTransport {
public:
	Result begin_result = Result::success();
	Result end_result = Result::success();
	std::deque<Result> responses;
	std::vector<std::vector<uint8_t>> commands;
	int begin_count = 0;
	int end_count = 0;
	int disconnect_count = 0;
	std::vector<std::wstring> readers = {L"Test NFC Reader 0"};
	Result list_result = Result::success();
	Result connect_result = Result::success();
	Result disconnect_result = Result::success();
	std::wstring connected_reader;
	bool connected = false;

	ReaderListResult list_readers() override {
		return {list_result, readers};
	}

	Result connect_reader(const std::wstring &reader_name) override {
		connected_reader = reader_name;
		connected = connect_result.ok();
		return connect_result;
	}

	Result disconnect_reader() override {
		++disconnect_count;
		connected = false;
		return disconnect_result;
	}

	bool is_connected() const override {
		return connected;
	}

	Result begin_transaction() override {
		++begin_count;
		return begin_result;
	}

	Result end_transaction() override {
		++end_count;
		return end_result;
	}

	Result transmit(const std::vector<uint8_t> &command) override {
		commands.push_back(command);
		if (responses.empty()) {
			return Result::failure(ErrorCode::PCSC_ERROR, "No scripted response");
		}
		Result next = std::move(responses.front());
		responses.pop_front();
		return next;
	}
};

struct TestCase {
	std::string name;
	std::function<void()> body;
};

std::vector<TestCase> tests;

void add_test(std::string name, std::function<void()> body) {
	tests.push_back({std::move(name), std::move(body)});
}

void require(bool condition, const std::string &message) {
	if (!condition) {
		throw std::runtime_error(message);
	}
}

template <typename T>
void require_equal(const T &actual, const T &expected, const std::string &message) {
	if (actual != expected) {
		throw std::runtime_error(message);
	}
}

Result page(std::initializer_list<uint8_t> bytes) {
	std::vector<uint8_t> response(bytes);
	response.push_back(0x90);
	response.push_back(0x00);
	return Result::success(std::move(response));
}

Result ok_status() {
	return Result::success({0x90, 0x00});
}

void test_profile_encodes_page_commands() {
	auto profile = nfc::ReaderProfile::pcsc_page_addressed();
	require_equal(profile.make_get_uid(), std::vector<uint8_t>({0xFF, 0xCA, 0x00, 0x00, 0x00}), "UID command mismatch");
	require_equal(profile.make_read_page(4), std::vector<uint8_t>({0xFF, 0xB0, 0x00, 0x04, 0x04}), "Read command mismatch");
	require_equal(profile.make_write_page(5, {1, 2, 3, 4}), std::vector<uint8_t>({0xFF, 0xD6, 0x00, 0x05, 0x04, 1, 2, 3, 4}), "Write command mismatch");
}

void test_validate_accepts_ntag215_cc() {
	FakeSession session;
	session.responses.push_back(page({0xE1, 0x10, 0x3E, 0x00}));
	Ntag215Device device(session, nfc::ReaderProfile::pcsc_page_addressed());

	Result result = device.validate_tag();

	require(result.ok(), result.message);
	require_equal(session.commands.size(), size_t(1), "Validation should read one page");
	require_equal(session.begin_count, 1, "Validation should begin a transaction");
	require_equal(session.end_count, 1, "Validation should end a transaction");
}

void test_validate_rejects_other_capacity() {
	FakeSession session;
	session.responses.push_back(page({0xE1, 0x10, 0x12, 0x00}));
	Ntag215Device device(session, nfc::ReaderProfile::pcsc_page_addressed());

	Result result = device.validate_tag();

	require_equal(result.code, ErrorCode::TAG_PROFILE_MISMATCH, "Wrong capacity should be rejected");
}

void test_read_returns_exact_requested_bytes() {
	FakeSession session;
	session.responses.push_back(page({1, 2, 3, 4}));
	session.responses.push_back(page({5, 6, 7, 8}));
	Ntag215Device device(session, nfc::ReaderProfile::pcsc_page_addressed());

	Result result = device.read_bytes(5);

	require(result.ok(), result.message);
	require_equal(result.data, std::vector<uint8_t>({1, 2, 3, 4, 5}), "Read should trim the last page");
	require_equal(session.commands.size(), size_t(2), "Read should issue two page commands");
}

void test_read_rejects_invalid_lengths_without_io() {
	FakeSession session;
	Ntag215Device device(session, nfc::ReaderProfile::pcsc_page_addressed());

	require_equal(device.read_bytes(0).code, ErrorCode::INVALID_LENGTH, "Zero length should fail");
	require_equal(device.read_bytes(101).code, ErrorCode::INVALID_LENGTH, "Overlength should fail");
	require(session.commands.empty(), "Invalid lengths must not access hardware");
	require_equal(session.begin_count, 0, "Invalid lengths must not start transactions");
}

void test_write_preserves_partial_page_and_verifies() {
	FakeSession session;
	session.responses.push_back(page({9, 8, 7, 6})); // Preserve trailing bytes of page 5.
	session.responses.push_back(ok_status());          // Write page 4.
	session.responses.push_back(ok_status());          // Write page 5.
	session.responses.push_back(page({1, 2, 3, 4}));  // Verify page 4.
	session.responses.push_back(page({5, 8, 7, 6}));  // Verify page 5.
	Ntag215Device device(session, nfc::ReaderProfile::pcsc_page_addressed());

	Result result = device.write_bytes({1, 2, 3, 4, 5});

	require(result.ok(), result.message);
	require_equal(session.commands.size(), size_t(5), "Unexpected write command count");
	require_equal(session.commands[2], std::vector<uint8_t>({0xFF, 0xD6, 0x00, 0x05, 0x04, 5, 8, 7, 6}), "Partial page bytes were not preserved");
	require_equal(session.begin_count, 1, "Write should use one transaction");
	require_equal(session.end_count, 1, "Write should release its transaction");
}

void test_write_detects_verify_mismatch() {
	FakeSession session;
	session.responses.push_back(ok_status());
	session.responses.push_back(page({1, 2, 3, 9}));
	Ntag215Device device(session, nfc::ReaderProfile::pcsc_page_addressed());

	Result result = device.write_bytes({1, 2, 3, 4});

	require_equal(result.code, ErrorCode::VERIFY_FAILED, "Mismatched readback should fail verification");
	require_equal(session.end_count, 1, "Verification failure must release the transaction");
}

void test_failed_read_releases_transaction_and_returns_no_partial_data() {
	FakeSession session;
	session.responses.push_back(page({1, 2, 3, 4}));
	session.responses.push_back(Result::failure(ErrorCode::CARD_REMOVED, "Card removed"));
	Ntag215Device device(session, nfc::ReaderProfile::pcsc_page_addressed());

	Result result = device.read_bytes(5);

	require_equal(result.code, ErrorCode::CARD_REMOVED, "Removal code should be preserved");
	require(result.data.empty(), "Failed reads must not expose partial data");
	require_equal(session.end_count, 1, "Failed read must release the transaction");
}

void test_get_uid_validates_response_length() {
	FakeSession session;
	session.responses.push_back(page({0x04, 1, 2, 3, 4, 5}));
	Ntag215Device device(session, nfc::ReaderProfile::pcsc_page_addressed());

	Result result = device.get_uid();

	require_equal(result.code, ErrorCode::INVALID_RESPONSE, "NTAG215 UID must contain seven bytes");
	require_equal(session.end_count, 1, "Malformed UID must release the transaction");
}

void test_controller_requires_selection_for_multiple_readers() {
	FakeSession transport;
	transport.readers = {L"Reader A", L"Reader B"};
	NfcController controller(transport, nfc::ReaderProfile::pcsc_page_addressed());

	Result result = controller.open(L"");

	require_equal(result.code, ErrorCode::READER_SELECTION_REQUIRED, "Multiple readers should require an explicit choice");
	require(transport.connected_reader.empty(), "Controller must not choose among multiple readers");
}

void test_controller_opens_named_reader_after_validation() {
	FakeSession transport;
	transport.readers = {L"Reader A", L"Reader B"};
	transport.responses.push_back(page({0xE1, 0x10, 0x3E, 0x00}));
	NfcController controller(transport, nfc::ReaderProfile::pcsc_page_addressed());

	Result result = controller.open(L"Reader B");

	require(result.ok(), result.message);
	require(controller.is_open(), "Controller should remain open after validation");
	require_equal(transport.connected_reader, std::wstring(L"Reader B"), "Controller connected to the wrong reader");
}

void test_controller_disconnects_after_validation_failure() {
	FakeSession transport;
	transport.responses.push_back(page({0xE1, 0x10, 0x12, 0x00}));
	NfcController controller(transport, nfc::ReaderProfile::pcsc_page_addressed());

	Result result = controller.open(L"");

	require_equal(result.code, ErrorCode::TAG_PROFILE_MISMATCH, "Validation error should be preserved");
	require(!controller.is_open(), "Failed validation must leave the controller closed");
	require_equal(transport.disconnect_count, 1, "Failed validation must disconnect the card handle");
}

void test_controller_closes_stale_session_after_removal() {
	FakeSession transport;
	transport.responses.push_back(page({0xE1, 0x10, 0x3E, 0x00}));
	transport.responses.push_back(Result::failure(ErrorCode::CARD_REMOVED, "Card removed"));
	NfcController controller(transport, nfc::ReaderProfile::pcsc_page_addressed());
	require(controller.open(L"").ok(), "Controller setup failed");

	Result result = controller.read_bytes(4);

	require_equal(result.code, ErrorCode::CARD_REMOVED, "Removal error should be preserved");
	require(!controller.is_open(), "Removal must invalidate the open session");
	require_equal(transport.disconnect_count, 1, "Removal must disconnect the stale handle");
}

void test_pcsc_errors_map_to_public_codes() {
	require(nfc::map_pcsc_error(SCARD_E_NO_READERS_AVAILABLE, nfc::PcscOperation::LIST_READERS).ok(), "No readers should be a successful empty enumeration");
	require_equal(nfc::map_pcsc_error(SCARD_E_NO_SERVICE, nfc::PcscOperation::CONNECT).code, ErrorCode::SERVICE_UNAVAILABLE, "Service failure mapping mismatch");
	require_equal(nfc::map_pcsc_error(SCARD_E_NO_SMARTCARD, nfc::PcscOperation::CONNECT).code, ErrorCode::NO_CARD, "No-card mapping mismatch");
	require_equal(nfc::map_pcsc_error(SCARD_W_REMOVED_CARD, nfc::PcscOperation::TRANSMIT).code, ErrorCode::CARD_REMOVED, "Removal mapping mismatch");
	require_equal(nfc::map_pcsc_error(SCARD_W_RESET_CARD, nfc::PcscOperation::TRANSACTION).code, ErrorCode::CARD_RESET, "Reset mapping mismatch");
	require_equal(nfc::map_pcsc_error(SCARD_E_READER_UNAVAILABLE, nfc::PcscOperation::TRANSMIT).code, ErrorCode::READER_REMOVED, "Reader mapping mismatch");
	require_equal(nfc::map_pcsc_error(0x12345678L, nfc::PcscOperation::TRANSMIT).code, ErrorCode::PCSC_ERROR, "Unknown errors should retain a generic code");
}

void test_profile_maps_security_status_to_protected_tag() {
	auto profile = nfc::ReaderProfile::pcsc_page_addressed();
	Result protected_result = profile.parse_response(Result::success({0x69, 0x82}), 0, ErrorCode::WRITE_FAILED);

	require_equal(protected_result.code, ErrorCode::TAG_PROTECTED, "Security status should identify a protected tag");
	require_equal(protected_result.apdu_status, uint16_t(0x6982), "APDU status should be retained");
}

} // namespace

int main() {
	add_test("profile encodes page commands", test_profile_encodes_page_commands);
	add_test("validate accepts NTAG215 CC", test_validate_accepts_ntag215_cc);
	add_test("validate rejects another capacity", test_validate_rejects_other_capacity);
	add_test("read returns exact requested bytes", test_read_returns_exact_requested_bytes);
	add_test("read rejects invalid lengths without I/O", test_read_rejects_invalid_lengths_without_io);
	add_test("write preserves partial page and verifies", test_write_preserves_partial_page_and_verifies);
	add_test("write detects verify mismatch", test_write_detects_verify_mismatch);
	add_test("failed read releases transaction", test_failed_read_releases_transaction_and_returns_no_partial_data);
	add_test("get UID validates response length", test_get_uid_validates_response_length);
	add_test("controller requires reader selection", test_controller_requires_selection_for_multiple_readers);
	add_test("controller opens named reader", test_controller_opens_named_reader_after_validation);
	add_test("controller disconnects after validation failure", test_controller_disconnects_after_validation_failure);
	add_test("controller closes stale session after removal", test_controller_closes_stale_session_after_removal);
	add_test("PCSC errors map to public codes", test_pcsc_errors_map_to_public_codes);
	add_test("profile maps protected tag status", test_profile_maps_security_status_to_protected_tag);

	int failures = 0;
	for (const TestCase &test : tests) {
		try {
			test.body();
			std::cout << "PASS  " << test.name << '\n';
		} catch (const std::exception &error) {
			++failures;
			std::cerr << "FAIL  " << test.name << ": " << error.what() << '\n';
		}
	}

	std::cout << tests.size() - failures << "/" << tests.size() << " tests passed\n";
	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
