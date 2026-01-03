/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifdef INCLUDE_HUNTERCLASS
// define REGISTER_HUNTERCLASS(ID, MASK_ID, TEAM_ID, DEATHFUNC, HANDLEFIREFUNC, DODMGFUNC, SPAWNFUNC, TAKEDMGFUNC)
REGISTER_HUNTERCLASS(ID_CIVIC, MASK_ID_CIVIC, TEAM_CIVIC, OnCivicDeath, OnCivicFire, OnCivicDoDmg, OnCivicSpawn, OnCivicTakeDmg) // Civic
REGISTER_HUNTERCLASS(ID_HUNTER, MASK_ID_HUNTER, TEAM_HUNTER, OnHunterDeath, OnHunterFire, OnHunterDoDmg, OnHunterSpawn, OnHunterTakeDmg) // Hunter
//REGISTER_HUNTERCLASS(ID_DISGUISER, MASK_ID_DISGUISER, TEAM_HUNTER, ) // Disguiser

#else
#ifndef GAME_SERVER_GAMEMODES_HUNTER_H
#define GAME_SERVER_GAMEMODES_HUNTER_H

#include <game/generated/server_data.h>
#include <game/server/entities/character.h>
#include <game/server/entities/projectile.h>
#include <game/server/gamecontroller.h>
#include <game/server/weapons.h>
#include <game/server/weapons/shotgun.h>

#define HUNTERN_VERSION "0.5a0_p1"
#define HUNTERN_REPO "https://github.com/Hu1night/DDNet-PvP/branches"

enum HUNTERNCONFIG
{
	NUM_HUNT_GRENADE_SFX = 5,
	NUM_SNAP_IDS = 1,
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
	MAX_HUNTERS = 16,
};

namespace HunterClass
{
	enum EClassID
	{
		ID_NONE,
#		define REGISTER_HUNTERCLASS(ID, MASK_ID, TEAM_ID, DEATHFUNC, HANDLEFIREFUNC, DODMGFUNC, SPAWNFUNC, TAKEDMGFUNC) \
		ID,
#			define INCLUDE_HUNTERCLASS
#				include <game/server/gamemodes/hunter.h>
#			undef INCLUDE_HUNTERCLASS
#		undef REGISTER_HUNTERCLASS
		NUM_CLASS_ID,
	};

	enum EClassMask
	{
		MASK_CLASS_NONE,
#		define REGISTER_HUNTERCLASS(ID, MASK_ID, TEAM_ID, DEATHFUNC, HANDLEFIREFUNC, DODMGFUNC, SPAWNFUNC, TAKEDMGFUNC) \
		MASK_ID = 1 << (ID - 1),
#			define INCLUDE_HUNTERCLASS
#				include <game/server/gamemodes/hunter.h>
#			undef INCLUDE_HUNTERCLASS
#		undef REGISTER_HUNTERCLASS
		NUM_CLASS_MASK,
	};
};

class CGameControllerHunter : public IGameController
{
public:
	CGameControllerHunter();
	~CGameControllerHunter();
	void OnInit() override;

	int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override;
	bool OnCharacterHandleFire(class CWeapon *pWeapon, vec2 &Direction) override;
	void OnCharacterSpawn(class CCharacter *pChr) override;
	int OnCharacterTakeDamage(class CCharacter *pChr, vec2 &Force, int &Dmg, int From, int WeaponType, int WeaponID, bool IsExplosion) override;

	void OnSnap(int SnappingClient,
			CNetObj_GameInfo *pGameInfoObj, CNetObj_GameInfoEx *pGameInfoEx, CNetObj_GameData *pGameDataObj,
			protocol7::CNetObj_GameData *pGameData, protocol7::CNetObj_GameDataTeam *pGameDataTeam, protocol7::CNetObj_GameDataFlag *pGameDataFlag) override;

protected:
	int m_Class[MAX_CLIENTS];
	int m_aSnapIDs[NUM_SNAP_IDS];

	int m_HunterFragNumHit;
	int m_HunterFragNumNoHit;

	// Civic class
	bool OnCivicFire(class CWeapon *pWeapon, vec2 Direction);
	void OnCivicSpawn(class CCharacter *pChr);
	int OnCivicDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	int OnCivicDoDmg(class CCharacter *pChr, vec2 &Force, int &Dmg, int From, int WeaponType, int WeaponID, bool IsExplosion);
	int OnCivicTakeDmg(class CCharacter *pChr, vec2 &Force, int &Dmg, int From, int WeaponType, int WeaponID, bool IsExplosion);

	// Hunter class
	bool OnHunterFire(class CWeapon *pWeapon, vec2 Direction);
	void OnHunterSpawn(class CCharacter *pChr);
	int OnHunterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	int OnHunterDoDmg(class CCharacter *pChr, vec2 &Force, int &Dmg, int From, int WeaponType, int WeaponID, bool IsExplosion);
	int OnHunterTakeDmg(class CCharacter *pChr, vec2 &Force, int &Dmg, int From, int WeaponType, int WeaponID, bool IsExplosion);
	static bool OnHunterGrenadeCollide(class CProjectile *pProj, vec2 Pos, CCharacter *pHit, bool EndOfLife);

protected:
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
	};

	bool IsAlive(int CID)
	{ 
		class CPlayer *pPlayer = GetPlayerIfInRoom(CID);
		return pPlayer && pPlayer->GetTeam() != TEAM_SPECTATORS &&
				((pPlayer->GetCharacter() && pPlayer->GetCharacter()->IsAlive()) || !pPlayer->m_RespawnDisabled);
	}
	bool IsAlive(class CPlayer *pPlayer)
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
	bool IsPlaying(int CID)
	{ 
		class CPlayer *pPlayer = GetPlayerIfInRoom(CID);
		return pPlayer && pPlayer->GetTeam() != TEAM_SPECTATORS;
	}
	bool IsPlaying(CPlayer *pPlayer)
	{ 
		return pPlayer && pPlayer->GetTeam() != TEAM_SPECTATORS;
	}
	int GetRand() { return rand(); }
};

#endif // GAME_SERVER_GAMEMODES_HUNTER_H4
#endif // INCLUDE_HUNTERCLASS
