// NetPlayMsgDef.h

#pragma once

#include <string.h>
#include <stdint.h>

// Network Byte Swap Functions
uint16_t netPlayByteSwap(uint16_t);
uint32_t netPlayByteSwap(uint32_t);
uint64_t netPlayByteSwap(uint64_t);

#pragma pack(push,4)

enum netPlayMsgType
{
	NETPLAY_AUTH_REQ,
	NETPLAY_AUTH_RESP,
	NETPLAY_LOAD_ROM_REQ,
	NETPLAY_SYNC_STATE_REQ,
	NETPLAY_SYNC_STATE_RESP,
	NETPLAY_RUN_FRAME_REQ,
	NETPLAY_CLIENT_STATE,
	NETPLAY_INFO_MSG,
	NETPLAY_ERROR_MSG,
	NETPLAY_CHAT_MSG,
	NETPLAY_PING_REQ,
	NETPLAY_PING_RESP,
};

enum netPlayerId
{
	NETPLAY_SPECTATOR = -1,
	NETPLAY_PLAYER1,
	NETPLAY_PLAYER2,
	NETPLAY_PLAYER3,
	NETPLAY_PLAYER4
};

static const uint32_t NETPLAY_MAGIC_NUMBER = 0xaa55aa55;

struct netPlayMsgHdr
{
	uint32_t  magic[2];
	uint32_t  msgId;
	uint32_t  msgSize;

	netPlayMsgHdr( uint32_t id, uint32_t size = sizeof(netPlayMsgHdr) )
	{
		magic[0] = NETPLAY_MAGIC_NUMBER;
		magic[1] = NETPLAY_MAGIC_NUMBER;
		msgId = id; msgSize = size;
	}

	void toHostByteOrder()
	{
		magic[0] = netPlayByteSwap(magic[0]);
		magic[1] = netPlayByteSwap(magic[1]);
		msgId    = netPlayByteSwap(msgId);
		msgSize  = netPlayByteSwap(msgSize);
	};
	void toNetworkByteOrder()
	{
		magic[0] = netPlayByteSwap(magic[0]);
		magic[1] = netPlayByteSwap(magic[1]);
		msgId    = netPlayByteSwap(msgId);
		msgSize  = netPlayByteSwap(msgSize);
	}
};

struct netPlayAuthReq
{
	netPlayMsgHdr  hdr;

	netPlayAuthReq(void)
		: hdr(NETPLAY_AUTH_REQ, sizeof(netPlayAuthReq))
	{
	}

	void toHostByteOrder()
	{
		hdr.toHostByteOrder();
	}

	void toNetworkByteOrder()
	{
		hdr.toNetworkByteOrder();
	}
};

struct netPlayAuthResp
{
	netPlayMsgHdr  hdr;

	char playerId;
	char userName[64];
	char pswd[72];

	netPlayAuthResp(void)
		: hdr(NETPLAY_AUTH_RESP, sizeof(netPlayAuthResp)), playerId(NETPLAY_SPECTATOR)
	{
		memset(pswd, 0, sizeof(pswd));
	}

	void toHostByteOrder()
	{
		hdr.toHostByteOrder();
	}

	void toNetworkByteOrder()
	{
		hdr.toNetworkByteOrder();
	}
};

struct netPlayTextMsgFlags
{
	static const uint32_t DISCONNECT = 0x00000001;
	static const uint32_t    WARNING = 0x00000002;
	static const uint32_t       INFO = 0x00000004;
};

template <size_t N=8>
struct netPlayTextMsg
{
	netPlayMsgHdr  hdr;

	unsigned short code;
	unsigned short flags;
	unsigned short dataSize;

	netPlayTextMsg(int type)
		: hdr(type, sizeof(netPlayTextMsg)), code(0), flags(0), dataSize(0)
	{
		hdr.msgSize = sizeof(*this) - N + 1;
		memset(data, 0, N);
	}

	void setFlag(uint32_t flag)
	{
		flags |= flag;
	}

	bool isFlagSet(uint32_t flag)
	{
		return (flags & flag) ? true : false;
	}

	const char *getBuffer()
	{
		return &data[0];
	}

	int printf(const char* format, ...)
	{
		int retval;
		va_list args;
		va_start(args, format);
		retval = ::vsnprintf(data, N, format, args);
		va_end(args);

		if (retval > static_cast<int>(N-1))
		{
			retval = static_cast<int>(N-1);
		}
		dataSize = retval;

		hdr.msgSize = sizeof(*this) - N + retval + 1;

		return retval;
	}

	void assign(const char *msg)
	{
		int i=0;

		while ( (i < (N-1)) && (msg[i] != 0) )
		{
			data[i] = msg[i]; i++;
		}

		data[i] = 0;
		dataSize = i;

		hdr.msgSize = sizeof(*this) - N + i + 1;
	}

	void append(const char *msg)
	{
		int i=dataSize, j=0;

		while ( (i < (N-1)) && (msg[j] != 0) )
		{
			data[i] = msg[j]; i++; j++;
		}
		data[i] = 0;
		dataSize = i;

		hdr.msgSize = sizeof(*this) - N + i + 1;
	}

	void toHostByteOrder()
	{
		hdr.toHostByteOrder();
		code  = netPlayByteSwap(code);
		flags = netPlayByteSwap(flags);
		dataSize = netPlayByteSwap(dataSize);
	}

	void toNetworkByteOrder()
	{
		hdr.toNetworkByteOrder();
		code  = netPlayByteSwap(code);
		flags = netPlayByteSwap(flags);
		dataSize = netPlayByteSwap(dataSize);
	}
private:
	char data[N];
};

struct netPlayLoadRomReq
{
	netPlayMsgHdr  hdr;

	uint32_t  fileSize;
	char fileName[256];

	netPlayLoadRomReq(void)
		: hdr(NETPLAY_LOAD_ROM_REQ, sizeof(netPlayLoadRomReq)), fileSize(0)
	{
		memset(fileName, 0, sizeof(fileName));
	}

	void toHostByteOrder()
	{
		hdr.toHostByteOrder();
		fileSize  = netPlayByteSwap(fileSize);
	}

	void toNetworkByteOrder()
	{
		hdr.toNetworkByteOrder();
		fileSize  = netPlayByteSwap(fileSize);
	}
};


struct netPlayRunFrameReq
{
	netPlayMsgHdr  hdr;

	uint32_t  flags;
	uint32_t  frameNum;
	uint8_t   ctrlState[4];
	uint8_t   catchUpThreshold;

	netPlayRunFrameReq(void)
		: hdr(NETPLAY_RUN_FRAME_REQ, sizeof(netPlayRunFrameReq)), flags(0), frameNum(0), catchUpThreshold(10)
	{
		memset( ctrlState, 0, sizeof(ctrlState) );
	}

	void toHostByteOrder()
	{
		hdr.toHostByteOrder();
		flags    = netPlayByteSwap(flags);
		frameNum = netPlayByteSwap(frameNum);
	}

	void toNetworkByteOrder()
	{
		hdr.toNetworkByteOrder();
		flags    = netPlayByteSwap(flags);
		frameNum = netPlayByteSwap(frameNum);
	}
};

struct netPlayClientState
{
	netPlayMsgHdr  hdr;

	uint32_t  flags;
	uint32_t  frameRdy; // What frame we have input ready for
	uint32_t  frameRun;
	uint32_t  opsFrame; // Last frame for ops data
	uint32_t  opsChkSum;
	uint32_t  ramChkSum;
	uint8_t   ctrlState[4];

	static constexpr uint32_t  PAUSE_FLAG  = 0x0001;
	static constexpr uint32_t  DESYNC_FLAG = 0x0002;

	netPlayClientState(void)
		: hdr(NETPLAY_CLIENT_STATE, sizeof(netPlayClientState)), flags(0),
		frameRdy(0), frameRun(0), opsChkSum(0), ramChkSum(0)
	{
		memset( ctrlState, 0, sizeof(ctrlState) );
	}

	void toHostByteOrder()
	{
		hdr.toHostByteOrder();
		flags     = netPlayByteSwap(flags);
		frameRdy  = netPlayByteSwap(frameRdy);
		frameRun  = netPlayByteSwap(frameRun);
		opsFrame  = netPlayByteSwap(opsFrame);
		opsChkSum = netPlayByteSwap(opsChkSum);
		ramChkSum = netPlayByteSwap(ramChkSum);
	}

	void toNetworkByteOrder()
	{
		hdr.toNetworkByteOrder();
		flags     = netPlayByteSwap(flags);
		frameRdy  = netPlayByteSwap(frameRdy);
		frameRun  = netPlayByteSwap(frameRun);
		opsFrame  = netPlayByteSwap(opsFrame);
		opsChkSum = netPlayByteSwap(opsChkSum);
		ramChkSum = netPlayByteSwap(ramChkSum);
	}
};

struct netPlayPingReq
{
	netPlayMsgHdr  hdr;

	uint64_t  hostTimeStamp;

	netPlayPingReq(void)
		: hdr(NETPLAY_PING_REQ, sizeof(netPlayPingReq)), hostTimeStamp(0)
	{
	}

	void toHostByteOrder()
	{
		hdr.toHostByteOrder();
		hostTimeStamp = netPlayByteSwap(hostTimeStamp);
	}

	void toNetworkByteOrder()
	{
		hdr.toNetworkByteOrder();
		hostTimeStamp = netPlayByteSwap(hostTimeStamp);
	}
};

struct netPlayPingResp
{
	netPlayMsgHdr  hdr;

	uint64_t  hostTimeStamp;

	netPlayPingResp(void)
		: hdr(NETPLAY_PING_RESP, sizeof(netPlayPingResp)), hostTimeStamp(0)
	{
	}

	void toHostByteOrder()
	{
		hdr.toHostByteOrder();
		hostTimeStamp = netPlayByteSwap(hostTimeStamp);
	}

	void toNetworkByteOrder()
	{
		hdr.toNetworkByteOrder();
		hostTimeStamp = netPlayByteSwap(hostTimeStamp);
	}
};


#pragma pack(pop)
