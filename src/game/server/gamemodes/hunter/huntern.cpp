/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "huntern.h"

#include <engine/shared/console.h>
#include <game/server/entities/character.h>
#include <game/server/weapons.h>
#include <game/version.h>

CGameControllerHunterN::CGameControllerHunterN() :
	CGameControllerHunter()
{
	INSTANCE_CONFIG_INT(&m_GameoverTime, "htn_gameover_time", 6, 0, 0x7FFFFFFF, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "结算界面时长秒数 (整数, 默认6, 限制0~2147483647)");
	INSTANCE_CONFIG_INT(&m_HunterDeathBroadcast, "htn_hunt_death_broadcast", 1, 0, 2, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "是否广播猎人死亡(1=仅猎人广播, 2=全体广播) (开关, 默认0, 限制0~2)");
	INSTANCE_CONFIG_INT(&m_HunterDeathSmoke, "htn_hunt_death_smoke", 0, 0, 1, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "猎人死亡是否使用出生烟 (开关, 默认0, 限制0~1)");
	INSTANCE_CONFIG_INT(&m_HunterListBroadcast, "htn_hunt_list_broadcast", 0, 0, 1, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "是否全体广播猎人列表 (开关, 默认0, 限制0~1)");
	INSTANCE_CONFIG_INT(&m_HunterRatio, "htn_hunt_ratio", 4, 2, MAX_CLIENTS, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "几个玩家里选取一个猎人 (整数, 默认4, 限制2~64)");
	INSTANCE_CONFIG_INT(&m_Wincheckdeley, "htn_wincheck_deley", 200, 0, 0x7FFFFFFF, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "终局判断延时毫秒 (整数, 默认200, 限制0~2147483647)");

	InstanceConsole()->Register("htn_setrng", "?i[rngid]", CFGFLAG_CHAT | CFGFLAG_INSTANCE, ConSetRng, this, "设置玩家抽取随机器");
	InstanceConsole()->Register("htn_setclass", "i[class-id] ?i[CID] ?i[team-id]", CFGFLAG_CHAT | CFGFLAG_INSTANCE, ConSetClass, this, "给玩家设置职业");
	InstanceConsole()->Register("htn_giveweapon", "i[weapon-id] i[slot] ?i[CID] ?i[ammo-num] ?i[is-powerup]", CFGFLAG_CHAT | CFGFLAG_INSTANCE, ConGiveWeapon, this, "给玩家武器");
	InstanceConsole()->Register("htn_setheal", "i[health] ?i[armor] ?i[CID] ?i[max-health] ?i[max-armor]", CFGFLAG_CHAT | CFGFLAG_INSTANCE, ConSetHeal, this, "给玩家血量和盾");
	InstanceConsole()->Register("htn_revive", "?i[CID]", CFGFLAG_CHAT | CFGFLAG_INSTANCE, ConRevive, this, "复活");
	InstanceConsole()->Register("htn_sign", "s[message]", CFGFLAG_CHAT | CFGFLAG_INSTANCE, ConSign, this, "打字");

	m_apRng[0] = new CClientIDRNGMask();
	m_apRng[1] = new CClientIDRNGWeight();
	m_RngEnabled = 0;

	m_pGameType = "hunterN";
	m_GameFlags = IGF_SURVIVAL | IGF_ROUND_TIMER_ROUND;
}

void CGameControllerHunterN::OnInit()
{
	CGameControllerHunter::OnInit();
	mem_zero(&m_aHiddenScore, sizeof(m_aHiddenScore));
	mem_zero(&m_aTeam, sizeof(m_aTeam));
	mem_zero(&m_aMapRotations, sizeof(m_aMapRotations));
	mem_zero(&m_aMapMask, sizeof(m_aMapMask));
	ResetGame();
}

bool CGameControllerHunterN::CanChangeTeam(class CPlayer *pPlayer, int &JoinTeam)
{
	OnPlayerLeave(pPlayer);
	if(pPlayer->GetTeam() != TEAM_SPECTATORS && !IsAlive(pPlayer)) // when player is dead in survival, then join spec
		JoinTeam = TEAM_SPECTATORS;
	return true;
}

void CGameControllerHunterN::DoWincheckMatch()
{
	if(m_GameInfo.m_MatchNum <= 0 || m_GameInfo.m_MatchCurrent < m_GameInfo.m_MatchNum)
		return;

	int PrevMapIndex = m_MapIndex;
	if(CycleMap())
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "地图循环: '%s' -> '%s'", GameServer()->Teams()->GetMapName(PrevMapIndex), GameServer()->Teams()->GetMapName(m_MapIndex));
		SendChatTarget(-1, aBuf);
	}

	SetGameState(IGS_END_MATCH, m_GameoverTime);
}

void CGameControllerHunterN::DoWincheckRound()
{
	bool IsTimeEnd = (m_GameInfo.m_TimeLimit > 0 && (Server()->Tick() - m_GameStartTick) >= m_GameInfo.m_TimeLimit * Server()->TickSpeed() * 60);

	if(m_DoWincheckTick != Server()->Tick() && !IsTimeEnd)
		return;

	int TeamCount[NUM_HUNTER_TEAMS] = {0};
	int PlayingCount = 0;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(IsPlaying(i))
			PlayingCount++;
		if(IsAlive(i))
			TeamCount[m_aTeam[i]] += 1;
	}

	std::bitset<NUM_HUNTER_TEAMS> EndType;
	for(int i = 0; i < NUM_HUNTER_TEAMS; ++i)
		EndType.set(i, TeamCount[i]);

	if(!IsTimeEnd && EndType.count() > 1)
		return;

	// 游戏结束
	SetGameState(IGS_END_ROUND, PlayingCount > 2 ? m_GameoverTime : m_GameoverTime / 3);

	m_aTeamscore[TEAM_RED] = TeamCount[TEAM_CIVIC];
	m_aTeamscore[TEAM_BLUE] = TeamCount[TEAM_HUNTER];

	
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		CPlayer *pPlayer = GetPlayerIfInRoom(i);
		if(!IsPlaying(pPlayer))
			continue;
		int Score = m_aHiddenScore[i];
		if(IsAlive(pPlayer))
		{
			if(m_aTeam[i] != TEAM_HUNTER)
				Score += 2;
		}
		else
		{
			char aBuf[32];
			str_format(aBuf, sizeof(aBuf), "本回合你拿到了 %d 分", Score);
			SendChatTarget(i, aBuf);
		}

		pPlayer->m_Score += Score;
	}

	if(IsTimeEnd)
	{
		//SendChatTarget(-1, m_HunterList);
		SendChatTarget(-1, "游戏结束！");
		GameWorld()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
	}
	else if(EndType.none()) // nobody
	{
		SendChatTarget(-1, "两人幸终！");
	}
	else if(EndType == 1 << TEAM_CIVIC)
	{
		//SendChatTarget(-1, m_HunterList);
		SendChatTarget(-1, "平民胜利！");
		//GameWorld()->CreateSoundGlobal(SOUND_CTF_CAPTURE);

		m_aTeamscore[TEAM_BLUE] = -m_aTeamscore[TEAM_BLUE];
	}
	else if(EndType == 1 << TEAM_HUNTER) // no red
	{
		//SendChatTarget(-1, m_HunterList);
		SendChatTarget(-1, "猎人胜利！");
		GameWorld()->CreateSoundGlobal(SOUND_CTF_CAPTURE);

		m_aTeamscore[TEAM_RED] = -m_aTeamscore[TEAM_RED];
	}
}

bool CGameControllerHunterN::IsDisruptiveLeave(class CPlayer *pPlayer) const
{
	return false;
	// bool IsDisruptive = !IsEndRound() || !IsPlaying(pPlayer);
	// if(IsAlive(pPlayer) && pPlayer->GetCharacter())
	// {
	// 	pPlayer->GetCharacter()->Die(pPlayer->GetCID(), WEAPON_GAME);
	// }	
	// return IsDisruptive;
}

int CGameControllerHunterN::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	if(!IsGameRunning())
		return DEATH_SKIP_SCORE;

	int Return = DEATH_NO_KILL_MSG | DEATH_SKIP_SCORE |
			CGameControllerHunter::OnCharacterDeath(pVictim, pKiller, Weapon);

	int VictimCID = pVictim->GetPlayer()->GetCID();
	int KillerCID = pKiller->GetCID();

	char aBuf[96];
	if(m_aClass[VictimCID] == HunterClass::ID_HUNTER)
	{
		if(m_HunterDeathSmoke)
			GameWorld()->CreatePlayerSpawn(pVictim->m_Pos);

		m_HunterLeft--;

		str_format(aBuf, sizeof(aBuf), m_HunterLeft ? "Hunter '%s' was defeated! %d Hunter left." : "Hunter '%s' was defeated!", Server()->ClientName(VictimCID), m_HunterLeft);

		if(!m_HunterLeft ||
			m_HunterDeathBroadcast == 2)
		{
			SendChatTarget(-1, aBuf);
			GameWorld()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
		}
		else
		{
			for(int i = 0; i < MAX_CLIENTS; ++i)
			{
				CPlayer *pPlayer = GetPlayerIfInRoom(i);
				if(!pPlayer)
					continue;
				if((m_HunterDeathBroadcast > 0 && m_aClass[i] == HunterClass::ID_HUNTER) ||
					!IsPlaying(pPlayer) || !IsAlive(i))
				{
					SendChatTarget(pPlayer->GetCID(), aBuf);
					GameWorld()->CreateSoundGlobal(SOUND_CTF_CAPTURE, CmaskOne(pPlayer->GetCID()));
				}
				else
					GameWorld()->CreateSoundGlobal(SOUND_CTF_DROP, CmaskOne(pPlayer->GetCID()));
			}	
		}
	}
	else
		GameWorld()->CreateSoundGlobal(SOUND_CTF_DROP);

	if(Weapon >= WEAPON_WORLD)
	{
		if(pKiller != pVictim->GetPlayer())
		{
			if(m_aClass[VictimCID] > HunterClass::ID_NONE && m_aClass[VictimCID] <= NUM_CLASS_DATA)
				m_aHiddenScore[KillerCID] += // add score on kill
						m_aClassData[m_aClass[VictimCID] - 1].KillScore(m_aTeam[VictimCID] != m_aTeam[KillerCID] ? NORMAL_KILL : TEAM_KILL); // IsTeamKill

			str_format(aBuf, sizeof(aBuf), "你被 '%s' 的%s所杀...", Server()->ClientName(KillerCID), m_apWeaponName[Weapon + 1]);
		}
		else
			str_format(aBuf, sizeof(aBuf), "你被 %s 所杀...", m_apWeaponName[Weapon + 1]);
		SendChatTarget(VictimCID, aBuf);
		str_format(aBuf, sizeof(aBuf), "本回合你拿到了 %d 分", m_aHiddenScore[VictimCID]);
		SendChatTarget(VictimCID, aBuf);
	}

	SendHunterListChat(VictimCID);

	m_DoWincheckTick = Server()->Tick() + ((Server()->TickSpeed() * m_Wincheckdeley) / 1000);

	// send the kill message
	CNetMsg_Sv_KillMsg Msg;
	Msg.m_Killer = KillerCID;
	Msg.m_Victim = VictimCID;
	Msg.m_Weapon = Weapon;
	Msg.m_ModeSpecial = 0; // TODO:: make it team

	CNetMsg_Sv_KillMsg FakeMsg(Msg);
	FakeMsg.m_Killer = VictimCID;
	FakeMsg.m_Weapon = WEAPON_WORLD; // This makes the killer Anonymous

	for(int i = 0; i < MAX_CLIENTS; ++i)
		if(GetPlayerIfInRoom(i))
			Server()->SendPackMsg(IsAlive(i) ? &FakeMsg : &Msg, MSGFLAG_VITAL, i);

	for(int i = 0; i < MAX_CLIENTS; ++i)
		if(Server()->IsSixup(i) && !IsAlive(i))
			UpdateGameInfo(i); // Sixup

	return Return; // TODO:: make it team
}

void CGameControllerHunterN::OnCharacterSpawn(CCharacter *pChr)
{
	int CID = pChr->GetPlayer()->GetCID();
	if(!IsGameRunning())
	{
		if(++m_aClass[CID] >= NUM_CLASS)
			SetClass(CID, HunterClass::ID_NONE + 1);
	}
	else
		GameServer()->SendBroadcast(m_aClassData[maximum(m_aClass[CID] - 1, 0)].SpawnMsg(), CID, true);

	CGameControllerHunter::OnCharacterSpawn(pChr);
}

void CGameControllerHunterN::OnPlayerJoin(class CPlayer *pPlayer)
{
	int CID = pPlayer->GetCID();

	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "HunterN mod版本: %s : %s", HUNTERN_VERSION, GIT_SHORTREV_HASH ? GIT_SHORTREV_HASH :
#			ifdef CONF_DEBUG
				__DATE__
#			else
				""
#			endif
			);

	SendChatTarget(CID, aBuf);
	str_format(aBuf, sizeof(aBuf), "开源地址: %s", HUNTERN_REPO);
	SendChatTarget(CID, aBuf);

	if(IsGameInLoop())
		return;

	KillPlayer(pPlayer);
	OnPlayerLeave(pPlayer);

	str_format(aBuf, sizeof(aBuf), "本局的 %d 个Hunter是：", static_cast<int>(m_HunterMask.count()));
	MakeHunterList(aBuf, sizeof(aBuf));
	SendChatTarget(CID, aBuf);
	SendChatTarget(CID, "1. 每局开始时会秘密随机选择玩家成为猎人或平民 玩家只知道自己身份 猎人的目标是消灭所有平民");
	SendChatTarget(CID, "2. 猎人使用高伤武器、瞬杀追踪锤(20伤,长按追踪)和破片榴弹 猎人死亡则通知其他猎人");
	SendChatTarget(CID, "3. 平民没有锤子且只能使用常规武器 且死亡原因和死后聊天仅旁观/死人可见");

	for(int i = 0; i < MAX_CLIENTS; ++i)
		if(Server()->IsSixup(i) && !IsAlive(i))
			UpdateGameInfo(i); // Sixup
}

void CGameControllerHunterN::OnPlayerLeave(class CPlayer *pPlayer)
{
	int CID = pPlayer->GetCID();
	m_aClass[CID] = HunterClass::ID_NONE;
	m_aHiddenScore[CID] = 0;
	m_aTeam[CID] = TEAM_NONE;
	m_Rng.Remove(CID);
}

void CGameControllerHunterN::OnPlayerSnap(class CPlayer *pPlayer, int SnappingClient,
		CNetObj_ClientInfo *pClientInfo, CNetObj_PlayerInfo *pPlayerInfo, CNetObj_SpectatorInfo *pSpectatorInfo, // 0.6
		protocol7::CNetObj_PlayerInfo *pPlayerInfo7, protocol7::CNetObj_SpectatorInfo *pSpectatorInfo7, // 0.7
		CNetObj_DDNetSpectatorInfo *pDDNetSpectatorInfo, CNetObj_DDNetPlayer *pDDNetPlayer)
{
	CGameControllerHunter::OnPlayerSnap(pPlayer, SnappingClient,
				pClientInfo, pPlayerInfo, pSpectatorInfo, // 0.6
				pPlayerInfo7, pSpectatorInfo7, // 0.7
				pDDNetSpectatorInfo, pDDNetPlayer); // ddnet
	
	if(SnappingClient < 0)
		return;
	int PlayerCID = pPlayer->GetCID();
	CPlayer *pSnappingPlayer = GetPlayerIfInRoom(SnappingClient);
	if(!pSnappingPlayer)
		return;
	if(!Server()->IsSixup(SnappingClient))
	{
		bool IsRoundEnd = (IsEndRound() || IsEndMatch());// && GetGameStateTimer() < m_GameoverTime * Server()->TickSpeed();
		int TeamID = GetPlayerGameTeam(PlayerCID);

		if((m_HunterListBroadcast && IsAlive(pPlayer)) || IsRoundEnd ?
					IsPlaying(pPlayer) && m_aTeam[PlayerCID] != TEAM_NONE :
					!IsAlive(pSnappingPlayer) && IsAlive(pPlayer))
		{
			pPlayerInfo->m_Team = TeamID; // show teams
			//pPlayerInfo->m_Score = pPlayerInfo->m_Score + m_aHiddenScore[PlayerCID];
		}
		else if(PlayerCID == SnappingClient)
		{
			if(IsGameRunning() && pPlayer->GetCharacter() &&
					m_GameStartTick + (Server()->TickSpeed() * 3) >= Server()->Tick())
			{
				pClientInfo->m_UseCustomColor = 1;
				pClientInfo->m_ColorBody = TeamID ? 10551132 : 65372; // magic :D
				pClientInfo->m_ColorFeet = TeamID ? 10551132 : 65372;
				if(TeamID == TEAM_HUNTER - 1)
					SnapFlag(m_aSnapIDs[1], pPlayer->GetCharacter()->m_Pos, TeamID);
			}
		}
	}
	else // Sixup
	{}
}

void CGameControllerHunterN::OnPreTick()
{
	return;
	if(m_VoteCloseTime)
		return;

	if(IsGameRunning() && m_DoWincheckTick + (Server()->TickSpeed() * 2) > Server()->Tick())
	{
		CNetMsg_Sv_VoteSet MsgSet = {0};
		CNetMsg_Sv_VoteStatus MsgStat = {0};
		int Total = GetInRoundPlayer();
		int Yes = GetAlivePlayer();
		MsgSet.m_pDescription = "目前还存活的玩家";
		MsgSet.m_pReason = "";
		MsgSet.m_Timeout = Yes + 1;
		MsgStat.m_Total = Total;
		MsgStat.m_Yes = Yes;
		MsgStat.m_No = 0;
		MsgStat.m_Pass = 1;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!IsPlaying(i))
				continue;
			Server()->SendPackMsg(&MsgSet, MSGFLAG_VITAL, i);
			Server()->SendPackMsg(&MsgStat, MSGFLAG_VITAL, i);
		}
	}
	else
	{
		CNetMsg_Sv_VoteSet MsgSet = {0};
		MsgSet.m_pDescription = "";
		MsgSet.m_pReason = "";
		for(int i = 0; i < MAX_CLIENTS; i++)
			if(IsPlaying(i))
				Server()->SendPackMsg(&MsgSet, MSGFLAG_VITAL, i);
	}
}

void CGameControllerHunterN::OnSnap(int SnappingClient,
		CNetObj_GameInfo *pGameInfoObj, CNetObj_GameInfoEx *pGameInfoEx, CNetObj_GameData *pGameDataObj,
		protocol7::CNetObj_GameData *pGameData, protocol7::CNetObj_GameDataTeam *pGameDataTeam, protocol7::CNetObj_GameDataFlag *pGameDataFlag)
{
	CGameControllerHunter::OnSnap(SnappingClient,
			pGameInfoObj, pGameInfoEx, pGameDataObj, // 0.6
			pGameData, pGameDataTeam, pGameDataFlag); // 0.7

	if(SnappingClient < 0)
		return;
	if(!Server()->IsSixup(SnappingClient))
	{
		if(m_HunterListBroadcast || !IsAlive(SnappingClient))
		{
			pGameInfoObj->m_GameFlags = GAMEFLAG_TEAMS;
		}
		if(IsEndRound() || IsEndMatch())
		{
			pGameInfoObj->m_GameFlags = GAMEFLAG_TEAMS;
			pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_GAMEOVER | GAMESTATEFLAG_PAUSED;
		}
	}
	else // Sixup
	{
		if(!IsAlive(SnappingClient))
		{
			pGameDataTeam = static_cast<protocol7::CNetObj_GameDataTeam *>(Server()->SnapNewItem(-protocol7::NETOBJTYPE_GAMEDATATEAM, 0, sizeof(protocol7::CNetObj_GameDataTeam)));
			if(!pGameDataTeam)
				return;

			pGameDataTeam->m_TeamscoreRed = 0;
			pGameDataTeam->m_TeamscoreBlue = 0;
		}
	}
}

void CGameControllerHunterN::OnWorldReset()
{
	if(!IsGameRunning())
		return;
	ResetGame();

	int HunterNum = GetHunterNum();
	CClientMask PlayerMask = GetPlayingMask();
	// for(int i = 0; i < MAX_CLIENTS; i++)
	// 	if(PlayerMask.test(i))
	// 	{
	// 		char aBuf[64] = {0};
	// 		str_format(aBuf, sizeof(aBuf), "%d - %c", i, m_Rng.m_aWeight[i] + '0');
	// 		SendChatTarget(-1, aBuf);
	// 	}
	CClientMask SelectMask;
	SelectMask.reset();
	GetRng(PlayerMask)->Update(PlayerMask);
	GetRng(PlayerMask)->Select(PlayerMask, HunterNum, SelectMask);
	m_HunterLeft = HunterNum;
	for(int i = 0, j = 0; i < MAX_CLIENTS; i++)
	{
		if(!SelectMask.test(i))
			continue;
		SetClass(i, HunterClass::ID_HUNTER);
		m_aTeam[i] = TEAM_HUNTER;
		m_HunterMask.set(i);
		str_copy(m_aaHunterName[j++], Server()->ClientName(i), MAX_NAME_LENGTH); // record name
	}

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!IsPlaying(i) || !IsAlive(i))
			continue;
		if(m_aClass[i])
			continue;
		SetClass(i, HunterClass::ID_CIVIC);
		m_aTeam[i] = TEAM_CIVIC;
	}

	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "本回合有 %d 个猎人Hunter has been selected.", HunterNum);
	SendChatTarget(-1, "——————欢迎来到HunterN猎人杀——————");
	SendChatTarget(-1, aBuf);
	SendChatTarget(-1, "规则：每回合秘密抽选猎人 猎人对战平民 活人看不到死人消息");
	SendChatTarget(-1, "      猎人双倍伤害 有瞬杀锤子(平民无锤)和破片榴弹(对自己无伤)");
	SendChatTarget(-1, "分辨队友并消灭敌人来取得胜利！Be warned! Sudden Death.");

	if(m_HunterListBroadcast)
		SendHunterListChat();
	
	for(int i = 0; i < MAX_CLIENTS; ++i)
		if(Server()->IsSixup(i) && GetPlayerIfInRoom(i))
			IsAlive(i) ? ResetGameInfo(i) : UpdateGameInfo(i); // Sixup
}
