
#include <Includes.h>
#include <PlayerKart.h>
#include <PhysXVehicleController.h>


#ifdef PC_BUILD
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
extern LPDIRECTINPUTDEVICE8 wheel;

#endif // !PC_BUILD


void PlayerKart::render(ID3D11DeviceContext* context,int camMode) {
	if (camMode != 0)
	{
		// Render Driver
		if (Driver)
			Driver->render(context);
		if (Head)
			Head->render(context);
	}

	if (Legs)
		Legs->render(context);
	if (Shoes)
		Shoes->render(context);
	if (Hands)
		Hands->render(context);

	// Render Kart
	if (Frame)
		Frame->render(context);
	if (Farings)
		Farings->render(context);
	if (Engine)
		Engine->render(context);
	if (FLWheel)
		FLWheel->render(context);
	if (FRWheel)
		FRWheel->render(context);
	if (FLTire)
		FLTire->render(context);
	if (FRTire)
		FRTire->render(context);
	if (RearWheels)
		RearWheels->render(context);
	if (RearTires)
		RearTires->render(context);
	if (SWheelHub)
		SWheelHub->render(context);
	if (SWheel)
		SWheel->render(context);
}
//extern bool	gMimicKeyInputs;
//extern PxVehicleDrive4WRawInputData gVehicleInputData;


void PlayerKart::update(ID3D11DeviceContext* context, float stepSize)

{
	static bool neutral = false;

	static SHORT MinThumbLX = 32767;
	static SHORT MaxThumbLX = -32767;
	static SHORT MinThumbLY = 32767;
	static SHORT MaxThumbLY = -32767;


	//if ((GetAsyncKeyState(VK_UP) < 0) || (GetKeyState(VK_DOWN) < 0) || (GetKeyState(VK_LEFT) < 0) || (GetKeyState(VK_RIGHT) < 0))
		mMimicKeyInputs = true;

	//mMimicKeyInputs = true;
#ifdef PC_BUILD
		if (GetAsyncKeyState(VK_XBUTTON1) < 0)
			camMode = min(camMode++, 3);
#else
		if (m_controller->m_camUp)
			m_controller->m_camUp = false;
#endif
		{
			
			
			
		}
#ifdef PC_BUILD
		if (GetAsyncKeyState(VK_XBUTTON2) < 0)
			camMode = max(camMode--, 0);
#else
		if (m_controller->m_camDown)
			m_controller->m_camDown = false;
#endif
		{
			
			
		}

	
#ifdef PC_BUILD
	if (GetAsyncKeyState(VK_UP) < 0) 
#else
	if (m_controller->m_forward)
#endif
	{
		//startAccelerateForwardsMode();
		//gVehicleInputData.setAnalogAccel(100.5);
		if (neutral)
		{
			//mVehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
			//mVehicle4W->mDriveDynData.setUseAutoGears(true);
			//neutral = false;
			////mVehicle4W->mDriveDynData.setEngineRotationSpeed(200.0f);
		}
		mVehicleInputData.setDigitalAccel(true);
		mVehicleInputData.setDigitalBrake(false);
		//cout << "accelerate" << endl;
	}
	else
		mVehicleInputData.setDigitalAccel(false);

	
#ifdef PC_BUILD
	if (GetKeyState(VK_DOWN) < 0)
#else
		if (m_controller->m_back)
#endif
	{

		//mVehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eNEUTRAL);
		//neutral = true;
		mVehicleInputData.setDigitalBrake(true);
		mVehicleInputData.setDigitalAccel(false);
		//gVehicleInputData.setAnalogAccel(false);
		//startBrakeMode();
	}
	else

		mVehicleInputData.setDigitalBrake(false);


	
#ifdef PC_BUILD
		if (GetKeyState(VK_LEFT) < 0)
#else
		if (m_controller->m_left)
#endif
	{
		//startTurnHardLeftMode();
		mVehicleInputData.setDigitalSteerLeft(true);
		mVehicleInputData.setDigitalSteerRight(false);
	}
	else
		mVehicleInputData.setDigitalSteerLeft(false);


	
#ifdef PC_BUILD
		if (GetKeyState(VK_RIGHT) < 0)
#else
		if (m_controller->m_right)
#endif
	{
		mVehicleInputData.setDigitalSteerLeft(false);
		mVehicleInputData.setDigitalSteerRight(true);
	}
	else
		mVehicleInputData.setDigitalSteerRight(false);

	//else
	//{
		//if (mMimicKeyInputs == true)
		//{
		//	releaseAllControls(this);
		//	//mVehicle4W->mDriveDynData.setUseAutoGears(false);
		//	//mVehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eNEUTRAL);
		//	//float engineSpeed = mVehicle4W->mDriveDynData.getEngineRotationSpeed();
		//	//mVehicle4W->mDriveDynData.setEngineRotationSpeed(engineSpeed * 0.999);
		//	//neutral = true;
		//}






	DWORD result = XInputGetState(0, &controllerState);
	if (result == ERROR_SUCCESS)
	{
		XINPUT_VIBRATION vib;
		if (wheelSpin)
			vib.wRightMotorSpeed = (WORD)(fabs(mVehicle4W->computeForwardSpeed()) * 6553.5f * 0.5);
		else
			vib.wRightMotorSpeed = (WORD)0;
		if (onGrass)
			vib.wLeftMotorSpeed = (WORD)(fabs(mVehicle4W->computeForwardSpeed()) * 6553.5f * 0.1);

		else
			vib.wLeftMotorSpeed = (WORD)0;



		XInputSetState(0, &vib);

		static bool AUp = true;
		static bool BUp = true;
		static bool YUp = true;
		static bool XUp = true;
		mMimicKeyInputs = false;


		//cout << "XBOX SUCCESS" << endl;
		if ((controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_X))
		{
			if (XUp)
			{
				physx::PxQuat Q(XMConvertToRadians(-100), PxVec3(0, 1, 0));
				physx::PxTransform trans(polePosition,Q);// physx::PxVec3(36, 1, 0.7f), Q);
				mVehicle4W->getRigidDynamicActor()->setGlobalPose(trans);
				mVehicle4W->getRigidDynamicActor()->setLinearVelocity(physx::PxVec3(0, 0, 0));
				mVehicle4W->getRigidDynamicActor()->setAngularVelocity(physx::PxVec3(0, 0, 0));
				mVehicle4W->setToRestState();
				lapTime = 0.0f;
			}
			XUp = false;
		}
		else
			XUp = true;


		if ((controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_A))
		{

			if (kartCC < 2 && AUp)
				kartCC = setCC(this, kartCC + 1);
			AUp = false;
		}
		else
			AUp = true;
		if ((controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_B))
		{
			if (BUp)
				if (kartCC > 0)
					kartCC = setCC(this, kartCC - 1);
			BUp = false;
		}
		else
			BUp = true;
		if ((controllerState.Gamepad.wButtons & XINPUT_GAMEPAD_Y))
		{
			if (YUp)
				if (camMode < 3)
					camMode++;
				else
					camMode = 0;
			YUp = false;
		}
		else
			YUp = true;

		SHORT thumbLX = controllerState.Gamepad.sThumbLX;
		SHORT thumbLY = controllerState.Gamepad.sThumbLY;
		BYTE triggerL = controllerState.Gamepad.bLeftTrigger;
		BYTE triggerR = controllerState.Gamepad.bRightTrigger;

		mVehicleInputData.setAnalogAccel(max(min((float)triggerR / 100.0f, 1.0f), 0.0f));
		mVehicleInputData.setAnalogBrake(max(min((float)triggerL / 255.0f, 1.0f), 0.0f));
		mVehicleInputData.setAnalogSteer(max(min((float)thumbLX / 32767.0f, 1.0f), -1.0f));

		if (mVehicleInputData.getAnalogAccel() > 0.0)
		{
			if (neutral)
			{
				mVehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
				mVehicle4W->mDriveDynData.setUseAutoGears(true);
				neutral = false;
			}
		}


		if (thumbLX < MinThumbLX)
			MinThumbLX = thumbLX;
		if (thumbLX > MaxThumbLX)
			MaxThumbLX = thumbLX;
		if (thumbLY < MinThumbLY)
			MinThumbLY = thumbLY;
		if (thumbLY > MaxThumbLY)
			MaxThumbLY = thumbLY;

	}

#ifdef PC_BUILD


	if (wheel)
	{
		// In game loop
		DIJOYSTATE2 state;
		wheel->Poll();
		wheel->GetDeviceState(sizeof(DIJOYSTATE2), &state);



		static bool AUp = true;
		static bool BUp = true;
		static bool YUp = true;
		static bool XUp = true;
		// Check buttons


		// Check buttons
		if (state.rgbButtons[0] & 0x80)
		{

			if (kartCC < 2 && AUp)
				kartCC = setCC(this, kartCC + 1);
			AUp = false;
			std::cout << "Button A Pressed" << std::endl;
		}
		else
			AUp = true;

		// Check buttons
		if (state.rgbButtons[1] & 0x80)
		{
			if (BUp)
				if (kartCC > 0)
					kartCC = setCC(this, kartCC - 1);
			BUp = false;
			std::cout << "Button B Pressed" << std::endl;
		}
		else
			BUp = true;


		// Check buttons
		if (state.rgbButtons[2] & 0x80)
		{
			if (XUp)
			{
				physx::PxQuat Q(XMConvertToRadians(-100), PxVec3(0, 1, 0));
				physx::PxTransform trans(physx::PxVec3(36, 1, 0.7f), Q);
				mVehicle4W->getRigidDynamicActor()->setGlobalPose(trans);
				mVehicle4W->getRigidDynamicActor()->setLinearVelocity(physx::PxVec3(0, 0, 0));
				mVehicle4W->getRigidDynamicActor()->setAngularVelocity(physx::PxVec3(0, 0, 0));
				mVehicle4W->setToRestState();
				lapTime = 0.0f;
			}
			XUp = false;
		}
		else
			XUp = true;

		if (state.rgbButtons[3] & 0x80)
		{
			if (YUp)
				if (camMode < 3)
					camMode++;
				else
					camMode = 0;
			YUp = false;
			std::cout << "Button Y Pressed" << std::endl;
		}
		else
			YUp = true;


		// Get wheel position
		int turn = state.lX - 32767.0f;
		cout << "wheelPos:" << turn << endl;

		// Get wheel position
		int  clutch = state.rglSlider[0];
		cout << "clutch:" << clutch << endl;


		int brake = 65535 - state.lRz;
		cout << "brake:" << brake << endl;
		int accel = 65535 - state.lY;
		cout << "accel:" << accel << endl;



		mVehicleInputData.setAnalogAccel(max(min((float)accel / 65535.0f, 1.0f), 0.0f));
		mVehicleInputData.setAnalogBrake(max(min((float)brake / 43000.0f, 1.0f), 0.0f));
		mVehicleInputData.setAnalogSteer(max(min((float)turn / 32767.0f, 1.0f), -1.0f) * 5.0f);

	}
#endif // !PC_BUILD
	//Update sound listen
	FMOD_VECTOR listenerPos = { 0,0,0 };
	FMOD_VECTOR listenerVel = { 0,0,0 };
	FMOD_VECTOR listenerForward = { 0.0f, 0.0f, 1.0f };
	FMOD_VECTOR listenerUp = { 0.0f, 1.0f, 0.0f };
	FMSystem->set3DListenerAttributes(0, &listenerPos, &listenerVel, &listenerForward, &listenerUp);
	FMSystem->update();


	physx::PxReal rpm = mVehicle4W->mDriveDynData.getEngineRotationSpeed();

	float pitch = pow( rpm,0.5f) / 5.0f;
	pitch= min(max(pitch, 1.0f), 3.0f);

	//Update sound
	FMOD_VECTOR soundPos = { 0,0,0 };  // Set the position of the sound source
	FMOD_VECTOR soundVel = { 0,0,0 };  // Set the velocity of the sound source
	channel2->setLoopCount(-1);
	channel2->setPitch(pitch );
	channel2->setVolume(10);
	channel2->set3DAttributes(&soundPos, &soundVel);
	channel2->setPaused(false);
	channel3->setLoopCount(-1);
	channel3->setPitch( 2.0);
	channel3->setVolume(5.0);
	channel3->set3DAttributes(&soundPos, &soundVel);
	channel3->setPaused(false);
	FMSystem->update();

	Kart::update(context, stepSize);
}


PlayerKart::~PlayerKart() 
{
	//~Kart();

}
