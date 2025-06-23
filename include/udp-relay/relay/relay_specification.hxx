// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

// clang-format off

// Type (16 bits)
//
// This field indicates the type of the packet. It appears at the start of each message (bytes 0–1) and defines how the rest of the packet is to be interpreted.
//
// Value	Name						Description
// -----	------------------------	-----------------------------------------------
// 0		First						Reserved marker (not a valid packet)
// 1		CreateAllocationRequest		Request to create a new relay allocation
// 2		CreateAllocationResponse	Response to the allocation request
// 3		ChallengeResponse			Used for password or challenge-based auth
// 4		JoinSessionRequest			Request to join an existing session
// 5		JoinSessionResponse			Response to the join request
// 6		Last						Reserved marker (not a valid packet)
//
// -------------------------------------------------------------------------------------
//
// Length (16 bits)
//
// This field indicated additional payload length after password hash
//
// -------------------------------------------------------------------------------------
//
// MagicCookie (32 bits)
//
// Has constant value of 0x37e7c7e3, used to check if packet is relay protocol
//
// -------------------------------------------------------------------------------------
//
// Transaction ID (32 bits x 4)
//
// A unique GUID value indicating a specific request/response transaction between client and relay.
//
// -------------------------------------------------------------------------------------
//
// Session ID (32 bits x 4)
//
// A unique GUID value which would be used in Create Allocation Request. If left empty, relay will generate it in a Create Allocation Response.
//
// -------------------------------------------------------------------------------------
//
// Password Hash (64 bits)
//
// A hash value of a public or private relay password. Could be empty if relay public.
// If public password is used, relay may respond with a Challenge Response
// In that case you need to decode Challenge Secret with a hash of Secret Cookie Key #1
// then XOR password with a decoded secret and XOR it with a hash of Secret Cookie Key #2
// After which you need repeat your request with a newly encoded password
//
// -------------------------------------------------------------------------------------



// Create Allocation Request scheme
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 a b c d e f 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|            Type               |           Length              |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                           Magic Cookie                        |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                       Transaction ID (1/4)                    |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                       Transaction ID (2/4)                    |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                       Transaction ID (3/4)                    |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                       Transaction ID (4/4)                    |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                         Session ID (1/4)                      |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                         Session ID (2/4)                      |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                         Session ID (3/4)                      |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                         Session ID (4/4)                      |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                      Password Hash (64-bit)                   |
//|                                                               |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// Challenge Response scheme
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 a b c d e f 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|            Type               |           Length              |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                           Magic Cookie                        |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                       Transaction ID (1/4)                    |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                       Transaction ID (2/4)                    |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                       Transaction ID (3/4)                    |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                       Transaction ID (4/4)                    |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                         Secret (64-bit)                       |
//|                                                               |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

// Create Allocation Response scheme
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 a b c d e f 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|            Type               |           Length              |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                           Magic Cookie                        |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                       Transaction ID (1/4)                    |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                       Transaction ID (2/4)                    |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                       Transaction ID (3/4)                    |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                       Transaction ID (4/4)                    |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                         Session ID (1/4)                      |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                         Session ID (2/4)                      |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                         Session ID (3/4)                      |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|                         Session ID (4/4)                      |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+