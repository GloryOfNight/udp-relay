# UDP Relay
Simple udp relay that relays packets between two peers.

Sometimes there is a need to make connection between to peers that are behind NAT possible. Inspired by [TURN](https://datatracker.ietf.org/doc/html/rfc8656), but unlike TURN, this relay is simplified and specializes on relaying only udp packets between two peers.

For peers to communitate thru such relay, clients only required to send handshake packet with same guid(128bit) values. Relay would create mapping for these clients and start relaying packets between them. After clients exchange handhsakes setup is done. Relay would relay packets indefenetly, but when both peers stop communite with each other for long period of time - repeating handhsake process might be required.

It's recommended to modify relay code to better suit your specific needs, like changing default handshake packet. There is also few security conserns, if security important for you, you might want to address them.

It's not possible for this relay to have more then 2 peers within one mapping. Relay uses ip:port to identify other mapped address.

It's single thread only. Thats is intentional.

You can find all possible commandline arguments with `--help`.

[![Windows](https://github.com/GloryOfNight/udp-relay/actions/workflows/windows.yml/badge.svg)](https://github.com/GloryOfNight/udp-relay/actions/workflows/windows.yml)
[![Linux](https://github.com/GloryOfNight/udp-relay/actions/workflows/linux.yml/badge.svg)](https://github.com/GloryOfNight/udp-relay/actions/workflows/linux.yml)

# How it works

`peer A |NAT| <-> relay <-> |NAT| peer B. `

Basicly, to have connection trhu such relay you need to negotiate between clients only two things
- ip address of relay
- guid value used to map clients between each other

After clients start receive packets from relay - that would indicate that connection is established.
Relay would close mapping after 30 seconds of inactivity (by default).

```
// peers start sending handshake values to relay
Peer A -- handhsake packet with guid (1,2,3,4) -->  Relay *acknowledges handshake packet*
Peer B -- handhsake packet with guid (1,2,3,4) -->  Relay *creates mapping between Peer A and Peer B*

// when you starting to receive handhsake packets on peers side, that mean relay is established
Peer A -- handhsake packet with guid (1,2,3,4) -->  Relay *Peer A has mapping for Peer B*
Peer B <-- handhsake packet with guid (1,2,3,4) --  Relay *Peer A has mapping for Peer B*

Peer B -- handhsake packet with guid (1,2,3,4) -->  Relay *Peer B has mapping for Peer A*
Peer A <-- handhsake packet with guid (1,2,3,4) --  Relay *Peer B has mapping for Peer A*

// now you can start communication freely via relay.
// it's crutial to use same socket or bind same port values while you want to utilize relay.
// note that if communication between peers stop for ~30 seconds - relay would clear mapping for addresses.
```

By default, relay expects following header for handshake. 
Those fields that optional mostly needed for client application to establish handshake protocol. Only required one for relay to function - m_guid.

```c++
struct handshake_header
{
	uint16_t m_type{};    // optional
	uint16_t m_length{};  // optional
	guid m_guid{};        // required - 4 x uint32
	int64_t m_time{};     // optional
};
```
> [!NOTE]
> You probably should tailor [handshake header](include/udp-relay/types.hxx#L43) for your specific needs.
