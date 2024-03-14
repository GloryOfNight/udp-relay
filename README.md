# UDP Relay
Same as [TURN](https://datatracker.ietf.org/doc/html/rfc8656), sometimes there is a need to make connection between to peers that are behind NAT possible. But, unlike TURN extension, that's a relay is very simple to integate and specializes in relaying only udp packets from Peer A/B to Peer B/A (between two peers).

For peers to communitate thru such relay, clients only required to send handshake packet with same guid(128bit) values. Relay would create mapping for these clients and start relaying packets between them. After clients exchange handhsakes setup is done. Relay would relay packets indefenetly, but when both peers stop communite with each other for long period of time - repeating handhsake process might be required. 

It's recommended to modify relay code to better suit your specific needs. There is also a security conserns, if security important for you, you might want to address it.

It's not possible for this relay to have more then 2 peers within one mapping.

You can check commandline arguments with `--help`.

# How it works

```
Peer A -- handhsake packet with guid (1,2,3,4) --->  Relay *acknowledges handshake packet*
Peer B -- handhsake packet with guid (1,2,3,4) --->  Relay *creates mapping for Peer A and Peer B*
Peer C -- handhsake packet with guid (1,2,3,4) --->  Relay *drops packet*
Peer D -- unkown packet from unknown peer      --->  Relay *drops packet* 

Peer A -- handhsake packet with guid (1,2,3,4) --->  Relay *Peer A has mapping for Peer B* -- relay packet ---> Peer B
Peer B -- handhsake packet with guid (1,2,3,4) --->  Relay *Peer B has mapping for Peer A* -- relay packet ---> Peer A
```
