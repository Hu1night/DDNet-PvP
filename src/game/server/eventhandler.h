/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_EVENTHANDLER_H
#define GAME_SERVER_EVENTHANDLER_H

#include <base/system.h>
#include <base/vmath.h>
#include <engine/shared/protocol.h>

inline CClientMask CmaskAll() { CClientMask Mask; return Mask.set(); }
inline CClientMask CmaskOne(int ClientID) {  CClientMask Mask; return Mask.set(ClientID); }
inline CClientMask CmaskSet(CClientMask Mask, int ClientID) { return Mask.set(ClientID); }
inline CClientMask CmaskUnset(CClientMask Mask, int ClientID) { return Mask.set(ClientID, false); }
inline CClientMask CmaskAllExceptOne(int ClientID) { return ~CmaskOne(ClientID); }
inline bool CmaskIsSet(CClientMask Mask, int ClientID) { return Mask.test(ClientID); }

class CEventHandler
{
	static const int MAX_EVENTS = 128;
	static const int MAX_DATASIZE = 128 * 64;

	int m_aTypes[MAX_EVENTS]; // TODO: remove some of these arrays
	int m_aOffsets[MAX_EVENTS];
	int m_aSizes[MAX_EVENTS];
	CClientMask m_aClientMasks[MAX_EVENTS];
	char m_aData[MAX_DATASIZE];

	class CGameContext *m_pGameServer;
	class IGameController *m_pController;

	int m_CurrentOffset;
	int m_NumEvents;

public:
	CGameContext *GameServer() const { return m_pGameServer; }
	IGameController *Controller() const { return m_pController; }
	void SetGameServer(CGameContext *pGameServer, IGameController *pController);

	CEventHandler();
	void *Create(int Type, int Size, CClientMask Mask = CmaskAll());
	void Clear();
	void Snap(int SnappingClient);

	bool OverrideEvent(int SnappingClient, int *Type, int *Size, const char **Data);
};

#endif
