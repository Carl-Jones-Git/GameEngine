#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include "Common\MoveLookController.h"
#include "Content\Scene.h"
#include "Content\TextRenderer.h"

// Renders Direct2D and 3D content on the screen.
namespace App4
{
	class App4Main : public DX::IDeviceNotify
	{
	public:
		App4Main(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~App4Main();
		void CreateWindowSizeDependentResources();
		void Update();
		bool Render();

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;
		// TODO: Replace with your own content renderers.
		std::unique_ptr<Scene> m_sceneRenderer;
		std::unique_ptr<TextRenderer> m_fpsTextRenderer;

		MoveLookController^  m_controller;
		// Rendering loop timer.
		DX::StepTimer m_timer;
	};
}