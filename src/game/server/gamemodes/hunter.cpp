/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "hunter.h"

#include <game/version.h>
#include <game/server/entities/character.h>
#include <game/server/weapons.h>

CGameControllerHunter::CGameControllerHunter() :
	IGameController()
{
	INSTANCE_CONFIG_INT(&m_HunterFragNumHit, "htn_hunt_frag_num_hit", 16, 0, 512, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "猎人榴弹直接砸到人产生的破片数量 (整数, 默认16, 限制0~268435455)");
	INSTANCE_CONFIG_INT(&m_HunterFragNumNoHit, "htn_hunt_frag_num_nohit", 32, 0, 512, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "猎人榴弹没砸到人产生的破片数量 (整数, 默认32, 限制0~268435455)");

	m_pGameType = "hunter";
	m_GameFlags = IGF_ROUND_TIMER_ROUND;

	m_MinimumPlayers = 2;
}

CGameControllerHunter::~CGameControllerHunter()
{
	for(int i = 0; i < NUM_SNAP_IDS; i++)
		Server()->SnapFreeID(m_aSnapIDs[i]);
}

void CGameControllerHunter::OnInit()
{
	mem_zero(&m_Class, sizeof(m_Class));

	for(int i = 0; i < NUM_SNAP_IDS; i++)
		m_aSnapIDs[i] = Server()->SnapNewID();
}

int CGameControllerHunter::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	int Return = DEATH_NORMAL;
	int VictimCID = pVictim->GetPlayer()->GetCID();
	//int KillerCID = pKiller->GetCID();

#	define REGISTER_HUNTERCLASS(ID, MASK_ID, TEAM_ID, DEATHFUNC, HANDLEFIREFUNC, DODMGFUNC, SPAWNFUNC, TAKEDMGFUNC) \
		if(m_Class[VictimCID] == HunterClass::ID) \
			Return |= DEATHFUNC(pVictim, pKiller, Weapon);
#		define INCLUDE_HUNTERCLASS
#			include <game/server/gamemodes/hunter.h>
#		undef INCLUDE_HUNTERCLASS
#	undef REGISTER_HUNTERCLASS

	return Return;
}

bool CGameControllerHunter::OnCharacterHandleFire(class CWeapon *pWeapon, vec2 &Direction)
{
#define REGISTER_HUNTERCLASS(ID, MASK_ID, TEAM_ID, DEATHFUNC, HANDLEFIREFUNC, DODMGFUNC, SPAWNFUNC, TAKEDMGFUNC) \
	if(m_Class[pWeapon->Character()->GetPlayer()->GetCID()] == HunterClass::ID) \
		return HANDLEFIREFUNC(pWeapon, Direction);
#	define INCLUDE_HUNTERCLASS
#		include <game/server/gamemodes/hunter.h>
#	undef INCLUDE_HUNTERCLASS
#undef REGISTER_HUNTERCLASS
	return true;
}

void CGameControllerHunter::OnCharacterSpawn(CCharacter *pChr)
{
	int CID = pChr->GetPlayer()->GetCID();
#	define REGISTER_HUNTERCLASS(ID, MASK_ID, TEAM_ID, DEATHFUNC, HANDLEFIREFUNC, DODMGFUNC, SPAWNFUNC, TAKEDMGFUNC) \
		if(m_Class[CID] == HunterClass::ID) \
		{ \
			SPAWNFUNC(pChr); \
			return; \
		}
#		define INCLUDE_HUNTERCLASS
#			include <game/server/gamemodes/hunter.h>
#		undef INCLUDE_HUNTERCLASS
#	undef REGISTER_HUNTERCLASS
}

int CGameControllerHunter::OnCharacterTakeDamage(class CCharacter *pChr, vec2 &Force, int &Dmg, int From, int WeaponType, int WeaponID, bool IsExplosion)
{
	int CID = pChr->GetPlayer()->GetCID();
	int Return = DAMAGE_NORMAL;

#	define REGISTER_HUNTERCLASS(ID, MASK_ID, TEAM_ID, DEATHFUNC, HANDLEFIREFUNC, DODMGFUNC, SPAWNFUNC, TAKEDMGFUNC) \
		if(m_Class[From] == HunterClass::ID) \
			Return |= DODMGFUNC(pChr, Force, Dmg, From, WeaponType, WeaponID, IsExplosion);
#		define INCLUDE_HUNTERCLASS
#			include <game/server/gamemodes/hunter.h>
#		undef INCLUDE_HUNTERCLASS
#	undef REGISTER_HUNTERCLASS

#	define REGISTER_HUNTERCLASS(ID, MASK_ID, TEAM_ID, DEATHFUNC, HANDLEFIREFUNC, DODMGFUNC, SPAWNFUNC, TAKEDMGFUNC) \
		if(m_Class[CID] == HunterClass::ID) \
			Return |= TAKEDMGFUNC(pChr, Force, Dmg, From, WeaponType, WeaponID, IsExplosion);
#		define INCLUDE_HUNTERCLASS
#			include <game/server/gamemodes/hunter.h>
#		undef INCLUDE_HUNTERCLASS
#	undef REGISTER_HUNTERCLASS

	return Return;
}

// Civic class
int CGameControllerHunter::OnCivicDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	GameWorld()->CreateSoundGlobal(SOUND_CTF_DROP);
	return DEATH_NORMAL;
}
bool CGameControllerHunter::OnCivicFire(class CWeapon *pWeapon, vec2 Direction)
{
	return true;
}
int CGameControllerHunter::OnCivicDoDmg(class CCharacter *pChr, vec2 &Force, int &Dmg, int From, int WeaponType, int WeaponID, bool IsExplosion)
{
	return DAMAGE_NORMAL;
}
void CGameControllerHunter::OnCivicSpawn(class CCharacter *pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_GUN, WEAPON_ID_PISTOL, 10);

	GameWorld()->CreateSoundGlobal(SOUND_CTF_GRAB_PL, CmaskOne(pChr->GetPlayer()->GetCID()));
}
int CGameControllerHunter::OnCivicTakeDmg(class CCharacter *pChr, vec2 &Force, int &Dmg, int From, int WeaponType, int WeaponID, bool IsExplosion)
{
	return DAMAGE_NORMAL;
}

// Hunter class
bool CGameControllerHunter::OnHunterFire(class CWeapon *pWeapon, vec2 Direction)
{
	if(pWeapon->IsReloading() || !pWeapon->GetAmmo())
		return true;

	// if(pWeapon->GetWeaponID() == WEAPON_ID_HAMMER)
	// {
	// 	CEntity *pClosetChar = GameWorld()->ClosestEntity(pWeapon->Pos(), 32.f * 100.f, CGameWorld::ENTTYPE_CHARACTER, pWeapon->Character());
	// 	if(pClosetChar)
	// 	{
	// 		vec2 From = pWeapon->Pos();
	// 		vec2 ToDir = normalize(pClosetChar->m_Pos - From);
	// 		vec2 To = From + (ToDir * 96.f);
	// 	}
	// }
	if(pWeapon->GetWeaponID() == WEAPON_ID_GRENADE)
	{
		int ClientID = pWeapon->Character()->GetPlayer()->GetCID();
		int Lifetime = pWeapon->Character()->CurrentTuning()->m_GrenadeLifetime * Server()->TickSpeed();

		vec2 ProjStartPos = pWeapon->Pos() + Direction * pWeapon->GetProximityRadius() * 0.75f;

		//CProjectile *pProj =
		new CProjectile(
			GameWorld(),
			WEAPON_GRENADE, //Type
			pWeapon->GetWeaponID(), //WeaponID
			ClientID, //Owner
			ProjStartPos, //Pos
			Direction, //Dir
			6.0f, // Radius
			Lifetime, //Span
			CGameControllerHunter::OnHunterGrenadeCollide);

		GameWorld()->CreateSound(pWeapon->Pos(), SOUND_GRENADE_FIRE);

		return false;
	}

	return true;
}
void CGameControllerHunter::OnHunterSpawn(class CCharacter *pChr)
{
	pChr->IncreaseHealth(10);
	pChr->GiveWeapon(WEAPON_GUN, WEAPON_ID_PISTOL, 10);
	pChr->ForceSetWeapon(WEAPON_HAMMER, WEAPON_ID_HAMMER, -1);

	GameWorld()->CreateSoundGlobal(SOUND_CTF_GRAB_EN, CmaskOne(pChr->GetPlayer()->GetCID()));
}
int CGameControllerHunter::OnHunterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
	return DEATH_NORMAL;
}
int CGameControllerHunter::OnHunterDoDmg(class CCharacter *pChr, vec2 &Force, int &Dmg, int From, int WeaponType, int WeaponID, bool IsExplosion)
{
	if(WeaponID == WEAPON_ID_HAMMER && WeaponType == WEAPON_HAMMER)
		Dmg = 20;
	else if(WeaponID != WEAPON_ID_GRENADE) // no more dmg for hunter grenade
	{
		Dmg *= 2;
		if(((WeaponID == WEAPON_ID_LASER || WeaponID == WEAPON_ID_EXPLODINGLASER) && WeaponType == WEAPON_LASER) ||
				(WeaponID == WEAPON_ID_NINJA && WeaponType == WEAPON_NINJA))
			Dmg -= 1; // Decrease dmg for laser & ninja
	}
	return DAMAGE_NORMAL;
}
int CGameControllerHunter::OnHunterTakeDmg(class CCharacter *pChr, vec2 &Force, int &Dmg, int From, int WeaponType, int WeaponID, bool IsExplosion)
{
	if(pChr->GetPlayer()->GetCID() == From)
		return DAMAGE_NO_DAMAGE | DAMAGE_NO_INDICATOR;
	return DAMAGE_NORMAL;
}
bool CGameControllerHunter::OnHunterGrenadeCollide(CProjectile *pProj, vec2 Pos, CCharacter *pHit, bool EndOfLife)
{
	if(pHit && pHit->GetPlayer()->GetCID() == pProj->GetOwner())
		return false;

	pProj->GameWorld()->CreateExplosion(Pos, pProj->GetOwner(), WEAPON_GRENADE, pProj->GetWeaponID(), g_pData->m_Weapons.m_aId[WEAPON_GRENADE].m_Damage, pProj->GetOwner() < 0);
	pProj->GameWorld()->CreateSound(Pos, SOUND_GRENADE_EXPLODE);

	static const float PerAngle = pi * 2 / RAND_MAX;
	int FragNum = pHit ?
			((CGameControllerHunter *)(pProj->Controller()))->m_HunterFragNumHit :
			((CGameControllerHunter *)(pProj->Controller()))->m_HunterFragNumNoHit;
	vec2 FragPos = pHit ?
			Pos :
			pProj->GetPos((pProj->Server()->Tick() - pProj->m_StartTick - 1) / (float)pProj->Server()->TickSpeed());

	for(int i = 0; i < FragNum; i++) // Create Fragments
	{	
		vec2 Dir = direction(PerAngle * rand());
		new CProjectile(
				pProj->GameWorld(),
				WEAPON_SHOTGUN, //Type
				pProj->GetWeaponID(), //WeaponID
				pProj->GetOwner(), //Owner
				FragPos, //Pos
				Dir * 0.5f, //Dir
				6.0f, // Radius
				10, //Span
				[](CProjectile *pProj, vec2 Pos, CCharacter *pHit, bool EndOfLife) -> bool // Callback
						{
							if(pHit)
							{
								if(pHit->GetPlayer()->GetCID() == pProj->GetOwner())
									return false;

								pHit->TakeDamage(vec2(0, 0), g_pData->m_Weapons.m_Shotgun.m_pBase->m_Damage, pProj->GetOwner(), WEAPON_GRENADE, pProj->GetWeaponID(), false);
							}

							return true;
						}
				);
	}

	float Angle = PerAngle * rand();
	for(int i = 0; i < HUNTERNCONFIG::NUM_HUNT_GRENADE_SFX; i++)
		pProj->GameWorld()->CreateExplosionParticle(FragPos + direction(Angle + i * (pi * 2 / HUNTERNCONFIG::NUM_HUNT_GRENADE_SFX)) * (3.8f * 32.f), -1);

	return true;
}

void CGameControllerHunter::OnSnap(int SnappingClient,
			CNetObj_GameInfo *pGameInfoObj, CNetObj_GameInfoEx *pGameInfoEx, CNetObj_GameData *pGameDataObj,
			protocol7::CNetObj_GameData *pGameData, protocol7::CNetObj_GameDataTeam *pGameDataTeam, protocol7::CNetObj_GameDataFlag *pGameDataFlag)
{
	CPlayer *pPlayer = GetPlayerIfInRoom(SnappingClient);
	if(!pPlayer)
		return;
	CCharacter *pChr = pPlayer->GetCharacter();
	if(m_Class[SnappingClient] == HunterClass::ID_HUNTER)
	{
		if(pChr)
		{
			CWeapon *pWeapon = pChr->CurrentWeapon();
			if(pWeapon && pWeapon->GetWeaponID() == WEAPON_ID_HAMMER && !pWeapon->IsReloading() && pChr->GetInput()->m_Fire & 1) //&& pChr->GetInput()->m_Fire & 1)
			{
				CEntity *pClosestChar = GameWorld()->ClosestEntity(pChr->m_Pos, 256.f * 32.f, CGameWorld::ENTTYPE_CHARACTER, pChr);
				if(pClosestChar)
				{
					vec2 From = pWeapon->Pos();
					vec2 ToDir = normalize(pClosestChar->m_Pos - From);
					vec2 To = From + (ToDir * 96.f);

					if(!SnapLaser(m_aSnapIDs[0], From, To, Server()->Tick() - 3))
						return;
				}
			}
		}
	}
}
