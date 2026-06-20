/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "huntern.h"

#include <engine/shared/console.h>
#include <game/server/entities/character.h>
#include <game/server/entities/textentity.h>
#include <game/server/weapons.h>
#include <game/version.h>

void CGameControllerHunterN::ConShowRng(IConsole::IResult *pResult, void *pUserData)
{
	CGameControllerHunterN *pSelf = (CGameControllerHunterN *)pUserData;

	for(int i = 0; i < (int)(sizeof(CGameControllerHunterN::m_apRng) / sizeof(CGameControllerHunterN::m_apRng[0])); i++)
	{
		if(!pSelf->m_apRng[i] || !pSelf->m_apRng[i]->m_apNameDesc)
		{
			pSelf->InstanceConsole()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "instance", "");
			continue;
		}
		pSelf->InstanceConsole()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "instance", pSelf->m_apRng[i]->m_apNameDesc);
	}
}

void CGameControllerHunterN::ConSetRng(IConsole::IResult *pResult, void *pUserData)
{
	CGameControllerHunterN *pSelf = (CGameControllerHunterN *)pUserData;

	if(pResult->NumArguments() > 0)
	{
		int RngID = pResult->GetInteger(0);
		if(RngID < 0 || RngID >= (int)(sizeof(CGameControllerHunterN::m_apRng) / sizeof(CGameControllerHunterN::m_apRng[0])) || !pSelf->m_apRng[RngID])
		{
			pSelf->InstanceConsole()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "instance", "RngID is invaild");
			return;
		}
		pSelf->m_RngEnabled = RngID;
		pSelf->InstanceConsole()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "instance", "Rng is set");
	}
	else
	{
		CGameControllerHunterN::ConShowRng(pResult, pUserData);
	}
}

void CGameControllerHunterN::ConSetClass(IConsole::IResult *pResult, void *pUserData)
{
	CGameControllerHunterN *pSelf = (CGameControllerHunterN *)pUserData;

	int CID = pResult->NumArguments() > 1 ? pResult->GetInteger(1) : pResult->m_ClientID;
	CPlayer *pPlayer = pSelf->GetPlayerIfInRoom(CID);
	if(!pPlayer) // If the player does not exist
	{
		pSelf->InstanceConsole()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "huntern", "invalid client id");
		return;
	}
	bool IsClassSet = false;
#	define REGISTER_HUNTERCLASS(ID, MASK_ID, TEAM_ID, DEATHFUNC, HANDLEFIREFUNC, DODMGFUNC, SPAWNFUNC, TAKEDMGFUNC) \
		if(HunterClass::ID == pResult->GetInteger(0)) \
		{ \
			IsClassSet = true; \
			pSelf->SetClass(CID, HunterClass::ID); \
			pSelf->m_aTeam[CID] = TEAM_ID; \
			if(pPlayer->GetCharacter() && pPlayer->GetCharacter()->IsAlive()) \
				pSelf->OnCharacterSpawn(pPlayer->GetCharacter()); \
		}
#		include <game/server/gamemodes/hunter.h>
#	undef REGISTER_HUNTERCLASS
	if(pResult->NumArguments() > 2)
		pSelf->m_aTeam[CID] = pResult->GetInteger(2); // Team
	if(!IsClassSet)
		pSelf->InstanceConsole()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "huntern", "invalid class id");
}

void CGameControllerHunterN::ConGiveWeapon(IConsole::IResult *pResult, void *pUserData)
{
	IGameController *pSelf = (IGameController *)pUserData;

	CPlayer *pPlayer = pSelf->GetPlayerIfInRoom((pResult->NumArguments() > 2) ? pResult->GetInteger(2) : pResult->m_ClientID);
	if(!pPlayer) 
		pSelf->InstanceConsole()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "huntern", "invalid client id");
	else if(!pPlayer->GetCharacter() || !pPlayer->GetCharacter()->IsAlive())
		pSelf->InstanceConsole()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "huntern", "character is dead");
	else
	{
		pPlayer->GetCharacter()->RemoveWeapon((pResult->GetInteger(1) < NUM_WEAPONS && pResult->GetInteger(1) >= 0) ? pResult->GetInteger(1) : 0); // Slot
		if(!pResult->GetInteger(4)) // Give Weapon
			pPlayer->GetCharacter()->GiveWeapon((pResult->GetInteger(1) < NUM_WEAPONS && pResult->GetInteger(1) >= 0) ? pResult->GetInteger(1) : 0, // Slot
				pResult->GetInteger(0), // Type
					(pResult->NumArguments() > 3) ? pResult->GetInteger(3) : -1); // ammo
		else // Powerup Weapon
			pPlayer->GetCharacter()->SetPowerUpWeapon(pResult->GetInteger(0), // Type
					(pResult->NumArguments() > 3) ? pResult->GetInteger(3) : -1); // ammo
	}
}

void CGameControllerHunterN::ConSetHeal(IConsole::IResult *pResult, void *pUserData)
{
	IGameController *pSelf = (IGameController *)pUserData;

	CPlayer *pPlayer = pSelf->GetPlayerIfInRoom((pResult->NumArguments() > 2) ? pResult->GetInteger(2) : pResult->m_ClientID);
	if(!pPlayer)
		pSelf->InstanceConsole()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "huntern", "invalid client id");
	else if(!pPlayer->GetCharacter() || !pPlayer->GetCharacter()->IsAlive())
		pSelf->InstanceConsole()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "huntern", "character is dead");
	else
	{
		if(pResult->NumArguments() > 3) // Set m_MaxHealth
			pPlayer->GetCharacter()->SetMaxHealth(pResult->GetInteger(3) > 0 ? pResult->GetInteger(3) : 0); // math maximum(pResult->GetInteger(3), 0);
		if(pResult->NumArguments() > 4) // Set m_MaxArmor
			pPlayer->GetCharacter()->SetMaxArmor(pResult->GetInteger(4) > 0 ? pResult->GetInteger(4) : 0); // math maximum(pResult->GetInteger(4), 0);

		pPlayer->GetCharacter()->SetHealth(pResult->GetInteger(0)); // Set Health
		if(pResult->NumArguments() > 1)
			pPlayer->GetCharacter()->SetArmor(pResult->GetInteger(1)); // Set Armor
	}
}

void CGameControllerHunterN::ConRevive(IConsole::IResult *pResult, void *pUserData)
{
	CGameControllerHunterN *pSelf = (CGameControllerHunterN *)pUserData;

	int CID = pResult->NumArguments() > 0 ? pResult->GetInteger(0) : pResult->m_ClientID;
	CPlayer *pPlayer = pSelf->GetPlayerIfInRoom(CID);
	if(!pPlayer) // If the player does not exist
		pSelf->InstanceConsole()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "huntern", "invalid client id");
	else if(pPlayer->GetCharacter() && pPlayer->GetCharacter()->IsAlive())
		pSelf->InstanceConsole()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "huntern", "character is alive");
	else
	{	if(pSelf->m_aTeam[CID] == TEAM_NONE)
			pSelf->m_aTeam[CID] = TEAM_CIVIC;
		pPlayer->TryRespawn();}
}

void CGameControllerHunterN::ConSign(IConsole::IResult *pResult, void *pUserData)
{
	IGameController *pSelf = (IGameController *)pUserData;

	CPlayer *pPlayer = pSelf->GetPlayerIfInRoom(pResult->m_ClientID);
	if(!pPlayer) // If the player does not exist
		pSelf->InstanceConsole()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "huntern", "invalid client id");
	else
		new CTextEntity(pSelf->GameWorld(), pPlayer->m_ViewPos, CTextEntity::TYPE_LASER, 12, CTextEntity::ALIGN_MIDDLE, (char *)pResult->GetString(0));
}

// void CGameControllerHunterN::ConSelectMask(IConsole::IResult *pResult, void *pUserData)
// {
// 	CGameControllerHunterN *pSelf = (CGameControllerHunterN *)pUserData;

// 	int Mask = pResult->GetInteger(0);

// 	for(int i = 0; i < ; ++i)
// 	{
		
// 	}
// }
