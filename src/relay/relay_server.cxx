// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/relay/relay_server.hxx"

#include "udp-relay/log.hxx"
#include "udp-relay/networking/network_utils.hxx"
#include "udp-relay/relay/relay_consts.hxx"
#include "udp-relay/relay/relay_types.hxx"
#include "udp-relay/relay/relay_utils.hxx"
#include "udp-relay/utils.hxx"

#include <array>
#include <thread>
#include <vector>

relay_server::relay_server(const relay_server_params& params)
{
	m_params = params;
	m_lastTickTime = std::chrono::steady_clock::now();
}

void relay_server::start()
{
	LOG(Info, Relay, "Begin relay initialization. Port: {}. Worked port range {}-{} (external {}-{})", m_params.m_primaryPort, m_params.m_workerPortStart, m_params.m_workerPortEnd, m_params.m_workerExternalPortStart, m_params.m_workerExternalPortEnd);

	m_socket = udpsocketFactory::createUdpSocket();
	if (m_socket == nullptr || !m_socket->isValid())
	{
		LOG(Error, Relay, "Failed to create socket!");
		return;
	}

	if (!m_socket->bind(m_params.m_primaryPort))
	{
		LOG(Error, Relay, "Failed bind socket to {0} port", m_params.m_primaryPort);
		return;
	}

	if (!m_socket->setSendTimeoutUs(ur::consts::sendTimeoutUs))
	{
		LOG(Error, Relay, "Failed set socket send timeout");
		return;
	}

	if (!m_socket->setRecvTimeoutUs(ur::consts::recvTimeoutUs))
	{
		LOG(Error, Relay, "Failed set socket recv timeout");
		return;
	}

	int32_t actualSendBufferSize{};
	if (!m_socket->setSendBufferSize(ur::consts::desiredSendBufferSize, actualSendBufferSize))
		LOG(Error, Relay, "Error with setting send buffer size");

	int32_t actualRecvBufferSize{};
	if (!m_socket->setRecvBufferSize(ur::consts::desiredRecvBufferSize, actualRecvBufferSize))
		LOG(Error, Relay, "Error with setting recv buffer size");

	LOG(Info, Relay, "Relay main socket initialized on port: {}. SendBufSize: {}. RecvBufSize: {}.", m_socket->getPort(), actualSendBufferSize, actualRecvBufferSize);

	m_workers.reserve(m_params.m_workerPortEnd - m_params.m_workerPortStart);
	for (uint16_t i = m_params.m_workerPortStart; i < m_params.m_workerPortEnd; ++i)
	{
		relay_worker_params workerParams{};
		workerParams.workerId = i - m_params.m_workerPortStart;
		workerParams.port = i;

		auto& newRelayWorker = m_workers.emplace_back(std::make_unique<relay_worker>(workerParams));
		m_worker_threads.emplace_back(std::jthread(&relay_worker::start, newRelayWorker.get()));
	}

	m_running = true;
	while (m_running)
	{
		update();
	}
}

void relay_server::stop()
{
	LOG(Info, Relay, "Relay stop requested. Stopping all.");

	m_running = false;
	for (auto& worker : m_workers)
	{
		worker->stop();
	}
}

void relay_server::update()
{
	std::chrono::time_point<std::chrono::steady_clock> prevTickTime = m_lastTickTime;
	m_lastTickTime = std::chrono::steady_clock::now();

	if (!m_socket->waitForReadUs(1'000'000))
		return;

	int32_t bytesRecv;
	do
	{
		auto& buffer = m_recv_buffer; // alias

		bytesRecv = m_socket->recvFrom(buffer.data(), buffer.size(), &m_recv_addr);
		if (bytesRecv == -1)
			break;

		if (bytesRecv < ur::getHeaderSize())
			continue;

		// read header
		const ur::packetType type = static_cast<ur::packetType>(ur::ntoh16(*reinterpret_cast<uint16_t*>(buffer[0])));
		const uint16_t length = ur::ntoh16(*reinterpret_cast<uint16_t*>(buffer[2]));
		const guid transactionId = guid::ntoh(*reinterpret_cast<guid*>(buffer[8]));
		const uint32_t magicCookie = ur::ntoh32(*reinterpret_cast<uint32_t*>(buffer[4]));

		const bool bRelayProtocolPacket = magicCookie == ur::consts::magicCookie && type > ur::packetType::First && type < ur::packetType::Last;

		if (!bRelayProtocolPacket)
			continue;

		const bool bPacketSizeOk = bytesRecv < ur::getMinPacketSizeForType(type);
		const bool bPacketLengthOk = bytesRecv < ur::getMinPacketLengthForType(type);

		if (!bPacketSizeOk || !bPacketLengthOk)
			continue;

		switch (type)
		{
		case ur::packetType::CreateAllocationRequest:
			processAllocationRequest();
			break;
		case ur::packetType::JoinSessionRequest:
			// process join
			break;
		default:
			break;
		}
	} while (true);
}

void relay_server::processAllocationRequest()
{
	auto& buffer = m_recv_buffer; // alias

	const guid sessionId = guid::ntoh(*reinterpret_cast<guid*>(buffer[24]));
	const uint32_t password = ur::ntoh32(*reinterpret_cast<uint32_t*>(buffer[40]));

	const uint32_t xorPassword = password ^ m_params.m_secretKey1;
	if (xorPassword == m_params.m_publicPassword)
	{
		challengeResponse();
		return;
	}
	else if (xorPassword == m_params.m_privatePassword)
	{
		// allocate response
		return;
	}

	for (auto& challenge : m_challenges)
	{
		const uint32_t xorSecretPassword = xorPassword ^ challenge.m_secret;
		if (challenge.m_addr == m_recv_addr && xorSecretPassword == m_params.m_publicPassword)
		{
			// allocate response
			return;
		}
	}

	errorResponse(ur::errorType::WrongPassword, "Wrong password");
}

void relay_server::challengeResponse()
{
	relay_server_challenge* newChallenge{};
	for (size_t i = 0; i < m_challenges.size(); ++i)
	{
		auto duration = m_lastTickTime - m_challenges[i].m_sendTime;
		if (duration > std::chrono::milliseconds(m_params.m_challengeTimeoutMs))
			newChallenge = &m_challenges[i];
	}

	if (!newChallenge)
	{
		errorResponse(ur::errorType::Busy, "Too many requests. Try again later.");
		return;
	}

	newChallenge->m_addr = m_recv_addr;
	newChallenge->m_secret = ur::randRange<uint32_t>(0, UINT32_MAX);
	newChallenge->m_sendTime = std::chrono::steady_clock::now();

	constexpr uint16_t basePacketSize = ur::getMinPacketSizeForType(ur::packetType::ChallengeResponse);
	constexpr uint16_t basePacketLength = ur::getMinPacketLengthForType(ur::packetType::ChallengeResponse);

	std::array<uint8_t, basePacketSize> responseBuffer{};

	prepareResponseHeader(responseBuffer, ur::packetType::ChallengeResponse, basePacketLength);

	const uint32_t encodedSecret = newChallenge->m_secret ^ m_params.m_secretKey2;
	const uint32_t encodedSecretNo = ur::hton32(encodedSecret);
	uint16_t* secretPtr = reinterpret_cast<uint16_t*>(responseBuffer[24]);
	*secretPtr = encodedSecretNo;

	const int32_t bytesSend = m_socket->sendTo(responseBuffer.data(), responseBuffer.size(), &m_recv_addr);

	if (bytesSend == -1) [[unlikely]]
	{
		const int32_t errorNo = ur::getLastErrno();
		const std::string errorStr = ur::errnoToString(errorNo);
		LOG(Warning, Relay, "Failed send challenge response for - {}; Error: {} - {}", m_recv_addr.toString(), errorNo, errorStr);
		return;
	}

	LOG(Verbose, Relay, "Challenge response sent {}", m_recv_addr.toString());
}

void relay_server::allocationResponse()
{
}

void relay_server::errorResponse(ur::errorType errorType, std::string_view message)
{
	constexpr uint16_t MessageCopySizeCap = 1024; // avoid long messages
	const uint16_t messageCopySize = MessageCopySizeCap > message.size() ? message.size() : MessageCopySizeCap;

	constexpr uint16_t basePacketSize = ur::getMinPacketSizeForType(ur::packetType::ErrorResponse);
	constexpr uint16_t basePacketLength = ur::getMinPacketLengthForType(ur::packetType::ErrorResponse);
	const uint16_t totalPacketLength = basePacketLength + messageCopySize;

	std::vector<uint8_t> responseBuffer{};
	responseBuffer.resize(basePacketSize + messageCopySize);

	prepareResponseHeader(responseBuffer, ur::packetType::ChallengeResponse, totalPacketLength);

	uint16_t* type = reinterpret_cast<uint16_t*>(responseBuffer[24]);
	*type = ur::hton16(static_cast<uint16_t>(errorType));

	uint16_t* length = reinterpret_cast<uint16_t*>(responseBuffer[26]);
	*length = ur::hton16(message.size());

	std::memcpy(responseBuffer.data() + basePacketSize, message.data(), messageCopySize);

	const int32_t bytesSend = m_socket->sendTo(responseBuffer.data(), responseBuffer.size(), &m_recv_addr);

	if (bytesSend == -1) [[unlikely]]
	{
		const int32_t errorNo = ur::getLastErrno();
		const std::string errorStr = ur::errnoToString(errorNo);
		LOG(Warning, Relay, "Failed send error response for - {}; Error: {} - {}", m_recv_addr.toString(), errorNo, errorStr);
		return;
	}

	LOG(Verbose, Relay, "Error response sent {}", m_recv_addr.toString());
}
