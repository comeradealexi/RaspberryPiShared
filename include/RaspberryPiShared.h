#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <type_traits>
#include <cstring>
#include "RaspberryPiLog.h"

namespace RaspberryPi
{
	namespace VideoSurveillance
	{
		enum class DoorStatus
		{
			Open,
			Closed,
			Max,
		};

		static const char* k_szDoorStatusStrings[] =
		{
			"Open",
			"Closed",
		};
		static_assert(sizeof(k_szDoorStatusStrings) / sizeof(k_szDoorStatusStrings[0]) == (size_t)DoorStatus::Max, "Ensure Enum Matches String List");

		static inline const char* DoorStatusToString(DoorStatus ds)
		{
			return k_szDoorStatusStrings[(size_t)ds];
		}

		struct PacketHeader
		{
			uint32_t m_uiHash;
			uint32_t m_uiSize;
		};
		static constexpr uint32_t k_PacketHeaderSize = sizeof(PacketHeader);
		static constexpr char k_Port[] = "4999";
		static constexpr char k_PacketIdentifier[] = "VSID_RPI";

		static inline uint32_t CreatePacketHash(const char* pData, uint32_t uiDataSize)
		{
			const uint32_t Start = 0;
			const uint32_t End = uiDataSize - 1;

			uint32_t uiHash = pData[Start] + pData[End] + pData[Start];
			uiHash += pData[Start] * (pData[End] == 0 ? pData[End] + 1 : pData[End]);
			uiHash = uiHash << (pData[Start] % 16);
			uiHash = uiHash << (pData[End] % 4);
			for (size_t i = 0; i < uiDataSize; i++)
			{
				if ((pData[i] % 2) == 0)
					uiHash += pData[i] * pData[End - i] * uiHash;
				else
					uiHash += pData[i] + pData[End - i] + uiHash;
			}
			return uiHash * uiHash * uiHash;
		}

		template <typename T>
		static inline T OffsetByHeader(T pData)
		{
			if (std::is_pointer<T>::value)
				return (pData + k_PacketHeaderSize);
			else 
				return (pData - k_PacketHeaderSize);
		}

		static inline bool VerifyPacket(const char* pData, uint32_t uiDataSize)
		{
			if (uiDataSize >= k_PacketHeaderSize)
			{
				PacketHeader* pHeader = (PacketHeader*)pData;
				if (pHeader->m_uiSize == uiDataSize - k_PacketHeaderSize)
				{
					uint32_t uiHash = CreatePacketHash(pData + k_PacketHeaderSize, pHeader->m_uiSize);
					if (uiHash == pHeader->m_uiHash)
						return true;
				}
			}
			return false;
		}

		static void CreatePacket(char* szDest, const char* szSrcData, uint32_t uiSourceDataSize)
		{
			#warning "Consider using the type save CreatePacket function instead of this C-Style one"
			PacketHeader* pHeader = (PacketHeader*)szDest;
			pHeader->m_uiHash = CreatePacketHash(szSrcData, uiSourceDataSize);
			pHeader->m_uiSize = uiSourceDataSize;
			char* szDataPtr = szDest + k_PacketHeaderSize;
			memcpy(szDataPtr, szSrcData, uiSourceDataSize);
		}

		template <typename T>
		static void CreatePacket(void* pDataDest, const T& dataSource)
		{
			PacketHeader* pHeader = (PacketHeader*)pDataDest;

			const char* szSrcData = (const char*) &dataSource;
			auto uiSourceDataSize = sizeof(T);

			pHeader->m_uiHash = CreatePacketHash(szSrcData, uiSourceDataSize);
			pHeader->m_uiSize = uiSourceDataSize;
			char* szDataPtr = (char*)pDataDest + k_PacketHeaderSize;
			memcpy(szDataPtr, szSrcData, uiSourceDataSize);
		}
	}
}