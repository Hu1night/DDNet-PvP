/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifdef REGISTER_HUNTERCLASS
// define REGISTER_HUNTERCLASS(ID, MASK_ID, TEAM_ID, DEATHFUNC, HANDLEFIREFUNC, DODMGFUNC, SPAWNFUNC, TAKEDMGFUNC)
REGISTER_HUNTERCLASS(ID_CIVIC, MASK_ID_CIVIC, TEAM_CIVIC, OnCivicDeath, OnCivicFire, OnCivicDoDmg, OnCivicSpawn, OnCivicTakeDmg) // Civic
REGISTER_HUNTERCLASS(ID_HUNTER, MASK_ID_HUNTER, TEAM_HUNTER, OnHunterDeath, OnHunterFire, OnHunterDoDmg, OnHunterSpawn, OnHunterTakeDmg) // Hunter
//REGISTER_HUNTERCLASS(ID_DISGUISER, MASK_ID_DISGUISER, TEAM_HUNTER, OnHunterDeath, OnDisguiserFire, OnHunterDoDmg, OnHunterSpawn, OnHunterTakeDmg) // Disguiser
//REGISTER_HUNTERCLASS(ID_REVIVER, MASK_ID_REVIVER, TEAM_CIVIC, OnCivicDeath, OnCivicFire, OnCivicDoDmg, OnCivicSpawn, OnCivicTakeDmg) // Civic

#else
#ifndef GAME_SERVER_GAMEMODES_HUNTER_H
#define GAME_SERVER_GAMEMODES_HUNTER_H

#include <game/generated/server_data.h>
#include <game/server/entities/character.h>
#include <game/server/entities/projectile.h>
#include <game/server/gamecontroller.h>
#include <game/server/weapons.h>
#include <game/server/weapons/shotgun.h>

namespace HunterClass
{
	enum EClassID
	{
		ID_NONE,
#		define REGISTER_HUNTERCLASS(ID, MASK_ID, TEAM_ID, DEATHFUNC, HANDLEFIREFUNC, DODMGFUNC, SPAWNFUNC, TAKEDMGFUNC) \
		ID,
#			include <game/server/gamemodes/hunter.h>
#		undef REGISTER_HUNTERCLASS
		NUM_CLASS_ID,
	};

	enum EClassMask
	{
		MASK_CLASS_NONE,
#		define REGISTER_HUNTERCLASS(ID, MASK_ID, TEAM_ID, DEATHFUNC, HANDLEFIREFUNC, DODMGFUNC, SPAWNFUNC, TAKEDMGFUNC) \
		MASK_ID = 1 << (ID - 1),
#			include <game/server/gamemodes/hunter.h>
#		undef REGISTER_HUNTERCLASS
		NUM_CLASS_MASK,
	};
};

enum HUNTERNCONFIG
{
	NUM_HUNT_GRENADE_SFX = 5,
	NUM_SNAP_IDS = 3,
};

enum 
{
	TEAM_NONE = 0,
	TEAM_CIVIC,
	TEAM_HUNTER,
	NUM_HUNTER_TEAMS,
};

enum
{
	MIN_HUNTERS = 1,
	MAX_HUNTERS = 16,
};

class CGameControllerHunter : public IGameController
{
public:
	CGameControllerHunter();
	~CGameControllerHunter();
	void OnInit() override;

	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	virtual bool OnCharacterHandleFire(class CWeapon *pWeapon, vec2 &Direction) override;
	virtual void OnCharacterSpawn(class CCharacter *pChr) override;
	virtual int OnCharacterTakeDamage(class CCharacter *pChr, vec2 &Force, int &Dmg, int From, int WeaponType, int WeaponID, bool IsExplosion) override;
	virtual void OnPlayerSnap(class CPlayer *pPlayer, int SnappingClient,
			CNetObj_ClientInfo *pClientInfo, CNetObj_PlayerInfo *pPlayerInfo, CNetObj_SpectatorInfo *pSpectatorInfo, // 0.6
			protocol7::CNetObj_PlayerInfo *pPlayerInfo7, protocol7::CNetObj_SpectatorInfo *pSpectatorInfo7, // 0.7
			CNetObj_DDNetSpectatorInfo *pDDNetSpectatorInfo, CNetObj_DDNetPlayer *pDDNetPlayer) override;
	virtual void OnSnap(int SnappingClient,
			CNetObj_GameInfo *pGameInfoObj, CNetObj_GameInfoEx *pGameInfoEx, CNetObj_GameData *pGameDataObj,
			protocol7::CNetObj_GameData *pGameData, protocol7::CNetObj_GameDataTeam *pGameDataTeam, protocol7::CNetObj_GameDataFlag *pGameDataFlag) override;

protected:
	int m_aClass[MAX_CLIENTS];
	void *m_apClassCustomData[MAX_CLIENTS];
	int m_aSnapIDs[NUM_SNAP_IDS];

	int m_HunterFragNumHit;
	int m_HunterFragNumNoHit;
	int m_HunterTrackCooldown;
	int m_HunterTrackTime;

	void SetClass(int CID, int ClassID)
	{
		m_aClass[CID] = ClassID;
	}

	// Civic class
	bool OnCivicFire(class CWeapon *pWeapon, vec2 Direction);
	void OnCivicSpawn(class CCharacter *pChr);
	int OnCivicDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	int OnCivicDoDmg(class CCharacter *pChr, vec2 &Force, int &Dmg, int From, int WeaponType, int WeaponID, bool IsExplosion);
	int OnCivicTakeDmg(class CCharacter *pChr, vec2 &Force, int &Dmg, int From, int WeaponType, int WeaponID, bool IsExplosion);

	// Hunter class
	int m_TrackTick[MAX_CLIENTS];
	bool OnHunterFire(class CWeapon *pWeapon, vec2 Direction);
	void OnHunterSpawn(class CCharacter *pChr);
	int OnHunterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	int OnHunterDoDmg(class CCharacter *pChr, vec2 &Force, int &Dmg, int From, int WeaponType, int WeaponID, bool IsExplosion);
	int OnHunterTakeDmg(class CCharacter *pChr, vec2 &Force, int &Dmg, int From, int WeaponType, int WeaponID, bool IsExplosion);

protected:
	CNetObj_Flag *SnapFlag(int SnapID, vec2 Pos, int Team)
	{
		CNetObj_Flag *pObj = static_cast<CNetObj_Flag *>(Server()->SnapNewItem(NETOBJTYPE_FLAG, SnapID, sizeof(CNetObj_Flag)));
		if(pObj)
		{
			pObj->m_X = Pos.x;
			pObj->m_Y = Pos.y;
			pObj->m_Team = Team;
		}
		return pObj;
	}
	CNetObj_Laser *SnapLaser(int SnapID, vec2 From, vec2 To, int StartTick)
	{
		CNetObj_Laser *pObj = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, SnapID, sizeof(CNetObj_Laser)));
		if(pObj)
		{
			pObj->m_FromX = From.x;
			pObj->m_FromY = From.y;
			pObj->m_X = To.x;
			pObj->m_Y = To.y;
			pObj->m_StartTick = StartTick;
		}
		return pObj;
	}

	bool IsAlive(int CID) const
	{ 
		class CPlayer *pPlayer = GetPlayerIfInRoom(CID);
		return pPlayer && pPlayer->GetTeam() != TEAM_SPECTATORS &&
				((pPlayer->GetCharacter() && pPlayer->GetCharacter()->IsAlive()) || !pPlayer->m_RespawnDisabled);
	}
	bool IsAlive(class CPlayer *pPlayer) const
	{ 
		return pPlayer && GameServer()->GetPlayerDDRTeam(pPlayer->GetCID()) == GameWorld()->Team() &&
				pPlayer->GetTeam() != TEAM_SPECTATORS &&
				((pPlayer->GetCharacter() && pPlayer->GetCharacter()->IsAlive()) || !pPlayer->m_RespawnDisabled);
	}
	// bool IsAlive(class CCharacter *pChr)
	// {
	// 	return pChr && pChr->GetPlayer()->GetTeam() != TEAM_SPECTATORS &&
	// 		(pChr->IsAlive() || !pChr->GetPlayer()->m_RespawnDisabled);
	// }
	bool IsPlaying(int CID) const
	{ 
		class CPlayer *pPlayer = GetPlayerIfInRoom(CID);
		return pPlayer && pPlayer->GetTeam() != TEAM_SPECTATORS;
	}
	bool IsPlaying(CPlayer *pPlayer) const
	{ 
		return pPlayer && pPlayer->GetTeam() != TEAM_SPECTATORS;
	}
	static int GetRand() { return rand(); }
};

#endif // GAME_SERVER_GAMEMODES_HUNTER_H
#endif // REGISTER_HUNTERCLASS
