#include <iostream>
#include <WS2tcpip.h>
#include <MSWSock.h>
#pragma comment (lib, "WS2_32.lib")
#pragma comment (lib, "mswsock.lib")
#pragma comment (lib, "lua53.lib")

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include <vector>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <atomic>
#include <chrono>
#include <queue>
#include <fstream>

using namespace std;
using namespace chrono;
/********************************************************/
#include <windows.h>  
#include <sqlext.h>  
#include <string>
#include "protocol.h"
#include "PathFinder.h"
constexpr auto MAX_PACKET_SIZE = 255;
constexpr auto MAX_BUF_SIZE = 1024;
constexpr auto MAX_USER = 10000;

constexpr auto VIEW_RADIUS = 7;

constexpr auto MOVE_COUNT = 3;

enum ENUMOP { OP_RECV, OP_SEND, OP_ACCEPT, OP_RANDOM_MOVE, OP_PLAYER_MOVE, OP_NPC_MOVE_END, OP_PLAYER_HP_RECOVERY };
enum ATKTYPE { PEACE, WAR };
enum MOVETYPE { HOLD, ROAM };
enum MONSTERSTATE { IDLE, BATTLE };
const unsigned char HOLD_IDLE = 0x0;
const unsigned char HOLD_BATTLE = 0x1;
const unsigned char ROAM_IDLE = 0x2;
const unsigned char ROAM_BATTLE = 0x3;
#define UNICODE  

PathFinder* g_PathFinder;

void show_error() {
	printf("error\n");
}
SQLHENV henv;
SQLHDBC hdbc;
SQLHSTMT hstmt = 0;
SQLRETURN retcode;

bool map_data[WORLD_WIDTH][WORLD_HEIGHT];
void read_map()
{
	ifstream fp("map.txt");
	//if (fp == NULL) cout << "널포인터임" << endl;

	string number;
	int i = 0, j = 0;


	while (!fp.eof()) {
		fp >> number;
		if (number == "0xff")
		{
			if (j < WORLD_WIDTH)
			{
				map_data[i][j] = true;
				++j;
			}
			else
			{
				j = 0;
				++i;
				map_data[i][j] = true;
				++j;
			}
		}
		else if (number == " ")
		{

		}
		else
		{
			if (j < WORLD_WIDTH)
			{
				map_data[i][j] = false;
				++j;
			}
			else
			{
				j = 0;
				++i;
				map_data[i][j] = false;
				++j;
			}
		}
	}
}

/************************************************************************
/* HandleDiagnosticRecord : display error/warning information
/*
/* Parameters:
/* hHandle ODBC handle
/* hType Type of handle (SQL_HANDLE_STMT, SQL_HANDLE_ENV, SQL_HANDLE_DBC)
/* RetCode Return code of failing command
/************************************************************************/
void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
	SQLSMALLINT iRec = 0;
	SQLINTEGER iError;
	WCHAR wszMessage[1000];
	WCHAR wszState[SQL_SQLSTATE_SIZE + 1];
	if (RetCode == SQL_INVALID_HANDLE) {
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	}
	while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
		(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS) {
		// Hide data truncated..
		if (wcsncmp(wszState, L"01004", 5)) {
			fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
		}
	}
}

void DB_find_user_data(string name, int user_id);

void DB_update_user_data(string name, short posX, short posY, short exp, short hp, short level);
/********************************************************/

struct event_type {
	int obj_id;
	ENUMOP event_id;
	high_resolution_clock::time_point wakeup_time;
	int target_id;
	int rest_moveCount;

	constexpr bool operator < (const event_type& left) const
	{
		return (wakeup_time > left.wakeup_time);
	}
};

priority_queue<event_type> timer_queue;
mutex timer_lock;

enum C_STATUS { ST_FREE, ST_ALLOC, ST_ACTIVE, ST_SLEEP, ST_DEAD };

struct EXOVER {
	WSAOVERLAPPED	over;
	ENUMOP			op;
	unsigned char			io_buf[MAX_BUF_SIZE];
	union {
		WSABUF			wsabuf;
		SOCKET			c_socket;
		int				p_id;
		int				rest_move;
	};
};

struct CLIENT {
	mutex	m_cl;
	SOCKET	m_s;
	int		m_id;
	EXOVER  m_recv_over;
	int   m_prev_size;
	char  m_packe_buf[MAX_PACKET_SIZE];
	atomic <C_STATUS> m_status;

	short x, y;
	short m_exp;
	short m_hp = 100;
	short m_level = 1;
	char m_name[MAX_ID_LEN + 1];
	unsigned m_move_time;
	high_resolution_clock::time_point m_last_move_time;

	unordered_set <int> view_list;
	lua_State* L;
	mutex	lua_l;

	short respawn = 0;

	MOVETYPE move_type;
	ATKTYPE atk_type;
	MONSTERSTATE state;
	int move_count = -1;
};

CLIENT g_clients[NPC_ID_START + NUM_NPC];
HANDLE g_iocp;
SOCKET l_socket;
Pos StartPosition;

void add_timer(int obj_id, ENUMOP op_type, int duration, int target_id = 0, int rest_moveCount = -1)
{
	timer_lock.lock();
	event_type ev{ obj_id, op_type, high_resolution_clock::now() + milliseconds(duration), target_id ,rest_moveCount };
	timer_queue.push(ev);
	timer_lock.unlock();
}
/**********************************************************************************************************/

struct SECTOR_INFO {
	unordered_set<CLIENT*> m_clients;											//섹터에 있는 클라이언트들
	mutex m_lock;													//섹터마다의 lock
};

SECTOR_INFO g_ObjectListSector[WORLD_WIDTH][WORLD_HEIGHT];			//섹터마다의 object관리

/**********************************************************************************************************/
wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}
/******************************************************/
void set_user_data(int user_id, short x, short y, short exp, short hp, short level)
{
	g_clients[user_id].x = x;
	g_clients[user_id].y = y;
	g_clients[user_id].m_exp = exp;
	g_clients[user_id].m_hp = hp;
	g_clients[user_id].m_level = level;
	if (g_clients[user_id].m_hp != g_clients[user_id].m_level * 100)
		add_timer(user_id, OP_PLAYER_HP_RECOVERY, 5000);
}
/*****************************************************/

bool in_atk_range(int a, int b)
{
	if (abs(g_clients[a].x - g_clients[b].x) > 1) return false;
	if (abs(g_clients[a].y - g_clients[b].y) > 1) return false;
	return true;
}

bool is_player(int id)
{
	return id < NPC_ID_START;
}

bool is_near(int a, int b)
{
	if (abs(g_clients[a].x - g_clients[b].x) > VIEW_RADIUS) return false;
	if (abs(g_clients[a].y - g_clients[b].y) > VIEW_RADIUS) return false;
	return true;
}

unordered_set<int> get_nearVl(int id)
{
	unordered_set<int> viewlist;
	CLIENT& u = g_clients[id];

	int u_viewRangeX_Min = max(u.x - VIEW_RADIUS, 0);
	int u_viewRangeX_Max = min(u.x + VIEW_RADIUS + 1, WORLD_WIDTH);
	int u_viewRangeY_Min = max(u.y - VIEW_RADIUS, 0);
	int u_viewRangeY_Max = min(u.y + VIEW_RADIUS + 1, WORLD_HEIGHT);

	for (int i = u_viewRangeX_Min; i < u_viewRangeX_Max; ++i)
	{
		for (int j = u_viewRangeY_Min; j < u_viewRangeY_Max; ++j)
		{
			for (auto& cl : g_ObjectListSector[i][j].m_clients)
			{
				if (cl == &u)continue;
				viewlist.emplace(cl->m_id);
			}
		}
	}
	return viewlist;
}

void send_packet(int user_id, void* p)
{
	unsigned char* buf = reinterpret_cast<unsigned char*>(p);

	CLIENT& u = g_clients[user_id];

	EXOVER* exover = new EXOVER;
	exover->op = OP_SEND;
	ZeroMemory(&exover->over, sizeof(exover->over));
	exover->wsabuf.buf = (char*)exover->io_buf;
	exover->wsabuf.len = buf[0];
	memcpy(exover->io_buf, buf, buf[0]);
	WSASend(u.m_s, &exover->wsabuf, 1, NULL, 0, &exover->over, NULL);
}

void send_login_ok_packet(int user_id)
{
	sc_packet_login_ok p;
	p.exp = g_clients[user_id].m_exp;
	p.hp = g_clients[user_id].m_hp;
	p.id = user_id;
	p.level = g_clients[user_id].m_level;
	p.size = sizeof(p);
	p.type = S2C_LOGIN_OK;
	p.x = g_clients[user_id].x;
	p.y = g_clients[user_id].y;

	send_packet(user_id, &p);
}

void send_login_fail_packet(int user_id)
{
	sc_packet_login_fail p;
	p.size = sizeof(p);
	p.type = S2C_LOGIN_FAIL;

	send_packet(user_id, &p);
}

void send_enter_packet(int user_id, int o_id)
{
	sc_packet_enter p;
	p.id = o_id;
	p.size = sizeof(p);
	p.type = S2C_ENTER;
	p.x = g_clients[o_id].x;
	p.y = g_clients[o_id].y;
	strcpy_s(p.name, g_clients[o_id].m_name);
	p.o_type = O_HUMAN;

	g_clients[user_id].m_cl.lock();
	g_clients[user_id].view_list.emplace(o_id);
	g_clients[user_id].m_cl.unlock();

	send_packet(user_id, &p);
}

void send_leave_packet(int user_id, int o_id)
{
	sc_packet_leave p;
	p.id = o_id;
	p.size = sizeof(p);
	p.type = S2C_LEAVE;

	g_clients[user_id].m_cl.lock();
	g_clients[user_id].view_list.erase(o_id);
	g_clients[user_id].m_cl.unlock();

	send_packet(user_id, &p);
}

void send_move_packet(int user_id, int mover)
{
	sc_packet_move p;
	p.id = mover;
	p.size = sizeof(p);
	p.type = S2C_MOVE;
	p.x = g_clients[mover].x;
	p.y = g_clients[mover].y;
	p.move_time = g_clients[mover].m_move_time;

	send_packet(user_id, &p);
}

void send_chat_packet(int user_id, int chatter, wchar_t mess[])
{
	sc_packet_chat p;
	p.id = chatter;
	p.size = sizeof(p);
	p.type = S2C_CHAT;
	if (chatter < NPC_ID_START) {
		string name = g_clients[chatter].m_name;
		name += ": ";
		wstring namePmess;
		namePmess.assign(name.begin(), name.end());
		namePmess += mess;

		wcscpy_s(p.mess, namePmess.c_str());
	}
	else
		wcscpy_s(p.mess, mess);
	send_packet(user_id, &p);

}

void send_stat_change_packet(int user_id)
{
	sc_packet_stat_change p;
	p.hp = g_clients[user_id].m_hp;
	p.exp = g_clients[user_id].m_exp;
	p.level = g_clients[user_id].m_level;
	p.size = sizeof(p);
	p.type = S2C_STAT_CHANGE;

	send_packet(user_id, &p);
}

void activate_npc(int id)
{
	if (g_clients[id].m_status == ST_DEAD) return;

	C_STATUS old_state = ST_SLEEP;

	if (true == atomic_compare_exchange_strong(&g_clients[id].m_status, &old_state, ST_ACTIVE))
		add_timer(id, OP_RANDOM_MOVE, 1000);
}

void do_move(int user_id, int direction)
{
	CLIENT& u = g_clients[user_id];
	int x = u.x;
	int y = u.y;

	switch (direction) {
	case D_UP: if (y > 0) y--; break;
	case D_DOWN: if (y < (WORLD_HEIGHT - 1)) y++; break;
	case D_LEFT: if (x > 0) x--; break;
	case D_RIGHT: if (x < (WORLD_WIDTH - 1)) x++; break;
	default:
		cout << "Unknown Direction from Client move packet!\n";
		DebugBreak();
		exit(-1);
	}
	if (map_data[y][x]) {

		/*****************클라이언트가 이동한 후 클라 정보 추가*******************/
		g_ObjectListSector[x][y].m_lock.lock();

		g_ObjectListSector[u.x][u.y].m_clients.erase(&u);
		g_ObjectListSector[x][y].m_clients.emplace(&u);

		g_ObjectListSector[x][y].m_lock.unlock();
		/************************************************************************ */
		u.x = x;
		u.y = y;


		g_clients[user_id].m_cl.lock();
		unordered_set <int> old_vl = g_clients[user_id].view_list;
		g_clients[user_id].m_cl.unlock();
		unordered_set<int> new_vl;

		for (auto& vl_id : get_nearVl(user_id))
		{
			CLIENT& cl = g_clients[vl_id];
			if (ST_SLEEP == cl.m_status) activate_npc(vl_id);
			if (ST_ACTIVE != cl.m_status) continue;
			if (ST_DEAD != cl.m_status)
			{
				activate_npc(cl.m_id);
				if (cl.atk_type == WAR)
				{
					if (cl.view_list.size() == 0)
						cl.view_list.emplace(user_id);
					cl.state = BATTLE;
				}
			}
			if (vl_id == user_id) continue;
			new_vl.insert(vl_id);
		}
		send_move_packet(user_id, user_id);

		for (auto np : new_vl) {
			if (0 == old_vl.count(np)) {		// Oject가 새로 시야에 들어왔을 때
				send_enter_packet(user_id, np);
				if (false == is_player(np))
					continue;
				g_clients[np].m_cl.lock();
				if (0 == g_clients[np].view_list.count(user_id)) {
					g_clients[np].m_cl.unlock();
					send_enter_packet(np, user_id);
				}
				else {
					g_clients[np].m_cl.unlock();
					send_move_packet(np, user_id);
				}

			}
			else {							// 계속 시야에 존재하고 있을 때
				if (false == is_player(np)) continue;
				g_clients[np].m_cl.lock();
				if (0 != g_clients[np].view_list.count(user_id)) {
					g_clients[np].m_cl.unlock();
					send_move_packet(np, user_id);
				}
				else {
					g_clients[np].m_cl.unlock();
					send_enter_packet(np, user_id);
				}
			}
		}

		for (auto old_p : old_vl) {			// Object가 시야에서 벗어났을 때
			if (0 == new_vl.count(old_p)) {
				send_leave_packet(user_id, old_p);
				if (false == is_player(old_p)) continue;
				g_clients[old_p].m_cl.lock();
				if (0 != g_clients[old_p].view_list.count(user_id)) {
					g_clients[old_p].m_cl.unlock();
					send_leave_packet(old_p, user_id);
				}
				else {
					g_clients[old_p].m_cl.unlock();
				}
			}
		}

	}
}

void random_move_npc(int id)
{
	int x = g_clients[id].x;
	int y = g_clients[id].y;
	unordered_set<int> old_vl = get_nearVl(id);
	switch (rand() % 4) {
	case 0: if (x < (WORLD_WIDTH - 1)) x++; break;
	case 1: if (x > 0) x--; break;
	case 2: if (y < (WORLD_HEIGHT - 1)) y++; break;
	case 3: if (y > 0) y--; break;
	}
	if (map_data[y][x]) {
		g_clients[id].x = x;
		g_clients[id].y = y;
		unordered_set<int> new_vl = get_nearVl(id);

		for (auto& vl_id : new_vl)
		{
			if (false == is_player(vl_id))
				continue;
			if (ST_ACTIVE != g_clients[vl_id].m_status) continue;
			g_clients[vl_id].m_cl.lock();
			if (0 != g_clients[vl_id].view_list.count(id)) {
				g_clients[vl_id].m_cl.unlock();
				send_move_packet(vl_id, id);
			}
			else {
				g_clients[vl_id].m_cl.unlock();
				send_enter_packet(vl_id, id);
			}
		}
		for (auto& vl_id : old_vl)
		{
			if (0 != new_vl.count(vl_id)) continue;

			g_clients[vl_id].m_cl.lock();
			if (0 != g_clients[vl_id].view_list.count(id)) {
				g_clients[vl_id].m_cl.unlock();
				send_leave_packet(vl_id, id);
			}
			else
				g_clients[vl_id].m_cl.unlock();
		}

		g_clients[id].m_cl.lock();

		if (g_clients[id].move_count != -1)
		{
			g_clients[id].move_count++;
		}

		g_clients[id].m_cl.unlock();
	}
}

/*****************************************************************/
void player_level_up(int player_id)
{
	CLIENT& player = g_clients[player_id];

	player.m_cl.lock();
	player.m_exp -= pow(2, player.m_level - 1) * 100;
	player.m_level++;
	player.m_hp = player.m_level * 100;
	player.m_cl.unlock();

	DB_update_user_data(player.m_name, player.x, player.y, player.m_exp, player.m_hp, player.m_level);
	wstring s = s2ws(to_string(player.m_level) + " 레벨업");
	send_chat_packet(player_id, player_id, (wchar_t*)s.c_str());
}

void on_npc_dead(int id)
{
	g_clients[id].m_cl.lock();
	g_clients[id].m_status = ST_DEAD;
	g_clients[id].m_level += 1;
	g_clients[id].m_exp += 5; 
	g_clients[id].m_cl.unlock();
	/*************클라이언트가 나갈때 섹터에서 빼줘야 하지 않을가?************/
	g_ObjectListSector[g_clients[id].x][g_clients[id].y].m_lock.lock();

	g_ObjectListSector[g_clients[id].x][g_clients[id].y].m_clients.erase(&g_clients[id]);

	g_ObjectListSector[g_clients[id].x][g_clients[id].y].m_lock.unlock();
	/*************************************************************************/
	for (int i = 0; i < NPC_ID_START; ++i)
	{
		if (g_clients[i].view_list.count(id) != 0)
		{
			send_leave_packet(i, id);
		}
	}	

}

// 플레이어의 공격
void attack(int player_id, int monster_id)
{
	CLIENT& player = g_clients[player_id];
	CLIENT& monster = g_clients[monster_id];
	if (monster.m_status == ST_DEAD) return;
	if (true == is_player(monster_id))return;
	if (monster.view_list.size() == 0)
		monster.view_list.emplace(player_id);
	string message = player.m_name;

	monster.state = BATTLE;
	monster.m_hp -= player.m_level * 5;
	string a = "에게 ";
	wstring s = s2ws(monster.m_name + a + to_string(player.m_level * 5) + "의 데미지를 줌");
	send_chat_packet(player_id, monster_id, (wchar_t*)s.c_str());

	if (monster.m_hp <= 0)
	{
		player.m_exp += monster.m_exp;
		string a = " 을 잡아 ";
		wstring s = s2ws(monster.m_name + a + to_string(monster.m_exp) + " 경험치 얻음");
		send_chat_packet(player_id, monster_id, (wchar_t*)s.c_str());

		if (player.m_exp > pow(2, player.m_level - 1) * 100)
		{
			player_level_up(player_id);

		}

		send_stat_change_packet(player_id);
		on_npc_dead(monster_id);
	}
}
void on_player_dead(int player_id)
{
	CLIENT& player = g_clients[player_id];

	wstring s = s2ws("사망");
	player.m_cl.lock();
	player.m_exp = player.m_exp / 2;
	player.m_hp = player.m_level * 100;
	// 시작위치로
	player.x = StartPosition.x;
	player.y = StartPosition.y;
	player.m_cl.unlock();

	DB_update_user_data(player.m_name, player.x, player.y, player.m_exp, player.m_hp, player.m_level);

	for (auto& i : player.view_list) {
		send_leave_packet(i, player_id);
	}

	for (auto& i : get_nearVl(player_id)) {
		if (ST_SLEEP == g_clients[i].m_status)
			activate_npc(i);
		if (ST_ACTIVE == g_clients[i].m_status) {
			send_enter_packet(player_id, i);
			if (true == is_player(i))
				send_enter_packet(i, player_id);
		}
	}
	for (int i = NPC_ID_START; i < NPC_ID_START + NUM_NPC; ++i)
	{
		if (g_clients[i].view_list.count(player_id) != 0) {
			g_clients[i].view_list.erase(player_id);
		}
	}
	send_move_packet(player_id, player_id);
}

// 몬스터의 공격
void attack_npc(int monster_id, int player_id)
{
	CLIENT& player = g_clients[player_id];
	CLIENT& monster = g_clients[monster_id];

	if (player.m_hp == player.m_level * 100)
		add_timer(player_id, OP_PLAYER_HP_RECOVERY, 5000);
	player.m_cl.lock();
	player.m_hp -= monster.m_level * 5;
	player.m_cl.unlock();
	string a = "가 ";
	wstring s = s2ws(monster.m_name + a + to_string(monster.m_level * 5) + "의 데미지를 줌");
	send_chat_packet(player_id, monster_id, (wchar_t*)s.c_str());

	if (player.m_hp <= 0)
	{
		on_player_dead(player_id);
	}

	send_stat_change_packet(player_id);

}

void follow(int chaser, int chased)
{
	Pos chaser_pos{ g_clients[chaser].x, g_clients[chaser].y };
	Pos chased_pos{ g_clients[chased].x, g_clients[chased].y };
	unordered_set<int> old_vl = get_nearVl(chaser);
	/*****************클라이언트가 이동한 후 클라 정보 추가*******************/
	g_ObjectListSector[chaser_pos.x][chaser_pos.y].m_lock.lock();

	g_ObjectListSector[chaser_pos.x][chaser_pos.y].m_clients.erase(&g_clients[chaser]);

	g_ObjectListSector[chaser_pos.x][chaser_pos.y].m_lock.unlock();
	/************************************************************************ */
	if (g_PathFinder->is_needAstar(chaser_pos, chased_pos)) {
		cout << chaser_pos.x << ", " << chaser_pos.y << endl;
		cout << chased_pos.x << ", " << chased_pos.y << endl;
		Pos temp = *(g_PathFinder->execute_Astar(chaser_pos, chased_pos)).begin();
		
		g_clients[chaser].m_cl.lock();
		g_clients[chaser].x = temp.x;
		g_clients[chaser].y = temp.y;
		g_clients[chaser].m_cl.unlock();
		
	}
	else
	{
		Pos curr_pos{ g_clients[chaser].x,g_clients[chaser].y };
		Pos dest_pos{ g_clients[chased].x,g_clients[chased].y };

		if (curr_pos.x != dest_pos.x) {
			curr_pos.x += clamp(dest_pos.x - curr_pos.x, -1, 1);
		}
		if (curr_pos.y != dest_pos.y) {
			curr_pos.y += clamp(dest_pos.y - curr_pos.y, -1, 1);
		}
		g_clients[chaser].m_cl.lock();
		g_clients[chaser].x = curr_pos.x;
		g_clients[chaser].y = curr_pos.y;
		g_clients[chaser].m_cl.unlock();
	}
	/*****************클라이언트가 이동한 후 클라 정보 추가*******************/
	g_ObjectListSector[g_clients[chaser].x][g_clients[chaser].y].m_lock.lock();

	g_ObjectListSector[g_clients[chaser].x][g_clients[chaser].y].m_clients.emplace(&g_clients[chaser]);

	g_ObjectListSector[g_clients[chaser].x][g_clients[chaser].y].m_lock.unlock();
	/************************************************************************ */
	unordered_set<int> new_vl = get_nearVl(chaser);

	for (auto& vl_id : new_vl)
	{
		if (false == is_player(vl_id))
			continue;
		if (ST_ACTIVE != g_clients[vl_id].m_status) continue;
		g_clients[vl_id].m_cl.lock();
		if (0 != g_clients[vl_id].view_list.count(chaser)) {
			g_clients[vl_id].m_cl.unlock();
			send_move_packet(vl_id, chaser);
		}
		else {
			g_clients[vl_id].m_cl.unlock();
			send_enter_packet(vl_id, chaser);
		}
	}
	for (auto& vl_id : old_vl)
	{
		if (0 != new_vl.count(vl_id)) continue;

		g_clients[vl_id].m_cl.lock();
		if (0 != g_clients[vl_id].view_list.count(chaser)) {
			g_clients[vl_id].m_cl.unlock();
			send_leave_packet(vl_id, chaser);
		}
		else
			g_clients[vl_id].m_cl.unlock();
	}
}

void spawn_npc(int id)
{
	g_clients[id].m_cl.lock();
	g_clients[id].m_status = ST_ACTIVE;
	g_clients[id].m_hp = 100+ g_clients[id].m_level*5;
	g_clients[id].m_cl.unlock();
	/*************클라이언트가 나갈때 섹터에서 빼줘야 하지 않을가?************/
	g_ObjectListSector[g_clients[id].x][g_clients[id].y].m_lock.lock();

	g_ObjectListSector[g_clients[id].x][g_clients[id].y].m_clients.emplace(&g_clients[id]);

	g_ObjectListSector[g_clients[id].x][g_clients[id].y].m_lock.unlock();
	/*************************************************************************/

	for (int i = 0; i < NPC_ID_START; ++i)
	{
		if (is_near(i, id) == true)
		{
			g_clients[i].view_list.emplace(id);
			send_enter_packet(i, id);
		}
	}
}

void behavior_npc(int id)
{
	CLIENT& monster = g_clients[id];
	if (monster.m_status == ST_DEAD)
	{
		monster.respawn++;
		if (monster.respawn > 30)
		{
			spawn_npc(id);
			monster.respawn = 0;
			if (monster.move_type == ROAM)monster.m_status = ST_ACTIVE;
			else monster.m_status = ST_SLEEP;
			monster.view_list.clear();
		}
		return;
	}

	if (monster.move_type == HOLD)
	{
		if (monster.state == IDLE)
		{
			return;
		}
		else if (monster.state == BATTLE)
		{
			if (0 != monster.view_list.size())
				if (in_atk_range(*monster.view_list.begin(), id))
					attack_npc(id, *monster.view_list.begin());
				else
					follow(id, *monster.view_list.begin());
		}
	}
	if (monster.move_type == ROAM)
	{
		if (monster.state == IDLE)
		{
			random_move_npc(id);
		}
		else if (monster.state == BATTLE)
		{
			if (0 != monster.view_list.size())
				if (in_atk_range(*monster.view_list.begin(), id))
					attack_npc(id, *monster.view_list.begin());
				else
					follow(id, *monster.view_list.begin());
		}
	}
}
void process_atk_packet(int user_id)
{
	CLIENT& u = g_clients[user_id];
	unordered_set<int> vl = u.view_list;
	for (int id : vl)
	{
		CLIENT& cl = g_clients[id];
		if (u.x == cl.x && u.y == cl.y + 1)
			attack(user_id, id);
		if (u.x == cl.x && u.y == cl.y - 1)
			attack(user_id, id);
		if (u.x == cl.x + 1 && u.y == cl.y)
			attack(user_id, id);
		if (u.x == cl.x - 1 && u.y == cl.y)
			attack(user_id, id);
	}
}
/*****************************************************************/
void enter_game(int user_id, char name[])
{
	g_clients[user_id].m_cl.lock();
	strcpy_s(g_clients[user_id].m_name, name);
	g_clients[user_id].m_name[MAX_ID_LEN] = NULL;
	send_login_ok_packet(user_id);
	g_clients[user_id].m_status = ST_ACTIVE;
	g_clients[user_id].m_cl.unlock();

	for (auto& i : get_nearVl(user_id)) {
		if (ST_SLEEP == g_clients[i].m_status)
			activate_npc(i);
		if (ST_ACTIVE == g_clients[i].m_status) {
			send_enter_packet(user_id, i);
			if (true == is_player(i))
				send_enter_packet(i, user_id);
		}
	}

}

void process_packet(int user_id, char* buf)
{
	switch (buf[1]) {
	case C2S_LOGIN: {
		cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(buf);
		DB_find_user_data(packet->name, user_id);
		enter_game(user_id, packet->name);
	}
				  break;
	case C2S_MOVE: {
		cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(buf);
		g_clients[user_id].m_move_time = packet->move_time;
		do_move(user_id, packet->direction);
	}
				 break;
	case C2S_ATTACK:
	{
		cs_packet_attack* packet = reinterpret_cast<cs_packet_attack*>(buf);
		process_atk_packet(user_id);
	}
	break;
	case C2S_CHAT: {
		cs_packet_chat* packet = reinterpret_cast<cs_packet_chat*>(buf);
		
		for (auto& id : get_nearVl(user_id))
		{
			if (false == is_player(id))continue;
			send_chat_packet(id, user_id, packet->message);
		}
		send_chat_packet(user_id, user_id, packet->message);
	}
				 break;
	default:
		cout << "Unknown Packet Type Error!\n";
		DebugBreak();
		exit(-1);
	}
}

void initialize_clients()
{
	for (int i = 0; i < MAX_USER; ++i) {
		g_clients[i].m_id = i;

		g_clients[i].m_status = ST_FREE;
	}
}

void disconnect(int user_id)
{
	/*************클라이언트가 나갈때 섹터에서 빼줘야 하지 않을가?************/
	g_ObjectListSector[g_clients[user_id].x][g_clients[user_id].y].m_lock.lock();

	g_ObjectListSector[g_clients[user_id].x][g_clients[user_id].y].m_clients.erase(&g_clients[user_id]);

	g_ObjectListSector[g_clients[user_id].x][g_clients[user_id].y].m_lock.unlock();
	/*************************************************************************/
	send_leave_packet(user_id, user_id);

	g_clients[user_id].m_cl.lock();
	g_clients[user_id].m_status = ST_ALLOC;

	closesocket(g_clients[user_id].m_s);
	for (int i = 0; i < NPC_ID_START; ++i) {
		CLIENT& cl = g_clients[i];
		if (user_id == cl.m_id) continue;
		//cl.m_cl.lock();
		if (ST_ACTIVE == cl.m_status)
			send_leave_packet(cl.m_id, user_id);
		//cl.m_cl.unlock();
	}
	g_clients[user_id].m_status = ST_FREE;
	g_clients[user_id].m_cl.unlock();
}

void recv_packet_construct(int user_id, int io_byte)
{
	CLIENT& cu = g_clients[user_id];
	EXOVER& r_o = cu.m_recv_over;

	int rest_byte = io_byte;
	unsigned char* p = r_o.io_buf;
	int packet_size = 0;
	if (0 != cu.m_prev_size) packet_size = cu.m_packe_buf[0];
	while (rest_byte > 0) {
		if (0 == packet_size) packet_size = *p;
		if (packet_size <= rest_byte + cu.m_prev_size) {
			memcpy(cu.m_packe_buf + cu.m_prev_size, p, packet_size - cu.m_prev_size);
			p += packet_size - cu.m_prev_size;
			rest_byte -= packet_size - cu.m_prev_size;
			packet_size = 0;
			process_packet(user_id, cu.m_packe_buf);
			cu.m_prev_size = 0;
		}
		else {
			memcpy(cu.m_packe_buf + cu.m_prev_size, p, rest_byte);
			cu.m_prev_size += rest_byte;
			rest_byte = 0;
			p += rest_byte;
		}
	}
}

void worker_thread()
{
	while (true) {
		DWORD io_byte;
		ULONG_PTR key;
		WSAOVERLAPPED* over;
		GetQueuedCompletionStatus(g_iocp, &io_byte, &key, &over, INFINITE);

		EXOVER* exover = reinterpret_cast<EXOVER*>(over);
		int user_id = static_cast<int>(key);
		CLIENT& cl = g_clients[user_id];

		switch (exover->op) {
		case OP_RECV:
			if (0 == io_byte) {
				DB_update_user_data(cl.m_name, cl.x, cl.y, cl.m_exp, cl.m_hp, cl.m_level);
				disconnect(user_id);
			}
			else {
				recv_packet_construct(user_id, io_byte);
				ZeroMemory(&cl.m_recv_over.over, sizeof(cl.m_recv_over.over));
				DWORD flags = 0;
				WSARecv(cl.m_s, &cl.m_recv_over.wsabuf, 1, NULL, &flags, &cl.m_recv_over.over, NULL);
			}
			break;
		case OP_SEND:
			if (0 == io_byte) {
				DB_update_user_data(cl.m_name, cl.x, cl.y, cl.m_exp, cl.m_hp, cl.m_level);
				disconnect(user_id);
			}
			delete exover;
			break;
		case OP_ACCEPT: {
			int user_id = -1;
			for (int i = 0; i < MAX_USER; ++i) {
				lock_guard<mutex> gl{ g_clients[i].m_cl };
				if (ST_FREE == g_clients[i].m_status) {
					g_clients[i].m_status = ST_ALLOC;
					user_id = i;
					break;
				}
			}

			SOCKET c_socket = exover->c_socket;
			if (-1 == user_id)
				closesocket(c_socket);
			else {
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(c_socket), g_iocp, user_id, 0);
				CLIENT& nc = g_clients[user_id];
				nc.m_prev_size = 0;
				nc.m_recv_over.op = OP_RECV;
				ZeroMemory(&nc.m_recv_over.over, sizeof(nc.m_recv_over.over));
				nc.m_recv_over.wsabuf.buf = (char*)nc.m_recv_over.io_buf;
				nc.m_recv_over.wsabuf.len = MAX_BUF_SIZE;
				nc.m_s = c_socket;
				nc.view_list.clear();
				DWORD flags = 0;
				/*************클라이언트를 넣을때 섹터에 넣어줘야하지 않을가?************/
				g_ObjectListSector[nc.x][nc.y].m_lock.lock();

				g_ObjectListSector[nc.x][nc.y].m_clients.emplace(&nc);

				g_ObjectListSector[nc.x][nc.y].m_lock.unlock();
				/*************************************************************************/
				WSARecv(c_socket, &nc.m_recv_over.wsabuf, 1, NULL, &flags, &nc.m_recv_over.over, NULL);
			}
			c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			exover->c_socket = c_socket;
			ZeroMemory(&exover->over, sizeof(exover->over));
			AcceptEx(l_socket, c_socket, exover->io_buf, NULL, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, NULL, &exover->over);
		}
					  break;
		case OP_RANDOM_MOVE: {
			behavior_npc(user_id);
			/*bool keep_alive = false;
			if (g_clients[user_id].move_type == HOLD) {
				for (auto& i : get_nearVl(user_id)) {
					if (i >= NPC_ID_START) continue;
					if (ST_ACTIVE == g_clients[i].m_status) {
						keep_alive = true;
						break;
					}
				}
				if (true == keep_alive)add_timer(user_id, OP_RANDOM_MOVE, 1000);
				else g_clients[user_id].m_status = ST_SLEEP;
			}
			else
			{*/
				add_timer(user_id, OP_RANDOM_MOVE, 1000);
			//}
			delete exover;
		}
						   break;
		case OP_PLAYER_HP_RECOVERY: {
			g_clients[user_id].m_cl.lock();
			g_clients[user_id].m_hp = min(g_clients[user_id].m_level * 100,
				g_clients[user_id].m_hp + g_clients[user_id].m_level * 10);
			g_clients[user_id].m_cl.unlock();
			send_stat_change_packet(user_id);
			if (g_clients[user_id].m_hp != g_clients[user_id].m_level * 100)
				add_timer(user_id, OP_PLAYER_HP_RECOVERY, 5000);
		}break;
		case OP_PLAYER_MOVE: {
			g_clients[user_id].lua_l.lock();
			lua_State* L = g_clients[user_id].L;
			lua_getglobal(L, "event_player_move");
			lua_pushnumber(L, exover->p_id);
			int error = lua_pcall(L, 1, 0, 0);
			if (error) cout << lua_tostring(L, -1);
			//lua_pop(L, 1);
			g_clients[user_id].lua_l.unlock();

			delete exover;
		}
						   break;
		case OP_NPC_MOVE_END: {
			g_clients[user_id].lua_l.lock();
			lua_State* L = g_clients[user_id].L;
			lua_getglobal(L, "event_npc_move_end");
			lua_pushnumber(L, exover->p_id);
			int error = lua_pcall(L, 1, 0, 0);
			if (error) cout << lua_tostring(L, -1);
			//lua_pop(L, 1);
			g_clients[user_id].lua_l.unlock();
			g_clients[user_id].m_status = ST_SLEEP;
			delete exover;
		}
							break;
		default:
			cout << "Unknown Operation in worker_thread!!!\n";
			while (true);
		}
	}
}

int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	wchar_t* mess = (wchar_t*)lua_tostring(L, -1);

	send_chat_packet(user_id, my_id, mess);
	lua_pop(L, 3);
	return 0;
}

int API_get_x(lua_State* L)
{
	int obj_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = g_clients[obj_id].x;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int obj_id = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = g_clients[obj_id].y;
	lua_pushnumber(L, y);
	return 1;
}

void init_npc()
{
	ifstream fp("monster_position.txt");
	int i = NPC_ID_START;
	string number;
	while (!fp.eof()) {
		g_clients[i].m_s = 0;
		g_clients[i].m_id = i;

		fp >> number;
		g_clients[i].x = atoi(number.c_str());
		fp >> number;
		g_clients[i].y = atoi(number.c_str());

		g_clients[i].state = IDLE;
		int level = rand() % 5+1;
		g_clients[i].m_level = level;
		int type = (rand() % 4);

		fp >> number;
		cout << g_clients[i].x << " " << g_clients[i].y << " " << number << endl;
		switch (atoi(number.c_str()))
		{
		case 0:
			g_clients[i].atk_type = PEACE;
			g_clients[i].move_type = HOLD;
			sprintf_s(g_clients[i].m_name, "Green");
			g_clients[i].m_exp = (level + 1) * 5;
			g_clients[i].m_status = ST_SLEEP;
			break;
		case 1:
			g_clients[i].atk_type = PEACE;
			g_clients[i].move_type = ROAM;
			sprintf_s(g_clients[i].m_name, "Yellow");
			g_clients[i].m_exp = (level + 1) * 10;
			g_clients[i].m_status = ST_ACTIVE;
			add_timer(i, OP_RANDOM_MOVE, 1000);
			break;
		case 2:
			g_clients[i].atk_type = WAR;
			g_clients[i].move_type = HOLD;
			sprintf_s(g_clients[i].m_name, "Purple");
			g_clients[i].m_exp = (level + 1) * 10;
			g_clients[i].m_status = ST_SLEEP;
			break;
		case 3:
			g_clients[i].atk_type = WAR;
			g_clients[i].move_type = ROAM;
			sprintf_s(g_clients[i].m_name, "Blue");
			g_clients[i].m_exp = (level + 1) * 20;
			g_clients[i].m_status = ST_ACTIVE;
			add_timer(i, OP_RANDOM_MOVE, 1000);
			break;
		}


		lua_State* L = g_clients[i].L = luaL_newstate();
		luaL_openlibs(L);
		luaL_loadfile(L, "NPC.LUA");
		int error = lua_pcall(L, 0, 0, 0);
		if (error) cout << lua_tostring(L, -1);
		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		error = lua_pcall(L, 1, 0, 0);
		if (error) cout << lua_tostring(L, -1);
		/*************클라이언트를 넣을때 섹터에 넣어줘야하지 않을가?************/
		g_ObjectListSector[g_clients[i].x][g_clients[i].y].m_lock.lock();

		g_ObjectListSector[g_clients[i].x][g_clients[i].y].m_clients.emplace(&g_clients[i]);

		g_ObjectListSector[g_clients[i].x][g_clients[i].y].m_lock.unlock();
		/*************************************************************************/

		lua_register(L, "API_send_message", API_SendMessage);
		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
		i++;
	}
	//for (int i = NPC_ID_START; i < NPC_ID_START + NUM_NPC; ++i) {

	//	g_clients[i].m_s = 0;
	//	g_clients[i].m_id = i;

	//	g_clients[i].x = rand() % WORLD_WIDTH;
	//	g_clients[i].y = rand() % WORLD_HEIGHT;
	//	while (!map_data[g_clients[i].y][g_clients[i].x])
	//	{
	//		g_clients[i].x = rand() % WORLD_WIDTH;
	//		g_clients[i].y = rand() % WORLD_HEIGHT;
	//	}

	//	g_clients[i].state = IDLE;
	//	int level = rand() % 5;
	//	g_clients[i].m_level = level;
	//	int type = (rand() % 4);

	//	printf("%d ", g_clients[i].x);
	//	printf("%d\n", g_clients[i].y);
	//	fp << g_clients[i].x << " " << g_clients[i].y << " " << type << endl;

	//	switch (type)
	//	{
	//	case 0:
	//		g_clients[i].atk_type = PEACE;
	//		g_clients[i].move_type = HOLD;
	//		sprintf_s(g_clients[i].m_name, "Green");
	//		g_clients[i].m_exp = (level + 1) * 5;
	//		g_clients[i].m_status = ST_SLEEP;
	//		break;
	//	case 1:
	//		g_clients[i].atk_type = PEACE;
	//		g_clients[i].move_type = ROAM;
	//		sprintf_s(g_clients[i].m_name, "Yellow");
	//		g_clients[i].m_exp = (level + 1) * 10;
	//		g_clients[i].m_status = ST_ACTIVE;
	//		add_timer(i, OP_RANDOM_MOVE, 1000);
	//		break;
	//	case 2:
	//		g_clients[i].atk_type = WAR;
	//		g_clients[i].move_count = HOLD;
	//		sprintf_s(g_clients[i].m_name, "Purple");
	//		g_clients[i].m_exp = (level + 1) * 10;
	//		g_clients[i].m_status = ST_SLEEP;
	//		break;
	//	case 3:
	//		g_clients[i].atk_type = WAR;
	//		g_clients[i].move_count = ROAM;
	//		sprintf_s(g_clients[i].m_name, "Blue");
	//		g_clients[i].m_exp = (level + 1) * 20;
	//		g_clients[i].m_status = ST_ACTIVE;
	//		add_timer(i, OP_RANDOM_MOVE, 1000);
	//		break;
	//	}


	//	lua_State* L = g_clients[i].L = luaL_newstate();
	//	luaL_openlibs(L);
	//	luaL_loadfile(L, "NPC.LUA");
	//	int error = lua_pcall(L, 0, 0, 0);
	//	if (error) cout << lua_tostring(L, -1);
	//	lua_getglobal(L, "set_uid");
	//	lua_pushnumber(L, i);
	//	error = lua_pcall(L, 1, 0, 0);
	//	if (error) cout << lua_tostring(L, -1);
	//	/*************클라이언트를 넣을때 섹터에 넣어줘야하지 않을가?************/
	//	g_ObjectListSector[g_clients[i].x][g_clients[i].y].m_lock.lock();

	//	g_ObjectListSector[g_clients[i].x][g_clients[i].y].m_clients.emplace(&g_clients[i]);

	//	g_ObjectListSector[g_clients[i].x][g_clients[i].y].m_lock.unlock();
	//	/*************************************************************************/

	//	lua_register(L, "API_send_message", API_SendMessage);
	//	lua_register(L, "API_get_x", API_get_x);
	//	lua_register(L, "API_get_y", API_get_y);
	//}
	fp.close();
}

void do_timer()
{
	while (true) {
		this_thread::sleep_for(1ms);
		while (true) {
			timer_lock.lock();
			if (true == timer_queue.empty()) {
				timer_lock.unlock();
				break;
			}
			auto now_t = high_resolution_clock::now();
			event_type temp_ev = timer_queue.top();
			if (timer_queue.top().wakeup_time > high_resolution_clock::now()) {
				timer_lock.unlock();
				break;
			}
			event_type ev = timer_queue.top();
			timer_queue.pop();
			timer_lock.unlock();
			switch (ev.event_id) {
			case OP_RANDOM_MOVE: {
				EXOVER* over = new EXOVER;
				over->op = ev.event_id;
				PostQueuedCompletionStatus(g_iocp, 1, ev.obj_id, &over->over);
			}
							   break;
			case OP_PLAYER_HP_RECOVERY: {
				EXOVER* over = new EXOVER;
				over->op = ev.event_id;
				PostQueuedCompletionStatus(g_iocp, 1, ev.obj_id, &over->over);
			}
									  break;
			}
		}
	}
}

void DB_update_user_data(string name, short posX, short posY, short exp, short hp, short level)
{
	wstring command = L"EXEC update_user_data ";
	wstring temp;
	temp.assign(name.begin(), name.end());
	command += temp.c_str();
	command += 44;
	command += 32;


	command += to_wstring(posX);
	command += 44;
	command += 32;

	command += to_wstring(posY);
	command += 44;
	command += 32;

	command += to_wstring(exp);
	command += 44;
	command += 32;

	command += to_wstring(hp);
	command += 44;
	command += 32;

	command += to_wstring(level);


	string printcom;
	printcom.assign(command.begin(), command.end());
	cout << printcom << endl;

	const int NAME_LEN = 20;

	SQLWCHAR szUser_name[NAME_LEN];
	SQLINTEGER dUser_posX = 0, dUser_posY = 0;
	SQLLEN cbName = 0, cbPosX = 0, cbPosY = 0;

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

	retcode = SQLExecDirect(hstmt, (SQLWCHAR*)command.c_str(), SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		cout << "업데이트 성공\n";
	}
	else HandleDiagnosticRecord(hdbc, SQL_HANDLE_DBC, retcode);
}

void DB_insert_user_data(string name, short posX, short posY, short exp, short hp, short level)
{
	wstring command = L"EXEC insert_user_data ";
	wstring temp;
	temp.assign(name.begin(), name.end());
	command += temp.c_str();
	command += 44;
	command += 32;


	command += to_wstring(posX);
	command += 44;
	command += 32;

	command += to_wstring(posY);
	command += 44;
	command += 32;

	command += to_wstring(exp);
	command += 44;
	command += 32;

	command += to_wstring(hp);
	command += 44;
	command += 32;

	command += to_wstring(level);


	string printcom;
	printcom.assign(command.begin(), command.end());
	cout << printcom << endl;

	const int NAME_LEN = 20;

	SQLWCHAR szUser_name[NAME_LEN];
	SQLINTEGER dUser_posX = 0, dUser_posY = 0;
	SQLLEN cbName = 0, cbPosX = 0, cbPosY = 0;

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

	retcode = SQLExecDirect(hstmt, (SQLWCHAR*)command.c_str(), SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		cout << "삽입 성공\n";
	}
	else HandleDiagnosticRecord(hdbc, SQL_HANDLE_DBC, retcode);
}

void DB_connect()
{
	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"game_db_odbc", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					cout << "DB 연결 성공\n";
				}
				else HandleDiagnosticRecord(hdbc, SQL_HANDLE_DBC, retcode);
			}
		}
	}
}

void DB_disconnect()
{
	SQLCancel(hstmt);
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	SQLFreeHandle(SQL_HANDLE_ENV, henv);
}

void DB_find_user_data(string name, int user_id)
{
	wstring command = L"EXEC select_user_data ";
	wstring temp;
	temp.assign(name.begin(), name.end());
	command += temp.c_str();

	string printcom;
	printcom.assign(command.begin(), command.end());

	const int NAME_LEN = 20;

	SQLWCHAR szUser_name[NAME_LEN];
	SQLINTEGER dUser_posX = 0, dUser_posY = 0, dUser_exp = 0, dUser_hp = 0, dUser_level = 0;
	SQLLEN cbName = 0, cbPosX = 0, cbPosY = 0, cbExp = 0, cbHp = 0, cbLevel = 0;

	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

	retcode = SQLExecDirect(hstmt, (SQLWCHAR*)command.c_str(), SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		// Bind columns 1, 2, and 3  
		retcode = SQLBindCol(hstmt, 1, SQL_C_WCHAR, szUser_name, NAME_LEN, &cbName);
		retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &dUser_posX, 100, &cbPosX);
		retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &dUser_posY, 100, &cbPosY);
		retcode = SQLBindCol(hstmt, 4, SQL_C_LONG, &dUser_exp, 100, &cbExp);
		retcode = SQLBindCol(hstmt, 5, SQL_C_LONG, &dUser_hp, 100, &cbHp);
		retcode = SQLBindCol(hstmt, 6, SQL_C_LONG, &dUser_level, 100, &cbLevel);

		retcode = SQLFetch(hstmt);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			for (int i = 0; i < NUM_NPC; ++i)
			{
				if (g_clients[i].m_status == ST_FREE)continue;
					if (name == g_clients[i].m_name)
					{
						send_login_fail_packet(user_id);
						disconnect(user_id);
						return;
					}
			}
			set_user_data(user_id, dUser_posX, dUser_posY, dUser_exp, dUser_hp, dUser_level);
			printf("%d: %ls %d %d %d %d %d \n", user_id, szUser_name, dUser_posX, dUser_posY, dUser_exp, dUser_hp, dUser_level);

		}
		else {
			//g_clients[user_id].x = StartPosition.x;
			//g_clients[user_id].y = StartPosition.y;
			g_clients[user_id].x = rand()%WORLD_WIDTH;
			g_clients[user_id].y = rand() % WORLD_HEIGHT;
			while (!map_data[g_clients[user_id].x][g_clients[user_id].y])
			{
				g_clients[user_id].x = rand() % WORLD_WIDTH;
				g_clients[user_id].y = rand() % WORLD_HEIGHT;
			}
			g_clients[user_id].m_exp = 0;
			g_clients[user_id].m_hp = 32700;
			g_clients[user_id].m_level = 1;
			DB_insert_user_data(name, g_clients[user_id].x, g_clients[user_id].y, g_clients[user_id].m_exp, g_clients[user_id].m_hp, g_clients[user_id].m_level);
		}
	}
	else HandleDiagnosticRecord(hdbc, SQL_HANDLE_DBC, retcode);
}

int main()
{
	
	StartPosition = { 2,2 };
	read_map();
	g_PathFinder = new PathFinder(map_data);
	WSADATA WSAData;

	WSAStartup(MAKEWORD(2, 2), &WSAData);

	/**********************************/
	DB_connect();
	/**********************************/

	cout << "NPC Initialization start.\n";
	init_npc();
	cout << "NPC Initialization finished\n";

	l_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN s_address;
	memset(&s_address, 0, sizeof(s_address));
	s_address.sin_family = AF_INET;
	s_address.sin_port = htons(SERVER_PORT);
	s_address.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	::bind(l_socket, reinterpret_cast<sockaddr*>(&s_address), sizeof(s_address));

	listen(l_socket, SOMAXCONN);

	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);

	initialize_clients();

	CreateIoCompletionPort(reinterpret_cast<HANDLE>(l_socket), g_iocp, 999, 0);
	SOCKET c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	EXOVER accept_over;
	ZeroMemory(&accept_over.over, sizeof(accept_over.over));
	accept_over.op = OP_ACCEPT;
	accept_over.c_socket = c_socket;
	AcceptEx(l_socket, c_socket, accept_over.io_buf, NULL, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, NULL, &accept_over.over);

	vector <thread> worker_threads;
	for (int i = 0; i < 4; ++i) worker_threads.emplace_back(worker_thread);

	thread timer_thread{ do_timer };
	for (auto& th : worker_threads) th.join();
	DB_disconnect();
}