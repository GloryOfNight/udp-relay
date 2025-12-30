# UDP Relay
Simple UDP packet relay that relays packets between two peers.

Allows client peers anonymously connect with each other, should they known common relay address and 128-bit guid value.

Relay doesn't wrap packets in data-types, it uses handshake process to identify clients and establish channel between them.

It's recommended to modify the relay to tailor for your specific handshake process.

Relay doesn't support more then 2 clients per channel. Multiple relay instances may solve that for you.

Relay automatically closes established channels after certain period of no communication between peers has passed.

This project is single-threaded by design.

You can find all available command-line arguments with `--help`.

[![Windows](https://github.com/GloryOfNight/udp-relay/actions/workflows/windows.yml/badge.svg)](https://github.com/GloryOfNight/udp-relay/actions/workflows/windows.yml)
[![Linux](https://github.com/GloryOfNight/udp-relay/actions/workflows/linux.yml/badge.svg)](https://github.com/GloryOfNight/udp-relay/actions/workflows/linux.yml)

# How it works

`peer A |NAT| <-> relay <-> |NAT| peer B.`

To start communication, first you need to agree on the following between peers:
- IP address of the relay
- Unique GUID (128-bit) value

Then you can start sending handshake packets every second or so until your client receives a packet back from the relay. This means a connection has been established, and you can proceed to send other packet data.

This process looks like this:
```
// peer A and B start sending handshake values to relay using the same GUID value
Peer A -- handshake packet with GUID (1,2,3,4) -->  Relay *acknowledges handshake packet*
Peer B -- handshake packet with GUID (1,2,3,4) -->  Relay *creates mapping between Peer A and Peer B*

// when you start to receive handshake packets on peers, that means relay is established
Peer A -- handshake packet with GUID (1,2,3,4) -->  Relay *Peer A has mapping for Peer B*
Peer B <-- handshake packet with GUID (1,2,3,4) --  Relay *Peer A has mapping for Peer B*

Peer B -- handshake packet with GUID (1,2,3,4) -->  Relay *Peer B has mapping for Peer A*
Peer A <-- handshake packet with GUID (1,2,3,4) --  Relay *Peer B has mapping for Peer A*

// now you can start communication freely via relay.
// it's crucial to use the same socket or bind the same port values while you want to utilize the relay.
// note that if communication between peers stops for ~30 seconds - relay will clear the mapping for addresses.
```

By default, the relay expects the following header for the handshake. `magicNumber` used to identify handshake packets and `guid` can't be null. Relay expects network byte order.

```c++
// code from - include/udp-relay/relay.hxx

// MUST override here or use UDP_RELAY_SECRET_KEY env var
constexpr std::string_view handshake_secret_key_base64 = "Zkw2SThGM2VndjZBcEMxNWZrSk85VTd4S2VERDZYdXI=";

// override if you feel like it or you want break compatability
constexpr uint32_t handshake_magic_number_host = 0x4B28000;
constexpr uint32_t handshake_magic_number_hton = ur::net::hton32(handshake_magic_number_host);

struct alignas(8) handshake_header
{
	uint32_t m_magicNumber{handshake_magic_number_host}; // relay packet identifier
	uint16_t m_length{};								 // support handhsake extensions
	uint16_t m_flags{};									 // support handhsake extensions
	guid m_guid{};										 // channel identifier (128-bit, 4 x uint32_t)
	uint64_t m_nonce{};									 // security nonce
	hmac m_mac{};					                     // hmac (32 bytes array)
};
constexpr uint16_t handshake_min_size = 64;
static_assert(sizeof(handshake_header) == handshake_min_size);

// relay uses HMAC_sha256 for message authentication
HMAC(EVP_sha256(), secret_key, 32, &nonce, sizeof(nonce), mac, &mac_len);
```

# Build

> [!WARNING]
> AVOID downloading source code from main branch directly since it might contain in-progress code. Only do that from tags or releases page!

Project depends on `OpenSSL::Crypto` component for additional packet validation.

Project uses certain C++23 features, like `<print>` and `<stacktrace>`. Expected to be compiled without issue using Clang >= 18.1.3 or MSVC >= 19.44.

### List all available presets:
```
cmake --list-presets
cmake --build --list-presets
```

### Start building
```
cmake --preset <preset>
cmake --build --preset <build-preset>
```
