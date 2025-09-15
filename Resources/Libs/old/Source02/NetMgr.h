#pragma once
#include <string>
#define MAX_PLAYERS 3
#define MAX_LOBBYS 3
using namespace std;

struct ObjData
{
	float px, py, pz;
	float qx, qy, qz,qw;
};

struct Lobby
{
	string name = "lobby01";
	int numPlayers = 0;
	string playerHandles[MAX_PLAYERS];
};

class NetMgr
{
public:
	bool host = false;
	int num_lobbys=0;
	Lobby lobbys[MAX_LOBBYS];
	void createLobby();
	int findLobbys();
	void joinLobby(int lobby);
	void startGame();
	void sendObjData(ObjData* objData);
	ObjData* recieveObjData();
};

