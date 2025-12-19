/*!
	@brief Biker delivery [EVENT]
	@author essien
	@date 18.12.2025
*/




/*!

	@define

!*/


#define D_BIKER_MIN						(90 * 60)
#define D_BIKER_MAX						(150 * 60)

#define D_BIKER_TIMEOUT 				(30 * 60)

#define D_BIKER_VEHICLE_ID 				455
#define D_BIKER_MAPICON 				51
#define D_MAX_SPAWN 					8

#define D_BIKER_FRAC_MONEY  			200000
#define D_BIKER_FRAC_MATS 				30000

#define D_BIKER_PLAYER_MONEY 			100000
#define D_BIKER_PLAYER_REP 				1500

#define INVALID_CLUB 					(-1)




/*!

	@ЗАГЛУШКИ К СИСТЕМЕ ЧТОБЫ РАБОТАЛО

!*/

#define INVALID_MAP_ICON 				-1
#define INVALID_CHECKPOINT 				-1
#define function:%1(%2) 				forward %1(%2); public %1(%2)
#define COLOR_LIGHTBLUE					(0xADD8E6FF)

#define STR_WHITE						"{FFFFFF}"
#define STR_LIGHTBLUE					"{ADD8E6}"

#define MAX_BIKERS 11

new l_string[144] = EOS;

enum D_FRACTION_DATA
{
    biker_id,
    biker_name[24]
};

new BikerFractions[MAX_BIKERS][D_FRACTION_DATA] =
{
    { 0,  "n/a" },
    { 1,  "Hells Angels MC" },
    { 2,  "Mongols MC" },
    { 3,  "Pagans MC" },
    { 4,  "Outlaws MC" },
    { 5,  "Sons of Silence MC" },
    { 6,  "Warlocks MC" },
    { 7,  "Highwaymen MC" },
    { 8,  "Bandidos MC" },
    { 9,  "Free Souls MC" },
    { 10, "Vagos MC" }
};


enum P_PLAYER_DATA
{
	fraction
};
new PlayerFraction[MAX_PLAYERS][P_PLAYER_DATA];


bool:IsBikerPlayer(playerid)
{
    if(!PlayerFraction[playerid][fraction]) return false;
    else return true;
}


SendMessageToBiker(const text[])
{
    for(new i; i < MAX_PLAYERS; i++) 
    	if(IsBikerPlayer(i)) SendClientMessage(i, COLOR_LIGHTBLUE, text);
}


SendMessageToOtherBiker(const text[], other)
{
    for(new i; i < MAX_PLAYERS; i++)
    {
    	if(IsBikerPlayer(i))
    	{
    		if(PlayerFraction[i][fraction] == other) continue;
    		SendClientMessage(i, COLOR_LIGHTBLUE, text);
    	}
    } 
}


SendMessageToUsBiker(const text[], us)
{
    for(new i; i < MAX_PLAYERS; i++)
    {
    	if(IsBikerPlayer(i))
    	{
    		if(PlayerFraction[i][fraction] != us) continue;
    		SendClientMessage(i, COLOR_LIGHTBLUE, text);
    	}
    } 
    	 
}


GetVehicleDriverID(vehicleid)
{
    for (new i; i < MAX_PLAYERS; i++)
    {
        if(GetPlayerVehicleID(i) != vehicleid) continue;
        if(GetPlayerState(i) == PLAYER_STATE_DRIVER) return i;
    }

    return INVALID_PLAYER_ID;
}


/*!

	@ПЕРЕМЕННЫЕ

!*/


enum D_BIKER_STATE
{
    D_BIKER_NONE,
    D_BIKER_ACTIVE
};


enum D_BIKER_EVENT
{
    D_BIKER_STATE:states,
    d_spawn,
    carid,
    fraction_owner,
    last_driver,
    start_time,

    timer_icon,
    timer_notify,
    timer_timeout
};
new BikerEvent[D_BIKER_EVENT];


enum D_BIKE_SPAWN_POS
{
    Float:x,
    Float:y,
    Float:z,
    Float:r
};
new DeliveryVehicleSpawn[D_MAX_SPAWN][D_BIKE_SPAWN_POS] =
{
    { 2081.8970,-2006.9373,13.9873,269.4189 },
    { 2080.1389,-2019.8335,13.9837,269.6956 },
    { 2080.1313,-2033.3491,13.9844,270.4236 },
    { 2080.1367,-2046.9878,13.9845,268.8507 },
    { -2449.1304,-120.7151,26.5687,93.3258 },
    { -2447.6011,-125.1134,26.5827,93.3922 },
    { 1681.7417,2318.3223,11.2570,270.1104 },
    { 1680.8486,2358.8562,11.2536,270.8129 }
};


enum D_BIKER_POINT
{
    Float:x,
    Float:y,
    Float:z
};
new DeliveryPoints[MAX_BIKERS][D_BIKER_POINT] =
{
    { 0.0,0.0,0.0 },
    { 701.1597,-446.3981,16.3359 },
    { -1295.5288,2710.1636,50.0625 },
    { -2120.5686,-2496.7698,30.6250 },
    { -300.7595,1319.7745,54.2191 },
    { 1238.4785,180.2144,20.0276 },
    { 658.0650,1692.3461,6.9922 },
    { 34.9405,-2639.2678,40.4264 },
    { -1941.2568,2402.6355,49.4922 },
    { -203.3469,2602.5283,62.7031 },
    { -314.6039,1753.0490,42.7547 }
};


new D_BIKER_EVENT_TIMER; // GLOBAL TIMER
new D_BIKER_ICON[MAX_PLAYERS] = INVALID_MAP_ICON;
new DELIVERY_NOTIFY = 0;
new DELIVERY_CHECKPOINT[MAX_PLAYERS] = INVALID_CHECKPOINT;


enum 
{
	DELIVERY_END_NONE = 0,
    DELIVERY_END_DELIVERED,
    DELIVERY_END_TIMEOUT,
    DELIVERY_END_DESTROYED
};




/*!

	BASE SYSTEM

!*/


bool:IsDeliveryEventActive() return (BikerEvent[states] == D_BIKER_ACTIVE);					// Check start event 			
bool:IsDeliveryVehicle(vehicleid) return (vehicleid == BikerEvent[carid]);				   // Проверка машины

Delivery_OnGameModeInit()
{
	Delivery_Reset_Event();
    Delivery_ScheduleEvent();
}


Delivery_OnPlayerStateChange(playerid, newstate)
{
	if(IsDeliveryEventActive() && IsDeliveryVehicle(GetPlayerVehicleID(playerid))) 
		Delivery_CheckVehDriver(playerid, newstate);
}


Delivery_OnPlayerExitVehicle(playerid, vehicleid)
{
	if(IsDeliveryEventActive() && IsDeliveryVehicle(vehicleid)) 
		Delivery_DisableCheckpoint(playerid);
}


Delivery_OnVehicleDeath(vehicleid)
{
	if(IsDeliveryEventActive() && IsDeliveryVehicle(vehicleid))
		Delivery_Stop_Event(DELIVERY_END_DESTROYED);
}


Delivery_OnEnterCheckpoint(playerid)
{
	if(IsDeliveryEventActive() && IsDeliveryVehicle(GetPlayerVehicleID(playerid))) 
		Delivery_EndCheckpoint(playerid);
}




/*!

	SYSTEM CORE

!*/


function:Delivery_StartEvent()																		// Start event
{
	if(IsDeliveryEventActive()) return false;

	BikerEvent[states] = D_BIKER_ACTIVE;
	BikerEvent[start_time] = gettime();
	BikerEvent[fraction_owner] = INVALID_CLUB;
	BikerEvent[last_driver] = INVALID_PLAYER_ID;
	BikerEvent[d_spawn] = random(D_MAX_SPAWN);

	Delivery_CreateVehicle();
	Delivery_Icon();

	BikerEvent[timer_icon] = SetTimer("Delivery_UpdateIcon", 3000, true);
	BikerEvent[timer_notify] = SetTimer("Delivery_NotifyIdle", 300000, true);
	BikerEvent[timer_timeout] = SetTimer("Delivery_Timeout", D_BIKER_TIMEOUT * 1000, false);

	format(l_string, sizeof l_string, "[Событие] "STR_WHITE"Начато событие: "STR_LIGHTBLUE"'Доставка деталей'"STR_WHITE". Flatbed припаркован в районе "STR_LIGHTBLUE"#%d", BikerEvent[d_spawn]);
	SendMessageToBiker(l_string);
	SendMessageToBiker("[Событие] Заберите и доставьте груз в течение "STR_LIGHTBLUE"30"STR_WHITE" минут. Установлена метка на карте");	

	return true;
}


Delivery_CreateVehicle()																	// create vehicle
{
	new b_spawn = BikerEvent[d_spawn];

	BikerEvent[carid] = CreateVehicle(D_BIKER_VEHICLE_ID, DeliveryVehicleSpawn[b_spawn][x], 
		DeliveryVehicleSpawn[b_spawn][y], DeliveryVehicleSpawn[b_spawn][z], DeliveryVehicleSpawn[b_spawn][r], 0, 0, -1);

	return true;
}


Delivery_Icon() 																			// icon only for bikers
{
	if(!IsDeliveryEventActive()) return false;

	new b_spawn = BikerEvent[d_spawn];

	for(new i; i < MAX_PLAYERS; i++)
	{
		if(!IsBikerPlayer(i)) continue;
		D_BIKER_ICON[i] = SetPlayerMapIcon(i, D_BIKER_ICON[i], DeliveryVehicleSpawn[b_spawn][x], 
		DeliveryVehicleSpawn[b_spawn][y], DeliveryVehicleSpawn[b_spawn][z] + 1.0, D_BIKER_MAPICON, 0, MAPICON_LOCAL);
	}

	return true;
}


Delivery_CheckVehDriver(playerid, newstate)													// Проверка водителя 
{	
	if(newstate != PLAYER_STATE_DRIVER) return true;

	if(!IsBikerPlayer(playerid))
	{
		RemovePlayerFromVehicle(playerid);
		SendClientMessage(playerid, 0xFFFFFFFF, "Доступ к транспорту имеют только байкеры.");
		return false;
	}
	else Delivery_Vehicle_Capture(playerid);

	return true;
}


Delivery_Vehicle_Capture(playerid)											// Проверка на перехват/захват
{
	if(playerid == BikerEvent[last_driver]) return Delivery_SetCheckpoint(playerid);

	new const pname[24];
    GetPlayerName(playerid, pname, sizeof pname); // Заглушка

	if(PlayerFraction[playerid][fraction] != BikerEvent[fraction_owner])
	{
		BikerEvent[fraction_owner] = PlayerFraction[playerid][fraction];
		BikerEvent[last_driver] = playerid;

		SendMessageToOtherBiker("[Событие] "STR_WHITE"Действует событие: "STR_LIGHTBLUE"'Доставка деталей'"STR_WHITE".", BikerEvent[fraction_owner]);
		format(l_string, sizeof l_string, "[Событие] Flatbed перехвачен клубом "STR_LIGHTBLUE"%s"STR_WHITE". Перехватите или уничтожьте транспорт", BikerFractions[BikerEvent[fraction_owner]][biker_name]);
		SendMessageToOtherBiker(l_string, BikerEvent[fraction_owner]);

		SendMessageToUsBiker("[Событие] "STR_WHITE"Действует событие: "STR_LIGHTBLUE"'Доставка деталей'"STR_WHITE".", BikerEvent[fraction_owner]);
		format(l_string, sizeof l_string, "[Событие] %s "STR_WHITE"находится за рулем Flatbed с деталями. Помогите в доставке деталей на склад фракции", pname); 
		SendMessageToUsBiker(l_string, BikerEvent[fraction_owner]);

		Delivery_SetCheckpoint(playerid);

		return true;
	}

	else
	{
		BikerEvent[last_driver] = playerid;

		SendMessageToUsBiker("[Событие] "STR_WHITE"Действует событие: "STR_LIGHTBLUE"'Доставка деталей'"STR_WHITE".", BikerEvent[fraction_owner]);
		format(l_string, sizeof l_string, "[Событие] %s "STR_WHITE"находится за рулем Flatbed с деталями. Помогите в доставке деталей на склад фракции", pname); 
		SendMessageToUsBiker(l_string, BikerEvent[fraction_owner]);

		Delivery_SetCheckpoint(playerid);

    	return true;
	}
}


Delivery_SetCheckpoint(playerid) 																// Чекпоинт водителю
{
	if(DELIVERY_CHECKPOINT[playerid] != INVALID_CHECKPOINT) DisablePlayerCheckpoint(playerid);

	new frac_id = PlayerFraction[playerid][fraction];		// Заглушка заменить на норм

	DELIVERY_CHECKPOINT[playerid] = SetPlayerCheckpoint(playerid, DeliveryPoints[frac_id][x], 
	DeliveryPoints[frac_id][y], DeliveryPoints[frac_id][z], 4.0);
	return true;
}


Delivery_EndCheckpoint(playerid)																// Доставка в чекопинт
{
	if(!IsBikerPlayer(playerid)) return false;
	if(GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return false;

	if(!IsPlayerInRangeOfPoint(playerid, DeliveryPoints[BikerEvent[fraction_owner]][x], 
    	DeliveryPoints[BikerEvent[fraction_owner]][y], DeliveryPoints[BikerEvent[fraction_owner]][z], 4.0)) return false;

	Delivery_DisableCheckpoint(playerid);
    Delivery_Stop_Event(DELIVERY_END_DELIVERED);
	return true;
}


Delivery_Reward(driverid, club)																// REWARD'S (типо должны быть тут)
{
	GivePlayerMoney(driverid, club); // заглушка
	return true;
}


Delivery_Stop_Event(reason)																		// Завершение ивента
{
	switch(reason)
	{
		case DELIVERY_END_DELIVERED:
		{
			new const pname[24];
		    GetPlayerName(BikerEvent[last_driver], pname, sizeof pname); // Заглушка

			format(l_string, sizeof l_string, "[Событие] "STR_WHITE"Завершено событие: "STR_LIGHTBLUE"'Доставка деталей'"STR_WHITE". Груз был доставлен клубом %s", BikerFractions[BikerEvent[fraction_owner]][biker_name]); 
			SendMessageToBiker(l_string);
			SendMessageToUsBiker("[Событие] "STR_WHITE"Ваша фракция получила: 200000$ на банк клуба / 1500 рейтинга / 30000 материалов", BikerEvent[fraction_owner]);
			format(l_string, sizeof l_string, "[Событие] "STR_WHITE"Игрок %s получил за доставку груза: 100000$ / 1500 рейтинга", pname); 
			SendMessageToUsBiker(l_string, BikerEvent[fraction_owner]);

			Delivery_Reward(BikerEvent[last_driver], BikerEvent[fraction_owner]);
		}
		case DELIVERY_END_TIMEOUT: SendMessageToBiker("[Событие] "STR_WHITE"Завершено событие: "STR_LIGHTBLUE"'Доставка деталей'"STR_WHITE". Flatbed не был доставлен");

		case DELIVERY_END_DESTROYED: SendMessageToBiker("[Событие] "STR_WHITE"Завершено событие: "STR_LIGHTBLUE"'Доставка деталей'"STR_WHITE". Flatbed был уничтожен");

		case DELIVERY_END_NONE: return false;
	}

	Delivery_DestroyEvent();
	return true;
}




/*!

	TIMERS

!*/


function:Delivery_ScheduleEvent()										 				// Таймер запуска 90-150 минут между ивентами			
{
	new schedule = (random(D_BIKER_MAX - D_BIKER_MIN) + D_BIKER_MIN) * 1000;
	D_BIKER_EVENT_TIMER = SetTimer("Delivery_StartEvent", schedule, false);
	return true;
}


function:Delivery_UpdateIcon()															// Обновление иконки
{
	if (!IsDeliveryEventActive()) return false;
	
	new Float:pos[3];
	GetVehiclePos(BikerEvent[carid], pos[0], pos[1], pos[2]);

	for(new i; i < MAX_PLAYERS; i++)
	{
		if(!IsBikerPlayer(i)) continue;

		if(D_BIKER_ICON[i] != INVALID_MAP_ICON) SetPlayerMapIcon(i, D_BIKER_ICON[i], pos[0], pos[1], pos[2] + 1.0, D_BIKER_MAPICON, 0, MAPICON_LOCAL);
	}

	return true;
}


function:Delivery_NotifyIdle()														// Если бездействует транспорт 10-10-5 мин
{
	if(!IsDeliveryEventActive()) return false;
	
	new delivery_driver = GetVehicleDriverID(BikerEvent[carid]);

	if(delivery_driver == INVALID_PLAYER_ID)
	{
		DELIVERY_NOTIFY++;

		if(DELIVERY_NOTIFY == 2 || DELIVERY_NOTIFY == 4 || DELIVERY_NOTIFY == 5)
		{
			new Float:delivery_remaining = float(D_BIKER_TIMEOUT - (gettime() - BikerEvent[start_time])) / 60.0;

			format(l_string, sizeof l_string, "[Событие] "STR_WHITE"Действует событие: "STR_LIGHTBLUE"'Доставка деталей'"STR_WHITE". Flatbed припаркован в районе "STR_LIGHTBLUE"#%d", BikerEvent[d_spawn]);
			SendMessageToBiker(l_string);
			format(l_string, sizeof l_string, "[Событие] Заберите и доставьте груз в течение "STR_LIGHTBLUE"%.0f"STR_WHITE" минут. Установлена метка на карте", delivery_remaining);
			SendMessageToBiker(l_string);
		}		
	}
	else DELIVERY_NOTIFY = 0;

	return true;
}


function:Delivery_Timeout()														// Таймер 30 минут истек
{
	if(!IsDeliveryEventActive()) return false;
	
	Delivery_Stop_Event(DELIVERY_END_TIMEOUT);

	return true;
}




/*!

	DESTROY SYSTEM

!*/


Delivery_DestroyEvent()
{
	if(!IsDeliveryEventActive()) return false;

	if(BikerEvent[carid] != INVALID_VEHICLE_ID)
	{
		DestroyVehicle(BikerEvent[carid]);
		BikerEvent[carid] = INVALID_VEHICLE_ID;
	}

	for(new i; i < MAX_PLAYERS; i++)
	{
		if(!IsBikerPlayer(i)) continue;
		RemovePlayerMapIcon(i, D_BIKER_ICON[i]);
		D_BIKER_ICON[i] = INVALID_MAP_ICON;

		if(DELIVERY_CHECKPOINT[i] != INVALID_CHECKPOINT) 
		{
			DisablePlayerCheckpoint(i);
			DELIVERY_CHECKPOINT[i] = INVALID_CHECKPOINT;
		}
	}

	KillTimer(BikerEvent[timer_icon]);
    KillTimer(BikerEvent[timer_notify]);
    KillTimer(BikerEvent[timer_timeout]);

    BikerEvent[timer_icon] = 0;
    BikerEvent[timer_notify] = 0;
    BikerEvent[timer_timeout] = 0;

	Delivery_Reset_Event();
	Delivery_ScheduleEvent();

	return true;
}


Delivery_Reset_Event()
{
    BikerEvent[states] = D_BIKER_NONE;
    BikerEvent[carid] = INVALID_VEHICLE_ID;
    BikerEvent[fraction_owner] = -1;
    BikerEvent[d_spawn] = 0;
    BikerEvent[last_driver] = INVALID_PLAYER_ID;
    BikerEvent[start_time] = 0;

    KillTimer(D_BIKER_EVENT_TIMER);
}


Delivery_DisableCheckpoint(playerid)
{
	if(DELIVERY_CHECKPOINT[playerid] != INVALID_CHECKPOINT) 
	{
		DisablePlayerCheckpoint(playerid);
		DELIVERY_CHECKPOINT[playerid] = INVALID_CHECKPOINT;
	}
	return true;
}






/*!

	FOR TEST !!!!!

!*/

CMD:delivery_test(playerid, params[])   			// /delivery_test 0 - выкл 1 - вкл
{
	if(sscanf(params, "i", params[0])) return true;

	PlayerPlaySound(playerid, 17006, 0.0, 0.0, 0.0);

	if(params[0] == 0)
	{
		Delivery_DestroyEvent();
		SendClientMessage(playerid, -1, "Остановили");
		return true;
	}

	else if(params[0] == 1)
	{
		Delivery_StartEvent();
		SendClientMessage(playerid, -1, "Запустили");
		return true;
	}

	return true;
}

CMD:delivery_frac(playerid, params[])   			// /delivery_frac 0 - без фраки 1-10 мото
{
	if(sscanf(params, "i", params[0])) return true;

	if(params[0] > 10) return true;

	PlayerPlaySound(playerid, 17006, 0.0, 0.0, 0.0);

	PlayerFraction[playerid][fraction] = params[0];
	SendClientMessage(playerid, -1, "Смена фракции");

	return true;
}

CMD:delivery_goto(playerid)
{
	new Float:pos[3];
	GetVehiclePos(BikerEvent[carid], pos[0], pos[1], pos[2]);

	SetPlayerPos(playerid, pos[0], pos[1], pos[2] + 1.0);

	return true;
}