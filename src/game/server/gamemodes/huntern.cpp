/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "huntern.h"

#include <game/version.h>
#include <game/server/entities/character.h>
#include <game/server/weapons.h>

CGameControllerHunterN::CGameControllerHunterN() :
	CGameControllerHunter()
{
	INSTANCE_CONFIG_INT(&m_GameoverTime, "htn_gameover_time", 6, 0, 0xFFFFFFF, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "结算界面时长秒数 (整数, 默认6, 限制0~268435455)");
	INSTANCE_CONFIG_INT(&m_HunterDeathBroadcast, "htn_hunt_death_broadcast", 1, 0, 2, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "是否广播猎人死亡(1=仅猎人广播, 2=全体广播) (开关, 默认0, 限制0~2)");
	INSTANCE_CONFIG_INT(&m_HunterDeathEffect, "htn_hunt_death_effert", 0, 0, 1, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "猎人死亡是否使用出生烟 (开关, 默认0, 限制0~1)");
	INSTANCE_CONFIG_INT(&m_HunterListBroadcast, "htn_hunt_list_broadcast", 0, 0, 1, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "是否全体广播猎人列表 (开关, 默认0, 限制0~1)");
	INSTANCE_CONFIG_INT(&m_HunterRatio, "htn_hunt_ratio", 4, 2, MAX_CLIENTS, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "几个玩家里选取一个猎人 (整数, 默认4, 限制2~64)");
	INSTANCE_CONFIG_INT(&m_Wincheckdeley, "htn_wincheck_deley", 200, 0, 0xFFFFFFF, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "终局判断延时毫秒 (整数, 默认200, 限制0~268435455)");

	m_pGameType = "hunterN";
	m_GameFlags = IGF_SURVIVAL | IGF_ROUND_TIMER_ROUND;
}

void CGameControllerHunterN::OnInit()
{
	CGameControllerHunter::OnInit();
	mem_zero(&m_HiddenScore, sizeof(m_HiddenScore));
	m_SelectMask.reset();
	mem_zero(&m_Team, sizeof(m_Team));
	//mem_zero(&m_TeamMask, sizeof(m_TeamMask));
	ResetGame();
}

bool CGameControllerHunterN::CanChangeTeam(class CPlayer *pPlayer, int &JoinTeam)
{
	ResetPlayer(pPlayer->GetCID());
	if(pPlayer->GetTeam() != TEAM_SPECTATORS && !IsAlive(pPlayer)) // when player is dead in survival, allow to join spec
		JoinTeam = TEAM_SPECTATORS;
	if(JoinTeam == TEAM_SPECTATORS)
	{
		m_Team[pPlayer->GetCID()] = TEAM_NONE;
	}
	return true;
}

void CGameControllerHunterN::DoWincheckRound()
{
	bool IsTimeEnd = (m_GameInfo.m_TimeLimit > 0 && (Server()->Tick() - m_GameStartTick) >= m_GameInfo.m_TimeLimit * Server()->TickSpeed() * 60);

	if(m_DoWincheckTick && !IsTimeEnd)  // 时间没有结束且延时终局
	{
		if(m_DoWincheckTick >= 0)
			--m_DoWincheckTick; // 计时
		return;
	}

	int TeamCount[NUM_HUNTER_TEAMS] = {0};

	for(int i = 0; i < MAX_CLIENTS; ++i) // 计数玩家
	{
		if(IsAlive(i))
			TeamCount[m_Team[i]] += 1; // 计数活着的玩家
	}

	std::bitset<NUM_HUNTER_TEAMS> EndType;
	for(int i = 0; i < NUM_HUNTER_TEAMS; ++i)
		EndType.set(i, TeamCount[i]);

	if(!IsTimeEnd && EndType.count() > 1) // 如果不是回合限时结束则需某队存活
	{
		m_DoWincheckTick = -1;
		return;
	}

	// 游戏结束
	SetGameState(IGS_END_ROUND, m_GameoverTime); // EndRound();

	m_aTeamscore[TEAM_RED] = TeamCount[TEAM_CIVIC]; // 队伍分数形式显示双方人数
	m_aTeamscore[TEAM_BLUE] = TeamCount[TEAM_HUNTER];

	for(int i = 0; i < MAX_CLIENTS; ++i) // 进行玩家分数和隐藏分操作
	{
		CPlayer *pPlayer = GetPlayerIfInRoom(i);
		if(!IsPlaying(pPlayer))
			continue;
		if(IsAlive(pPlayer))
			m_HiddenScore[i] += 2; // 存活加分

		pPlayer->m_Score += m_HiddenScore[i]; // 添加隐藏分
	}

	if(IsTimeEnd) // 终局聊天消息
	{
		//SendChatTarget(-1, m_HunterList);
		SendChatTarget(-1, "游戏结束！");
		GameWorld()->CreateSoundGlobal(SOUND_CTF_CAPTURE);
		return;
	}
	else if(EndType.none()) // nobody
	{
		SendChatTarget(-1, "两人幸终！");
		return;
	}
	else if(EndType == 1 << TEAM_CIVIC) // no blue
	{
		//SendChatTarget(-1, m_HunterList);
		SendChatTarget(-1, "平民胜利！");
		//GameWorld()->CreateSoundGlobal(SOUND_CTF_CAPTURE); // 猎人死的时候够吵了

		m_aTeamscore[TEAM_BLUE] = -m_aTeamscore[TEAM_BLUE]; // 反转蓝队分数 显示"红队胜利"
		return;
	}
	else if(EndType == 1 << TEAM_HUNTER) // no red
	{
		//SendChatTarget(-1, m_HunterList); // 猎人胜利不显示列表（因为平民被打死的时候已经显示过了）
		SendChatTarget(-1, "猎人胜利！");
		GameWorld()->CreateSoundGlobal(SOUND_CTF_CAPTURE);

		m_aTeamscore[TEAM_RED] = -m_aTeamscore[TEAM_RED]; // 反转红队分数 就会显示"蓝队胜利"
		return;
	}
}

bool CGameControllerHunterN::IsDisruptiveLeave(class CPlayer *pPlayer) const
{
	return !IsEndRound();
}

int CGameControllerHunterN::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	if(!IsGameRunning()) // 如果游戏在正常运行
		return DEATH_SKIP_SCORE; // 跳过内置分数逻辑

	int Return = DEATH_NO_KILL_MSG | DEATH_SKIP_SCORE | CGameControllerHunter::OnCharacterDeath(pVictim, pKiller, Weapon);

	int VictimCID = pVictim->GetPlayer()->GetCID();
	int KillerCID = pKiller->GetCID();

	if(m_Class[VictimCID] == HunterClass::ID_HUNTER)
	{
		int VictimCID = pVictim->GetPlayer()->GetCID();
		//int KillerCID = pKiller->GetCID();

		if(m_HunterDeathEffect)
			GameWorld()->CreatePlayerSpawn(pVictim->m_Pos);

		m_HunterLeft--;

		char aBuf[64];

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
				if((m_HunterDeathBroadcast > 0 && m_Class[i] == HunterClass::ID_HUNTER) ||
					IsPlaying(pPlayer) || !IsAlive(i))
				{
					SendChatTarget(pPlayer->GetCID(), aBuf);
					GameWorld()->CreateSoundGlobal(SOUND_CTF_CAPTURE, CmaskOne(pPlayer->GetCID()));
				}
				else
					GameWorld()->CreateSoundGlobal(SOUND_CTF_DROP, CmaskOne(pPlayer->GetCID()));
			}	
		}
	}

	if(pKiller != pVictim->GetPlayer()) // 不是自杀
	{
		m_HiddenScore[KillerCID] += // 给予杀手分数
				m_aKillScoreData[m_Class[pVictim->GetPlayer()->GetCID()]] // Class
				[m_Team[pVictim->GetPlayer()->GetCID()] == m_Team[pKiller->GetCID()]]; // IsTeamKill

		if(Weapon >= WEAPON_WORLD)
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "你被 '%s' 的%s所杀", Server()->ClientName(pKiller->GetCID()), m_apWeaponName[Weapon + 1]);

			SendChatTarget(VictimCID, aBuf); // 给被弄死的人发
		}
	}

	m_DoWincheckTick = ((Server()->TickSpeed() * m_Wincheckdeley) / 1000); // 延时终局

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

	return Return; // 隐藏死因并跳过内置分数逻辑 // TODO:: make it team
}

void CGameControllerHunterN::OnCharacterSpawn(CCharacter *pChr)
{
	if(!IsGameRunning())
	{
		pChr->IncreaseHealth(10);

		pChr->GiveWeapon(WEAPON_GUN, WEAPON_ID_PISTOL, 10);
		pChr->GiveWeapon(WEAPON_HAMMER, WEAPON_ID_HAMMER, -1);
		return;
	}
	else
	{
		CGameControllerHunter::OnCharacterSpawn(pChr);

		GameServer()->SendBroadcast(m_apClassSpawnMsg[maximum(m_Class[pChr->GetPlayer()->GetCID()] - 1, 0)], pChr->GetPlayer()->GetCID(), true);
	}
}

void CGameControllerHunterN::OnPlayerJoin(class CPlayer *pPlayer) // 使新进旁观者收到猎人列表
{
	int CID = pPlayer->GetCID();
	ResetPlayer(CID);

	char aBuf[320];
	str_format(aBuf, sizeof(aBuf), "HunterN mod版本: %s : %s", HUNTERN_VERSION, GIT_SHORTREV_HASH ? GIT_SHORTREV_HASH :
#			ifdef CONF_DEBUG
				__DATE__
#			else
				""
#			endif
			);

	SendChatTarget(CID, aBuf);
#ifdef CONF_DEBUG
	str_format(aBuf, sizeof(aBuf), "开源地址: %s", HUNTERN_REPO);
	SendChatTarget(CID, aBuf);
#endif

	if(!IsGameRunning())
		return;

	pPlayer->m_RespawnDisabled = true;
	pPlayer->KillCharacter();
	str_format(aBuf, sizeof(aBuf), "本局的 %d 个Hunter是：", static_cast<int>(m_HunterMask.count()));
	MakeHunterList(aBuf, sizeof(aBuf));
	SendChatTarget(CID, aBuf);
	SendChatTarget(CID, "1. 每局开始时会秘密随机选择玩家成为猎人或平民 玩家只知道自己身份 猎人的目标是消灭所有平民");
	SendChatTarget(CID, "2. 猎人使用高伤武器、瞬杀追踪锤(20伤,长按追踪)和破片榴弹 猎人死亡则通知其他猎人");
	SendChatTarget(CID, "3. 平民没有锤子且只能使用常规武器 且死亡原因和死后聊天仅旁观/死人可见");
}

void CGameControllerHunterN::OnPlayerLeave(class CPlayer *pPlayer)
{
	ResetPlayer(pPlayer->GetCID());
}

void CGameControllerHunterN::OnPlayerSnap(class CPlayer *pPlayer, int SnappingClient,
		CNetObj_ClientInfo *pClientInfo, CNetObj_PlayerInfo *pPlayerInfo, CNetObj_SpectatorInfo *pSpectatorInfo, // 0.6
		protocol7::CNetObj_PlayerInfo *pPlayerInfo7, protocol7::CNetObj_SpectatorInfo *pSpectatorInfo7, // 0.7
		CNetObj_DDNetSpectatorInfo *pDDNetSpectatorInfo, CNetObj_DDNetPlayer *pDDNetPlayer)
{
	if(SnappingClient < 0)
		return;
	CPlayer *pSnappingPlayer = GetPlayerIfInRoom(SnappingClient);
	if(!pSnappingPlayer)
		return;
	if(!Server()->IsSixup(SnappingClient))
	{
		if((pPlayer->GetCID() != SnappingClient && (!IsAlive(pSnappingPlayer) || IsPlaying(pSnappingPlayer))) ||
				IsEndRound() || IsEndMatch())
		{
			pPlayerInfo->m_Team = m_Team[pPlayer->GetCID()] - 1; // show teams
			if(Server()->Tick() & (1 << 6))
				StrToInts(&pClientInfo->m_Clan0, 3, m_apClassName[m_Team[pPlayer->GetCID()] - 1]);
		}
		if(pPlayer->GetCID() == SnappingClient)
		{
			//pPlayerInfo->m_Score = pPlayerInfo->m_Score + m_HiddenScore[SnappingClient];
		}
		if(pDDNetPlayer->m_AuthLevel == AUTHED_ADMIN)
		{
			pPlayerInfo->m_Latency = Server()->Tick() & (1 << 4) ? 114 : 514;
			pDDNetPlayer->m_AuthLevel = AUTHED_NO; // fuck you cheater
		}
	}
	else
	{}
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
		if(!IsAlive(SnappingClient))
		{
			pGameInfoObj->m_GameFlags = GAMEFLAG_TEAMS;
		}
		if(IsEndRound())
		{
			pGameInfoObj->m_GameFlags = GAMEFLAG_TEAMS;
			pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_GAMEOVER | GAMESTATEFLAG_PAUSED;
		}
	}
	else
	{}
}

void CGameControllerHunterN::OnWorldReset()
{
	int HunterNum = minimum(GetHunterNum(), (int)MAX_HUNTERS);
	if(HunterNum <= 0)
		return;
	ResetPlayer();
	ResetGame();

	for(int i = 0; i < HunterNum; i++)
	{
		CClientMask Mask;
		if(HunterNum > 1)
			Mask = m_SelectMask;
		else
			Mask.set();
		UpdateSelectMask(m_SelectMask);
		int CID = RandHunter(m_SelectMask);
		if(CID == -1)
			return;
		m_Class[CID] = HunterClass::ID_HUNTER;
		m_Team[CID] = TEAM_HUNTER;
		m_SelectMask.set(CID, false);
		m_HunterMask.set(CID);
		str_copy(m_aaHunterName[i], Server()->ClientName(CID), sizeof(m_aaHunterName[i])); // record name
	}

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!IsPlaying(i) && IsAlive(i))
			continue;
		if(m_Class[i])
			continue;
		m_Class[i] = HunterClass::ID_CIVIC;
		m_Team[i] = TEAM_CIVIC;
	}

	char aBuf[320];
	str_format(aBuf, sizeof(aBuf), "本回合有 %d 个猎人Hunter has been selected.", static_cast<int>(m_HunterMask.count()));
	SendChatTarget(-1, "——————欢迎来到HunterN猎人杀——————");
	SendChatTarget(-1, aBuf);
	SendChatTarget(-1, "规则：每回合秘密抽选猎人 猎人对战平民 活人看不到死人消息");
	SendChatTarget(-1, "      猎人双倍伤害 有瞬杀锤子(平民无锤)和破片榴弹(对自己无伤)");
	SendChatTarget(-1, "分辨队友并消灭敌人来取得胜利！Be warned! Sudden Death.");
	if(m_HunterListBroadcast)
	{
		str_format(aBuf, sizeof(aBuf), "本局的 %d 个Hunter是：", static_cast<int>(m_HunterMask.count()));
		MakeHunterList(aBuf, sizeof(aBuf));
		SendChatTarget(-1, aBuf);
	}

	m_HunterLeft = HunterNum;
}
