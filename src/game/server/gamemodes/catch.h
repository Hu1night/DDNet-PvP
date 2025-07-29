#ifndef GAME_SERVER_GAMEMODES_CATCH_H
#define GAME_SERVER_GAMEMODES_CATCH_H

#include "instagib.h"

#include <game/server/entities/character.h>
#include <game/server/player.h>
#include <game/server/weapons.h>

#include <game/server/entities/dumbentity.h>
#include <game/server/gamecontroller.h>

template<class T>
class CGameControllerCatch : public T
{
private:
	// config
	int m_WinnerBonus;
	int m_MinimumPlayers;

	// states
	int m_aCaughtBy[MAX_CLIENTS];
	int m_aNumCaught[MAX_CLIENTS];
	class CDumbEntity *m_apHearts[MAX_CLIENTS];
	int m_aHeartID[MAX_CLIENTS];
	int m_aHeartKillTick[MAX_CLIENTS];

	struct Path
	{
		vec2 m_aPathPoints[MAX_CLIENTS];
		int m_PathIndex;

		int Prev(int Num) { return (m_PathIndex + MAX_CLIENTS - Num) % MAX_CLIENTS; }
		int Next(int Num) { return (m_PathIndex + Num) % MAX_CLIENTS; }
		int Index() { return Prev(1); }
		vec2 LatestPoint() { return m_aPathPoints[Index()]; }
		vec2 PrevPoint(int Num) { return m_aPathPoints[Prev(Num + 1)]; }

		Path() { m_PathIndex = 0; }

		void Init(vec2 Point)
		{
			for(auto &P : m_aPathPoints)
				P = Point;
		}

		void RecordPoint(vec2 Point)
		{
			m_aPathPoints[m_PathIndex] = Point;
			m_PathIndex = Next(1);
		}
	};

	// path
	Path m_aCharPath[MAX_CLIENTS];
	vec2 m_aLastPosition[MAX_CLIENTS];
	float m_aCharInertia[MAX_CLIENTS];
	float m_aCharMoveDist[MAX_CLIENTS];

public:
	CGameControllerCatch();

	void RegisterConfig()
	{
		INSTANCE_CONFIG_INT(&m_WinnerBonus, "winner_bonus", 100, 0, 2000, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "amount of points given to winner")
		INSTANCE_CONFIG_INT(&m_MinimumPlayers, "minimum_players", 5, 1, MAX_CLIENTS, CFGFLAG_CHAT | CFGFLAG_INSTANCE, "how many players required to trigger match end")
	}

	void Catch(class CPlayer *pVictim)
	{
		CPlayer *TopPlayer = nullptr;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			CPlayer *pPlayer = this->GetPlayerIfInRoom(i);
			if(pPlayer && pPlayer->GetCharacter() && pPlayer->GetCharacter()->IsAlive() && (!TopPlayer || pPlayer->m_Score > TopPlayer->m_Score))
				TopPlayer = pPlayer;
		}

		// catch by top player
		if(TopPlayer)
		{
			vec2 Pos = m_aCharPath[TopPlayer->GetCID()].PrevPoint(m_aNumCaught[TopPlayer->GetCID()]);
			Catch(pVictim, TopPlayer, Pos);
		}
	}

	void Catch(class CPlayer *pVictim, class CPlayer *pBy, vec2 Pos)
	{
		if(m_aCaughtBy[pVictim->GetCID()] != -1)
			return;

		if(m_apHearts[pVictim->GetCID()])
		{
			m_apHearts[pVictim->GetCID()]->Destroy();
			m_apHearts[pVictim->GetCID()] = nullptr;
			m_aHeartKillTick[pVictim->GetCID()] = -1;
		}

		m_apHearts[pVictim->GetCID()] = new CDumbEntity(this->GameWorld(), CDumbEntity::TYPE_HEART, Pos);
		m_aHeartID[pVictim->GetCID()] = m_aNumCaught[pBy->GetCID()];
		m_aNumCaught[pBy->GetCID()]++;

		m_aCaughtBy[pVictim->GetCID()] = pBy->GetCID();
	}

	void Release(class CPlayer *pPlayer, bool IsKillRelease)
	{
		if(m_aCaughtBy[pPlayer->GetCID()] == -1)
			return;

		int By = m_aCaughtBy[pPlayer->GetCID()];

		m_aNumCaught[By]--;
		m_aCaughtBy[pPlayer->GetCID()] = -1;

		if(IsKillRelease)
		{
			// do heart kill animation
			m_aHeartKillTick[pPlayer->GetCID()] = this->Server()->Tick() + (m_aHeartID[pPlayer->GetCID()] + 1) * 2;
		}
		else
		{
			// remove last heart
			int LastHeartID = m_aNumCaught[By];

			// find the last heart
			int WhoHasLastHeart = pPlayer->GetCID();
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(m_aCaughtBy[i] != By)
					continue;

				if(m_aHeartID[i] == LastHeartID)
				{
					WhoHasLastHeart = i;
					break;
				}
			}

			m_apHearts[WhoHasLastHeart]->Destroy();
			m_apHearts[WhoHasLastHeart] = m_apHearts[pPlayer->GetCID()];
			m_aHeartID[WhoHasLastHeart] = m_aHeartID[pPlayer->GetCID()];
			m_apHearts[pPlayer->GetCID()] = nullptr;
			m_aHeartID[pPlayer->GetCID()] = -1;
		}

		pPlayer->m_RespawnDisabled = false;
		pPlayer->m_RespawnTick = this->Server()->Tick() + this->Server()->TickSpeed();
		pPlayer->Respawn();
	}

	// event
	virtual void OnInit() override
	{
		T::OnInit();

		mem_zero(m_apHearts, sizeof(m_apHearts));
		mem_zero(m_aCharMoveDist, sizeof(m_aCharMoveDist));
		mem_zero(m_aNumCaught, sizeof(m_aNumCaught));

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			m_aCaughtBy[i] = -1;
			m_aHeartKillTick[i] = -1;
		}
	}

	virtual void OnPreTick() override
	{
		T::OnPreTick();

		if(!this->IsRunning())
			return;

		float PointDist = 50.0f;

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			CPlayer *pPlayer = this->GetPlayerIfInRoom(i);
			if(pPlayer && pPlayer->GetCharacter() && pPlayer->GetCharacter()->IsAlive())
			{
				CCharacter *pChar = pPlayer->GetCharacter();
				vec2 DeltaPos = pChar->GetPos() - m_aLastPosition[i];
				m_aLastPosition[i] = pChar->GetPos();

				vec2 LastPoint = m_aCharPath[i].LatestPoint();
				vec2 Dir = normalize(pChar->GetPos() - LastPoint);

				if(fabs(length(DeltaPos)) < 1e-6)
				{
					m_aCharInertia[i] = m_aCharInertia[i] / 1.05f;
					m_aCharMoveDist[i] += m_aCharInertia[i];
					Dir = Dir * 0.15f;
				}
				else
				{
					m_aCharInertia[i] = length(DeltaPos);
					m_aCharMoveDist[i] += length(DeltaPos);
				}

				int Iteration = 0;
				while(m_aCharMoveDist[i] > PointDist)
				{
					m_aCharMoveDist[i] -= PointDist;
					Iteration++;
					vec2 Point = LastPoint + Dir * PointDist * Iteration;
					m_aCharPath[i].RecordPoint(Point);
				}
			}
		}

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_apHearts[i])
			{
				if(m_aHeartKillTick[i] != -1 && m_aHeartKillTick[i] < this->Server()->Tick())
				{
					this->GameWorld()->CreateSound(m_apHearts[i]->GetPos(), SOUND_PLAYER_DIE);
					this->GameWorld()->CreateDeath(m_apHearts[i]->GetPos(), i);
					m_apHearts[i]->Destroy();
					m_apHearts[i] = nullptr;
					m_aHeartID[i] = -1;
					m_aHeartKillTick[i] = -1;
					continue;
				}

				int CaughtBy = m_aCaughtBy[i];
				CPlayer *pPlayer = this->GetPlayerIfInRoom(CaughtBy);
				if(pPlayer && pPlayer->GetCharacter() && pPlayer->GetCharacter()->IsAlive())
				{
					float Interp = m_aCharMoveDist[CaughtBy] / PointDist;
					vec2 Point = m_aCharPath[CaughtBy].PrevPoint(m_aHeartID[i]);
					vec2 PrevPoint = m_aCharPath[CaughtBy].PrevPoint(m_aHeartID[i] + 1);
					vec2 TargetPos = mix(PrevPoint, Point, Interp);
					float Rate = 1.0f - (m_aHeartID[i] / (32.0f + 16.0f));
					vec2 Pos = mix(m_apHearts[i]->m_Pos, TargetPos, (25.0f * Rate * Rate / (float)this->Server()->TickSpeed()));
					m_apHearts[i]->MoveTo(Pos);
				}
			}
		}
	}

	virtual void OnWorldReset() override
	{
		T::OnWorldReset();

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			m_aCaughtBy[i] = -1;
			m_aNumCaught[i] = 0;
			m_aHeartKillTick[i] = -1;
			if(m_apHearts[i])
			{
				m_apHearts[i]->Destroy();
				m_apHearts[i] = nullptr;
			}
		}
	}

	virtual void OnKill(class CPlayer *pPlayer) override
	{
		T::OnKill(pPlayer);

		int ClientID = pPlayer->GetCID();

		if(m_aNumCaught[ClientID] > 0)
		{
			for(int i = 0; i < MAX_CLIENTS; ++i)
			{
				CPlayer *pPlayer = this->GetPlayerIfInRoom(i);
				if(pPlayer && m_aCaughtBy[pPlayer->GetCID()] == ClientID)
					Release(pPlayer, true);
			}

			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "'%s' released.", this->Server()->ClientName(ClientID));
			this->SendChatTarget(-1, aBuf);
		}
	}

	virtual void OnPlayerJoin(class CPlayer *pPlayer) override
	{
		T::OnPlayerJoin(pPlayer);

		// don't do anything during warmup
		if(this->IsWarmup())
			return;

		Catch(pPlayer);
	}

	virtual void OnPlayerLeave(class CPlayer *pPlayer) override
	{
		T::OnPlayerLeave(pPlayer);

		Release(pPlayer, false);
	}

	virtual bool OnPlayerTryRespawn(class CPlayer *pPlayer, vec2 Pos) override
	{
		if(m_aCaughtBy[pPlayer->GetCID()] == -1)
			return true;

		int By = m_aCaughtBy[pPlayer->GetCID()];
		CPlayer *pKiller = this->GetPlayerIfInRoom(By);

		if(!pKiller || !pKiller->GetCharacter() || !pKiller->GetCharacter()->IsAlive())
		{
			m_aCaughtBy[pPlayer->GetCID()] = -1;
			return true;
		}

		pPlayer->CancelSpawn();
		return false;
	}

	virtual void OnPlayerChangeTeam(class CPlayer *pPlayer, int FromTeam, int ToTeam) override
	{
		T::OnPlayerChangeTeam(pPlayer, FromTeam, ToTeam);

		if(ToTeam == TEAM_SPECTATORS)
			Release(pPlayer, false);
		else
			Catch(pPlayer);
	}

	virtual void OnCharacterSpawn(class CCharacter *pChr) override
	{
		T::OnCharacterSpawn(pChr);

		int ClientID = pChr->GetPlayer()->GetCID();
		m_aLastPosition[ClientID] = pChr->GetPos();
		m_aCharMoveDist[ClientID] = 0;
		m_aCharPath[ClientID].Init(pChr->GetPos());
		m_aCharInertia[ClientID] = 20.0f;
	}

	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon) override
	{
		// don't do anything during warmup
		if(this->IsWarmup())
			return DEATH_NORMAL;

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			CPlayer *pPlayer = this->GetPlayerIfInRoom(i);
			if(pPlayer && m_aCaughtBy[pPlayer->GetCID()] == pVictim->GetPlayer()->GetCID())
			{
				Release(pPlayer, true);
			}
		}

		// allow respawn, only check caught status on respawn (handles mutual kills)
		pVictim->GetPlayer()->m_RespawnDisabled = false;
		pVictim->GetPlayer()->m_RespawnTick = this->Server()->Tick() + this->Server()->TickSpeed();
		pVictim->GetPlayer()->Respawn();

		// this also covers suicide
		if(!pKiller->GetCharacter() || !pKiller->GetCharacter()->IsAlive())
		{
			return DEATH_NORMAL;
		}

		Catch(pVictim->GetPlayer(), pKiller, pVictim->GetPos());

		return DEATH_NORMAL;
	}

	virtual bool CanDeadPlayerFollow(const class CPlayer *pSpectator, const class CPlayer *pTarget) override
	{
		return m_aCaughtBy[pSpectator->GetCID()] == pTarget->GetCID();
	}

	virtual void DoWincheckMatch() override
	{
		T::DoWincheckMatch();

		CPlayer *pAlivePlayer = 0;
		int AlivePlayerCount = 0;
		int TotalPlayerCount = 0;
		for(int i = 0; i < MAX_CLIENTS; ++i)
		{
			CPlayer *pPlayer = this->GetPlayerIfInRoom(i);
			if(pPlayer && pPlayer->GetTeam() != TEAM_SPECTATORS)
			{
				TotalPlayerCount++;

				if(!pPlayer->m_RespawnDisabled || (pPlayer->GetCharacter() && pPlayer->GetCharacter()->IsAlive()))
				{
					++AlivePlayerCount;
					pAlivePlayer = pPlayer;
				}
			}
		}

		if(AlivePlayerCount == 1) // 1 winner
		{
			if(TotalPlayerCount >= m_MinimumPlayers)
			{
				pAlivePlayer->m_Score += m_WinnerBonus;
				this->EndMatch();
			}
			else
			{
				for(int i = 0; i < MAX_CLIENTS; ++i)
				{
					CPlayer *pPlayer = this->GetPlayerIfInRoom(i);
					if(pPlayer && m_aCaughtBy[pPlayer->GetCID()] == pAlivePlayer->GetCID())
						Release(pPlayer, true);
				}
			}
		}

		// still counts scorelimit
		IGameController::DoWincheckMatch();
	}
};

typedef CGameControllerInstagib<CGameControllerCatch<CGameControllerDM>> CGameControllerZCatch;

#endif // GAME_SERVER_GAMEMODES_CATCH_H
