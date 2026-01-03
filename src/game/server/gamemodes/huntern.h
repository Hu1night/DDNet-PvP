/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#ifndef GAME_SERVER_GAMEMODES_HUNTERN_H
#define GAME_SERVER_GAMEMODES_HUNTERN_H

#include <game/server/gamemodes/hunter.h>

class CGameControllerHunterN : public CGameControllerHunter
{
public:
	CGameControllerHunterN();
	void OnInit() override;

	virtual bool CanChangeTeam(class CPlayer *pPlayer, int &JoinTeam) override;
	virtual bool CanDeadPlayerFollow(const class CPlayer *pSpectator, const class CPlayer *pTarget) override { return true; }
	virtual void DoWincheckRound() override;
	virtual bool IsDisruptiveLeave(class CPlayer *pPlayer) const override;
	virtual bool IsSpawnRandom() const override { return true; }
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	virtual void OnCharacterSpawn(class CCharacter *pChr) override;
	virtual void OnPlayerJoin(class CPlayer *pPlayer) override;
	virtual void OnPlayerLeave(class CPlayer *pPlayer) override;
	virtual void OnPlayerSnap(class CPlayer *pPlayer, int SnappingClient,
			CNetObj_ClientInfo *pClientInfo, CNetObj_PlayerInfo *pPlayerInfo, CNetObj_SpectatorInfo *pSpectatorInfo, // 0.6
			protocol7::CNetObj_PlayerInfo *pPlayerInfo7, protocol7::CNetObj_SpectatorInfo *pSpectatorInfo7, // 0.7
			CNetObj_DDNetSpectatorInfo *pDDNetSpectatorInfo, CNetObj_DDNetPlayer *pDDNetPlayer) override;
	//bool OnPlayerTryRespawn(class CPlayer *pPlayer, vec2 Pos) override;
	virtual void OnSnap(int SnappingClient,
			CNetObj_GameInfo *pGameInfoObj, CNetObj_GameInfoEx *pGameInfoEx, CNetObj_GameData *pGameDataObj,
			protocol7::CNetObj_GameData *pGameData, protocol7::CNetObj_GameDataTeam *pGameDataTeam, protocol7::CNetObj_GameDataFlag *pGameDataFlag) override;
	virtual void OnWorldReset() override;

protected:
	int m_HiddenScore[MAX_CLIENTS];
	int m_DoWincheckTick;
	CClientMask m_SelectMask;
	CClientMask m_HunterMask;
	//CClientMask m_TeamMask[NUM_HUNTER_TEAMS];

	char m_aaHunterName[MAX_HUNTERS][MAX_NAME_LENGTH];
	int m_HunterLeft;
	int m_Team[MAX_CLIENTS];
	int m_TeamLeft{NUM_HUNTER_TEAMS};

	int m_GameoverTime;
	int m_HunterDeathBroadcast;
	int m_HunterDeathEffect;
	int m_HunterListBroadcast;
	int m_HunterRatio;
	int m_Wincheckdeley;

// Toolbox
public:
	void MakeHunterList(char *aBuf, int Size)
	{
		int Num = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!m_HunterMask.test(i))
				continue;
			class CPlayer *pPlayer = GetPlayerIfInRoom(i);
			if(IsPlaying(pPlayer))
				str_append(aBuf, Server()->ClientName(i), Size);
			else
				str_append(aBuf, m_aaHunterName[Num], Size);
			Num++;
		}
	}

protected:
	void ResetPlayer()
	{
		mem_zero(&m_Team, sizeof(m_Team));
		mem_zero(&m_Class, sizeof(m_Class));
		mem_zero(&m_HiddenScore, sizeof(m_HiddenScore));
	}

	void ResetPlayer(int CID)
	{
		m_Class[CID] = HunterClass::ID_NONE;
		m_Team[CID] = TEAM_NONE;
		m_HiddenScore[CID] = 0;
		m_SelectMask.set(CID, false);
	}

	void ResetGame()
	{
		m_DoWincheckTick = -1;
		mem_zero(&m_aaHunterName, sizeof(m_aaHunterName));
		m_HunterMask.reset();
	}

	CClientMask GetPlayerMask()
	{
		CClientMask PlayerMask;
		PlayerMask.reset();
		for(int i = 0; i < MAX_CLIENTS; i++)
			if(GetPlayerIfInRoom(i))
				PlayerMask.set(i, true);
		return PlayerMask;
	}

	void UpdateSelectMask(CClientMask &Mask)
	{
		if(!Mask.none())
		{
			Mask &= GetPlayerMask();
		}
		else
			Mask = GetPlayerMask();
	}

	int GetPlayerNum()
	{
		int Num = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GetPlayerIfInRoom(i))
				Num++;
		}
		return Num;
	}

	int GetHunterNum() { return (GetPlayerNum() + 2) >> 2; }

	int RandHunter(CClientMask &Mask)
	{
		int PlayerRange = Mask.count();
		int RandSlot = GetRand() % PlayerRange;
		for(int i = 0, j = 0; i < MAX_CLIENTS; i++)
		{
			if(!Mask.test(i))
				continue;
			if(RandSlot != j)
			{
				j++;
				continue;
			}
			return i;
		}
		return -1;
	}

	const char *m_apClassSpawnMsg[HunterClass::NUM_CLASS_ID - 1] =
	{
		{"你是平民Civic! 找出并消灭猎人以胜利!     \n猎人双倍伤害 有瞬杀锤子和高爆榴弹"},
		{"     你是猎人Hunter! 合作消灭平民以胜利!\n     猎人双倍伤害 有瞬杀锤子和高爆榴弹\n     能长按锤子追踪最近玩家和无伤榴弹跳"},
	};
	const char *m_apClassName[HunterClass::NUM_CLASS_ID - 1] =
	{
		{"平民"},
		{"猎人"},
	};
	const char *m_apWeaponName[7] =
	{
		{"地刺"},
		{"锤子"},
		{"手枪"},
		{"霰弹"},
		{"榴弹"},
		{"激光"},
		{"忍者刀"},
	};
	const int m_aKillScoreData[HunterClass::NUM_CLASS_ID][2] =
	{
		{0, 0}, // ID_NONE
		{1, -1}, // ID_CIVIC
		{4, -2}, // ID_HUNTER
	};
};

#endif // GAME_SERVER_GAMEMODES_HUNTERN_H
