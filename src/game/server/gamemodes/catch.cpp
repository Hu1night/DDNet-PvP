#include "catch.h"

template<>
CGameControllerCatch<CGameControllerDM>::CGameControllerCatch()
: CGameControllerDM()
{
	m_GameFlags = IGF_MARK_SURVIVAL;
	RegisterConfig();
}

template<>
CGameControllerInstagib<CGameControllerCatch<CGameControllerDM>>::CGameControllerInstagib()
: CGameControllerCatch<CGameControllerDM>()
{
	m_pGameType = "zCatch";
	RegisterConfig();
}
