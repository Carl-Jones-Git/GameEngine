/*
 * Copyright (c) 2023 Carl Jones
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
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

