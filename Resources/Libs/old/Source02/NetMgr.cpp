#include "NetMgr.h"

void NetMgr::createLobby()
{
	//host can create a loby for players to join
	lobbys[0].name = "MyLobby01";
	lobbys[0].numPlayers = 3;
	lobbys[0].playerHandles[0] = "banana";
	lobbys[0].playerHandles[1] = "orange";
	lobbys[0].playerHandles[2] = "apple";
	num_lobbys = 1;
};
void NetMgr::joinLobby(int lobby)
{

};
int NetMgr::findLobbys()
{
	lobbys[0].name = "Lobby01";
	lobbys[0].numPlayers = 3;
	lobbys[0].playerHandles[0]="elefant";
	lobbys[0].playerHandles[1] = "ZZZZZZZ";
	lobbys[0].playerHandles[2] = "ZHello";
	lobbys[1].name = "Lobby02";
	lobbys[1].numPlayers = 1;
	lobbys[1].playerHandles[0] = "cat";

	lobbys[2].name = "Lobby03";
	lobbys[2].numPlayers = 2;
	lobbys[2].playerHandles[0] = "Ziggy";
	lobbys[2].playerHandles[1] = "Sam";

	num_lobbys = 3;
	return num_lobbys;
};
void NetMgr::startGame()
{
	//host can start a lobby after players have joined

};
void NetMgr::sendObjData(ObjData* objData)
{
	//get data packet 
	//fill ObjData struct
	//and return pointer?

};
ObjData* NetMgr::recieveObjData()
{
	//get data packet 
	//fill ObjData struct
	//and return pointer?
	ObjData objData;
	return &objData;
};
//if (!netMgr.host && false)//not yet implemented
//{
//	ObjData objDataSend;
//	objDataSend.px = trans.p.x;
//	objDataSend.py = trans.p.y;
//	objDataSend.pz = trans.p.z;
//	objDataSend.qx = trans.q.x;
//	objDataSend.qy = trans.q.y;
//	objDataSend.qz = trans.q.z;
//	objDataSend.qw = trans.q.w;
//	netMgr.sendObjData(&objDataSend);
//}

//if (!netMgr.host&& false)//not yet implemented
//{
//	physx::PxTransform trans2 = aiKarts[i]->mVehicle4W->getRigidDynamicActor()->getGlobalPose();

//	ObjData *objDataReceive= netMgr.recieveObjData();
//	trans2.p.x = objDataReceive->px;
//	trans2.p.y = objDataReceive->py;
//	trans2.p.z = objDataReceive->pz;
//	trans2.q.x = objDataReceive->qx;
//	trans2.q.y = objDataReceive->qy;
//	trans2.q.z = objDataReceive->qz;
//	trans2.q.w = objDataReceive->qw;
//	aiKarts[i]->mVehicle4W->getRigidDynamicActor()->setGlobalPose(trans2,true);
//}