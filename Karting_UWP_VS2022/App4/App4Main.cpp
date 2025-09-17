#include "pch.h"
#include "Common\MoveLookController.h"
#include "App4Main.h"
#include "Common\DirectXHelper.h"

using namespace App4;
using DirectX::XMFLOAT2;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Windows::System::Threading;
using namespace Concurrency;

// Loads and initializes application assets when the application is loaded.
App4Main::App4Main(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources)
{
	// Register to be notified if the Device is lost or recreated
	m_deviceResources->RegisterDeviceNotify(this);

	m_controller = ref new MoveLookController(CoreWindow::GetForCurrentThread());


	m_fpsTextRenderer = std::unique_ptr<TextRenderer>(new TextRenderer(m_deviceResources));
	// TODO: Replace this with your app's content initialization.
	m_sceneRenderer = std::unique_ptr<Scene>(new Scene(m_deviceResources, m_controller));

	

	// TODO: Change the timer settings if you want something other than the default variable timestep mode.
	// e.g. for 60 FPS fixed timestep update logic, call:
	/*
	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);
	*/


	

	auto bounds = m_deviceResources->GetLogicalSize();

	float GCTouchRectangleSize = 125.0f;

	m_controller->SetMoveRect(
		XMFLOAT2(0.0f, bounds.Height - GCTouchRectangleSize),
		XMFLOAT2(GCTouchRectangleSize, bounds.Height)
	);
	m_controller->SetFireRect(
		XMFLOAT2(bounds.Width - GCTouchRectangleSize, bounds.Height - GCTouchRectangleSize),
		XMFLOAT2(bounds.Width, bounds.Height)
	);

}

App4Main::~App4Main()
{
	// Deregister device notification
	m_deviceResources->RegisterDeviceNotify(nullptr);
}

// Updates application state when the window size changes (e.g. device orientation change)
void App4Main::CreateWindowSizeDependentResources() 
{
	// TODO: Replace this with the size-dependent initialization of your app's content.
	m_sceneRenderer->CreateWindowSizeDependentResources();
}

// Updates the application state once per frame.
void App4Main::Update() 
{
	// Update scene objects.
	m_timer.Tick([&]()
	{







		// TODO: Replace this with your app's content update functions.
		m_sceneRenderer->Update(m_timer);
		std::wstring textBuffer = m_sceneRenderer->getTextBuffer();
		//m_fpsTextRenderer->Update(m_timer);
		m_fpsTextRenderer->Update(textBuffer);
	});
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool App4Main::Render() 
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Reset the viewport to target the whole screen.
	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	// Reset render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	// Clear the back buffer and depth stencil view.
	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Render the scene objects.
	// TODO: Replace this with your app's content rendering functions.
	m_sceneRenderer->Render();
	m_fpsTextRenderer->Render();

	return true;
}

// Notifies renderers that device resources need to be released.
void App4Main::OnDeviceLost()
{
	m_sceneRenderer->ReleaseDeviceDependentResources();
	m_fpsTextRenderer->ReleaseDeviceDependentResources();
}

// Notifies renderers that device resources may now be recreated.
void App4Main::OnDeviceRestored()
{
	m_sceneRenderer->CreateDeviceDependentResources();
	m_fpsTextRenderer->CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
