// Copyright(c) 2025 Siarhei Dziki aka "GloryOfNight"

#include "udp-relay/version.hxx"

uint32_t ur::getVersion()
{
	return (((uint32_t)(UR_PROJECT_VERSION_MAJOR)) << 22U) | (((uint32_t)(UR_PROJECT_VERSION_MINOR)) << 12U) | ((uint32_t)(UR_PROJECT_VERSION_PATCH));
}

uint32_t ur::getVersionMajor()
{
	return UR_PROJECT_VERSION_MAJOR;
}

uint32_t ur::getVersionMinor()
{
	return UR_PROJECT_VERSION_MINOR;
}

uint32_t ur::getVersionPatch()
{
	return UR_PROJECT_VERSION_PATCH;
}