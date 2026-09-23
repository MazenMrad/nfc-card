# Provisional reader profile

Status: The software contains this profile. Physical tests are not complete.

The target reader model is not specified. Thus, the profile uses a provisional PC/SC command map.

The profile uses one NTAG page for each addressed PC/SC block. One NTAG page contains four bytes.

| Operation | Command | Required response |
|---|---|---|
| Get the UID | `FF CA 00 00 00` | Seven UID bytes and `90 00` |
| Read page P | `FF B0 00 P 04` | Four data bytes and `90 00` |
| Write page P | `FF D6 00 P 04 D0 D1 D2 D3` | `90 00` |

The addon reads page 3 during `open()`.

The required Capability Container value is `E1 10 3E 00`. This value identifies the expected writable NTAG215 profile.

This test does not prove that the card is an authentic NXP product. A modified NTAG215 can fail this test.

A compatible clone can pass this test.

The addon writes only pages 4 through 28. The `write_bytes()` method reads and compares each affected page after a write.

Each logical operation uses one PC/SC transaction.

## Physical validation

Record this information during the physical tests:

- Reader manufacturer and model.
- Reader firmware version.
- Windows version.
- Reader driver version.
- Negotiated PC/SC protocol.
- Address unit for P1 and P2.
- Permitted read length.
- Write unit size.
- UID response format.
- Status-word response format.
- Error for no card.
- Error for card removal or reset.
- Error for a protected write.
- Error for reader removal.

If the command map is different, add a named `ReaderProfile` for that reader.

CAUTION: Do not write to unknown addresses to identify a reader profile.
