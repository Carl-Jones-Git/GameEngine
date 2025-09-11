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

#include <Includes.h>
#include <AIKart.h>
#include <PhysXVehicleController.h>
XMVECTOR vecFromStr(string& str)
{
	float vec3out[3] = { 0.1f,0.1f ,0.1f };
	string vec = str.substr(str.find("(") + 1, str.find(")"));
	str = str.substr(str.find(")") + 1);
	for (int j = 0; j < 3; j++)
	{
		vec3out[j] = std::stof(vec.substr(0, vec.find(",")));
		vec = vec.substr(vec.find(",") + 1);
	}

	return XMVectorSet(vec3out[0] , vec3out[1], vec3out[2], 1);
}

void NavPoints::load(string path,float mapScale,float LHCoords)
{
	numNavPoints = 0;
	ti::XMLDocument doc;
	doc.LoadFile(path.c_str());
	if (doc.Error())
		std::cerr << "Error parsing XML: " << doc.ErrorStr() << std::endl;
	else
	{
		ti::XMLElement* scene = doc.RootElement();
		ti::XMLElement* model = scene->FirstChildElement("NavPoint");
		while (model)
		{

			string path = model->FirstChildElement("Name")->GetText();
			string data = model->FirstChildElement("Data")->GetText();

			cout << "NavPoint  Data : " << data << endl;
			pos[numNavPoints] = vecFromStr(data);		
			pos[numNavPoints]=DirectX::XMVectorSetX(pos[numNavPoints], DirectX::XMVectorGetX(pos[numNavPoints]) * mapScale );
			pos[numNavPoints]=DirectX::XMVectorSetZ(pos[numNavPoints], LHCoords*DirectX::XMVectorGetZ(pos[numNavPoints]) * mapScale);
			rad[numNavPoints] = 2.2 ;
			XMVECTOR speedV= vecFromStr(data);
			speed[numNavPoints] = (13.0-DirectX::XMVectorGetX(speedV)*0.7+ DirectX::XMVectorGetY(speedV)*5.0);
			cout << "speed=" << speed[numNavPoints] << endl;
			numNavPoints++;
			model = model->NextSiblingElement("NavPoint");
		}
	}
}
void AIKart::init(NavPoints* _navPoints)
{
	navPoints = _navPoints;
	currentNavPoint = 0;
	trackTireFriction = TIRE_TYPE_NORMAL_AI;
	trackTireFriction = TIRE_TYPE_WORN_AI;
	oldPos = physx::PxVec3(0,0,0);

	countDown = TIME_OUT;

}

void AIKart::update(ID3D11DeviceContext* context, float stepSize)

{
	static bool neutral = true;
	mMimicKeyInputs= false;

	physx::PxTransform trans = mVehicle4W->getRigidDynamicActor()->getGlobalPose();
	if(started)
		countDown-= stepSize;

	if (countDown <= 0)
	{
		physx::PxVec3 diff = trans.p - oldPos;
		if ((fabs(diff.x) < 0.75) && (fabs(diff.z) < 0.75))
		{
			currentNavPoint -= 1;
			if (currentNavPoint < 0)
				currentNavPoint=navPoints->numNavPoints - 1;

			int prev = currentNavPoint-1;
			if (prev <0 )
				prev = navPoints->numNavPoints+ prev;
			XMMATRIX M=DirectX::XMMatrixLookAtLH(navPoints->pos[prev], navPoints->pos[currentNavPoint], XMVectorSet(0, 1, 0, 1));
			XMVECTOR DXQ = DirectX::XMQuaternionRotationMatrix(M);
			XMVECTOR axis; float angle=0;
			DirectX::XMQuaternionToAxisAngle(&axis, &angle, DXQ);
			axis = DirectX::XMVector3Normalize(axis);
			physx::PxQuat Q = physx::PxQuat(-angle, physx::PxVec3(XMVectorGetX(axis), XMVectorGetY(axis), XMVectorGetZ(axis)));
			physx::PxVec3 P(XMVectorGetX(navPoints->pos[prev]),ground->CalculateYValueWorld(XMVectorGetX(navPoints->pos[prev]), XMVectorGetZ(navPoints->pos[prev])), XMVectorGetZ(navPoints->pos[prev]));
			physx::PxTransform T(P, Q);
			restart(P,Q);

		}

		oldPos = trans.p;
		countDown = TIME_OUT;
	}

	trans.p.y += 0.05;
	XMVECTOR quart = DirectX::XMLoadFloat4(&XMFLOAT4(trans.q.x, trans.q.y, trans.q.z, trans.q.w));
	XMVECTOR npk = DirectX::XMVector4Transform(navPoints->pos[currentNavPoint], XMMatrixInverse(NULL,XMMatrixRotationQuaternion(quart)*XMMatrixTranslation(trans.p.x, trans.p.y, trans.p.z)));
	if (started)
	{
		mVehicleInputData.setAnalogAccel(min(max((navPoints->speed[currentNavPoint] - mVehicle4W->computeForwardSpeed()) / (navPoints->speed[currentNavPoint] / 1.7), 0.0), 1.0));
		mVehicleInputData.setAnalogSteer( min(max(DirectX::XMVectorGetX(DirectX::XMVector3Normalize(npk)),-1.0),1.0));
		mMimicKeyInputs = false;

	}
	if (DirectX::XMVectorGetX(DirectX::XMVector3Length(npk)) < navPoints->rad[currentNavPoint])
		currentNavPoint++;
	if (currentNavPoint>=navPoints->numNavPoints)
		currentNavPoint = 0;
	Kart::update(context,stepSize);

}
AIKart::~AIKart() 
{


}
