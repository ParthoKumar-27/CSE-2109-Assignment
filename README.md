# CRC-16 Error Detection with Manchester Encoding and Socket Communication

## 1. Introduction

This project implements a simple simulated digital communication system between a Sender (client) and a Receiver (server). It demonstrates how data integrity is achieved with CRC-16 and how Manchester encoding represents bits at the physical layer. The system provides manual error injection to test how CRC detects corrupted frames.

Main goals:
- Append CRC-16 (CCITT, poly 0x1021) to messages for error detection.
- Manchester-encode data (packed — 2 bytes per input byte).
- Transmit frames over TCP with a 4-byte length prefix (robust framing).
- Allow manual error injection (single, multiple, odd, burst) and report flipped bit positions.
- Demonstrate detection of errors on the receiver side.

This is intended as an educational assignment to illustrate CRC, encoding, framing, and reliable network I/O.

---

## 2. System Overview

### Workflow

1. Sender reads a text message from the user.  
2. Sender computes CRC-16 over the plaintext bytes.  
3. CRC (2 bytes) is appended to the plaintext.  
4. The combined buffer (plaintext + CRC) is Manchester-encoded (packed format).  
5. Sender sends a 4‑byte big-endian length prefix (N) followed by N encoded bytes.  
6. Receiver reads the 4‑byte length header, then reads exactly N bytes.  
7. Receiver decodes the Manchester stream back to bytes, extracts CRC, recomputes CRC, and compares.  
8. Receiver prints the received message and whether the CRC check passed or failed.

---

## 3. Files in this repository

- `sender.c` — Sender (client).  
- `receiver.c` — Receiver (server).  
- `crc16.c` / `crc16.h` — CRC-16 CCITT implementation.  
- `manchester.c` / `manchester.h` — Packed Manchester encode/decode (each input byte → 16 encoded bits = 2 bytes).  
- `error.c` / `error.h` — Error injection helpers operating on encoded-bit indices.  
- `Makefile` — Build helpers and foreground run targets (`make server` / `make client`).  
- `README.md` — This file.

> Note: Implementations are split into `.c` files (non-inline) to avoid duplicate symbol/linker problems and to keep the build simple.

---

## 4. Build

Requirements:
- gcc (or other C compiler)
- POSIX-like environment (Linux, macOS, or WSL on Windows)

From the project directory:

```bash
make
```

This produces two executables:
- `sender`
- `receiver`

Clean build artifacts:
```bash
make clean
```

---

## 5. How to run (two separate terminals)

This project is set up to run the server and client in two separate foreground terminals (you requested no background processes). Use the `make server` and `make client` convenience targets or run the binaries directly.

Terminal A — start the receiver (server) in foreground:
```bash
# build if needed
make
# run receiver in this terminal
make server
# or directly:
# ./receiver
```

Terminal B — start the sender (client) in foreground:
```bash
# run sender in this terminal (defaults to 127.0.0.1:8080)
make client
# or directly:
# ./sender 127.0.0.1 8080
```

Important notes about current behavior (code state)
- The current `receiver` implementation handles one client session and then exits after the client disconnects. If you want the server to accept multiple clients sequentially, restart it after it exits or update `receiver.c` to continue the accept loop (recommended).
- The current `sender` exits immediately if it cannot connect to the server. Start `receiver` first to avoid the sender exiting. (If you prefer automatic reconnect, you can replace `sender.c` with the reconnecting variant — I can provide that.)

Stop either program by pressing Ctrl+C in the terminal where it runs.

---

## 6. Protocol & sizes (important details)

Packed Manchester encoding:
- Each data bit → 2 encoded bits.
- Each input byte (8 data bits) → 16 encoded bits → 2 encoded bytes.
- Sender appends 2 CRC bytes before encoding.

Formulas:
- message_len = number of bytes in the user message (e.g., `"This"` = 4)  
- total_input_bytes = message_len + 2 (CRC)  
- encoded_bytes_sent = total_input_bytes * 2  
- Receiver decodes: decoded_bytes = encoded_bytes_received / 2  
- Original message length at receiver = decoded_bytes - 2

Example:
- Message `"This"` (4 bytes) → total_input_bytes = 6 → encoded_bytes_sent = 12

Wire format:
```
[4-byte BE length N][N bytes encoded payload]
```
where N = encoded_bytes_sent. This length-prefix prevents issues caused by TCP stream fragmentation.

---

## 7. Error injection & interpreting flipped positions

- The sender flips encoded *bits* and reports 0-based encoded-bit indices `p`.
- Mapping helpers:
  - encoded_byte_index = p / 8  
  - bit_in_byte (MSB-first) = 7 - (p % 8)  
  - original_bit_index = p / 2  
  - original_byte_index = original_bit_index / 8  
  - original_bit_in_byte (MSB-first) = 7 - (original_bit_index % 8)


---

## 8. Testing features

Menu options in sender:
- 0 — No error  
- 1 — Single-bit flip  
- 2 — Two isolated single-bit flips  
- 3 — Odd number of bit flips (3)  
- 4 — Burst of 8 consecutive bit flips  
- 5 — Burst of 17 consecutive bit flips  
- 6 — Burst of 22 consecutive bit flips

CRC-16 (CCITT) will detect most single-bit, many odd and burst errors. Some rare bit patterns can evade any CRC — this project is for demonstration, not cryptographic guarantees.

---

## 9. Troubleshooting

- bind: Address already in use — another process is listening on the port. Find and stop it:
  - `sudo ss -ltnp | grep :8080`  
  - `sudo lsof -iTCP:8080 -sTCP:LISTEN -P -n`  
  - then `kill <PID>` (or `kill -9 <PID>` if necessary).  
- If you edited files on Windows, convert CRLF to LF:
  - `dos2unix Makefile` or `sed -i 's/\r$//' Makefile`.  
- If the sender exits because the server is not up:
  - Start `receiver` first and then run `sender`, or replace `sender.c` with a reconnecting variant.

---

## 10. Recommended code improvements (optional)

If you want the repository to be more robust, consider:
- Make `receiver` keep running and accept multiple clients (remove the `close(serv); return;` that currently exits after one client).
- Add reconnect logic to `sender` (so it waits and retries until the server comes back).
- Change flipped-position printing to `byte.bit` format for readability.
- Add a small header field with the original plaintext length (optional) so the receiver doesn't rely on `dec_len - 2` to find the payload length.
- Add unit tests for `man_encode()` ↔ `man_decode()` and `crc16()`.

---

## 11. Example Makefile targets

- `make` — build `sender` and `receiver`  
- `make server` — run `receiver` in the current terminal (foreground)  
- `make client` — run `sender` in the current terminal (foreground)  
- `make clean` — remove binaries and logs

---

## 12. References

- CRC polynomial (CRC-CCITT): x^16 + x^12 + x^5 + 1 (0x1021)

## 13. Environment used
- Tested on Linux / WSL (Windows Subsystem for Linux)

---

## 14. Conclusion

This project simulates a robust digital communication system featuring:

* Data integrity using **CRC-16**
* Physical-layer encoding via **Manchester coding**
* Realistic channel errors (bit and burst errors)
* **Socket-based sender/receiver architecture**

It demonstrates how digital communication protocols ensure reliability and detect transmission errors in practical systems. <br>
