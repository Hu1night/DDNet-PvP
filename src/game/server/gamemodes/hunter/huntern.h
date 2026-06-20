/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#ifndef GAME_SERVER_GAMEMODES_HUNTERN_H
#define GAME_SERVER_GAMEMODES_HUNTERN_H

#include <game/server/gamemodes/hunter.h>

#define HUNTERN_VERSION "0.5a0_p5"
#define HUNTERN_REPO "https://github.com/Hu1night/DDNet-PvP/branches"

class IClientIDRNG
{
public:
	IClientIDRNG() {};
	~IClientIDRNG() {};
	const char *m_apNameDesc;
	virtual void Update(CClientMask &SelectMask) {};
	virtual void Select(CClientMask &SelectMask, int Num, CClientMask &MaskSelect) {};
	virtual void Remove(int CID) {};
	unsigned int GetRand()
	{
		return rand();
		int r;
		secure_random_fill(&r, sizeof(r));
		return r;
	}
};

class CClientIDRNGMask : public IClientIDRNG
{
public:
	CClientMask m_Mask;
	CClientIDRNGMask()
	{
		m_apNameDesc = "抽牌随机 - 随机抽取玩家直到所有玩家都被抽取一次";
	};
	~CClientIDRNGMask() {};
	virtual void Update(CClientMask &SelectMask)
	{
		m_Mask &= SelectMask;
		if(m_Mask.none())
			m_Mask = SelectMask;
	}
	virtual void Select(CClientMask &SelectMask, int Num, CClientMask &MaskSelect)
	{
		for(int i = 0; i < Num; i++, Update(SelectMask))
		{
			int HunterSlot = GetRand() % m_Mask.count();
			int PlayerSlot = 0;
			for(int j = 0; j < MAX_CLIENTS; j++)
			{
				if(!m_Mask.test(j))
					continue;
				if(PlayerSlot++ < HunterSlot)
					continue;

				MaskSelect.set(j);
				m_Mask.set(j, false);
				break;
			}
		}
	}
	virtual void Remove(int CID) {};
};

class CClientIDRNGWeight : public IClientIDRNG
{
public:
	unsigned char m_aWeight[MAX_CLIENTS];
	CClientIDRNGWeight()
	{
		mem_zero(m_aWeight, sizeof(m_aWeight));
		m_apNameDesc = "权重随机 - 所有玩家每回合添加权重 抽到则减少权重";
	}
	virtual void Update(CClientMask &SelectMask) override
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(SelectMask.test(i))
				m_aWeight[i] += m_aWeight[i] ? 3 : 1;
			else
				m_aWeight[i] = 0;
		}
	}
	virtual void Select(CClientMask &SelectMask, int Num, CClientMask &MaskSelect) override
	{
		int WeightCount = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
			if(SelectMask.test(i))
				WeightCount += m_aWeight[i];
		if(!WeightCount)
			return;
		for(int i = 0; i < Num; i++)
		{
			int TargetWeight = GetRand() % WeightCount;
			for(int j = 0; j < MAX_CLIENTS; j++)
			{
				if(!SelectMask.test(j))
					continue;
				TargetWeight -= m_aWeight[j];
				if(TargetWeight > 0)
					continue;
				WeightCount -= m_aWeight[j];
				m_aWeight[j] = 0;
				SelectMask.set(j, false);
				MaskSelect.set(j);
				break;
			}
		}
	}
	virtual void Remove(int CID) override
	{
		m_aWeight[CID] = 0;
	}
};

class CGameControllerHunterN : public CGameControllerHunter
{
public:
	static void ConShowRng(IConsole::IResult *pResult, void *pUserData);
	static void ConSetRng(IConsole::IResult *pResult, void *pUserData);
	static void ConSetClass(IConsole::IResult *pResult, void *pUserData);
	static void ConGiveWeapon(IConsole::IResult *pResult, void *pUserData);
	static void ConSetHeal(IConsole::IResult *pResult, void *pUserData);
	static void ConRevive(IConsole::IResult *pResult, void *pUserData);
	static void ConSign(IConsole::IResult *pResult, void *pUserData);

public:
	CGameControllerHunterN();
	void OnInit() override;

	virtual bool CanChangeTeam(class CPlayer *pPlayer, int &JoinTeam) override;
	virtual bool CanDeadPlayerFollow(const class CPlayer *pSpectator, const class CPlayer *pTarget) override { return sizeof(CGameControllerHunterN); }
	virtual void DoWincheckMatch() override;
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
	virtual void OnPreTick() override;
	virtual void OnSnap(int SnappingClient,
			CNetObj_GameInfo *pGameInfoObj, CNetObj_GameInfoEx *pGameInfoEx, CNetObj_GameData *pGameDataObj,
			protocol7::CNetObj_GameData *pGameData, protocol7::CNetObj_GameDataTeam *pGameDataTeam, protocol7::CNetObj_GameDataFlag *pGameDataFlag) override;
	virtual void OnWorldReset() override;

protected:
	int m_GameoverTime;
	int m_HunterDeathBroadcast;
	int m_HunterDeathSmoke;
	int m_HunterListBroadcast;
	int m_HunterRatio;
	int m_Wincheckdeley;
	//int m_PlayerCountOffset;

	int m_aHiddenScore[MAX_CLIENTS];
	int m_DoWincheckTick;
	CClientMask m_HunterMask;

	CClientIDRNGMask m_Rng;
	IClientIDRNG *m_apRng[2];
	int m_RngEnabled;
	//CClientMask m_aTeamMask[NUM_HUNTER_TEAMS];

	char m_aaHunterName[MAX_HUNTERS][MAX_NAME_LENGTH];
	int m_HunterLeft;
	int m_aTeam[MAX_CLIENTS];
	int m_aClassSelectedTick[MAX_CLIENTS];

// Data
protected:
	enum 
	{
		NUM_CLASS = 2,
		NUM_CLASS_DATA = 4,
	};
	enum
	{
		NORMAL_KILL,
		TEAM_KILL,
		NUM_KILL_TYPE,
	};
	struct ClassData
	{
		const char *m_pName;
		const int m_aKlllScore[NUM_KILL_TYPE];
		const char *m_pSpawnMsg;
		const char *Name() { return m_pName; };
		int KillScore(int KillType) { return m_aKlllScore[clamp(KillType, (int)NORMAL_KILL, (int)NUM_KILL_TYPE - 1)]; };
		const char *SpawnMsg() { return m_pSpawnMsg; };
	};
	ClassData m_aClassData[NUM_CLASS_DATA] = 
	{
		{"平民", {1, -1}, "你是平民Civic! 找出并消灭猎人以胜利!     \n猎人双倍伤害 有瞬杀锤子和高爆榴弹"},
		{"猎人", {4, -2}, "     你是猎人Hunter! 合作消灭平民以胜利!\n     猎人双倍伤害 有瞬杀锤子和高爆榴弹\n     能长按锤子追踪最近玩家和无伤榴弹跳"},
		{"内鬼", {6, -2}, "     请输入文本1"},
		{"归命者", {2, -2}, "     请输入文本2"},
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

// Toolbox
public:
	IClientIDRNG *GetRng(CClientMask PlayerMask)
	{
		return m_apRng[m_RngEnabled];
	}
	void SendHunterListChat(int ClientID = -1)
	{
		char aBuf[320];
		str_format(aBuf, sizeof(aBuf), "本局的 %d 个Hunter是：", static_cast<int>(m_HunterMask.count()));
		MakeHunterList(aBuf, sizeof(aBuf));
		SendChatTarget(ClientID, aBuf);
	}
	void MakeHunterList(char *aBuf, int Size)
	{
		int Num = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!m_HunterMask.test(i))
				continue;
			// class CPlayer *pPlayer = GetPlayerIfInRoom(i);
			// if(IsPlaying(pPlayer))
			// 	str_append(aBuf, Server()->ClientName(i), Size);
			// else
				str_append(aBuf, m_aaHunterName[Num], Size);
			str_append(aBuf, ", ", Size);
			Num++;
		}
	}

protected:
	void KillPlayer(CPlayer *pPlayer)
	{
		pPlayer->KillCharacter();
		pPlayer->m_RespawnDisabled = true;
		pPlayer->m_RespawnTick = 0x7FFFFFFF;
	}

	void ResetGame()
	{
		mem_zero(&m_aClass, sizeof(m_aClass));
		mem_zero(&m_aHiddenScore, sizeof(m_aHiddenScore));
		mem_zero(&m_aTeam, sizeof(m_aTeam));
		mem_zero(&m_aaHunterName, sizeof(m_aaHunterName));
		m_HunterMask.reset();
		m_DoWincheckTick = -1;
	}

	bool IsInRound(int CID)
	{
		return IsPlaying(CID) && m_aTeam[CID] > TEAM_NONE && m_aTeam[CID] < NUM_HUNTER_TEAMS;
	}
	int GetInRoundPlayer()
	{
		int Count = 0;
		for(int i = 0; i < MAX_CLIENTS; ++i)
			if(IsInRound(i))
				Count++;
		return Count;
	}
	int GetAlivePlayer()
	{
		int Count = 0;
		for(int i = 0; i < MAX_CLIENTS; ++i)
			if(IsAlive(i))
				Count++;
		return Count;
	}

	int GetPlayerGameTeam(int CID) { return m_aTeam[CID] - 1; }

	CClientMask GetPlayerMask()
	{
		CClientMask PlayerMask;
		PlayerMask.reset();
		for(int i = 0; i < MAX_CLIENTS; i++)
			if(GetPlayerIfInRoom(i))
				PlayerMask.set(i, true);
		return PlayerMask;
	}

	CClientMask GetPlayingMask()
	{
		CClientMask PlayerMask;
		PlayerMask.reset();
		for(int i = 0; i < MAX_CLIENTS; i++)
			if(IsPlaying(i))
				PlayerMask.set(i, true);
		return PlayerMask;
	}

	// int GetPlayerNum()
	// {
	// 	int Num = 0;
	// 	for(int i = 0; i < MAX_CLIENTS; i++)
	// 	{
	// 		if(GetPlayerIfInRoom(i))
	// 			Num++;
	// 	}
	// 	return Num;
	// }

	int GetPlayingNum()
	{
		int Num = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(IsPlaying(i))
				Num++;
		}
		return Num;
	}

	int GetHunterNum() { return clamp((GetPlayingNum() + 1) / m_HunterRatio, (int)MIN_HUNTERS, (int)MAX_HUNTERS); }

	// Sixup
	void UpdateGameInfo(int CID)
	{
		protocol7::CNetMsg_Sv_GameInfo GameInfoMsg;
		GameInfoMsg.m_GameFlags = protocol7::GAMEFLAG_TEAMS | protocol7::GAMEFLAG_SURVIVAL;
		GameInfoMsg.m_ScoreLimit = m_GameInfo.m_ScoreLimit;
		GameInfoMsg.m_TimeLimit = m_GameInfo.m_TimeLimit;
		GameInfoMsg.m_MatchNum = m_GameInfo.m_MatchNum;
		GameInfoMsg.m_MatchCurrent = m_GameInfo.m_MatchCurrent;

		Server()->SendPackMsg(&GameInfoMsg, MSGFLAG_VITAL | MSGFLAG_NORECORD, CID);

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!IsAlive(i))
				continue;
			protocol7::CNetMsg_Sv_Team TeamMsg;
			TeamMsg.m_ClientID = i;
			TeamMsg.m_CooldownTick = 0;
			TeamMsg.m_Silent = true;
			TeamMsg.m_Team = m_aTeam[i] - 1;
			Server()->SendPackMsg(&TeamMsg, MSGFLAG_VITAL | MSGFLAG_NORECORD, CID);
		}
	}
	void ResetGameInfo(int CID)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!IsAlive(i))
				continue;
			protocol7::CNetMsg_Sv_Team TeamMsg;
			TeamMsg.m_ClientID = i;
			TeamMsg.m_CooldownTick = 0;
			TeamMsg.m_Silent = true;
			TeamMsg.m_Team = TEAM_RED;
			Server()->SendPackMsg(&TeamMsg, MSGFLAG_VITAL | MSGFLAG_NORECORD, CID);
		}
	}
};

#endif // GAME_SERVER_GAMEMODES_HUNTERN_H
