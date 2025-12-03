# UDP Relay
Simple UDP relay that relays packets between two peers.

Sometimes there is a need to make a connection between two peers that are behind NAT. Inspired by [TURN](https://datatracker.ietf.org/doc/html/rfc8656), but unlike TURN, this relay is simplified and intended only to relay UDP packets between two peers.

For peers to communicate through this relay, clients are only required to send a handshake packet with the same guid (128-bit) value. The relay will create a mapping for these clients and start relaying packets between them.

It's recommended to modify the relay code to better suit your specific needs, such as changing the default handshake packet. There are also a few security concerns; if security is important to you, you may want to address them.

This relay cannot have more than two peers within a single mapping. The relay uses ip:port to identify mapped addresses.

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
```c++
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

By default, the relay expects the following header for the handshake. magicNumber used to identify handshake packets and GUID can't be null. Relay expects header in network byte order, after communication channel is established - relay doesn't care about contents of received packets.

```c++
struct alignas(4) handshake_header     // aligned by 4
{
	uint32_t m_magicNumber{0x4B28000}; // required - 4 bytes - must be exact same number
	uint32_t m_reserved{};             // spacing
	guid m_guid{};                     // required - 4 x 4 bytes - must be not null
};
```
