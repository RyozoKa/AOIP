#pragma once

#if defined WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#define closesocket close
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif 

#ifdef EXPORT
#define AOIP_API __declspec(dllexport)
#else
#define AOIP_API __declspec(dllimport)
#endif

#ifdef WIN32

#define PACKED
#pragma pack(push, 1)

//#else
//#define PACKED __attribute__ ((__packed__))
#endif

//Streams can either be added or replaced (reconnect)
//The only way to differentiate them is by their channel blocks.
//We want an efficient way to check that. Using hashes isn't ideal because we can have anything from 1 to 128 channels from a single device and stream.
//For now, we'll just use a byte array and test

struct DEVICE
{
	char* DevName;
	unsigned int IP;
	unsigned char Channels;			//Number of channels streamed by this device
	unsigned char ChannelOffset;	//Starting offset for device channels e.g Channeloffset = 32, Channels = 32 resulting in channels 32 to 64 in the absolute range. Relative ranges are held in the Streams
};

struct StreamConfig
{
	char* StreamName;
	unsigned char ChOffset;
	unsigned char ChCount;
};

struct Interface
{
	char Name[128];	//Hardware name
	char Description[128];	//Readable Name
	unsigned int IP;	//Local interface IP
	unsigned char MAC[16];
	void* Handle;	//Internal
};

#define MAXSAMPLES 384 //Max number of total samples per packet @24 bits
#define FRAMESIZE 54
#define UDPPAYLOAD 42

#define S_INVALID 0x0
#define S_WAIT_OPEN 0x1
#define S_OPEN 0x2
#define S_WAIT_CLOSE 0x4
#define S_CLOSED 0x8
#define S_PENDING_INIT 0x10
#define S_PENDING_CONFIG 0x20

struct SAP
{
	union
	{
		unsigned char Flags;

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
		unsigned char bCompression : 1;
		unsigned char bEncrypted : 1;
		unsigned char MsgType : 1;
		unsigned char Reserved : 1;
		unsigned char AddrType : 1;
		unsigned char Version : 3;
#elif G_BYTE_ORDER == G_BIG_ENDIAN
		unsigned char Version : 3;
		unsigned char AddrType : 1;
		unsigned char Reserved : 1;
		unsigned char MsgType : 1;
		unsigned char bEncrypted : 1;
		unsigned char bCompression : 1;
#else 
#error "G_BYTE_ORDER should be big or little endian."
#endif

	} Flags;

	unsigned char AuthLen;
	unsigned short Hash;
	unsigned int SrcIP;
	char Type[16];
};

struct SDP
{
	SAP *SAPHeader;

	char* PTPMac;	//For transmission
	char* SDPData;

	char Raw[1024];

	//Media Data
	unsigned short TransmitterPort;
	unsigned int TransmitterIP;
	unsigned int MultigroupIP;
	char* DevName;
	unsigned int SampleRate; //Samples * frequency
	unsigned char Channels;
	unsigned char ByteDepth;	//Usually this will always be 24 bits or more, represented in bytes.

	//Control flow
	unsigned char PackSamples;
	unsigned char FrameSamples;
	unsigned char PacketPerFrame;	// FrameSamples / PackSamples

	//Per iteration
	unsigned char PacketIndex;

	//Sequence
	unsigned short Seq;

	//Access control, also used when a device is considered invalid
	unsigned char bFlags;

	unsigned short SampleIndex;
	unsigned int Timestamp;

	unsigned char PacketRate;

	unsigned int SessionID;
	unsigned int SessionVer;
	char* SessionLoc;
	unsigned short Size;

	char ChannelOffset;	//First channel, eg 32. 32-64 if the stream defines 32 channels

	unsigned char *SampleData;
	unsigned short DataSize;

	SOCKET Socket;


};

#ifdef WIN32
#pragma pack(pop)
#undef PACKED

#else
#undef PACKED
#endif

typedef void(*timer)(unsigned char Channels);

#include <atomic>
#include <iomanip>

#ifdef WIN32
#include <psapi.h>
#endif

#include <sstream>

#define VALIDINTERFACE(x) x->Handle

extern AOIP_API unsigned char Index;

extern AOIP_API std::atomic<unsigned short> ChannelCount;
extern AOIP_API SDP TransmissionStreams[16];
extern AOIP_API unsigned char NumTStreams;
extern AOIP_API SDP Streams[128];

AOIP_API Interface* GetInterfaces();
AOIP_API SDP* getSDPStreams();
AOIP_API unsigned char GetSDPCount();

//
//// Initializer functions, needs to be called in the order of declaration as seen below.
//
AOIP_API void InitializeIf(Interface*);
AOIP_API void InitializeSAP(const char* IP);
//It's crucial that the SampleDelay is some multiple of 16
AOIP_API void InitializeEngine(double _IntervalFreq, timer _Callback, unsigned char SampleDelay, float(*Buffer) /* at least 16*128*3 bytes in size */);

AOIP_API void SetDeviceName(const char*);
AOIP_API void CreateNewStream(const char* StreamName, unsigned char ChannelOffset, unsigned char NumChannels, unsigned char NumSamples, unsigned int SampleRate, unsigned int MultiIP);
AOIP_API void ClearTransmissionStreams();
//
//// Control flow functions
//
AOIP_API void BeginSAP();
AOIP_API void EndSAP();
AOIP_API void BeginSAPTransmission();
AOIP_API void EndSAPTransmission();

AOIP_API void BeginRTPRecv();
AOIP_API void EndRTPRecv();

AOIP_API void SetIPPrefix(unsigned char);

AOIP_API unsigned char* GetTxBuffer(unsigned char Index);

AOIP_API void AddStream(char* StreamName, unsigned char ChannelOffset);


AOIP_API void DelayUS(unsigned int usec);
AOIP_API void DumpBytes(const unsigned char*, unsigned int);

//AOIP_API extern unsigned char BData[2048];