//
// Copyright (c) 2003-2009, by Yet Another POD-Bot Development Team.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// $Id:$
//

#include <core.h>

ConVar rbot_quota("rbot_quota", "20");
ConVar rbot_forceteam("rbot_force_team", "any");
ConVar rbot_auto_players("rbot_auto_players", "-1"); // i don't even know what is this...
ConVar rbot_quota_save("rbot_quota_save", "-1");

ConVar rbot_difficulty("rbot_difficulty", "2");
ConVar rbot_minskill("rbot_min_skill", "1");
ConVar rbot_maxskill("rbot_max_skill", "100");

ConVar rbot_nametag("rbot_name_tag", "0");
ConVar rbot_ping("rbot_fake_ping", "1");
ConVar rbot_display_avatar("rbot_display_avatar", "1");

ConVar rbot_autovacate("rbot_auto_vacate", "0");
ConVar rbot_save_bot_names("rbot_save_bot_names", "0");

ConVar rbot_realistic_playtime("rbot_realistic_playtime", "0"); // 0 - Off , 1 - On
ConVar rbot_realistic_avg_amount("rbot_realistic_avg_amount", "5"); // AVG ~ RBOT
ConVar rbot_realistic_stay_mintime("rbot_realistic_stay_mintime", "15"); // in seocnds
ConVar rbot_realistic_stay_maxtime("rbot_realistic_stay_maxtime", "500"); // in seconds

ConVar rbot_think_fps("rbot_think_fps", "20.0");

// this is a bot manager class constructor
BotControl::BotControl(void)
{
	m_lastWinner = -1;

	m_economicsGood[TEAM_TERRORIST] = true;
	m_economicsGood[TEAM_COUNTER] = true;

	memset(m_bots, 0, sizeof(m_bots));
	InitQuota();
}

// this is a bot manager class destructor, do not use engine->GetMaxClients () here !!
BotControl::~BotControl(void)
{
	for (int i = 0; i < 32; i++)
	{
		if (m_bots[i])
		{
			delete m_bots[i];
			m_bots[i] = nullptr;
		}
	}
}

// this function calls gamedll player() function, in case to create player entity in game
void BotControl::CallGameEntity(entvars_t* vars)
{
	if (g_isMetamod)
	{
		CALL_GAME_ENTITY(PLID, "player", vars);
		return;
	}

	static EntityPtr_t playerFunction = nullptr;

	if (playerFunction == nullptr)
		playerFunction = (EntityPtr_t)g_gameLib->GetFunctionAddr("player");

	if (playerFunction != nullptr)
		(*playerFunction) (vars);
}

// this function completely prepares bot entity (edict) for creation, creates team, skill, sets name etc, and
// then sends result to bot constructor
int BotControl::CreateBot(String name, int skill, int personality, int team, int member)
{
	edict_t* bot = nullptr;
	if (g_numWaypoints < 1) // don't allow creating bots with no waypoints loaded
	{
		ServerPrint("No any waypoints for this map, Cannot Add RBOT");
		ServerPrint("You can input 'rbot wp menu' to make waypoint");
		CenterPrint("No any waypoints for this map, Cannot Add RBOT");
		CenterPrint("You can input 'rbot wp menu' to make waypoint");
		return -1;
	}
	else if (g_waypointsChanged) // don't allow creating bots with changed waypoints (distance tables are messed up)
	{
		CenterPrint("Waypoints has been changed. Load waypoints again...");
		return -1;
	}

	if (skill <= 0 || skill > 100)
	{
		if (rbot_difficulty.GetInt() >= 4)
			skill = 100;
		else if (rbot_difficulty.GetInt() == 3)
			skill = engine->RandomInt(79, 99);
		else if (rbot_difficulty.GetInt() == 2)
			skill = engine->RandomInt(50, 79);
		else if (rbot_difficulty.GetInt() == 1)
			skill = engine->RandomInt(30, 50);
		else if (rbot_difficulty.GetInt() == 0)
			skill = engine->RandomInt(1, 30);
		else
		{
			int maxSkill = rbot_maxskill.GetInt();
			int minSkill = (rbot_minskill.GetInt() == 0) ? 1 : rbot_minskill.GetInt();

			if (maxSkill <= 100 && minSkill > 0)
				skill = engine->RandomInt(minSkill, maxSkill);
			else
				skill = engine->RandomInt(0, 100);
		}
	}

	if (personality < 0 || personality > 2)
	{
		int randomPrecent = engine->RandomInt(1, 3);

		if (randomPrecent == 1)
			personality = PERSONALITY_NORMAL;
		else if (randomPrecent == 2)
			personality = PERSONALITY_RUSHER;
		else
			personality = PERSONALITY_CAREFUL;
	}

	char outputName[33];
	bool addTag = true;

	// restore the bot name
	if (rbot_save_bot_names.GetBool() && !m_savedBotNames.IsEmpty())
	{
		sprintf(outputName, "%s", (char*)m_savedBotNames.Pop());
		addTag = false;
	}
	else if (name.GetLength() <= 0)
	{
		bool getName = false;
		if (!g_botNames.IsEmpty())
		{
			ITERATE_ARRAY(g_botNames, j)
			{
				if (!g_botNames[j].isUsed)
				{
					getName = true;
					break;
				}
			}
		}

		if (getName)
		{
			bool nameUse = true;

			while (nameUse)
			{
				NameItem& botName = g_botNames.GetRandomElement();
				if (!botName.isUsed)
				{
					nameUse = false;
					botName.isUsed = true;
					sprintf(outputName, "%s", (char*)botName.name);
				}
			}
		}
		else
			sprintf(outputName, "%i | RBOT", engine->RandomInt(100, 999)); // just pick ugly random name
	}
	else
		sprintf(outputName, "%s", (char*)name);

	char botName[64];
	if (rbot_nametag.GetInt() == 2 && addTag)
		snprintf(botName, sizeof(botName), "[RBOT] %s (%d)", outputName, skill);
	else if (rbot_nametag.GetInt() == 1 && addTag)
		snprintf(botName, sizeof(botName), "[RBOT] %s", outputName);
	else
		strncpy(botName, outputName, sizeof(botName));

	if (FNullEnt((bot = (*g_engfuncs.pfnCreateFakeClient) (botName))))
	{
		CenterPrint(" Unable to create RBOT, Maximum players reached (%d/%d)", engine->GetMaxClients(), engine->GetMaxClients());
		return -2;
	}

	int index = ENTINDEX(bot) - 1;

	InternalAssert(index >= 0 && index <= 32); // check index
	InternalAssert(m_bots[index] == nullptr); // check bot slot

	m_bots[index] = new Bot(bot, skill, personality, team, member);

	if (m_bots == nullptr)
		return -1;

	ServerPrint("Connecting RBOT - %s [Skill %d]", GetEntityName(bot), skill);

	return index;
}

// this function returns index of bot (using own bot array)
int BotControl::GetIndex(edict_t* ent)
{
	if (FNullEnt(ent))
		return -1;

	int index = ENTINDEX(ent) - 1;
	if (index < 0 || index >= 32)
		return -1;

	if (m_bots[index] != nullptr)
		return index;

	return -1; // if no edict, return -1;
}

// this function returns index of bot (using own bot array)
int BotControl::GetIndex(int index)
{
	if (index < 0 || index >= 32)
		return -1;

	if (m_bots[index] != nullptr)
		return index;

	return -1;
}

// this function finds a bot specified by index, and then returns pointer to it (using own bot array)
Bot* BotControl::GetBot(int index)
{
	if (index < 0 || index >= 32)
		return nullptr;

	if (m_bots[index] != nullptr)
		return m_bots[index];

	return nullptr; // no bot
}

// same as above, but using bot entity
Bot* BotControl::GetBot(edict_t* ent)
{
	return GetBot(GetIndex(ent));
}

// this function finds one bot, alive bot :)
Bot* BotControl::FindOneValidAliveBot(void)
{
	Array <int> foundBots;

	for (const auto& bot : m_bots)
	{
		if (bot != nullptr && bot->m_isAlive)
			foundBots.Push(bot->m_index);
	}

	if (!foundBots.IsEmpty())
		return m_bots[foundBots.GetRandomElement()];

	return nullptr;
}

void BotControl::PlayTimeExecution(void)
{
    if (!rbot_realistic_playtime.GetBool())
        return;

    // Only process if it's time to join/quit.
    if (m_randomJoinTime > engine->GetTime())
        return;

    // Determine the total number of bots to add, varying around the configured average.
    int base = rbot_realistic_avg_amount.GetInt();
    int variation = engine->RandomInt(-1, 1); // Variation: -1, 0, or 1
    int new_quota = base + variation;

    if (new_quota < 0)
		new_quota = 0;


	int current_count = 0;

	for (const auto& bot : g_botManager->m_bots) {
		if (bot == nullptr)
		    continue;

		current_count++;
	}

	int final_change = new_quota - current_count;

	if (final_change < 0) {
		while (final_change != 0) {
			g_botManager->RemoveRandom();
			final_change++;
		}
	} else {
		while (final_change != 0) {
			g_botManager->AddRandom();
			final_change--;
		}
	}

    m_randomJoinTime = AddTime(engine->RandomFloat(rbot_realistic_stay_mintime.GetInt(), rbot_realistic_stay_maxtime.GetInt()));
}

void BotControl::Think(void)
{
	g_botManager->PlayTimeExecution();

	extern ConVar rbot_stopbots;
	if (rbot_stopbots.GetBool())
		return;

	for (const auto& bot : m_bots)
	{
		if (bot == nullptr)
			continue;

		if (bot->m_thinkTimer < engine->GetTime() && bot->m_lastThinkTime < engine->GetTime())
		{
			bot->Think();
			bot->m_thinkTimer = AddTime(1.0f / rbot_think_fps.GetFloat());
		}
		
		if (bot->m_isAlive)
		{
			bot->m_moveAnglesForRunMove = bot->m_moveAngles;
			bot->m_moveSpeedForRunMove = bot->m_moveSpeed;
			bot->m_strafeSpeedForRunMove = bot->m_strafeSpeed;
			bot->FacePosition();
		}
		else
			bot->m_aimInterval = engine->GetTime();

		bot->RunPlayerMovement(); // run the player movement 
	}
}

// this function putting bot creation process to queue to prevent engine crashes
void BotControl::AddBot(const String& name, int skill, int personality, int team, int member)
{
	CreateItem queueID;

	// fill the holder
	queueID.name = name;
	queueID.skill = skill;
	queueID.personality = personality;
	queueID.team = team;
	queueID.member = member;

	// put to queue
	m_creationTab.Push(queueID);

	// keep quota number up to date
	if (GetBotsNum() + 1 > rbot_quota.GetInt())
		rbot_quota.SetInt(GetBotsNum() + 1);
}

void BotControl::CheckBotNum(void)
{
	if (rbot_auto_players.GetInt() == -1 && rbot_quota_save.GetInt() == -1)
		return;

	int needBotNumber = 0;
	if (rbot_quota_save.GetInt() != -1)
	{
		if (rbot_quota_save.GetInt() > 32)
			rbot_quota_save.SetInt(32);

		needBotNumber = rbot_quota_save.GetInt();

		File fp(FormatBuffer("%s/addons/rbot/rbot.cfg", GetModName()), "rt+");
		if (fp.IsValid())
		{
			const char quotaCvar[11] = { 's', 'y', 'p', 'b', '_', 'q', 'u', 'o', 't', 'a', ' ' };

			char line[256];
			bool changeed = false;
			while (fp.GetBuffer(line, 255))
			{
				bool trueCvar = true;
				for (int j = 0; (j < 11 && trueCvar); j++)
				{
					if (quotaCvar[j] != line[j])
						trueCvar = false;
				}

				if (!trueCvar)
					continue;

				changeed = true;

				int i = 0;
				for (i = 0; i <= 255; i++)
				{
					if (line[i] == 0)
						break;
				}
				i++;
				fp.Seek(-i, SEEK_CUR);

				if (line[11] == 0 || line[12] == 0 || line[13] == '"' ||
					line[11] == '\n' || line[12] == '\n')
				{
					changeed = false;
					fp.Print("//////////");
					break;
				}

				if (line[11] == '"')
				{
					fp.PutString(FormatBuffer("rbot_quota \"%s%d\"",
						needBotNumber > 10 ? "" : "0", needBotNumber));
				}
				else
					fp.PutString(FormatBuffer("rbot_quota %s%d",
						needBotNumber > 10 ? "" : "0", needBotNumber));

				ServerPrint("rbot_quota save to '%d' - C", needBotNumber);

				break;
			}

			if (!changeed)
			{
				fp.Seek(0, SEEK_END);
				fp.Print(FormatBuffer("\nrbot_quota \"%s%d\"\n",
					needBotNumber > 10 ? "" : "0", needBotNumber));
				ServerPrint("rbot_quota save to '%d' - A", needBotNumber);
			}

			fp.Close();
		}
		else
		{
			File fp2(FormatBuffer("%s/addons/rbot/rbot.cfg", GetModName()), "at");
			if (fp2.IsValid())
			{
				fp2.Print(FormatBuffer("\nrbot_quota \"%s%d\"\n",
					needBotNumber > 10 ? "" : "0", needBotNumber));
				ServerPrint("rbot_quota save to '%d' - A", needBotNumber);
				fp2.Close();
			}
			else
				ServerPrint("Unknow Problem - Cannot save rbot quota");
		}

		rbot_quota_save.SetInt(-1);
	}

	if (rbot_auto_players.GetInt() != -1)
	{
		if (rbot_auto_players.GetInt() > engine->GetMaxClients())
		{
			ServerPrint("Server Max Clients is %d, You cannot set this value", engine->GetMaxClients());
			rbot_auto_players.SetInt(engine->GetMaxClients());
		}

		needBotNumber = rbot_auto_players.GetInt() - GetHumansNum();
		if (needBotNumber <= 0)
			needBotNumber = 0;
	}

	rbot_quota.SetInt(needBotNumber);
}

int BotControl::AddBotAPI(const String& name, int skill, int team)
{
	if (g_botManager->GetBotsNum() + 1 > rbot_quota.GetInt())
		rbot_quota.SetInt(g_botManager->GetBotsNum() + 1);

	int resultOfCall = CreateBot(name, skill, -1, team, -1);

	// check the result of creation
	if (resultOfCall == -1)
	{
		m_creationTab.RemoveAll(); // something wrong with waypoints, reset tab of creation
		rbot_quota.SetInt(0); // reset quota
		ChartPrint("[RBOT] You can input [rbot sgdwp on] make the new waypoints!!");
	}
	else if (resultOfCall == -2)
	{
		m_creationTab.RemoveAll(); // maximum players reached, so set quota to maximum players
		rbot_quota.SetInt(GetBotsNum());
	}

	m_maintainTime = AddTime(0.2f);

	return resultOfCall;
}

// this function keeps number of bots up to date, and don't allow to maintain bot creation
// while creation process in process.
void BotControl::MaintainBotQuota(void)
{
	if (!m_creationTab.IsEmpty() && m_maintainTime < engine->GetTime())
	{
		CreateItem last = m_creationTab.Pop();

		int resultOfCall = CreateBot(last.name, last.skill, last.personality, last.team, last.member);

		// check the result of creation
		if (resultOfCall == -1)
		{
			m_creationTab.RemoveAll(); // something wrong with waypoints, reset tab of creation
			rbot_quota.SetInt(0); // reset quota
			
			ChartPrint("[RBOT] You can input [rbot sgdwp on] make the new waypoints.");
		}
		else if (resultOfCall == -2)
		{
			m_creationTab.RemoveAll(); // maximum players reached, so set quota to maximum players
			rbot_quota.SetInt(GetBotsNum());
		}

		m_maintainTime = AddTime(0.25f);
	}

	if (rbot_realistic_playtime.GetBool())
		return;

	g_botManager->CheckBotNum();
	if (m_maintainTime < engine->GetTime())
	{
		int botNumber = GetBotsNum();
		int maxClients = engine->GetMaxClients();
		int desiredBotCount = rbot_quota.GetInt();
		
		if (rbot_autovacate.GetBool())
			desiredBotCount = MinInt(desiredBotCount, maxClients - (GetHumansNum() + 1));
		
		if (botNumber > desiredBotCount)
		{
			RemoveRandom();
			m_maintainTime = AddTime(0.25f);
		}
		else if (botNumber < desiredBotCount && botNumber < maxClients)
		{
			AddRandom();
			m_maintainTime = AddTime(0.25f);
		}
		else
		{
			m_maintainTime = AddTime(1.0f);

			if (rbot_save_bot_names.GetBool() && !m_savedBotNames.IsEmpty()) // clear the saved names when quota balancing ended
				m_savedBotNames.Destory();
		}

		if (rbot_quota.GetInt() > maxClients)
			rbot_quota.SetInt(maxClients);
		else if (rbot_quota.GetInt() < 0)
			rbot_quota.SetInt(0);
	}
}

void BotControl::InitQuota(void)
{
	m_maintainTime = AddTime(2.0f);
	m_creationTab.RemoveAll();
	for (int i = 0; i < entityNum; i++)
		SetEntityActionData(i);
}

// this function fill server with bots, with specified team & personality
void BotControl::FillServer(int selection, int personality, int skill, int numToAdd)
{
	// always keep one slot
	int maxClients = rbot_autovacate.GetBool() ? engine->GetMaxClients() - 1 - (IsDedicatedServer() ? 0 : GetHumansNum()) : engine->GetMaxClients();

	if (GetBotsNum() >= maxClients - GetHumansNum())
		return;

	if (selection == 1 || selection == 2)
	{
		CVAR_SET_STRING("mp_limitteams", "0");
		CVAR_SET_STRING("mp_autoteambalance", "0");
	}
	else
		selection = 5;

	char teamDescs[6][12] =
	{
	   "",
	   {"Terrorists"},
	   {"CTs"},
	   "",
	   "",
	   {"Random"},
	};

	int toAdd = numToAdd == -1 ? maxClients - (GetHumansNum() + GetBotsNum()) : numToAdd;

	for (int i = 0; i <= toAdd; i++)
	{
		// since we got constant skill from menu (since creation process call automatic), we need to manually
		// randomize skill here, on given skill there.
		int randomizedSkill = 0;

		if (skill >= 0 && skill <= 20)
			randomizedSkill = engine->RandomInt(0, 20);
		else if (skill >= 20 && skill <= 40)
			randomizedSkill = engine->RandomInt(20, 40);
		else if (skill >= 40 && skill <= 60)
			randomizedSkill = engine->RandomInt(40, 60);
		else if (skill >= 60 && skill <= 80)
			randomizedSkill = engine->RandomInt(60, 80);
		else if (skill >= 80 && skill <= 99)
			randomizedSkill = engine->RandomInt(80, 99);
		else if (skill == 100)
			randomizedSkill = skill;

		AddBot("", randomizedSkill, personality, selection, -1);
	}

	rbot_quota.SetInt(toAdd);
	CenterPrint("Filling the server with %s rbots", &teamDescs[selection][0]);
}

// this function drops all bot clients from server (this function removes only rbots)
void BotControl::RemoveAll(void)
{
	CenterPrint("R-Bots are removed from server");

	for (const auto& bot : m_bots)
	{
		if (bot != nullptr) // is this slot used?
			bot->Kick();
	}

	m_creationTab.RemoveAll();

	// if everyone is kicked, clear the saved bot names
	if (rbot_save_bot_names.GetBool() && !m_savedBotNames.IsEmpty())
		m_savedBotNames.Destory();

	// reset cvars
	rbot_quota.SetInt(0);
	rbot_auto_players.SetInt(-1);
}

// this function remove random bot from specified team (if removeAll value = 1 then removes all players from team)
void BotControl::RemoveFromTeam(Team team, bool removeAll)
{
	for (const auto& bot : m_bots)
	{
		if (bot != nullptr && team == bot->m_team)
		{
			bot->Kick();

			if (!removeAll)
				break;
		}
	}
}

void BotControl::RemoveMenu(edict_t* ent, int selection)
{
	if ((selection > 4) || (selection < 1))
		return;

	char tempBuffer[1024], buffer[1024];
	memset(tempBuffer, 0, sizeof(tempBuffer));
	memset(buffer, 0, sizeof(buffer));

	int validSlots = (selection == 4) ? (1 << 9) : ((1 << 8) | (1 << 9));
	for (int i = ((selection - 1) * 8); i < selection * 8; ++i)
	{
		if ((m_bots[i] != nullptr) && !FNullEnt(m_bots[i]->GetEntity()))
		{
			validSlots |= 1 << (i - ((selection - 1) * 8));
			sprintf(buffer, "%s %1.1d. %s%s\n", buffer, i - ((selection - 1) * 8) + 1, GetEntityName(m_bots[i]->GetEntity()), GetTeam(m_bots[i]->GetEntity()) == TEAM_COUNTER ? " \\y(CT)\\w" : " \\r(T)\\w");
		}
		else if (!FNullEnt(g_clients[i].ent))
			sprintf(buffer, "%s %1.1d.\\d %s (Not rbot) \\w\n", buffer, i - ((selection - 1) * 8) + 1, GetEntityName(g_clients[i].ent));
		else
			sprintf(buffer, "%s %1.1d.\\d Null \\w\n", buffer, i - ((selection - 1) * 8) + 1);
	}

	sprintf(tempBuffer, "\\yRBOT Remove Menu (%d/4):\\w\n\n%s\n%s 0. Back", selection, buffer, (selection == 4) ? "" : " 9. More...\n");

	switch (selection)
	{
	case 1:
		g_menus[14].validSlots = validSlots & static_cast <unsigned int> (-1);
		g_menus[14].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[14]);
		break;

	case 2:
		g_menus[15].validSlots = validSlots & static_cast <unsigned int> (-1);
		g_menus[15].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[15]);
		break;

	case 3:
		g_menus[16].validSlots = validSlots & static_cast <unsigned int> (-1);
		g_menus[16].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[16]);
		break;

	case 4:
		g_menus[17].validSlots = validSlots & static_cast <unsigned int> (-1);
		g_menus[17].menuText = tempBuffer;

		DisplayMenuToClient(ent, &g_menus[17]);
		break;
	}
}

// this function kills all bots on server (only this dll controlled bots)
void BotControl::KillAll(int team)
{
	for (const auto& bot : m_bots)
	{
		if (bot != nullptr)
		{
			if (team != -1 && team != bot->m_team)
				continue;

			bot->Kill();
		}
	}

	CenterPrint("All bots are killed.");
}

// this function removes random bot from server (only rbots)
void BotControl::RemoveRandom(void)
{
	for (const auto& bot : m_bots)
	{
		if (bot != nullptr)  // is this slot used?
		{
			bot->Kick();
			break;
		}
	}
}

// this function sets bots weapon mode
void BotControl::SetWeaponMode(int selection)
{
	int tabMapStandart[7][Const_NumWeapons] =
	{
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Knife only
	   {-1,-1,-1, 2, 2, 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Pistols only
	   {-1,-1,-1,-1,-1,-1,-1, 2, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Shotgun only
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1, 2, 1, 2, 0, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 2,-1}, // Machine Guns only
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0, 0, 1, 0, 1, 1,-1,-1,-1,-1,-1,-1}, // Rifles only
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 2, 2, 0, 1,-1,-1}, // Snipers only
	   {-1,-1,-1, 2, 2, 0, 1, 2, 2, 2, 1, 2, 0, 2, 0, 0, 1, 0, 1, 1, 2, 2, 0, 1, 2, 1}  // Standard
	};

	int tabMapAS[7][Const_NumWeapons] =
	{
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Knife only
	   {-1,-1,-1, 2, 2, 0, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Pistols only
	   {-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // Shotgun only
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1, 1, 1, 1, 0, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 1,-1}, // Machine Guns only
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0,-1, 1, 0, 1, 1,-1,-1,-1,-1,-1,-1}, // Rifles only
	   {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, 0, 0,-1, 1,-1,-1}, // Snipers only
	   {-1,-1,-1, 2, 2, 0, 1, 1, 1, 1, 1, 1, 0, 2, 0,-1, 1, 0, 1, 1, 0, 0,-1, 1, 1, 1}  // Standard
	};

	char modeName[7][12] =
	{
	   {"Knife"},
	   {"Pistol"},
	   {"Shotgun"},
	   {"Machine Gun"},
	   {"Rifle"},
	   {"Sniper"},
	   {"Standard"}
	};
	selection--;

	for (int i = 0; i < Const_NumWeapons; i++)
	{
		g_weaponSelect[i].teamStandard = tabMapStandart[selection][i];
		g_weaponSelect[i].teamAS = tabMapAS[selection][i];
	}

	if (selection == 0)
		rbot_onlyknifemode.SetInt(1);
	else
		rbot_onlyknifemode.SetInt(0);

	CenterPrint("%s weapon mode selected", &modeName[selection][0]);
}

// this function lists bots currently playing on the server
void BotControl::ListBots(void)
{
	ServerPrintNoTag("%-3.5s %-9.13s %-17.18s %-3.4s %-3.4s %-3.4s", "index", "name", "personality", "team", "skill", "frags");

	for (const auto& client : g_clients)
	{
		edict_t* player = client.ent;

		// is this player slot valid
		if (IsValidBot(player) && GetBot(player))
			ServerPrintNoTag("[%-3.1d] %-9.13s %-17.18s %-3.4s %-3.1d %-3.1d", client.index, GetEntityName(player), GetBot(player)->m_personality == PERSONALITY_RUSHER ? "rusher" : GetBot(player)->m_personality == PERSONALITY_NORMAL ? "normal" : "careful", GetTeam(player) != 0 ? "CT" : "T", GetBot(player)->m_skill, static_cast <int> (player->v.frags));
	}
}

// this function returns number of rbot's playing on the server
int BotControl::GetBotsNum(void)
{
	int count = 0;
	for (const auto& bot : m_bots)
	{
		if (bot != nullptr)
			count++;
	}

	return count;
}

// this function returns number of humans playing on the server
int BotControl::GetHumansNum()
{
	int count = 0;
	for (const auto& client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (m_bots[client.index] == nullptr)
			count++;
	}

	return count;
}

// this function returns bot with highest frag
Bot* BotControl::GetHighestSkillBot(int team)
{
	Bot* highFragBot = nullptr;

	int bestIndex = 0;
	int bestSkill = -1;

	// search bots in this team
	for (const auto& bot : m_bots)
	{
		if (highFragBot != nullptr && highFragBot->m_team == team)
		{
			if (highFragBot->m_skill > bestSkill)
			{
				bestIndex = bot->m_index;
				bestSkill = highFragBot->m_skill;
			}
		}
	}

	return GetBot(bestIndex);
}

// this function decides is players on specified team is able to buy primary weapons by calculating players
// that have not enough money to buy primary (with economics), and if this result higher 80%, player is can't
// buy primary weapons.
void BotControl::CheckTeamEconomics(int team)
{
	if (GetGameMode() != MODE_BASE)
	{
		m_economicsGood[team] = true;
		return;
	}

	int numPoorPlayers = 0;
	int numTeamPlayers = 0;

	// start calculating
	for (const auto& client : g_clients)
	{
		if (client.index < 0)
			continue;

		if (m_bots[client.index] != nullptr && m_bots[client.index]->m_team == team)
		{
			if (m_bots[client.index]->m_moneyAmount <= 1500)
				numPoorPlayers++;

			numTeamPlayers++; // update count of team
		}
	}

	m_economicsGood[team] = true;

	if (numTeamPlayers <= 1)
		return;

	// if 80 percent of team have no enough money to purchase primary weapon
	if ((numTeamPlayers * 80) * 0.01 <= numPoorPlayers)
		m_economicsGood[team] = false;

	// winner must buy something!
	if (m_lastWinner == team)
		m_economicsGood[team] = true;
}

const char* Bot::GetName() {
	return STRING(pev->netname);
}

// this function free all bots slots (used on server shutdown)
void BotControl::Free(void)
{
	for (int i = 0; i < 32; i++)
	{
		if (m_bots[i] != nullptr)
		{
			if (rbot_save_bot_names.GetBool())
				m_savedBotNames.Push(STRING(m_bots[i]->GetEntity()->v.netname));

			m_bots[i]->m_stayTime = 0.0f;
			delete m_bots[i];
			m_bots[i] = nullptr;
		}
	}
}

// this function frees one bot selected by index (used on bot disconnect)
void BotControl::Free(int index)
{
	m_bots[index]->m_stayTime = 0.0f;
	delete m_bots[index];
	m_bots[index] = nullptr;
}

// this function controls the bot entity
Bot::Bot(edict_t* bot, int skill, int personality, int team, int member)
{
	char rejectReason[128];
	int clientIndex = ENTINDEX(bot);

	memset(reinterpret_cast <void*> (this), 0, sizeof(*this));

	m_connectTime = engine->GetTime() - engine->RandomFloat(100.0f, 2000.0f);

	pev = &bot->v;

	if (bot->pvPrivateData != nullptr)
		FREE_PRIVATE(bot);

	bot->pvPrivateData = nullptr;
	bot->v.frags = 0;

	// create the player entity by calling MOD's player function
	BotControl::CallGameEntity(&bot->v);

	// set all info buffer keys for this bot
	char* buffer = GET_INFOKEYBUFFER(bot);
	SET_CLIENT_KEYVALUE(clientIndex, buffer, "_vgui_menus", "0");

	m_current_staytime = 0.0f;

	if (g_gameVersion == HALFLIFE)
	{
		char c_topcolor[4], c_bottomcolor[4];
		sprintf(c_topcolor, "%d", engine->RandomInt(1, 254));
		sprintf(c_bottomcolor, "%d", engine->RandomInt(1, 254));
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "topcolor", c_topcolor);
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "bottomcolor", c_bottomcolor);
	}
	else
	{
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "rate", "3500.000000");
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "cl_updaterate", "20");
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "cl_lw", "1");
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "cl_lc", "1");
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "tracker", "0");
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "cl_dlmax", "128");
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "lefthand", "1");
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "friends", "0");
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "dm", "0");
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "ah", "1");
	}

	if (g_gameVersion != CSVER_VERYOLD && !rbot_ping.GetBool())
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "*bot", "1");

	rejectReason[0] = 0; // reset the reject reason template string
	MDLL_ClientConnect(bot, "rbot", FormatBuffer("%d.%d.%d.%d", engine->RandomInt(1, 255), engine->RandomInt(1, 255), engine->RandomInt(1, 255), engine->RandomInt(1, 255)), rejectReason);

	// should be set after client connect
	if (rbot_display_avatar.GetBool() && !g_botManager->m_avatars.IsEmpty())
		SET_CLIENT_KEYVALUE(clientIndex, buffer, "*sid", g_botManager->m_avatars.GetRandomElement());

	if (!IsNullString(rejectReason))
	{
		AddLogEntry(LOG_WARNING, "Server refused '%s' connection (%s)", GetEntityName(bot), rejectReason);
		ServerCommand("kick \"%s\"", GetEntityName(bot)); // kick the bot player if the server refused it
		bot->v.flags |= FL_KILLME;
	}

	MDLL_ClientPutInServer(bot);
	bot->v.flags |= FL_CLIENT;

	// initialize all the variables for this bot...
	m_notStarted = true;  // hasn't joined game yet
	m_difficulty = rbot_difficulty.GetInt(); // set difficulty
	m_basePingLevel = engine->RandomInt(11, 111);

	m_startAction = CMENU_IDLE;
	m_moneyAmount = 0;
	m_logotypeIndex = engine->RandomInt(0, 5);

	// initialize msec value
	m_msecInterval = engine->GetTime();
	m_msecVal = g_pGlobals->frametime * 1000.0f;

	m_isAlive = false;
	m_skill = skill;
	m_weaponBurstMode = BURST_DISABLED;

	m_lastThinkTime = engine->GetTime();
	m_frameInterval = engine->GetTime();
	m_aimInterval = engine->GetTime();

	switch (personality)
	{
	case 1:
		m_personality = PERSONALITY_RUSHER;
		m_baseAgressionLevel = engine->RandomFloat(0.8f, 1.2f);
		m_baseFearLevel = engine->RandomFloat(0.0f, 0.5f);
		break;

	case 2:
		m_personality = PERSONALITY_CAREFUL;
		m_baseAgressionLevel = engine->RandomFloat(0.0f, 0.3f);
		m_baseFearLevel = engine->RandomFloat(0.75f, 1.0f);
		break;

	default:
		m_personality = PERSONALITY_NORMAL;
		m_baseAgressionLevel = engine->RandomFloat(0.4f, 0.8f);
		m_baseFearLevel = engine->RandomFloat(0.4f, 0.8f);
		break;
	}

	memset(&m_ammoInClip, 0, sizeof(m_ammoInClip));
	memset(&m_ammo, 0, sizeof(m_ammo));

	m_currentWeapon = 0; // current weapon is not assigned at start
	m_voicePitch = engine->RandomInt(80, 120); // assign voice pitch

	m_agressionLevel = m_baseAgressionLevel;
	m_fearLevel = m_baseFearLevel;
	m_nextEmotionUpdate = engine->GetTime() + 0.5f;

	// just to be sure
	m_actMessageIndex = 0;
	m_pushMessageIndex = 0;

	// assign team and class
	m_wantedTeam = team;
	m_wantedClass = member;

	NewRound();
}

Bot::~Bot(void)
{
	// SwitchChatterIcon (false); // crash on CTRL+C'ing win32 console hlds
	DeleteSearchNodes();
	ResetTasks();

	char botName[64];
	ITERATE_ARRAY(g_botNames, j)
	{
		sprintf(botName, "[RBOT] %s", (char*)g_botNames[j].name);

		if (strcmp(g_botNames[j].name, GetEntityName(GetEntity())) == 0 || strcmp(botName, GetEntityName(GetEntity())) == 0)
		{
			g_botNames[j].isUsed = false;
			break;
		}
	}
}

// this function initializes a bot after creation & at the start of each round
void Bot::NewRound(void)
{
	int i = 0;

	// delete all allocated path nodes
	DeleteSearchNodes();
	m_itaimstart = engine->GetTime();
	m_aimStopTime = engine->GetTime();
	m_weaponSelectDelay = engine->GetTime();
	m_currentWaypointIndex = -1;
	check_lastWaypointIndex = -1;
	m_currentTravelFlags = 0;
	m_desiredVelocity = nullvec;
	m_destOrigin = nullvec;
	m_prevGoalIndex = -1;
	m_chosenGoalIndex = -1;
	m_myMeshWaypoint = -1;
	m_loosedBombWptIndex = -1;

	last_check_interval = engine->GetTime();

	m_duckDefuse = false;
	m_duckDefuseCheckTime = 0.0f;

	m_prevWptIndex = -1;

	m_navTimeset = engine->GetTime();

	// clear all states & tasks
	m_states = 0;
	ResetTasks();

	m_isVIP = false;
	m_isLeader = false;
	m_hasProgressBar = false;
	m_canChooseAimDirection = true;

	m_timeTeamOrder = 0.0f;
	m_askCheckTime = 0.0f;
	m_minSpeed = 260.0f;
	m_prevSpeed = 0.0f;
	m_prevOrigin = Vector(9999.0f, 9999.0f, 9999.0f);
	m_prevTime = engine->GetTime();
	m_blindRecognizeTime = engine->GetTime();

	m_viewDistance = 4096.0f;
	m_maxViewDistance = 4096.0f;

	m_pickupItem = nullptr;
	m_itemIgnore = nullptr;
	m_itemCheckTime = 0.0f;

	m_breakableEntity = nullptr;
	m_breakable = nullvec;
	m_timeDoorOpen = 0.0f;

	ResetCollideState();
	ResetDoubleJumpState();

	SetEnemy(nullptr);
	SetLastEnemy(nullptr);
	SetMoveTarget(nullptr);
	m_trackingEdict = nullptr;
	m_timeNextTracking = 0.0f;

	m_buttonPushTime = 0.0f;
	m_enemyUpdateTime = 0.0f;
	m_seeEnemyTime = 0.0f;
	m_oldCombatDesire = 0.0f;

	m_avoidEntity = nullptr;
	m_needAvoidEntity = 0;

	m_lastDamageType = -1;
	m_voteMap = 0;
	m_doorOpenAttempt = 0;
	m_aimFlags = 0;

	m_position = nullvec;
	m_campposition = nullvec;

	m_idealReactionTime = g_skillTab[m_skill / 20].minSurpriseTime;
	m_actualReactionTime = g_skillTab[m_skill / 20].minSurpriseTime;

	m_targetEntity = nullptr;
	m_followWaitTime = 0.0f;

	for (i = 0; i < Const_MaxHostages; i++)
		m_hostages[i] = nullptr;

	m_isReloading = false;
	m_reloadState = RSTATE_NONE;

	m_reloadCheckTime = 0.0f;
	m_shootTime = engine->GetTime();
	m_playerTargetTime = engine->GetTime();
	m_firePause = 0.0f;
	m_timeLastFired = 0.0f;

	m_grenadeCheckTime = 0.0f;
	m_isUsingGrenade = false;

	m_blindButton = 0;
	m_blindTime = 0.0f;
	m_jumpTime = 0.0f;
	m_isStuck = false;
	m_jumpFinished = false;

	m_sayTextBuffer.entityIndex = -1;
	m_sayTextBuffer.sayText[0] = 0x0;

	m_damageTime = 0.0f;
	m_zhCampPointIndex = -1;
	m_checkCampPointTime = 0.0f;

	if (!IsAlive(GetEntity())) // if bot died, clear all weapon stuff and force buying again
	{
		memset(&m_ammoInClip, 0, sizeof(m_ammoInClip));
		memset(&m_ammo, 0, sizeof(m_ammo));
		m_currentWeapon = 0;
	}

	m_nextBuyTime = AddTime(engine->RandomFloat(0.6f, 1.2f));
	m_inBombZone = false;

	m_shieldCheckTime = 0.0f;
	m_zoomCheckTime = 0.0f;
	m_strafeSetTime = 0.0f;
	m_combatStrafeDir = 0;
	m_fightStyle = 0;
	m_lastFightStyleCheck = 0.0f;

	m_checkWeaponSwitch = true;
	m_checkKnifeSwitch = true;

	m_radioEntity = nullptr;
	m_radioOrder = 0;
	m_defendedBomb = false;

	m_timeLogoSpray = AddTime(engine->RandomFloat(0.5f, 2.0f));
	m_spawnTime = engine->GetTime();
	pev->button = 0;

	m_timeCamping = 0;
	m_campDirection = 0;
	m_nextCampDirTime = 0;
	m_campButtons = 0;

	m_soundUpdateTime = 0.0f;
	m_heardSoundTime = engine->GetTime() - 8.0f;

	// clear its message queue
	for (i = 0; i < 32; i++)
		m_messageQueue[i] = CMENU_IDLE;

	m_actMessageIndex = 0;
	m_pushMessageIndex = 0;
	
	SetEntityWaypoint(GetEntity(), -2);

	// and put buying into its message queue
	if (g_gameVersion == HALFLIFE)
	{
		m_buyState = 7;
		m_buyingFinished = true;
		m_startAction = CMENU_IDLE;
	}
	else
	{
		m_buyingFinished = false;
		m_buyState = 0;
		PushMessageQueue(CMENU_BUY);
	}

	PushTask(TASK_NORMAL, TASKPRI_NORMAL, -1, 1.0f, true);

	// hear range based on difficulty
	m_maxhearrange = float(m_skill * engine->RandomFloat(7.0f, 15.0f));
	m_moveSpeed = pev->maxspeed;

	m_tempstrafeSpeed = engine->RandomInt(1, 2) == 1 ? pev->maxspeed : -pev->maxspeed;

	m_activeInfectionBomb = false;
}

// this function kills a bot (not just using ClientKill, but like the CSBot does)
// base code courtesy of Lazy (from bots-united forums!)
void Bot::Kill(void)
{
	edict_t* hurtEntity = (*g_engfuncs.pfnCreateNamedEntity) (MAKE_STRING("trigger_hurt"));

	if (FNullEnt(hurtEntity))
		return;

	hurtEntity->v.classname = MAKE_STRING(g_weaponDefs[m_currentWeapon].className);
	hurtEntity->v.dmg_inflictor = GetEntity();
	hurtEntity->v.dmg = 999999.0f;
	hurtEntity->v.dmg_take = 1.0f;
	hurtEntity->v.dmgtime = 2.0f;
	hurtEntity->v.effects |= EF_NODRAW;

	(*g_engfuncs.pfnSetOrigin) (hurtEntity, Vector(-4000, -4000, -4000));

	KeyValueData kv;
	kv.szClassName = const_cast <char*> (g_weaponDefs[m_currentWeapon].className);
	kv.szKeyName = "damagetype";
	kv.szValue = FormatBuffer("%d", (1 << 4));
	kv.fHandled = false;

	MDLL_KeyValue(hurtEntity, &kv);

	MDLL_Spawn(hurtEntity);
	MDLL_Touch(hurtEntity, GetEntity());

	(*g_engfuncs.pfnRemoveEntity) (hurtEntity);
}

void Bot::Kick(void)
{
	auto myName = GetEntityName(GetEntity());
	if (IsNullString(myName))
		return;

	ServerCommand("kick \"%s\"", GetEntityName(GetEntity()));
	CenterPrint("rbot '%s' kicked from the server", GetEntityName(GetEntity()));

	if (g_botManager->GetBotsNum() - 1 < rbot_quota.GetInt())
		rbot_quota.SetInt(g_botManager->GetBotsNum() - 1);

	//if (rbot_save_bot_names.GetBool() && !g_botManager->m_savedBotNames.IsEmpty()) CRASH...
	//	g_botManager->m_savedBotNames.PopNoReturn();
}

// this function handles the selection of teams & class
void Bot::StartGame(void)
{
	if (g_gameVersion == HALFLIFE)
	{
		m_notStarted = false;
		m_startAction = CMENU_IDLE;
		return;
	}

	// handle counter-strike stuff here...
	if (m_startAction == CMENU_TEAM)
	{
		m_startAction = CMENU_IDLE;  // switch back to idle

		const char* the_team = rbot_forceteam.GetString();

		if (the_team == nullptr || the_team[0] == '\0') {
			printf("NULLPTR or INVALID STRING ON m_startAction Bot::StartGame(void) ! \n");

			m_wantedTeam = 5;
		} else {
			if (rbot_forceteam.GetString()[0] == 'C' || rbot_forceteam.GetString()[0] == 'c' ||
			rbot_forceteam.GetString()[0] == '2')
			m_wantedTeam = 2;
		else if (rbot_forceteam.GetString()[0] == 'T' || rbot_forceteam.GetString()[0] == 't' ||
			rbot_forceteam.GetString()[0] == '1') // 1 = T, 2 = CT
			m_wantedTeam = 1;

		if (m_wantedTeam != 1 && m_wantedTeam != 2)
			m_wantedTeam = 5;
		}

		// select the team the bot wishes to join...
		FakeClientCommand(GetEntity(), "menuselect %d", m_wantedTeam);
	}
	else if (m_startAction == CMENU_CLASS)
	{
		m_startAction = CMENU_IDLE;  // switch back to idle

		int maxChoice = g_gameVersion == CSVER_CZERO ? 5 : 4;
		m_wantedClass = engine->RandomInt(1, maxChoice);

		// select the class the bot wishes to use...
		FakeClientCommand(GetEntity(), "menuselect %d", m_wantedClass);

		// bot has now joined the game (doesn't need to be started)
		m_notStarted = false;
	}
}
