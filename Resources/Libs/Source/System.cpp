
//
// System.cpp
//

#include <Includes.h>
#include <System.h>

// Private interface implementation

// Private constructor
System::System(HWND hwnd) {
	HRESULT hr = setupDeviceIndependentResources();
	if (SUCCEEDED(hr))
		hr = setupDeviceDependentResources(hwnd);
	if (SUCCEEDED(hr))
		hr = setupWindowDependentResources(hwnd);
}

// Public interface implementation

// System factory method
System* System::CreateDirectXSystem(HWND hwnd) {
	static bool _systemCreated = false;
	System *system = nullptr;
	if (!_systemCreated) {
		if (system = new System(hwnd))
			_systemCreated = true;
	}
	return system;
}

// Destructor
System::~System() {
	cout <<"System destructor called" << endl;

	// Clean up ImGui
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();


	if (renderTargetView)
		renderTargetView->Release();
	if (depthStencilView)
		depthStencilView->Release();
	if (depthStencilSRV)
		depthStencilSRV->Release();
	if (swapChain)
		swapChain->Release();
	if (context)
		context->Release();
	if (device)
		device->Release();
	if (defaultAdapter)
		defaultAdapter->Release();
	if (dxgiFactory)
		dxgiFactory->Release();
	//// Use DXGI 1.1
	//IDXGIFactory1* dxgiFactory = nullptr;
	//IDXGISwapChain* swapChain = nullptr;

	//IDXGIAdapter* defaultAdapter = nullptr;
	//ID3D11Device* device = nullptr;
	//ID3D11DeviceContext* context = nullptr;
	//D3D_FEATURE_LEVEL						supportedFeatureLevel;

	//ID3D11ShaderResourceView* depthStencilSRV = nullptr;
//	ID3D11Texture2D* depthStencilBuffer = nullptr;
}

// Setup DirectX interfaces that will be constant - they do not depend on any device
HRESULT System::setupDeviceIndependentResources() {
	HRESULT hr;
	hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&dxgiFactory);
	return hr;
}

// Setup DirectX interfaces that are dependent upon a specific device
HRESULT System::setupDeviceDependentResources(HWND hwnd) {
	
	// Get HWND client area
	RECT clientRect;
	GetClientRect(hwnd, &clientRect);
	LONG width = clientRect.right - clientRect.left;
	LONG height = clientRect.bottom - clientRect.top;
	
	// Get default adapter
	HRESULT hr = dxgiFactory->EnumAdapters(1, &defaultAdapter);

	// Create D3D device
	if (SUCCEEDED(hr)) {
		// Declare required feature levels.  Note: D3D_FEATURE_LEVEL_11_1 requires the DirectX 11.1 runtime to be installed (so Win 8 or Win 7 SP 1 with the Platform Update is required).  If this isn't available remove this entry from the dxFeatureLevels array otherwise D3D11CreateDevice (below) will fail and return E_INVALIDARG.
		D3D_FEATURE_LEVEL dxFeatureLevels[] = {
			//		D3D_FEATURE_LEVEL_12_1,
		//D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1, // try our luck...
			D3D_FEATURE_LEVEL_11_0 // ... but we're only interested in DX 11.0 support for this demo!
		};

		// Add code here (Create Device and Context)
		// Create Device and Context
		hr = D3D11CreateDevice(
			defaultAdapter,
			D3D_DRIVER_TYPE_UNKNOWN, // Specify TYPE_UNKNOWN since we're specifying our own adapter 'defaultAdapter'
			NULL,
			//0,
			//D3D11_CREATE_DEVICE_DEBUG,// |
			D3D11_CREATE_DEVICE_SINGLETHREADED, //|
//			D3D11_CREATE_DEVICE_BGRA_SUPPORT, // Needed for D2D interop
			dxFeatureLevels,
			2,
			D3D11_SDK_VERSION,
			&device,
			&supportedFeatureLevel,
			&context);
	}


	// Setup ImGui binding, after d3d built, Must be initialise with d3d context etc, so must be done in application
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(device, context);
	ImGui::StyleColorsDark();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	myFont1 = io.Fonts->AddFontFromFileTTF("../Resources/Libs/IMGUI/fonts/calibre/CalibreRegular.otf", 40.0f);
	myFont2 = io.Fonts->AddFontFromFileTTF("../Resources/Libs/IMGUI/fonts/calibre/CalibreBold.otf", 40.0f);



	UINT quality=141;
	device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 4, &quality);
	printf("quality=%d ",(int)quality);
	device->CheckMultisampleQualityLevels(DXGI_FORMAT_D24_UNORM_S8_UINT, 4, &quality);
	printf("dquality=%d ",(int)quality);

	// Setup Swapchain

	// Get root (IUnknown) interface for device
	IUnknown *deviceRootInterface = nullptr;

	if (SUCCEEDED(hr))
		hr = device->QueryInterface(__uuidof(IUnknown), (void**)&deviceRootInterface);

	// Add code here (Create Swap Chain)
	DXGI_SWAP_CHAIN_DESC scDesc;

	ZeroMemory(&scDesc, sizeof(DXGI_SWAP_CHAIN_DESC));


	// Setup swap chain description
	scDesc.BufferCount = 2;
	scDesc.BufferDesc.Width = width;
	scDesc.BufferDesc.Height = height;
	//scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.BufferDesc.RefreshRate.Numerator = 60;
	scDesc.BufferDesc.RefreshRate.Denominator = 1;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.OutputWindow = hwnd;
	scDesc.SampleDesc.Count = 4;
	scDesc.SampleDesc.Quality =0;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scDesc.Windowed = true;

	// Create swap chain
	if (SUCCEEDED(hr))
		hr = dxgiFactory->CreateSwapChain(deviceRootInterface, &scDesc, &swapChain);

	if (SUCCEEDED(hr)) {
		// MakeWindowAssociation for Alt+Enter full screen switching
		dxgiFactory->MakeWindowAssociation(hwnd, 0);
	}


	deviceRootInterface->Release();

	return hr;
}


// Setup window-specific resources including swap chain buffers, texture buffers and resource views that are dependant upon the host window size
HRESULT System::setupWindowDependentResources(HWND hwnd) {

	// Get HWND client area
	RECT clientRect;
	GetClientRect(hwnd, &clientRect);
	LONG width = clientRect.right - clientRect.left;
	LONG height = clientRect.bottom - clientRect.top;

	// Add code here (Create Render Target View)

	// Create Render Target View
	// Get the back buffer texture from the swap chain and derive the associated Render Target View (RTV)
	ID3D11Texture2D *backBuffer = nullptr;
	HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));

	if (SUCCEEDED(hr)) 
		hr = device->CreateRenderTargetView(backBuffer, 0, &renderTargetView);
	
	// Release references to the backBuffer since the render target view holds a reference to the backbuffer texture.
	if (backBuffer)
		backBuffer->Release();

	// Setup the Depth Stencil buffer and Depth Stencil View (DSV)

	// Add code here (Setup the Depth Stencil buffer and Depth Stencil View)

	D3D11_TEXTURE2D_DESC		depthStencilDesc;


	cout << width << height << endl;
	depthStencilDesc.Width = width;
	depthStencilDesc.Height = height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthStencilDesc.SampleDesc.Count =4; // Multi-sample properties must match the above DXGI_SWAP_CHAIN_DESC structure
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	if (SUCCEEDED(hr))
		hr = device->CreateTexture2D(&depthStencilDesc, 0, &depthStencilBuffer);
	
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	descDSV.Texture2D.MipSlice = 0;
	
	if (SUCCEEDED(hr))
		hr = device->CreateDepthStencilView(depthStencilBuffer, &descDSV, &depthStencilView);

	// Setup the description of the shader resource view		
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	if (SUCCEEDED(hr))
		hr = device->CreateShaderResourceView(depthStencilBuffer, &shaderResourceViewDesc, &depthStencilSRV);


	// Release un-needed references
	if (depthStencilBuffer)
		depthStencilBuffer->Release();
	depthStencilBuffer = nullptr;

	return hr;
}

// Resize swap chain buffers according to the given window client area
HRESULT System::resizeSwapChainBuffers(HWND hwnd) {
	// Get HWND client area
	RECT clientRect;
	GetClientRect(hwnd, &clientRect);
	LONG width = clientRect.right - clientRect.left;
	LONG height = clientRect.bottom - clientRect.top;
	// Detach the Render Target and Depth Stencil Views
	context->OMSetRenderTargets(0, nullptr, nullptr);
	// Release references to the swap chain's buffers by releasing references held on buffer view interfaces. 
	if (renderTargetView)
		renderTargetView->Release();
	if (depthStencilView)
		depthStencilView->Release();

	if (depthStencilSRV)
		depthStencilSRV->Release();





	// Resize swap chain buffers
	HRESULT hr = swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	if (SUCCEEDED(hr)|| HRESULT_CODE(hr)) {
		// Rebuild window size-dependent resources
		hr = setupWindowDependentResources(hwnd);
	}
	else
	{
		//LPTSTR error_text = NULL;
		//FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
		//	FORMAT_MESSAGE_ALLOCATE_BUFFER |
		//	FORMAT_MESSAGE_IGNORE_INSERTS,
		//	NULL,
		//	hr,
		//	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		//	(LPTSTR)&error_text, 0,
		//	NULL);
		cout<<"Cant resize buffers:"<< HRESULT_CODE(hr) <<endl;
	}
	
	return hr;
}

// Present back buffer to the screen
HRESULT System::presentBackBuffer() {
	return swapChain->Present(0, 0);
}

// Accessor methods
ID3D11Device* System::getDevice() {
	return device;
}

ID3D11DeviceContext* System::getDeviceContext() {
	return context;
}

ID3D11RenderTargetView* System::getBackBufferRTV() {
	return renderTargetView;
}

ID3D11DepthStencilView* System::getDepthStencil() {
	return depthStencilView;
}
ID3D11ShaderResourceView* System::getDepthStencilSRV() {
	return depthStencilSRV;
}
ID3D11Texture2D* System::getDepthStencilBuffer()
{
	return depthStencilBuffer;
}