// Scene.cpp - Part 1: Core Infrastructure and Initialization
/*
 * Copyright (c) 2023 Carl Jones
 * Licensed under MIT License
 */

#include "Scene.h"
#include <Includes.h>
#include <m_system.h>
#include <Effect.h>
#include <VertexStructures.h>
#include <Texture.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <DirectXMath.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <stdexcept>
#include <iostream>
#include <algorithm>

//using namespace DirectX;

// ============================================================================
// Configuration Constants
// ============================================================================


    // Resource paths - conditional compilation for different platforms
#ifdef PC_BUILD
    extern const wstring RESOURCE_PATH = L"..\\";
    extern const string SHADER_PATH = "Shaders\\cso\\";
#else
    extern const wstring RESOURCE_PATH = L"Assets\\";
    extern const string SHADER_PATH = "";
#endif


namespace {


    // Race track constants
    const XMVECTOR START_FINISH_POSITION = XMVectorSet(36.0f, 0.0f, 0.7f, 1.0f);
    constexpr float START_LINE_DISTANCE = 10.0f;
    constexpr float FINISH_LINE_DISTANCE = 10.0f;



    // Skin rendering parameters (used for subsurface scattering demo)
    float g_useTSD = 1.0f;           // Toggle for technique
    float g_lightDistance = 10.0f;   // Light distance for demo
    float g_lightVec[3] = { 0.0f, 10.0f, 10.0f }; // Light position
    float g_red = 1.0f;              // Red channel intensity
}

// ============================================================================
// External PhysX Variables (from PhysX module)
// ============================================================================

extern bool gVehicleOrderComplete;
extern PxScene* mScene;
extern PxVehicleDrivableSurfaceToTireFrictionPairs* gFrictionPairs;
extern ShapeUserData gShapeUserDatas;
extern physx::PxRigidDynamic* body[];
extern PxPhysics* mPhysics;
extern PxCooking* gCooking;
extern PxMaterial* mMaterial;
extern PxMaterial* grassMaterial;
extern PxRigidStatic* gDrivableGroundPlane;



// ============================================================================
// Factory Method Implementation
// ============================================================================

Scene* Scene::CreateScene(
    LONG width,
    LONG height,
    const wchar_t* wndClassName,
    const wchar_t* wndTitle,
    int nCmdShow,
    HINSTANCE hInstance,
    WNDPROC WndProc)
{
    static bool sceneCreated = false;

    if (sceneCreated) {
        std::cerr << "Error: Scene already created (singleton pattern)" << std::endl;
        return nullptr;
    }

    try {
        auto scene = new Scene(width, height, wndClassName, wndTitle,
            nCmdShow, hInstance, WndProc);
        sceneCreated = true;
        return scene;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to create scene: " << e.what() << std::endl;
        return nullptr;
    }
}

// ============================================================================
// Constructor - Initializes all subsystems
// ============================================================================

Scene::Scene(
    LONG width,
    LONG height,
    const wchar_t* wndClassName,
    const wchar_t* wndTitle,
    int nCmdShow,
    HINSTANCE hInstance,
    WNDPROC WndProc)
    : m_hInstance(hInstance)
    , m_windowHandle(nullptr)
    , m_accumulator(0.0f)
    , m_fixedTimeStep(1.0f / 60.0f)
    , m_physicsUpdatesPerFrame(1)
    , m_scenePhysicsUpdates(1)
    , m_kartPhysicsUpdates(1)
    , m_shouldRenderFrame(false)
    , m_audioSystem(nullptr)
    , m_menuState(MenuState::Intro)
    , m_menuKartRotation(-0.3f)
    , m_terrainResolution(200)
    , m_terrainScaleXZ(1.0f)
    , m_terrainScaleY(2.5f)
    , m_grassRenderPasses(0)
    ,m_grassShellHeight( 0.001666f / m_terrainScaleY)
{
    try {
        // Step 1: Register window class
        registerWindowClass(wndClassName, WndProc);

        // Step 2: Create window
        createWindow(wndClassName, wndTitle, width, height, nCmdShow);

        // Step 3: Initialize DirectX
        initializeDirectX();

        // Step 4: Initialize timing m_system
        initializeClock();

        // Step 5: Load all scene resources
        HRESULT hr = initialiseSceneResources();
        if (FAILED(hr)) {
            throw std::runtime_error("Failed to initialize scene resources");
        }

        std::cout << "Scene created successfully" << std::endl;
    }
    catch (const std::exception& e) {
        cleanup();
        throw;
    }
}

// ============================================================================
// Destructor - RAII cleanup in reverse order
// ============================================================================

Scene::~Scene() {
    std::cout << "Destroying scene..." << std::endl;
    cleanup();
    std::cout << "Scene destroyed" << std::endl;
}

void Scene::cleanup() {
    // Clean up in reverse order of creation

    // 1. Menu m_system
    m_menu.reset();

    // 2. Rendering effects
    m_shadowMap.reset();
    m_bloomEffect.reset();
    m_subsurfaceScattering.reset();

    // 3. Constant buffers (GPU resources released by ComPtr)
    m_sceneBufferCPU.reset();
    m_lightBufferCPU.reset();
    m_sceneBufferGPU.Reset();
    m_lightBufferGPU.Reset();

    // 4. Textures and visual resources
    m_cubeDayTexture.reset();
    m_dynamicCubeMap.reset();

    // 5. Scene geometry
    m_navigationPoints.reset();
    m_skyBox.reset();
    m_water.reset();
    m_debugOrb.reset();
    m_terrain.reset();
    m_physicsBox.reset();

    // 6. Animated m_sceneModels
    m_dragonModel.reset();
    m_nathanModel.reset();
    m_sophiaModel.reset();

    // 7. Foliage
    m_treeTemplate.reset();

    // 8. Particle systems
    m_dirtParticles.reset();
    m_smokeParticles.reset();

    // 9. Lens flare
    m_lensFlare.reset();

    // 10. Face model for skin rendering
    m_faceModel.reset();
    m_skinEffect.reset();

    // 11. Karts (AI karts auto-delete via unique_ptr)
    m_playerKart.reset();
    m_vehicleController.reset();

    // 12. Physics m_system
    if (m_physicsScene) {
        m_physicsScene.reset();
    }

    // 13. Audio m_system (library-owned pointer)
    if (m_audioSystem) {
        m_audioSystem->release();
        m_audioSystem = nullptr;
    }

    // 14. Core systems
    m_mainClock.reset();
    m_mainCamera.reset();
    m_system.reset();

    // 15. Window
    if (m_windowHandle) {
        DestroyWindow(m_windowHandle);
        m_windowHandle = nullptr;
    }
}

// ============================================================================
// Window Management
// ============================================================================

void Scene::registerWindowClass(const wchar_t* className, WNDPROC wndProc) {
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = wndProc;
    wcex.hInstance = m_hInstance;
    wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(nullptr, IDC_CROSS);
    wcex.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wcex.lpszClassName = className;

    if (!RegisterClassEx(&wcex)) {
        throw std::runtime_error("Failed to register window class");
    }
}

void Scene::createWindow(
    const wchar_t* className,
    const wchar_t* title,
    LONG width,
    LONG height,
    int nCmdShow)
{
    // Calculate window rect accounting for borders
    RECT windowRect = { 0, 0, width, height };
    const DWORD exStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    const DWORD style = WS_OVERLAPPEDWINDOW;

    AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);

    // Create window
    m_windowHandle = CreateWindowEx(
        exStyle,
        className,
        title,
        style | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        50, 50,  // Initial position
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        m_hInstance,
        this  // Pass 'this' pointer for retrieval in WndProc
    );

    if (!m_windowHandle) {
        throw std::runtime_error("Failed to create window");
    }

    ShowWindow(m_windowHandle, nCmdShow);
    UpdateWindow(m_windowHandle);
    SetFocus(m_windowHandle);
}

void Scene::destroyWindow() {
    if (m_windowHandle) {
        HWND handle = m_windowHandle;
        m_windowHandle = nullptr;
        DestroyWindow(handle);
    }
}

BOOL Scene::isMinimised() const {
    if (!m_windowHandle) {
        return TRUE;
    }

    WINDOWPLACEMENT placement = {};
    placement.length = sizeof(WINDOWPLACEMENT);

    if (GetWindowPlacement(m_windowHandle, &placement)) {
        return placement.showCmd == SW_SHOWMINIMIZED;
    }

    return FALSE;
}

// ============================================================================
// DirectX Initialization
// ============================================================================

void Scene::initializeDirectX() {
    m_system = std::unique_ptr<System>(System::CreateDirectXSystem(m_windowHandle));

    if (!m_system) {
        throw std::runtime_error("Failed to create DirectX m_system");
    }
}

HRESULT Scene::rebuildViewport() {
    if (!m_system) {
        return E_FAIL;
    }

    auto context = m_system->getDeviceContext();
    if (!context) {
        return E_FAIL;
    }

    // Bind render targets
    auto renderTarget = m_system->getBackBufferRTV();
    auto depthStencil = m_system->getDepthStencil();
    context->OMSetRenderTargets(1, &renderTarget, depthStencil);

    // Calculate m_viewport from window client area
    RECT clientRect;
    GetClientRect(m_windowHandle, &clientRect);

    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<FLOAT>(clientRect.right - clientRect.left);
    m_viewport.Height = static_cast<FLOAT>(clientRect.bottom - clientRect.top);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    context->RSSetViewports(1, &m_viewport);

    return S_OK;
}

HRESULT Scene::resizeResources() {
    if (!m_system) {
        return E_FAIL;
    }

    HRESULT hr = m_system->resizeSwapChainBuffers(m_windowHandle);
    if (FAILED(hr)) {
        return hr;
    }

    rebuildViewport();

    // Re-render scene if window is visible
    if (!isMinimised()) {
        renderScene();
    }

    return S_OK;
}

// ============================================================================
// Timing m_system
// ============================================================================

void Scene::initializeClock() {
    m_mainClock = std::unique_ptr<CGDClock>(
        CGDClock::CreateClock("m_mainClock", 3.0f)
    );

    if (!m_mainClock) {
        throw std::runtime_error("Failed to create game clock");
    }
}

void Scene::startClock() {
    if (m_mainClock) {
        m_mainClock->start();
    }
}

void Scene::stopClock() {
    if (m_mainClock) {
        m_mainClock->stop();
    }
}

void Scene::reportTimingData() {
    if (!m_mainClock) {
        return;
    }

    std::cout << "=== Timing Report ===" << std::endl;
    std::cout << "Actual time elapsed: " << m_mainClock->actualTimeElapsed() << "s" << std::endl;
    std::cout << "Game time elapsed: " << m_mainClock->gameTimeElapsed() << "s" << std::endl;
    m_mainClock->reportTimingData();
}

// ============================================================================
// Physics Integration
// ============================================================================
// //void Scene::convertModelToPhysX(
//    std::shared_ptr<Model> model,
//    XMVECTOR rotation,
//    XMVECTOR translation,
//    PhysXMode type,
//    int instance)
//Convert Model to PhysX Shape
void Scene::convertModelToPhysX(shared_ptr < Model> model, XMVECTOR R, XMVECTOR T, PhysXMode type, int instance)
{
    PxMaterial* material = mPhysics->createMaterial(0.0f, 0.0f, 0.6f);
    if (type == PhysXMode::Driveable)
        material = mPhysics->createMaterial(0.6f, 0.4f, 0.6f);
    //Get Vertices
    if (type == PhysXMode::Static || type == PhysXMode::Driveable)//Static
    {
        PxVec3* verts = (PxVec3*)malloc(sizeof(PxVec3) * model->getNumVert());
        PxTransform t(PxVec3(0, 0, 0), PxQuat(XMConvertToRadians(0), PxVec3(0, 1, 0)));
        for (int i = 0; i < model->getNumVert(); i++)
        {
            XMVECTOR pos = DirectX::XMVector3TransformCoord(XMVectorSet(model->getVertexBufferCPU()[i].pos.x, model->getVertexBufferCPU()[i].pos.y, model->getVertexBufferCPU()[i].pos.z, 1), model->getWorldMatrix());
            verts[i].x = DirectX::XMVectorGetX(pos);
            verts[i].y = DirectX::XMVectorGetY(pos) - 0.25;
            verts[i].z = DirectX::XMVectorGetZ(pos);
        }
        int count = 0;
        //Get faces
        PxU32* indices32 = (PxU32*)malloc(sizeof(PxU32) * model->getNumInd());
        for (int j = 0; j < model->getNumMeshes(); j++)
            for (int i = 0; i < model->getIndexCount()[j]; i++) {
                indices32[count] = (PxU32)model->getIndexBufferCPU()[count] + model->getBaseVertexOffset()[j];
                count++;
            }
        //Create PhysX TriangleMesh
        PxTriangleMesh* triMesh = createTriangleMesh(verts, model->getNumVert(), indices32, model->getNumFaces(), *mPhysics, *gCooking);
        PxRigidStatic* r = mPhysics->createRigidStatic(t);
        PxShape* shape = PxRigidActorExt::createExclusiveShape(*r, PxTriangleMeshGeometry(triMesh), *material);
        //Setup collision filters
        PxFilterData simFilterData(COLLISION_FLAG_OBSTACLE, COLLISION_FLAG_CHASSIS, 0, 0);
        shape->setSimulationFilterData(simFilterData);

        if (type == PhysXMode::Driveable)//make driveable
        {
            PxFilterData qryFilterData;
            setupDrivableSurface(qryFilterData);
            shape->setQueryFilterData(qryFilterData);
        }
        //Add the mesh to the PhysX Scene
        mScene->addActor(*r);
        free(verts);
        free(indices32);
    }
    else //Dynamic
    {
        PxTransform t(PxVec3(DirectX::XMVectorGetX(T), DirectX::XMVectorGetY(T) + 0.09, DirectX::XMVectorGetZ(T)), PxQuat(DirectX::XMVectorGetX(R), DirectX::XMVectorGetY(R), DirectX::XMVectorGetZ(R), DirectX::XMVectorGetW(R)));
        PxVec3* verts = (PxVec3*)malloc(sizeof(PxVec3) * model->getNumVert());

        for (int i = 0; i < model->getNumVert(); i++)
        {
            XMVECTOR pos = XMVectorSet(model->getVertexBufferCPU()[i].pos.x, model->getVertexBufferCPU()[i].pos.y, model->getVertexBufferCPU()[i].pos.z, 1);
            verts[i].x = DirectX::XMVectorGetX(pos);
            verts[i].y = DirectX::XMVectorGetY(pos);
            verts[i].z = DirectX::XMVectorGetZ(pos);
        }

        //Create PhysX ConvexMesh
        PxConvexMesh* convMesh = createConvexMesh(verts, model->getNumVert(), *mPhysics, *gCooking);
        PxRigidDynamic* r = mPhysics->createRigidDynamic(t);
        PxShape* shape = PxRigidActorExt::createExclusiveShape(*r, PxConvexMeshGeometry(convMesh), *material);
        //Setup collision filters
        PxFilterData simFilterData(COLLISION_FLAG_OBSTACLE, COLLISION_FLAG_GROUND | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE, 0, 0);
        shape->setSimulationFilterData(simFilterData);
        //Add the mesh to the PhysX Scene
        model->instances[instance].dynamicPX = r;

        mScene->addActor(*r);
        free(verts);
    }
}
//void Scene::convertModelToPhysX(
//    std::shared_ptr<Model> model,
//    XMVECTOR rotation,
//    XMVECTOR translation,
//    PhysXMode type,
//    int instance)
//{
//    if (!model || !mPhysics || !gCooking || !mScene) {
//        std::cerr << "Error: Invalid physics objects" << std::endl;
//        return;
//    }
//
//    // Create material with properties based on type
//    PxMaterial* material = nullptr;
//
//    switch (type) {
//    case PhysXMode::Driveable:
//        material = mPhysics->createMaterial(0.6f, 0.4f, 0.6f);
//        break;
//    case PhysXMode::Static:
//    case PhysXMode::Dynamic:
//    default:
//        material = mPhysics->createMaterial(0.0f, 0.0f, 0.6f);
//        break;
//    }
//
//    if (!material) {
//        std::cerr << "Error: Failed to create PhysX material" << std::endl;
//        return;
//    }
//
//    if (type == PhysXMode::Static || type == PhysXMode::Driveable) {
//        createStaticPhysicsObject(model, material, type);
//    }
//    else if (type == PhysXMode::Dynamic) {
//        createDynamicPhysicsObject(model, material, rotation, translation, instance);
//    }
//}

void Scene::createStaticPhysicsObject(
    std::shared_ptr<Model> model,
    PxMaterial* material,
    PhysXMode type)
{
    const int vertexCount = model->getNumVert();
    const int faceCount = model->getNumFaces();

    // Allocate vertex buffer
    std::vector<PxVec3> vertices(vertexCount);

    // Transform vertices to world space
    XMMATRIX worldMatrix = model->getWorldMatrix();
    for (int i = 0; i < vertexCount; ++i) {
        const auto& vertex = model->getVertexBufferCPU()[i];
        XMVECTOR pos = XMVectorSet(vertex.pos.x, vertex.pos.y, vertex.pos.z, 1.0f);
        pos = XMVector3TransformCoord(pos, worldMatrix);

        vertices[i].x = XMVectorGetX(pos);
        vertices[i].y = XMVectorGetY(pos) - 0.25f;  // Ground offset
        vertices[i].z = XMVectorGetZ(pos);
    }

    // Build index buffer
    std::vector<PxU32> indices(model->getNumInd());
    int indexOffset = 0;

    for (int meshIdx = 0; meshIdx < model->getNumMeshes(); ++meshIdx) {
        const int indexCount = model->getIndexCount()[meshIdx];
        const int baseVertex = model->getBaseVertexOffset()[meshIdx];

        for (int i = 0; i < indexCount; ++i) {
            indices[indexOffset] = static_cast<PxU32>(
                model->getIndexBufferCPU()[indexOffset] + baseVertex
                );
            ++indexOffset;
        }
    }

    // Create PhysX triangle mesh
    PxTriangleMesh* triangleMesh = createTriangleMesh(
        vertices.data(),
        vertexCount,
        indices.data(),
        faceCount,
        *mPhysics,
        *gCooking
    );

    if (!triangleMesh) {
        std::cerr << "Error: Failed to create triangle mesh" << std::endl;
        return;
    }

    // Create static actor
    PxTransform transform(PxVec3(0, 0, 0), PxQuat(0, PxVec3(0, 1, 0)));
    PxRigidStatic* actor = mPhysics->createRigidStatic(transform);

    if (!actor) {
        std::cerr << "Error: Failed to create static actor" << std::endl;
        return;
    }

    // Create shape and attach to actor
    PxShape* shape = PxRigidActorExt::createExclusiveShape(
        *actor,
        PxTriangleMeshGeometry(triangleMesh),
        *material
    );

    // Setup collision filtering
    PxFilterData filterData(
        COLLISION_FLAG_OBSTACLE,
        COLLISION_FLAG_CHASSIS,
        0, 0
    );
    shape->setSimulationFilterData(filterData);

    // Make surface driveable if specified
    if (type == PhysXMode::Driveable) {
        PxFilterData queryFilter;
        setupDrivableSurface(queryFilter);
        shape->setQueryFilterData(queryFilter);
    }

    // Add to scene
    mScene->addActor(*actor);
}

void Scene::createDynamicPhysicsObject(
    std::shared_ptr<Model> model,
    PxMaterial* material,
    XMVECTOR rotation,
    XMVECTOR translation,
    int instance)
{
    const int vertexCount = model->getNumVert();

    // Create PhysX transform
    PxTransform transform(
        PxVec3(
            XMVectorGetX(translation),
            XMVectorGetY(translation) + 0.09f,
            XMVectorGetZ(translation)
        ),
        PxQuat(
            XMVectorGetX(rotation),
            XMVectorGetY(rotation),
            XMVectorGetZ(rotation),
            XMVectorGetW(rotation)
        )
    );

    // Extract vertices in local space (no world transform for dynamic objects)
    std::vector<PxVec3> vertices(vertexCount);
    for (int i = 0; i < vertexCount; ++i) {
        const auto& vertex = model->getVertexBufferCPU()[i];
        vertices[i] = PxVec3(vertex.pos.x, vertex.pos.y, vertex.pos.z);
    }

    // Create convex mesh (required for dynamic objects)
    PxConvexMesh* convexMesh = createConvexMesh(
        vertices.data(),
        vertexCount,
        *mPhysics,
        *gCooking
    );

    if (!convexMesh) {
        std::cerr << "Error: Failed to create convex mesh" << std::endl;
        return;
    }

    // Create dynamic actor
    PxRigidDynamic* actor = mPhysics->createRigidDynamic(transform);

    if (!actor) {
        std::cerr << "Error: Failed to create dynamic actor" << std::endl;
        return;
    }

    // Create shape
    PxShape* shape = PxRigidActorExt::createExclusiveShape(
        *actor,
        PxConvexMeshGeometry(convexMesh),
        *material
    );

    // Setup collision filtering for dynamic objects
    PxFilterData filterData(
        COLLISION_FLAG_OBSTACLE,
        COLLISION_FLAG_GROUND | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE,
        0, 0
    );
    shape->setSimulationFilterData(filterData);

    // Store reference to actor in model instance
    model->instances[instance].dynamicPX = actor;

    // Add to scene
    mScene->addActor(*actor);
}

// ============================================================================
// Scene Loading from XML
// ============================================================================

//void Scene::loadScene(
//    const std::string& path,
//    std::shared_ptr<Effect> effect,
//    float mapScale,
//    float LHCoords)
//{
//    m_sceneModels.clear();
//
//    ti::XMLDocument doc;
//    if (doc.LoadFile(path.c_str()) != ti::XML_SUCCESS) {
//        std::cerr << "Error loading XML: " << path << " - "
//            << doc.ErrorStr() << std::endl;
//        return;
//    }
//
//    ti::XMLElement* sceneRoot = doc.RootElement();
//    if (!sceneRoot) {
//        std::cerr << "Error: No root element in XML" << std::endl;
//        return;
//    }
//
//    ti::XMLElement* modelElement = sceneRoot->FirstChildElement("Model");
//    if (!modelElement) {
//        std::cout << "Warning: No m_sceneModels found in scene file" << std::endl;
//        return;
//    }
//
//    loadModel(modelElement, &m_sceneModels, effect, mapScale, LHCoords);
//
//    // Apply special configurations to specific m_sceneModels
//    applySpecialModelConfigurations(effect);
//
//    std::cout << "Loaded scene: " << path << " ("
//        << m_sceneModels.size() << " m_sceneModels)" << std::endl;
//}
// Load m_sceneModels from XML - demonstrates external asset configuration
void Scene::loadScene(const string& path, shared_ptr<Effect> effect, float mapScale, float LHCoords)
{
    m_sceneModels.clear();
    ti::XMLDocument doc;
    doc.LoadFile(path.c_str());
    if (doc.Error())
        std::cerr << "Error parsing XML: " << doc.ErrorStr() << std::endl;
    else
    {
        ti::XMLElement* scene = doc.RootElement();
        ti::XMLElement* model = scene->FirstChildElement("Model");
        m_sceneModels.clear();

        if (model) // Load m_sceneModels if available
            loadModel(model, &m_sceneModels, effect);

        // Special handling for specific model types
        if (m_sceneModels.size() > 1)
        {
            // Model[0] is assumed to be the Racing Line
            m_sceneModels[0]->getMaterial(0)->setEmissive(XMFLOAT4(1, 1, 1, 1));

            // Custom sampler for racing line arrows
            D3D11_SAMPLER_DESC linearDesc;
            ZeroMemory(&linearDesc, sizeof(D3D11_SAMPLER_DESC));
            linearDesc.Filter = D3D11_FILTER_ANISOTROPIC;
            linearDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            linearDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            linearDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            linearDesc.MaxAnisotropy = 8;
            linearDesc.MipLODBias = 0.0f;
            linearDesc.MinLOD = 0.0f;
            linearDesc.MaxLOD = 0.0f;
            linearDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

            ID3D11SamplerState* sampler;
            m_system->getDevice()->CreateSamplerState(&linearDesc, &sampler);
            m_sceneModels[0]->setSampler(sampler);

            // Model[1] is assumed to be the boundary fence
            shared_ptr<Effect> effectA2C = make_shared<Effect>(effect);
            effectA2C->setCullMode(m_system->getDevice(), D3D11_CULL_NONE);
            effectA2C->setAlphaToCoverage(m_system->getDevice(), TRUE);
            m_sceneModels[1]->setEffect(effectA2C);
        }
    }
}
void Scene::applySpecialModelConfigurations(std::shared_ptr<Effect> effect) {
    if (m_sceneModels.size() < 2) {
        return;
    }

    auto device = m_system->getDevice();

    // Model[0]: Racing line with emissive material
    if (m_sceneModels[0]) {
        m_sceneModels[0]->getMaterial(0)->setEmissive(XMFLOAT4(1, 1, 1, 1));

        // Create custom sampler for racing line texture
        D3D11_SAMPLER_DESC samplerDesc = {};
        samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MaxAnisotropy = 8;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = 0.0f;

        ID3D11SamplerState* sampler = nullptr;
        if (SUCCEEDED(device->CreateSamplerState(&samplerDesc, &sampler))) {
            m_sceneModels[0]->setSampler(sampler);
        }
    }

    // Model[1]: Boundary fence with alpha-to-coverage
    if (m_sceneModels[1]) {
        auto fenceEffect = std::make_shared<Effect>(*effect);
        fenceEffect->setCullMode(device, D3D11_CULL_NONE);
        fenceEffect->setAlphaToCoverage(device, TRUE);
        m_sceneModels[1]->setEffect(fenceEffect);
    }
}

void Scene::loadModel(ti::XMLElement* model, vector<shared_ptr<Model>>* modelList,
    shared_ptr<Effect> effect, float mapScale, float LHCoords)
{
    ID3D11DeviceContext* context = m_system->getDeviceContext();
    ID3D11Device* device = m_system->getDevice();

    while (model)
    {
        string path = model->FirstChildElement("PathName")->GetText();
        string data = model->FirstChildElement("Data")->GetText();
        fs::path pathName(path);
        shared_ptr<Material> mat(new Material(device));

        // Load position, rotation and scale from XML
        XMVECTOR pos = vecFromStr(data);
        pos = DirectX::XMVectorSetX(pos, DirectX::XMVectorGetX(pos) * mapScale);
        pos = DirectX::XMVectorSetZ(pos, DirectX::XMVectorGetZ(pos) * mapScale);
        pos = DirectX::XMVectorSetY(pos, m_terrain->CalculateYValueWorld(
            DirectX::XMVectorGetX(pos), DirectX::XMVectorGetZ(pos)) + DirectX::XMVectorGetY(pos));

        XMVECTOR rot = vecFromStr(data);
        XMVECTOR scale = vecFromStr(data);
        scale = DirectX::XMVectorScale(scale, mapScale);
        XMMATRIX rotM = XMMatrixRotationX(DirectX::XMVectorGetX(rot)) *
            XMMatrixRotationY(DirectX::XMVectorGetY(rot)) *
            XMMatrixRotationZ(DirectX::XMVectorGetZ(rot));

        // Get PhysX mode from XML
        ti::XMLElement* physXEl = model->FirstChildElement("PhysX");
        string physX = "No";
        if (physXEl) physX = physXEl->GetText();

        if (physX.compare("Dynamic") == 0)
        {
            // Dynamic m_sceneModels cannot be scaled in PhysX, so pre-transform vertices
            XMMATRIX PreTransScaleMat = XMMatrixScaling(DirectX::XMVectorGetX(scale),
                DirectX::XMVectorGetY(scale),
                DirectX::XMVectorGetZ(scale));

            shared_ptr<Model> mod(new Model(context, device,
                wstring(pathName.wstring().substr(3, path.length())), effect, nullptr, -1, &PreTransScaleMat));

            modelList->push_back(mod);
            (*modelList)[modelList->size() - 1]->setWorldMatrix(
                rotM * XMMatrixTranslation(DirectX::XMVectorGetX(pos),
                    DirectX::XMVectorGetY(pos),
                    DirectX::XMVectorGetZ(pos)));

            (*modelList)[modelList->size() - 1]->update(context);
            XMVECTOR Q = XMQuaternionRotationMatrix(rotM);

            // Create dynamic PhysX model
            convertModelToPhysX((*modelList)[modelList->size() - 1], Q, pos, PhysXMode::Driveable);
        }
        else
        {
            shared_ptr<Model> mod(new Model(context, device,
                wstring(pathName.wstring().substr(3, path.length())), effect, nullptr));

            modelList->push_back(mod);
            (*modelList)[modelList->size() - 1]->setWorldMatrix(
                rotM * XMMatrixScaling(DirectX::XMVectorGetX(scale),
                    DirectX::XMVectorGetY(scale),
                    DirectX::XMVectorGetZ(scale)) *
                XMMatrixTranslation(DirectX::XMVectorGetX(pos),
                    DirectX::XMVectorGetY(pos),
                    DirectX::XMVectorGetZ(pos)));

            (*modelList)[modelList->size() - 1]->update(context);

            if (physX.compare("Static") == 0)
                convertModelToPhysX((*modelList)[modelList->size() - 1], XMVectorZero(), XMVectorZero(), PhysXMode::Static);
            else if (physX.compare("Driveable") == 0)
                convertModelToPhysX((*modelList)[modelList->size() - 1], XMVectorZero(), XMVectorZero(), PhysXMode::Driveable);
        }

        cout << "Loaded:" << pathName.string() << endl;

        // Load instances if available
        ti::XMLElement* modelInstance = model->FirstChildElement("Instance");
        while (modelInstance)
        {
            data = modelInstance->FirstChildElement("Data")->GetText();
            XMVECTOR posI = vecFromStr(data);
            posI = DirectX::XMVectorSetX(posI, DirectX::XMVectorGetX(posI) * mapScale);
            posI = DirectX::XMVectorSetZ(posI, DirectX::XMVectorGetZ(posI) * mapScale);
            posI = DirectX::XMVectorSetY(posI, m_terrain->CalculateYValueWorld(
                DirectX::XMVectorGetX(posI), DirectX::XMVectorGetZ(posI)) + DirectX::XMVectorGetY(posI));

            XMVECTOR rotI = vecFromStr(data);
            XMVECTOR scaleI = vecFromStr(data);
            scaleI = DirectX::XMVectorScale(scaleI, mapScale);
            XMMATRIX rotMI = XMMatrixRotationX(DirectX::XMVectorGetX(rotI)) *
                XMMatrixRotationY(DirectX::XMVectorGetY(rotI)) *
                XMMatrixRotationZ(DirectX::XMVectorGetZ(rotI));

            // Currently, PhysX properties of instances are inherited from parent
            if (physX.compare("Dynamic") == 0)
            {
                XMVECTOR QI = XMQuaternionRotationMatrix(rotMI);
                (*modelList)[modelList->size() - 1]->instances.push_back(
                    Instance(rotMI * XMMatrixTranslation(DirectX::XMVectorGetX(posI),
                        DirectX::XMVectorGetY(posI),
                        DirectX::XMVectorGetZ(posI)),
                        (*modelList)[modelList->size() - 1]->getMaterials(0)));

                convertModelToPhysX((*modelList)[modelList->size() - 1], QI, posI, PhysXMode::Driveable,
                    (*modelList)[modelList->size() - 1]->instances.size() - 1);
            }
            else
            {
                (*modelList)[modelList->size() - 1]->instances.push_back(
                    Instance(rotMI * XMMatrixScaling(DirectX::XMVectorGetX(scaleI),
                        DirectX::XMVectorGetY(scaleI),
                        DirectX::XMVectorGetZ(scaleI)) *
                        XMMatrixTranslation(DirectX::XMVectorGetX(posI),
                            DirectX::XMVectorGetY(posI),
                            DirectX::XMVectorGetZ(posI)),
                        (*modelList)[modelList->size() - 1]->getMaterials(0)));

                if (physX.compare("Static") == 0)
                    convertModelToPhysX((*modelList)[modelList->size() - 1], XMVectorZero(), XMVectorZero(), PhysXMode::Static);
                else if (physX.compare("Driveable") == 0)
                    convertModelToPhysX((*modelList)[modelList->size() - 1], XMVectorZero(), XMVectorZero(), PhysXMode::Driveable);
            }
            modelInstance = modelInstance->NextSiblingElement("Instance");
        }
        model = model->NextSiblingElement("Model");
    }
}

//void Scene::loadModel(
//    ti::XMLElement* modelElement,
//    std::vector<std::shared_ptr<Model>>* modelList,
//    std::shared_ptr<Effect> effect,
//    float mapScale,
//    float LHCoords)
//{
//    auto context = m_system->getDeviceContext();
//    auto device = m_system->getDevice();
//
//    while (modelElement) {
//        // Parse model data from XML
//        ModelData modelData = parseModelXML(modelElement, mapScale);
//
//        // Determine physics mode
//        PhysXMode physicsMode = PhysXMode::Static;
//        std::string physicsStr = getXMLText(modelElement, "PhysX", "No");
//
//        if (physicsStr == "Dynamic") {
//            physicsMode = PhysXMode::Dynamic;
//        }
//        else if (physicsStr == "Driveable") {
//            physicsMode = PhysXMode::Driveable;
//        }
//
//        // Create and configure model
//        std::shared_ptr<Model> model = createModel(
//            context, device, modelData, effect, physicsMode
//        );
//
//        if (model) {
//            modelList->push_back(model);
//            std::cout << "Loaded: " << modelData.path << std::endl;
//
//            // Load instances if present
//            //loadModelInstances(modelElement, model, physicsMode, mapScale);
//        }
//
//        modelElement = modelElement->NextSiblingElement("Model");
//    }
//}

ModelData Scene::parseModelXML(ti::XMLElement* element, float mapScale) {
    ModelData data;

    // Get path
    data.path = getXMLText(element, "PathName", "");

    // Parse transform data
    std::string transformData = getXMLText(element, "Data", "");

    data.position = vecFromStr(transformData);
    data.position = XMVectorSetX(data.position, XMVectorGetX(data.position) * mapScale);
    data.position = XMVectorSetZ(data.position, XMVectorGetZ(data.position) * mapScale);

    // Adjust Y position based on terrain
    if (m_terrain) {
        float terrainHeight = m_terrain->CalculateYValueWorld(
            XMVectorGetX(data.position),
            XMVectorGetZ(data.position)
        );
        data.position = XMVectorSetY(data.position,
            terrainHeight + XMVectorGetY(data.position));
    }

    data.rotation = vecFromStr(transformData);
    data.scale = vecFromStr(transformData);
    data.scale = XMVectorScale(data.scale, mapScale);

    return data;
}

std::string Scene::getXMLText(
    ti::XMLElement* element,
    const char* childName,
    const char* defaultValue) const
{
    auto child = element->FirstChildElement(childName);
    if (child && child->GetText()) {
        return child->GetText();
    }
    return defaultValue;
}

std::shared_ptr<Model> Scene::createModel(
    ID3D11DeviceContext* context,
    ID3D11Device* device,
    const ModelData& data,
    std::shared_ptr<Effect> effect,
    PhysXMode physicsMode)
{
    // Extract filename from path
    fs::path pathName(data.path);
    std::wstring modelPath = pathName.wstring().substr(3);

    // Create material
    auto material = std::make_shared<Material>(device);

    // Calculate rotation matrix
    XMMATRIX rotationMatrix =
        XMMatrixRotationX(XMVectorGetX(data.rotation)) *
        XMMatrixRotationY(XMVectorGetY(data.rotation)) *
        XMMatrixRotationZ(XMVectorGetZ(data.rotation));

    std::shared_ptr<Model> model;

    if (physicsMode == PhysXMode::Dynamic) {
        // Dynamic objects: pre-apply scale to vertices
        XMMATRIX preScaleMatrix = XMMatrixScaling(
            XMVectorGetX(data.scale),
            XMVectorGetY(data.scale),
            XMVectorGetZ(data.scale)
        );

        model = std::make_shared<Model>(
            context, device, modelPath, effect, nullptr, -1, &preScaleMatrix
        );

        // Set transform (no scale since it's baked in)
        model->setWorldMatrix(
            rotationMatrix *
            XMMatrixTranslation(
                XMVectorGetX(data.position),
                XMVectorGetY(data.position),
                XMVectorGetZ(data.position)
            )
        );

        model->update(context);

        // Create PhysX representation
        XMVECTOR quaternion = XMQuaternionRotationMatrix(rotationMatrix);
        convertModelToPhysX(model, quaternion, data.position, physicsMode);
    }
    else {
        // Static objects: apply scale in transform
        model = std::make_shared<Model>(
            context, device, modelPath, effect, nullptr
        );

        model->setWorldMatrix(
            rotationMatrix *
            XMMatrixScaling(
                XMVectorGetX(data.scale),
                XMVectorGetY(data.scale),
                XMVectorGetZ(data.scale)
            ) *
            XMMatrixTranslation(
                XMVectorGetX(data.position),
                XMVectorGetY(data.position),
                XMVectorGetZ(data.position)
            )
        );

        model->update(context);

        // Create PhysX representation if needed
        if (physicsMode != PhysXMode::Static) {
            convertModelToPhysX(model, XMVectorZero(), XMVectorZero(), physicsMode);
        }
    }

    return model;
}

// Input handling methods
void Scene::handleMouseLDrag(const POINT& disp) {
    if (m_menuState != MenuDone)
    {
        m_menuKartRotation += -disp.x * 0.01f;
    }
    else if (m_mainCamera->getFlying())
    {
        m_mainCamera->elevate((float)-disp.y * 0.01f);
        m_mainCamera->turn((float)disp.x * 0.01f);
    }
}

void Scene::handleMouseWheel(const short zDelta) {
    if (m_menuState >= MenuDone && m_mainCamera->getFlying())
        m_mainCamera->move(-zDelta * 0.01);
}

void Scene::handleKeyDown(const WPARAM keyCode, const LPARAM extKeyFlags) {
    if (keyCode == 'f' || keyCode == 'F')
        m_mainCamera->toggleFlying();

    if (keyCode == 'w' || keyCode == 'W')
        if (m_mainCamera->getFlying())
            m_mainCamera->move(0.5);

    if (keyCode == 's' || keyCode == 'S')
        if (m_mainCamera->getFlying())
            m_mainCamera->move(-0.5);

    if (keyCode == 'i' || keyCode == 'I')
    {
        if (g_lightDistance > 1.28f)
            g_lightDistance = g_lightDistance / 2.0;
        cout << "LightPos: " << m_lightBufferCPU[0].lightVec.z << endl;
    }

    if (keyCode == 'l' || keyCode == 'L')
    {
        g_red += 0.4f;
        cout << "Life: " << g_red << endl;
        m_lightBufferCPU[1].lightAmbient.w = g_red;
    }

    if (keyCode == 'k' || keyCode == 'K')
    {
        if (g_red > 1.0f)
            g_red -= 0.4f;
        m_lightBufferCPU[1].lightAmbient.w = g_red;
        cout << "Life: " << g_red << endl;
    }

    if (keyCode == 'o' || keyCode == 'O')
    {
        if (g_lightDistance < 8.0f)
            g_lightDistance = g_lightDistance * 2.0;
        cout << "LightPos: " << m_lightBufferCPU[0].lightVec.z << endl;
    }

    if ((keyCode == 't' || keyCode == 'T'))
        g_useTSD *= -1;

    if (keyCode == VK_ESCAPE)
        m_menuState = MainMenu;

    if (keyCode == VK_PRIOR)
        m_playerKart->camMode = max(m_playerKart->camMode--, 0);

    if (keyCode == VK_NEXT)
        m_playerKart->camMode = min(m_playerKart->camMode++, 3);

    if (keyCode == VK_SPACE)
    {
        m_playerKart->restart(m_playerKart->getPolePosition());
        m_playerKart->setLapTime(0.0f);
    }

    if (keyCode == VK_HOME)
    {
        if (m_playerKart->getKartCC() < 2)
            m_playerKart->setKartCC(setCC(m_playerKart.get(), m_playerKart->getKartCC() + 1));
    }

    if (keyCode == VK_END)
    {
        if (m_playerKart->getKartCC() > 0)
            m_playerKart->setKartCC(setCC(m_playerKart.get(), m_playerKart->getKartCC() - 1));
    }
}

void Scene::handleKeyUp(const WPARAM keyCode, const LPARAM extKeyFlags) {
    // Key up handling
}

// Quality settings adjustment based on m_menu selection
void Scene::updateRenderQuality()
{
    m_fixedTimeStep = 1.0f / 60.0f;

    if (m_menu->getQuality() == RenderQuality::Lowest)
    {
        m_fixedTimeStep = 1.0f / 25.0f;
        m_grassRenderPasses = 1;
        m_scenePhysicsUpdates = 1;
        m_kartPhysicsUpdates = 2;
    }
    else if (m_menu->getQuality() == RenderQuality::Low)
    {
        m_fixedTimeStep = 1.0f / 30.0f;
        m_grassRenderPasses = 3;
        m_scenePhysicsUpdates = 2;
    }
    else if (m_menu->getQuality() == RenderQuality::Medium)
    {
        m_fixedTimeStep = 1.0f / 30.0f;
        m_grassRenderPasses = 5;
        m_scenePhysicsUpdates = 2;
    }
    else if (m_menu->getQuality() == RenderQuality::High)
    {
        m_fixedTimeStep = 1.0f / 50.0f;
        m_grassRenderPasses = 10;
        m_scenePhysicsUpdates = 2;
    }
    else if (m_menu->getQuality() == RenderQuality::Highest)
    {
        m_fixedTimeStep = 1.0f / 60.0f;
        m_grassRenderPasses = 20;
        m_scenePhysicsUpdates = 2;
    }

    // Update shadow map size based on quality
    if (m_sceneBufferCPU->QUALITY != m_menu->getQuality())
    {
        m_sceneBufferCPU->QUALITY = m_menu->getQuality();
        mapCbuffer(m_system->getDeviceContext(), m_sceneBufferCPU.get(), m_sceneBufferGPU.Get(), sizeof(CBufferScene));

        int shadowMapWidth = 2048;
        if (m_menu->getQuality() == RenderQuality::Medium)
            shadowMapWidth = 4096;
        else if (m_menu->getQuality() > RenderQuality::Medium)
            shadowMapWidth = 8192;

        m_shadowMap->setMapSize(m_system->getDevice(), shadowMapWidth);
    }
}



// Main game loop implementation
HRESULT Scene::updateAndRenderScene() {
    HRESULT hr = S_OK;
    ID3D11DeviceContext* context = m_system->getDeviceContext();

    if (m_menuState < MenuDone)
    {
        // Rotate kart for m_menu display
        m_menuKartRotation += 0.01;
        PxQuat Q(m_menuKartRotation, PxVec3(0, 1, 0));
        physx::PxTransform startTransform(physx::PxVec3(0, -0.9f, 0), Q);
        static physx::PxTransform gameTransform = m_playerKart->getVehicle4W()->getRigidDynamicActor()->getGlobalPose();

        m_playerKart->getVehicle4W()->getRigidDynamicActor()->setGlobalPose(startTransform);
        m_playerKart->update(context, m_fixedTimeStep);

        // Show main m_menu
        m_menu->renderMainMenu(m_playerKart.get(), &m_menuState, m_fixedTimeStep);
        updateRenderQuality();

        m_playerKart->getVehicle4W()->getRigidDynamicActor()->setGlobalPose(gameTransform);
    }
    else if (m_menuState >= MenuDone)
    {
        hr = updateScene(context, m_mainCamera.get());
        if (SUCCEEDED(hr))
            hr = renderScene();
    }

    return hr;
}
// Main scene update with physics and animation
HRESULT Scene::updateScene(ID3D11DeviceContext* context, FirstPersonCamera* camera) {
    // Update clock
    m_mainClock->tick();
    double dT = m_mainClock->gameTimeDelta();
    double gT = m_mainClock->gameTimeElapsed();
    m_accumulator += dT;
    
    // Fixed timestep implementation
    if (m_accumulator < m_fixedTimeStep)
    {
        return S_OK;
    }
 
    m_shouldRenderFrame = true;
    m_accumulator = fmod(m_accumulator, m_fixedTimeStep);

    // Update scene time for animations
    m_sceneBufferCPU->Time = gT;
    mapCbuffer(context, m_sceneBufferCPU.get(), m_sceneBufferGPU.Get(), sizeof(CBufferScene));

    // Start AI karts with delays
    for (int i = 0; i < NUM_AI_VEHICLES; i++)
        if (gT > i * 8 + 5) m_aiKarts[i]->setStarted(true);

    // Update physics with multiple steps for stability
    for (int i = 0; i < m_physicsUpdatesPerFrame; i++) {
        for (int i = 0; i < m_kartPhysicsUpdates; i++)
            m_vehicleController->stepVehicles(m_fixedTimeStep * 1.6f / (m_physicsUpdatesPerFrame * m_kartPhysicsUpdates), m_allKarts.data(), NUM_VEHICLES);

        for (int i = 0; i < m_scenePhysicsUpdates; i++)
            m_physicsScene->step(m_fixedTimeStep * 1.6f / (m_physicsUpdatesPerFrame * m_scenePhysicsUpdates));
    }

    // Update kart graphics
    m_playerKart->update(context, m_fixedTimeStep);
    for (int i = 0; i < NUM_AI_VEHICLES; i++)
        m_aiKarts[i]->update(context, m_fixedTimeStep);

    // Update PhysX boxes graphics
    for (int i = 0; i < NUM_PHYSICS_BOXS; i++)
    {
        physx::PxTransform boxT = body[i]->getGlobalPose();
        XMVECTOR quart = DirectX::XMLoadFloat4(&XMFLOAT4(boxT.q.x, boxT.q.y, boxT.q.z, boxT.q.w));
        m_boxTransforms[i] = XMMatrixScaling(PHYSICS_BOX_HALF_SIZE, PHYSICS_BOX_HALF_SIZE, PHYSICS_BOX_HALF_SIZE) *
            XMMatrixRotationQuaternion(quart) *
            XMMatrixTranslation(boxT.p.x * 1.0, boxT.p.y + 0.2, boxT.p.z * 1.0);
    }

    // Update lap timer
    for (int i = 0; i < NUM_VEHICLES; i++)
        m_allKarts[i]->updateLapTimes(m_fixedTimeStep, START_LINE_DISTANCE, FINISH_LINE_DISTANCE, START_FINISH_POSITION);

    // Update player camera
    if (!m_mainCamera->getFlying())
        updatePlayerCamera(context, camera);

    // Update m_dragonModel animation and movement
    float r = 0;
    if (m_dragonModel->getCurrentAnim() == 2) r = -0.4 * m_fixedTimeStep;
    else if (m_dragonModel->getCurrentAnim() == 3) r = -0.2 * m_fixedTimeStep;
    else r = -0.2 * m_fixedTimeStep;

    m_dragonModel->setWorldMatrix(m_dragonModel->getWorldMatrix() *
        XMMatrixTranslation(20, 0, 30) *
        XMMatrixRotationY(r) *
        XMMatrixTranslation(-20, 0, -30));

    XMVECTOR dragonPos = XMVectorZero();
    dragonPos = XMVector3TransformCoord(dragonPos, m_dragonModel->getWorldMatrix());
    float dragonHeight = m_terrain->CalculateYValueWorld(DirectX::XMVectorGetX(dragonPos), DirectX::XMVectorGetZ(dragonPos));

    m_dragonModel->setWorldMatrix(m_dragonModel->getWorldMatrix() *
        XMMatrixTranslation(0, dragonHeight - DirectX::XMVectorGetY(dragonPos), 0));

    m_dragonModel->update(context);
    m_dragonModel->updateBones(gT);

    // Update Nathan animation
    m_nathanModel->setWorldMatrix(m_nathanModel->getWorldMatrix() *
        XMMatrixTranslation(0, 0, 0) *
        XMMatrixRotationY(r) *
        XMMatrixTranslation(0, 0, 0));

    m_nathanModel->update(context);
    m_nathanModel->updateBonesSubFrames(19, 52, gT);

    // Update Sophia animation
    m_sophiaModel->update(context);
    m_sophiaModel->updateBones(gT);

    // Time of day lighting changes
    float tod = sin(m_mainClock->gameTimeElapsed() / 20.0f) / 2 + 0.5f;
    float todBlue = tod * 0.5 + 0.5;

    m_lightBufferCPU[0].lightAmbient = XMFLOAT4(0.3 * tod, 0.3 * tod, 0.3 * todBlue, 1.0);
    m_lightBufferCPU[0].lightDiffuse = XMFLOAT4(0.8 * tod, 0.8 * tod, 1.0 * todBlue, 1.0);
    m_lightBufferCPU[0].lightSpecular = XMFLOAT4(0.8 * tod, 0.8 * tod, 1.0 * todBlue, 1.0);

    // Animate light position
    XMMATRIX rotM = XMMatrixRotationZ(gT) * XMMatrixTranslation(0, 5, 0);
    m_lightBufferCPU[1].lightVec.x = g_lightDistance * 0.625;
    m_lightBufferCPU[1].lightVec.z = g_lightDistance;
    m_lightBufferCPU[1].lightVec.y = 0;

    XMVECTOR lightVec = DirectX::XMLoadFloat4(&(m_lightBufferCPU[1].lightVec));
    lightVec = DirectX::XMVector3TransformCoord(lightVec, rotM);
    DirectX::XMStoreFloat4(&(m_lightBufferCPU[1].lightVec), lightVec);

    m_debugOrb->setWorldMatrix(XMMatrixScaling(0.5, 0.5, 0.5) *
        XMMatrixTranslation(m_lightBufferCPU[1].lightVec.x,
            m_lightBufferCPU[1].lightVec.y,
            m_lightBufferCPU[1].lightVec.z));

    m_debugOrb->update(context);

    // Update light positions based on kart positions
    physx::PxTransform trans = m_aiKarts[NUM_AI_VEHICLES - 1]->getVehicle4W()->getRigidDynamicActor()->getGlobalPose();
    trans.p.y += 0.05;

    m_lightBufferCPU[2].lightVec.x = trans.p.x;
    m_lightBufferCPU[2].lightVec.y = trans.p.y + 1;
    m_lightBufferCPU[2].lightVec.z = trans.p.z;

    // Update GPU light buffer
    mapCbuffer(context, m_lightBufferCPU.get(), m_lightBufferGPU.Get(), sizeof(CBufferLight) * NUM_LIGHTS_ACTIVE);

    return S_OK;
}
// Camera update following player kart
void Scene::updatePlayerCamera(ID3D11DeviceContext* context, FirstPersonCamera* camera)
{
    physx::PxTransform trans = m_playerKart->getVehicle4W()->getRigidDynamicActor()->getGlobalPose();
    trans.p.y += 0.05;

    if (m_playerKart->camMode == 3) // Birds eye view
    {
        // Top down view
        camera->setPos(XMVectorSet(trans.p.x, 15, trans.p.z, 1.0));
        camera->setLookAt(XMVectorSet(trans.p.x, 0, trans.p.z, 1));
        camera->setUp(XMVectorSet(0, 0, 1, 1));
    }
    else {
        XMVECTOR quart = DirectX::XMLoadFloat4(&XMFLOAT4(trans.q.x, trans.q.y, trans.q.z, trans.q.w));
        XMVECTOR campos = XMVectorSet(trans.p.x * 1.0, trans.p.y * 1.0 + 2.0f, trans.p.z * 1.0, 1.0);
        XMVECTOR dir = XMVectorSet(0, 0, 1.0, 1);
        static XMVECTOR oldDir = dir;
        dir = DirectX::XMVector4Transform(dir, XMMatrixRotationQuaternion(quart));
        static XMVECTOR oldcampos = camera->getPos();
        XMVECTOR target;
        float timeScale = m_fixedTimeStep * 60.0f;

        if (m_playerKart->camMode == 0) // First Person Camera
        {
            // Lerp between old and new camera positions for smooth movement
            float currentWeightDIR = 0.5 * timeScale;
            float currentWeightPOS = 0.7 * timeScale;
            target = DirectX::XMVectorSubtract(campos, DirectX::XMVectorScale(dir, 0.6f));
            dir = DirectX::XMVectorAdd(DirectX::XMVectorScale(dir, currentWeightDIR),
                DirectX::XMVectorScale(oldDir, 1.0f - currentWeightDIR));

            oldDir = dir;
            camera->setPos(DirectX::XMVectorAdd(DirectX::XMVectorScale(target, currentWeightPOS),
                DirectX::XMVectorScale(oldcampos, 1.0f - currentWeightPOS)));

            campos = XMVectorSet(trans.p.x, trans.p.y + 1.0f, trans.p.z, 1.0);
            camera->setHeight(DirectX::XMVectorGetY(campos));
            campos = DirectX::XMVectorSetY(campos, DirectX::XMVectorGetY(campos) - 1);
            camera->setLookAt(DirectX::XMVectorAdd(campos, DirectX::XMVectorScale(dir, 6.0)));
            camera->setUp(XMVectorSet(0, 1, 0, 1));
        }
        else if (m_playerKart->camMode == 1) // Third Person Camera close
        {
            float currentWeightDIR = 0.1 * timeScale;
            float currentWeightPOS = 0.1 * timeScale;
            target = DirectX::XMVectorSubtract(campos, DirectX::XMVectorScale(dir, 2.0f));
            dir = DirectX::XMVectorAdd(DirectX::XMVectorScale(dir, currentWeightDIR),
                DirectX::XMVectorScale(oldDir, 1.0f - currentWeightDIR));

            oldDir = dir;
            camera->setPos(DirectX::XMVectorAdd(DirectX::XMVectorScale(target, currentWeightPOS),
                DirectX::XMVectorScale(oldcampos, 1.0f - currentWeightPOS)));

            campos = XMVectorSet(trans.p.x, trans.p.y + 1.0f, trans.p.z, 1.0);
            camera->setHeight(DirectX::XMVectorGetY(campos));
            camera->setLookAt(campos);
            camera->setUp(XMVectorSet(0, 1, 0, 1));
        }
        else if (m_playerKart->camMode == 2) // Third Person Camera far
        {
            float currentWeightDIR = 0.05 * timeScale;
            float currentWeightPOS = 0.05 * timeScale;
            target = DirectX::XMVectorSubtract(campos, DirectX::XMVectorScale(dir, 2.0f));
            dir = DirectX::XMVectorAdd(DirectX::XMVectorScale(dir, currentWeightDIR),
                DirectX::XMVectorScale(oldDir, 1.0f - currentWeightDIR));

            oldDir = dir;
            camera->setPos(DirectX::XMVectorAdd(DirectX::XMVectorScale(target, currentWeightPOS),
                DirectX::XMVectorScale(oldcampos, 1.0f - currentWeightPOS)));

            campos = XMVectorSet(trans.p.x, trans.p.y + 1.0f, trans.p.z, 1.0);
            camera->setHeight(DirectX::XMVectorGetY(campos));
            camera->setLookAt(campos);
            camera->setUp(XMVectorSet(0, 1, 0, 1));
        }
        oldcampos = camera->getPos();
    }
    camera->update(context);
}
// Main resource setup for the application
HRESULT Scene::initialiseSceneResources() {
    ID3D11DeviceContext* context = m_system->getDeviceContext();
    ID3D11Device* device = m_system->getDevice();
    if (!device)
        return E_FAIL;

    // Set up m_viewport for the main window
    rebuildViewport();

    // Draw intro screen while loading
    if (m_menuState == Intro)
    {
        m_menu = std::make_unique<Menu>(m_system.get());
        m_menu->init(&m_networkManager);
        m_menu->renderIntro();
    }

    // Initialize FMOD audio m_system
    FMOD_RESULT result = FMOD::System_Create(&m_audioSystem);
    if (result != FMOD_OK)
    {
        m_audioSystem->release();
        delete m_audioSystem;
        m_audioSystem = nullptr;
        cout << "cant create sound m_system" << endl;
    }

    result = m_audioSystem->init(32, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK)
    {
        m_audioSystem->release();
        delete m_audioSystem;
        m_audioSystem = nullptr;
        cout << "cant init sound" << endl;
    }

    // Initialize PhysX
    m_physicsScene = std::make_unique < PhysXKarting>();

    // Create shadow map
    XMVECTOR lightVec = XMVectorSet(-124.0, 50, 60, 1.0);
    m_shadowMap = std::make_unique < ShadowMap>(device, lightVec, 4096);

    // Initialize post-processing effects
    m_bloomEffect = std::make_unique<BlurUtility>(m_system->getDevice(), context, 300, 240);
    m_subsurfaceScattering = std::make_unique <SkinUtility>(m_system->getDevice(), context, 1068, 712);

    // Setup main rendering effects
    shared_ptr<Effect> perPixelLightingEffect(new Effect(device, "Shaders\\cso\\per_pixel_lighting_vs.cso",
        "Shaders\\cso\\per_pixel_lighting_ps.cso",
        extVertexDesc, ARRAYSIZE(extVertexDesc)));

    shared_ptr<Effect> reflectionMappingEffect(new Effect(device, "Shaders\\cso\\reflection_map_vs.cso",
        "Shaders\\cso\\reflection_map_ps.cso",
        extVertexDesc, ARRAYSIZE(extVertexDesc)));

    // SkyBox effect with specialized render states
    shared_ptr<Effect> skyBoxEffect(new Effect(device, "Shaders\\cso\\sky_box_vs.cso",
        "Shaders\\cso\\sky_box_ps.cso",
        extVertexDesc, ARRAYSIZE(extVertexDesc)));
    skyBoxEffect->setCullMode(device, D3D11_CULL_FRONT);
    skyBoxEffect->setDepthFunction(device, D3D11_COMPARISON_LESS_EQUAL);

    // Skin rendering effect
    m_skinEffect = make_shared<Effect>(device, "Shaders\\cso\\per_pixel_lighting_vs.cso",
        "Shaders\\cso\\basic_skin_ps.cso",
        extVertexDesc, ARRAYSIZE(extVertexDesc));

    // Face texture setup
    Texture* faceTexture = new Texture(device, L"..\\Resources\\Textures\\Face\\IMG_9158ds.png");
    Texture* faceSpecTexture = new Texture(device, L"..\\Resources\\Textures\\Face\\IMG_9158s.png");
    Texture* faceNormTexture = new Texture(device, L"..\\Resources\\Textures\\Face\\IMG_9158ns15f9ix.png");

    ID3D11ShaderResourceView* faceTextureArray[] = { faceTexture->getShaderResourceView(),
                                                    faceNormTexture->getShaderResourceView(),
                                                    faceSpecTexture->getShaderResourceView() };

    shared_ptr<Material> skinMat(new Material(device));
    skinMat->setSpecular(XMFLOAT4(1.0, 1.0, 1.0, 1.0));
    skinMat->setTextures(faceTextureArray, 3);
    m_skinEffect->setCullMode(device, D3D11_CULL_NONE);

    m_faceModel = make_unique<Model>(context, device, wstring(L"..\\Resources\\Models\\Face\\Peter2.3ds"), m_skinEffect, skinMat);
    m_faceModel->setWorldMatrix(XMMatrixScaling(0.03, 0.03, 0.03) *
        XMMatrixRotationZ(XMConvertToRadians(-2)) *
        XMMatrixTranslation(-0.3, 5.2, 1));
    m_faceModel->update(context);

    // Setup cube environment texture
    m_cubeDayTexture = make_unique<Texture>(device, L"..\\Resources\\Textures\\grassenvmap1024.dds");
    ID3D11ShaderResourceView* cubeDayTextureSRV = m_cubeDayTexture->getShaderResourceView();

    // Bind cube texture to shader slot 6 (used by multiple effects)
    context->PSSetShaderResources(6, 1, &cubeDayTextureSRV);

    // Create skybox
    shared_ptr<Material> skyBoxMaterial(new Material(device));
    skyBoxMaterial->setTexture(Texture(device, L"..//Resources\\Textures\\nightenvmap1024.dds").getShaderResourceView());
    m_skyBox = make_unique< Box>(device, skyBoxEffect, skyBoxMaterial);
    m_skyBox->setWorldMatrix(m_skyBox->getWorldMatrix() * XMMatrixScaling(1000, 1000, 1000));
    m_skyBox->update(context);

    // Setup m_water rendering
    m_dynamicCubeMap = make_unique<DynamicCube>(device, context, XMVectorSet(-54.0, 1.0, 66.0, 1), 512);
    m_dynamicCubeMap->updateCubeCameras(context, XMVectorSet(-54.0, 0.0, 66.0, 1));
    ID3D11ShaderResourceView* dynamicCubeMapSRV = m_dynamicCubeMap->getSRV();
    context->PSSetShaderResources(6, 1, &dynamicCubeMapSRV);

    auto waterMaterial = std::make_shared<Material>(device);
    waterMaterial->setTexture(Texture(device, L"..\\Resources\\Textures\\Waves.dds").getShaderResourceView());

    shared_ptr<Effect> waterEffect(new Effect(device, "Shaders\\cso\\ocean_vs.cso",
        "Shaders\\cso\\ocean_ps.cso",
        extVertexDesc, ARRAYSIZE(extVertexDesc)));

    m_water = make_unique<Grid>(10, 10, device, waterEffect, waterMaterial);
    m_water->setWorldMatrix(XMMatrixScaling(3.0, 0.3, 3.0) *
        XMMatrixTranslation(-68.0, 0.9, 54.0));
    m_water->update(context);

    // Create box material for PhysX boxes
    shared_ptr<Material> boxMat(new Material(device, XMFLOAT4(-1.0, -1.0, 1.0, 1.0)));
    boxMat->setSpecular(XMFLOAT4(0.3, 0.3, 0.3, 0.01));
    boxMat->setUsage(DIFFUSE_MAP);
    boxMat->setTexture(Texture(device, L"..\\Resources\\Textures\\WoodCrate02.dds").getShaderResourceView());

    m_physicsBox = make_unique<Box>(device, perPixelLightingEffect, boxMat);
    //m_boxTransforms = make_unique<XMMATRIX[S];

    // Foliage and terrain setup
    shared_ptr<Effect> grassEffect(new Effect(device, "Shaders\\cso\\grass_vs.cso",
        "Shaders\\cso\\grass_ps.cso",
        extVertexDesc, ARRAYSIZE(extVertexDesc)));

    // Alpha blending for m_terrain
    grassEffect->setAlphaBlendEnable(device, TRUE);

    // Load m_terrain textures
    Texture grassAlpha = Texture(device, L"..\\Resources\\Textures\\grassAlpha.tif");
    Texture grassDiffuse = Texture(device, L"..\\Resources\\Textures\\BrightonKarting5.bmp");
    Texture grassColour = Texture(device, L"..\\Resources\\Textures\\grass_texture.dds");
    Texture groundNormals = Texture(device, L"..\\Resources\\Textures\\Seamless_Asphalt_Texture_NORMAL.jpg");
    Texture asphaltDiffuse = Texture(device, L"..\\Resources\\Textures\\asphalt.dds");
    Texture heightMap = Texture(device, L"..\\Resources\\Levels\\Terrain01.bmp");
    Texture noiseMap = Texture(device, L"..\\Resources\\Levels\\noise.bmp");

    ID3D11ShaderResourceView* noise = noiseMap.getShaderResourceView();
    context->PSSetShaderResources(8, 1, &noise);
    context->VSSetShaderResources(8, 1, &noise);

    ID3D11ShaderResourceView* grassTextureArray[] = {
        grassDiffuse.getShaderResourceView(),
        asphaltDiffuse.getShaderResourceView(),
        grassAlpha.getShaderResourceView(),
        grassColour.getShaderResourceView(),
        groundNormals.getShaderResourceView(),
        noiseMap.getShaderResourceView()
    };

    shared_ptr<Material> grassMaterial(new Material(device));
    grassMaterial->setTextures(grassTextureArray, 6);

    // Create grassy terrain
    float terrainOffsetXZ = -(m_terrainResolution * m_terrainScaleXZ) / 2.0f;
    m_terrain = make_unique<Terrain>(device, context, m_terrainResolution, m_terrainResolution,
        heightMap.getTexture(), grassEffect, grassMaterial);

    m_terrain->setWorldMatrix(XMMatrixScaling(m_terrainScaleXZ, m_terrainScaleY, m_terrainScaleXZ) *
        XMMatrixTranslation(terrainOffsetXZ, -0.085, terrainOffsetXZ));
    m_terrain->update(context);
    m_terrain->setColourMap(device, context, grassDiffuse.getTexture());

    // Load AI Kart navigation points
    m_navigationPoints = make_unique<NavPoints>();
    m_navigationPoints->load("..\\Resources\\Levels\\NavSet01.xml");

    // Load orb for rendering NavPoints
    shared_ptr<Material> beeMat(new Material(device));
    shared_ptr<Material> redMat(new Material(device, XMFLOAT4(1.0, 0.0, 0.0, 1.0)));
    redMat->setDiffuse(XMFLOAT4(1.0, 0.0, 0.0, 1.0));
    redMat->setSpecular(XMFLOAT4(1.0, 0.0, 0.0, 1.0));

    beeMat->setTexture(Texture(device, L"..\\Resources\\Textures\\orb.tif").getShaderResourceView());
    beeMat->setUsage(EMISSIVE_MAP);

    shared_ptr<Effect> emissiveEffect(new Effect(device, "Shaders\\cso\\per_pixel_lighting_vs.cso",
        "Shaders\\cso\\emissive_ps.cso",
        extVertexDesc, ARRAYSIZE(extVertexDesc)));

    m_debugOrb = make_unique< Model>(context, device, wstring(L"..\\Resources\\Models\\sphere.obj"), emissiveEffect, beeMat, 0);

    XMMATRIX beeInstanceMat = XMMatrixScaling(0.025, 0.025, 0.025) *
        XMMatrixTranslation(4.13, 0, 58.30);

    m_debugOrb->instances.push_back(Instance(beeInstanceMat, beeMat));

    XMMATRIX orbInstanceMat = XMMatrixScaling(0.5, 0.5, 0.5) *
        XMMatrixTranslation(0, m_terrain->CalculateYValueWorld(0, 0) + 5.0, 0);

    m_debugOrb->instances.push_back(Instance(orbInstanceMat, redMat));

    // Skinning effect for animated m_sceneModels
    shared_ptr<Effect> skinningEffect(new Effect(device, "Shaders\\cso\\skinning_vs.cso",
        "Shaders\\cso\\per_pixel_lighting_ps.cso",
        skinVertexDesc, ARRAYSIZE(skinVertexDesc)));

    // Load m_dragonModel model with animations
    Texture dragonNormalTexture = Texture(device, L"..\\Resources\\Models\\Black Dragon NEW\\textures\\Dragon_Nor.jpg");
    Texture dragonTexture = Texture(device, L"..\\Resources\\Models\\Black Dragon NEW\\textures\\Dragon_ground_color.jpg");

    ID3D11ShaderResourceView* dragonTextureArray[] = {
        dragonTexture.getShaderResourceView(),
        dragonNormalTexture.getShaderResourceView()
    };

    shared_ptr<Material> dragonMat(new Material(device, XMFLOAT4(1.0, 1.0, 1.0, 1.0)));
    dragonMat->setSpecular(XMFLOAT4(0.3, 0.3, 0.3, 0.01));
    dragonMat->setUsage(DIFFUSE_MAP | NORMAL_MAP);
    dragonMat->setTextures(dragonTextureArray, 2);

    m_dragonModel=make_unique<SkinnedModel>(context, device,
        wstring(L"..\\Resources\\Models\\Black Dragon NEW\\Dragon_Baked_Actions_fbx_7.4_binary.fbx"),
        skinningEffect, dragonMat);

    m_dragonModel->loadBones(device);
    m_dragonModel->setWorldMatrix(m_dragonModel->getWorldMatrix() *
        XMMatrixScaling(0.001, 0.001, 0.001) *
        XMMatrixTranslation(10, 0, 0) *
        XMMatrixTranslation(-20, 0 + m_terrain->CalculateYValueWorld(-20, -30), -30));

    m_dragonModel->setCurrentAnim(3);
    m_dragonModel->update(context);

    // Load Nathan character model
    Texture nathanTextureBlack = Texture(device, L"..\\Resources\\Models\\55-rp_nathan_animated_003_walking_fbx\\tex\\rp_nathan_animated_003_dif_black.jpg");
    Texture nathanTextureRed = Texture(device, L"..\\Resources\\Models\\55-rp_nathan_animated_003_walking_fbx\\tex\\rp_nathan_animated_003_dif_red.jpg");
    Texture nathanTextureBlue = Texture(device, L"..\\Resources\\Models\\55-rp_nathan_animated_003_walking_fbx\\tex\\rp_nathan_animated_003_dif_blue.jpg");
    Texture nathanNormalTexture = Texture(device, L"..\\Resources\\Models\\55-rp_nathan_animated_003_walking_fbx\\tex\\rp_nathan_animated_003_norm_small.dds");

    ID3D11ShaderResourceView* nathanTextureArrayBlack[] = {
        nathanTextureBlack.getShaderResourceView(),
        nathanNormalTexture.getShaderResourceView()
    };

    ID3D11ShaderResourceView* nathanTextureArrayRed[] = {
        nathanTextureRed.getShaderResourceView(),
        nathanNormalTexture.getShaderResourceView()
    };

    ID3D11ShaderResourceView* nathanTextureArrayBlue[] = {
        nathanTextureBlue.getShaderResourceView(),
        nathanNormalTexture.getShaderResourceView()
    };

    shared_ptr<Material> nathanBlackMat(new Material(device, XMFLOAT4(1.0, 1.0, 1.0, 1.0)));
    nathanBlackMat->setSpecular(XMFLOAT4(0.1, 0.1, 0.1, 0.001));
    nathanBlackMat->setUsage(DIFFUSE_MAP | NORMAL_MAP);

    shared_ptr<Material> nathanRedMat(new Material(device, nathanBlackMat));
    nathanRedMat->setTextures(nathanTextureArrayRed, 2);

    shared_ptr<Material> nathanBlueMat(new Material(device, nathanBlackMat));
    nathanBlueMat->setTextures(nathanTextureArrayBlue, 2);

    nathanBlackMat->setTextures(nathanTextureArrayBlack, 2);

    m_nathanModel = make_unique<SkinnedModel>(context, device,
        wstring(L"..\\Resources\\Models\\55-rp_nathan_animated_003_walking_fbx\\rp_nathan_animated_003_walking2.fbx"),
        skinningEffect, nathanBlackMat);

    m_nathanModel->loadBones(device);

    m_nathanModel->instances.push_back(Instance(m_nathanModel->getWorldMatrix() *
        XMMatrixScaling(0.022, 0.022, 0.022) *
        XMMatrixRotationY(XMConvertToRadians(180)) *
        XMMatrixTranslation(8.635422, 0 + m_terrain->CalculateYValueWorld(8.635422, 53.300213), 53.300213),
        nathanBlueMat));

    m_nathanModel->instances.push_back(Instance(m_nathanModel->getWorldMatrix() *
        XMMatrixScaling(0.022, 0.022, 0.022) *
        XMMatrixRotationY(XMConvertToRadians(180)) *
        XMMatrixTranslation(9.635422, 0 + m_terrain->CalculateYValueWorld(9.635422, 53.300213), 53.300213),
        nathanRedMat));

    m_nathanModel->setWorldMatrix(m_nathanModel->getWorldMatrix() *
        XMMatrixScaling(0.022, 0.022, 0.022) *
        XMMatrixTranslation(5, 0, 0) *
        XMMatrixTranslation(0, m_terrain->CalculateYValueWorld(0, 0), 0));

    m_nathanModel->setCurrentAnim(0);
    m_nathanModel->update(context);

    // Load Sophia character model
    Texture sophiaTextureYellow = Texture(device, L"..\\Resources\\Models\\35-rp_sophia_animated_003_idling_fbx\\tex\\rp_sophia_animated_003_dif_yellow.jpg");
    Texture sophiaTextureWhite = Texture(device, L"..\\Resources\\Models\\35-rp_sophia_animated_003_idling_fbx\\tex\\rp_sophia_animated_003_dif.jpg");
    Texture sophiaTexturePink = Texture(device, L"..\\Resources\\Models\\35-rp_sophia_animated_003_idling_fbx\\tex\\rp_sophia_animated_003_dif_pink.jpg");
    Texture sophiaNormalTexture = Texture(device, L"..\\Resources\\Models\\35-rp_sophia_animated_003_idling_fbx\\tex\\rp_sophia_animated_003_norm_small.dds");

    ID3D11ShaderResourceView* sophiaTextureArrayYellow[] = {
        sophiaTextureYellow.getShaderResourceView(),
        sophiaNormalTexture.getShaderResourceView()
    };

    ID3D11ShaderResourceView* sophiaTextureArrayWhite[] = {
        sophiaTextureWhite.getShaderResourceView(),
        sophiaNormalTexture.getShaderResourceView()
    };

    ID3D11ShaderResourceView* sophiaTextureArrayPink[] = {
        sophiaTexturePink.getShaderResourceView(),
        sophiaNormalTexture.getShaderResourceView()
    };

    shared_ptr<Material> sophiaMatYellow(new Material(device, XMFLOAT4(1.0, 1.0, 1.0, 1.0)));
    sophiaMatYellow->setSpecular(XMFLOAT4(0.1, 0.1, 0.1, 0.001));
    sophiaMatYellow->setUsage(DIFFUSE_MAP | NORMAL_MAP);
    sophiaMatYellow->setTextures(sophiaTextureArrayYellow, 2);

    shared_ptr<Material> sophiaMatWhite(new Material(device, sophiaMatYellow));
    sophiaMatWhite->setTextures(sophiaTextureArrayWhite, 2);

    shared_ptr<Material> sophiaMatPink(new Material(device, sophiaMatYellow));
    sophiaMatPink->setTextures(sophiaTextureArrayPink, 2);

    m_sophiaModel = make_unique< SkinnedModel>(context, device,
        wstring(L"..\\Resources\\Models\\35-rp_sophia_animated_003_idling_fbx\\rp_sophia_animated_003_idling.fbx"),
        skinningEffect, sophiaMatYellow);

    m_sophiaModel->loadBones(device);

    m_sophiaModel->instances.push_back(Instance(m_sophiaModel->getWorldMatrix() *
        XMMatrixRotationX(XMConvertToRadians(-90)) *
        XMMatrixRotationY(XMConvertToRadians(180)) *
        XMMatrixScaling(0.009, 0.009, 0.009) *
        XMMatrixTranslation(8.135422, 0 + m_terrain->CalculateYValueWorld(8.135422, 53.300213), 53.300213),
        sophiaMatWhite));

    m_sophiaModel->instances.push_back(Instance(m_sophiaModel->getWorldMatrix() *
        XMMatrixRotationX(XMConvertToRadians(-90)) *
        XMMatrixRotationY(XMConvertToRadians(180)) *
        XMMatrixScaling(0.009, 0.009, 0.009) *
        XMMatrixTranslation(9.135422, 0 + m_terrain->CalculateYValueWorld(9.135422, 53.300213), 53.300213),
        sophiaMatPink));

    m_sophiaModel->setWorldMatrix(m_sophiaModel->getWorldMatrix() *
        XMMatrixRotationX(XMConvertToRadians(-90)) *
        XMMatrixRotationY(XMConvertToRadians(180)) *
        XMMatrixScaling(0.009, 0.009, 0.009) *
        XMMatrixTranslation(10.135422, 0 + m_terrain->CalculateYValueWorld(10.135422, 53.300213), 53.300213));

    m_sophiaModel->setCurrentAnim(0);
    m_sophiaModel->update(context);

    // Generate random trees for foliage
    shared_ptr<Effect> treeEffect(new Effect(device, "Shaders\\cso\\tree_vs.cso",
        "Shaders\\cso\\tree_ps.cso",
        extVertexDesc, ARRAYSIZE(extVertexDesc)));

    treeEffect->setCullMode(device, D3D11_CULL_NONE);
    treeEffect->setAlphaToCoverage(device, TRUE);  // Alpha to coverage for foliage

    // Load m_treeTemplate textures
    Texture treeDiffuse = Texture(device, L"..\\Resources\\Textures\\m_treeTemplate.tif");
    shared_ptr<Material> treeMat(new Material(device, XMFLOAT4(0.3, 0, 0, 1.0)));
    treeMat->setSpecular(XMFLOAT4(0.0, 0.0, 0.0, 0.001));
    treeMat->setTexture(treeDiffuse.getShaderResourceView());

    // Load m_treeTemplate model
    m_treeTemplate = make_unique<Model>(context, device, wstring(L"..\\Resources\\Models\\m_treeTemplate.3ds"), treeEffect, treeMat);
    m_treeTemplate->setWorldMatrix(XMMatrixTranslation(14.0f, m_terrain->CalculateYValueWorld(13.0f, 20.0f), 20.0f));
    m_treeTemplate->update(context);

    // Create random m_treeTemplate instances
    for (int i = 0; i < NUM_TREES; i++)
    {
        float red = ((float)rand() / RAND_MAX) * 0.5;
        float green = (((float)rand() / RAND_MAX) * 0.5) + 0.5;
        float x = (((float)rand() / RAND_MAX) + 0.4) * 50.0f;
        float z = (((float)rand() / RAND_MAX) + 0.4) * 25.0f;

        if (((float)rand() / RAND_MAX) > 0.5) x = -x;
        if (((float)rand() / RAND_MAX) > 0.5) z = -z;

        // Ensure trees are placed on valid terrain
        while (m_terrain->getMapColour(x, z).x <= 0.0f)
        {
            x = (((float)rand() / RAND_MAX) + 0.4) * 50.0f;
            z = (((float)rand() / RAND_MAX) + 0.4) * 25.0f;
            if (((float)rand() / RAND_MAX) > 0.5) x = -x;
            if (((float)rand() / RAND_MAX) > 0.5) z = -z;
        }

        float s = (((float)rand() / RAND_MAX) + 0.5);
        float r = ((float)rand() / RAND_MAX);

        shared_ptr<Material> treeInstanceMat(new Material(device, XMFLOAT4(red, 0, 0, 1.0)));
        treeInstanceMat->setSpecular(XMFLOAT4(0.0, 0.0, 0.0, 0.001));
        treeInstanceMat->setTexture(treeDiffuse.getShaderResourceView());

        m_treeTemplate->instances.push_back(Instance(DirectX::XMMatrixRotationY(r) *
            XMMatrixScaling(s, s, s) *
            XMMatrixTranslation(x, m_terrain->CalculateYValueWorld(x, z), z),
            treeInstanceMat));
    }

    // Setup particle systems for kart effects
    shared_ptr<Material> dirtMat(new Material(device));
    Texture grassSkid = Texture(device, L"..\\Resources\\Textures\\grass_texture2.jpg");
    dirtMat->setTexture(grassSkid.getShaderResourceView());

    shared_ptr<Effect> dirtEffect(new Effect(device, "Shaders\\cso\\fire_vs.cso",
        "Shaders\\cso\\dirt_ps.cso",
        particleVertexDesc, ARRAYSIZE(particleVertexDesc)));

    m_dirtParticles = make_unique< ParticleSystem>(device, dirtEffect, dirtMat);
    m_dirtParticles->setWorldMatrix(XMMatrixScaling(0.7f, 1.5f, 0.7f) *
        XMMatrixTranslation(-1.0f, m_terrain->CalculateYValueWorld(-1.0f, 0.0f) + 0.3, 0.0f));
    m_dirtParticles->update(context);

    shared_ptr<Material> smokeMat(new Material(device));
    Texture smokeTexture = Texture(device, L"..\\Resources\\Textures\\Smoke.tif");
    smokeMat->setTexture(smokeTexture.getShaderResourceView());

    shared_ptr<Effect> smokeEffect(new Effect(device, "Shaders\\cso\\fire_vs.cso",
        "Shaders\\cso\\fire_ps.cso",
        particleVertexDesc, ARRAYSIZE(particleVertexDesc)));

    m_smokeParticles = make_unique <ParticleSystem>(device, smokeEffect, smokeMat);
    m_smokeParticles->setWorldMatrix(XMMatrixScaling(2.0f, 3.0f, 3.0f) *
        XMMatrixTranslation(-1.0f, m_terrain->CalculateYValueWorld(-1.0f, 0.0f) + 1.3f, 0.0f));
    m_smokeParticles->update(context);

    // Don't write transparent objects to depth buffer
    dirtEffect->setDepthWriteMask(device, D3D11_DEPTH_WRITE_MASK_ZERO);
    smokeEffect->setDepthWriteMask(device, D3D11_DEPTH_WRITE_MASK_ZERO);

    // Create lens m_lensFlare
    shared_ptr<Effect> flareEffect(new Effect(device, "Shaders\\cso\\flare_vs.cso",
        "Shaders\\cso\\flare_ps.cso",
        flareVertexDesc, ARRAYSIZE(flareVertexDesc)));

    // Create custom flare blend state
    ID3D11BlendState* flareBlendState = flareEffect->getBlendState();
    D3D11_BLEND_DESC blendDesc;
    flareBlendState->GetDesc(&blendDesc);
    blendDesc.AlphaToCoverageEnable = FALSE;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    flareBlendState->Release();
    device->CreateBlendState(&blendDesc, &flareBlendState);
    flareEffect->setBlendState(flareBlendState);

    // Load flare textures
    Texture flare0Texture = Texture(device, L"..\\Resources\\Textures\\m_lensFlare\\corona.png");
    Texture flare1Texture = Texture(device, L"..\\Resources\\Textures\\m_lensFlare\\divine.png");
    Texture flare2Texture = Texture(device, L"..\\Resources\\Textures\\m_lensFlare\\extendring.png");

    shared_ptr<Material> flareMat0(new Material(device, XMFLOAT4(1, 1, 1, (float)0 / NUM_FLARES)));
    flareMat0->setTexture(flare0Texture.getShaderResourceView());

    // Create m_lensFlare
    m_lensFlare = make_unique<Flare>(XMFLOAT3(-1250.0, 520.0, 1000.0), device, flareEffect, flareMat0);

    for (int i = 1; i < NUM_FLARES; i++)
    {
        shared_ptr<Material> flareInstanceMat(new Material(device,
            XMFLOAT4(randM1P1() * 0.5 + 0.5, randM1P1() * 0.5 + 0.5, randM1P1() * 0.5 + 0.5, (float)i / NUM_FLARES)));

        if (randM1P1() > 0.0f)
            flareInstanceMat->setTexture(flare1Texture.getShaderResourceView());
        else
            flareInstanceMat->setTexture(flare2Texture.getShaderResourceView());

        m_lensFlare->instances.push_back(Instance(XMMatrixIdentity(), flareInstanceMat));
    }

    // Initialize PhysX karts scene
    m_physicsScene->initScenePX();

    // Load m_sceneModels from Scene.xml (created with LevelEditor)
    loadScene("..\\Resources\\Levels\\Scene01_RH_X2.xml", perPixelLightingEffect);

    // Create PhysX terrain from heightfield
    m_physicsScene->CreateHeightField(m_terrain->getHeightArray(), m_terrainResolution, m_terrainScaleXZ, m_terrainScaleY);

    // Load karts
    m_kartWrapNames.push_back("GreyCamo.png");
    m_kartWrapNames.push_back("RedCamo.png");
    m_kartWrapNames.push_back("YellowCamo.png");
    m_kartWrapNames.push_back("GreenCamo.png");
    m_kartWrapNames.push_back("BlueCamo.png");
    m_kartWrapNames.push_back("PurpleCamo.png");
    m_kartWrapNames.push_back("WhiteStripe.png");
    m_kartWrapNames.push_back("BlackStripe.png");
    m_kartWrapNames.push_back("RedStripe.png");
    m_kartWrapNames.push_back("YellowStripe.png");
    m_kartWrapNames.push_back("GreenStripe.png");
    m_kartWrapNames.push_back("BlueStripe.png");
    m_kartWrapNames.push_back("PurpleStripe.png");

    for (int i = 0; i < m_kartWrapNames.size(); i++)
    {
        shared_ptr<Texture> tmpTex(new Texture(device,
            StringToWString(string("..\\Resources\\Models\\Kart\\" + m_kartWrapNames[i]))));
        m_kartTextures.push_back(tmpTex);
    }

    // Create karts with random wraps
    for (int i = 0; i < NUM_AI_VEHICLES - 1; i++)
        m_aiKarts[i] = make_unique < AIKart>(m_audioSystem, device, context, m_terrain.get(), m_navigationPoints.get(),
            perPixelLightingEffect, m_kartTextures[(int)(rand021() * (float)m_kartTextures.size())]);
   
    m_aiKarts[NUM_AI_VEHICLES - 1] = make_unique < AIKart>(m_audioSystem, device, context, m_terrain.get(), m_navigationPoints.get(),
        emissiveEffect, m_kartTextures[(int)(rand021() * (float)m_kartTextures.size())]);

    int playerKartWrap = (int)(rand021() * (float)m_kartTextures.size());
    m_playerKart = make_unique<PlayerKart>(m_audioSystem, device, context, m_terrain.get(),
        perPixelLightingEffect, m_kartTextures[playerKartWrap]);

    m_menu->setPlayerKartTextures(&m_kartWrapNames, &m_kartTextures, playerKartWrap);

    // Assign karts to array for batch PhysX updates
    m_allKarts[0] = m_playerKart.get();
    for (int i = 0; i < NUM_AI_VEHICLES; i++)
        m_allKarts[i + 1] = m_aiKarts[i].get();

    physx::PxVec3 pP(36.0f, 0, 1.2f);
    pP.y = m_terrain->CalculateYValueWorld(pP.x, pP.z) + pP.y;

    // Initialize PhysX vehicle controller
    m_vehicleController = make_unique < PhysXVehicleController>();
    m_playerKart->setVehicle4W(m_vehicleController->initVehiclePX(pP.x, pP.y, pP.z, -100.0f, 0));
    m_playerKart->setStartPosition(pP, 0);

    for (int i = 0; i < NUM_AI_VEHICLES; i++)
    {
        m_aiKarts[i]->setVehicle4W(m_vehicleController->initVehiclePX(
            pP.x + floor((i + 1) / 2) * 2, pP.y, pP.z - 2.0f * ((i + 1) % 2), -100.0f, i + 1));
        m_aiKarts[i]->setStartPosition(pP, i + 1);
    }

    // Setup main camera
    m_mainCamera = make_unique <FirstPersonCamera>(device, XMVectorSet(-9.0, 2.0, 17.0, 1.0f),
        XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f),
        XMVectorSet(0.8f, 0.0f, -1.0f, 1.0f));

    m_mainCamera->setFlying(false);
    float cutOff = cos(XMConvertToRadians(45));
    cout << "cutOff" << cutOff << endl;

    // Setup light constant buffers
    m_lightBufferCPU.reset(static_cast<CBufferLight*>(_aligned_malloc(sizeof(CBufferLight) * MAX_LIGHTS, 16)));
  
    // Fill out light properties
    m_lightBufferCPU[0].lightVec = XMFLOAT4(-1250.0, 1000.0, 5.0, 1.0);
    m_lightBufferCPU[0].lightAmbient = XMFLOAT4(0.3, 0.3, 0.5, 1.0);
    m_lightBufferCPU[0].lightDiffuse = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
    m_lightBufferCPU[0].lightSpecular = XMFLOAT4(0.9, 0.9, 0.9, 1.0);
    m_lightBufferCPU[0].lightAttenuation = XMFLOAT4(1.0, 0.0, 0.0, 10000.0);
    m_lightBufferCPU[0].lightCone = XMFLOAT4(0.0, -1.0, 0.0, 0);

    // Additional lights setup...
    m_lightBufferCPU[1].lightVec = XMFLOAT4(0, g_lightDistance * 0.625, g_lightDistance, 1.0);
    m_lightBufferCPU[1].lightAmbient = XMFLOAT4(0.0, 0.0, 0.0, g_red);
    m_lightBufferCPU[1].lightDiffuse = XMFLOAT4(2.5, 2.5, 2.5, 1.0);
    m_lightBufferCPU[1].lightSpecular = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
    m_lightBufferCPU[1].lightAttenuation = XMFLOAT4(0.01, 0.0, 0.9, 10.0);
    m_lightBufferCPU[1].lightCone = XMFLOAT4(0.0, -1.0, 0.0, cutOff);

    m_lightBufferCPU[2].lightVec = XMFLOAT4(0, 1, 0, 1.0);
    m_lightBufferCPU[2].lightAmbient = XMFLOAT4(0.3, 0.3, 0.3, 1.0);
    m_lightBufferCPU[2].lightDiffuse = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
    m_lightBufferCPU[2].lightSpecular = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
    m_lightBufferCPU[2].lightAttenuation = XMFLOAT4(1.0, 0.2, 0.1, 10.0);
    m_lightBufferCPU[2].lightCone = XMFLOAT4(0.0, -1.0, 0.0, 0);

    m_lightBufferCPU[3].lightVec = XMFLOAT4(0, 10, 15, 1.0);
    m_lightBufferCPU[3].lightAmbient = XMFLOAT4(0.3, 0.1, 0.0, 1.0);
    m_lightBufferCPU[3].lightDiffuse = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
    m_lightBufferCPU[3].lightSpecular = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
    m_lightBufferCPU[3].lightAttenuation = XMFLOAT4(1.0, 0.1, 0.05, 10.0);
    m_lightBufferCPU[3].lightCone = XMFLOAT4(0.0, -1.0, 0.0, cutOff);

    m_lightBufferCPU[4].lightVec = XMFLOAT4(0, 10, 30, 1.0);
    m_lightBufferCPU[4].lightAmbient = XMFLOAT4(0.3, 0.1, 0.0, 1.0);
    m_lightBufferCPU[4].lightDiffuse = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
    m_lightBufferCPU[4].lightSpecular = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
    m_lightBufferCPU[4].lightAttenuation = XMFLOAT4(1.0, 0.1, 0.05, 10.0);
    m_lightBufferCPU[4].lightCone = XMFLOAT4(0.0, -1.0, 0.0, cutOff);

    m_lightBufferCPU[5].lightVec = XMFLOAT4(-40, 10, 0, 1.0);
    m_lightBufferCPU[5].lightAmbient = XMFLOAT4(0.3, 0.1, 0.0, 1.0);
    m_lightBufferCPU[5].lightDiffuse = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
    m_lightBufferCPU[5].lightSpecular = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
    m_lightBufferCPU[5].lightAttenuation = XMFLOAT4(1.0, 0.1, 0.05, 10.0);
    m_lightBufferCPU[5].lightCone = XMFLOAT4(0.0, -1.0, 0.0, cutOff);

    m_lightBufferCPU[6].lightVec = XMFLOAT4(-80, 10, 0, 1.0);
    m_lightBufferCPU[6].lightAmbient = XMFLOAT4(0.3, 0.3, 0.3, 1.0);
    m_lightBufferCPU[6].lightDiffuse = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
    m_lightBufferCPU[6].lightSpecular = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
    m_lightBufferCPU[6].lightAttenuation = XMFLOAT4(1.0, 0.1, 0.05, 10.0);
    m_lightBufferCPU[6].lightCone = XMFLOAT4(0.0, -1.0, 0.0, cutOff);

    m_lightBufferCPU[7].lightVec = XMFLOAT4(40, 10, 0, 1.0);
    m_lightBufferCPU[7].lightAmbient = XMFLOAT4(0.3, 0.1, 0.0, 1.0);
    m_lightBufferCPU[7].lightDiffuse = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
    m_lightBufferCPU[7].lightSpecular = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
    m_lightBufferCPU[7].lightAttenuation = XMFLOAT4(1.0, 0.1, 0.05, 10.0);
    m_lightBufferCPU[7].lightCone = XMFLOAT4(0.0, -1.0, 0.0, cutOff);

    m_lightBufferCPU[8].lightVec = XMFLOAT4(80, 10, 0, 1.0);
    m_lightBufferCPU[8].lightAmbient = XMFLOAT4(0.3, 0.3, 0.0, 1.0);
    m_lightBufferCPU[8].lightDiffuse = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
    m_lightBufferCPU[8].lightSpecular = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
    m_lightBufferCPU[8].lightAttenuation = XMFLOAT4(1.0, 0.1, 0.05, 10.0);
    m_lightBufferCPU[8].lightCone = XMFLOAT4(0.0, -1.0, 0.0, cutOff);

    // Create GPU constant buffer
    D3D11_BUFFER_DESC cbufferDesc;
    D3D11_SUBRESOURCE_DATA cbufferInitData;
    ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));
    ZeroMemory(&cbufferInitData, sizeof(D3D11_SUBRESOURCE_DATA));

    cbufferDesc.ByteWidth = sizeof(CBufferLight) * MAX_LIGHTS;
    cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbufferInitData.pSysMem = m_lightBufferCPU.get();

    HRESULT hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &m_lightBufferGPU);

    // Map buffer to GPU
    mapCbuffer(context, m_lightBufferCPU.get(), m_lightBufferGPU.Get(), sizeof(CBufferLight) * NUM_LIGHTS_ACTIVE);
    context->VSSetConstantBuffers(2, 1, m_lightBufferGPU.GetAddressOf());
    context->PSSetConstantBuffers(2, 1, m_lightBufferGPU.GetAddressOf());

    // Scene constant buffer setup
    //m_sceneBufferCPU = (CBufferScene*)_aligned_malloc(sizeof(CBufferScene), 16);
    m_sceneBufferCPU.reset(static_cast<CBufferScene*>( _aligned_malloc(sizeof(CBufferScene), 16)));
    // Fill out scene properties
    m_sceneBufferCPU->windDir = XMFLOAT4(1, 0, 0, 1);
    m_sceneBufferCPU->Time = 0.0;
    m_sceneBufferCPU->grassHeight = 0.2;
    m_sceneBufferCPU->USE_SHADOW_MAP = true;
    m_sceneBufferCPU->QUALITY = m_menu->getQuality();
    m_sceneBufferCPU->fog = 0.15f;
    m_sceneBufferCPU->numLights = NUM_LIGHTS_ACTIVE;

    cbufferInitData.pSysMem = m_sceneBufferCPU.get();
    cbufferDesc.ByteWidth = sizeof(CBufferScene);

    hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &m_sceneBufferGPU);
    if (FAILED(hr)) {
        // Get more detailed error information
        char errorMsg[256];
        sprintf_s(errorMsg, "Failed to create scene buffer GPU: 0x%08X", hr);
        throw std::runtime_error(errorMsg);
    }
    mapCbuffer(context, m_sceneBufferCPU.get(), m_sceneBufferGPU.Get(), sizeof(CBufferScene));
    context->VSSetConstantBuffers(3, 1, m_sceneBufferGPU.GetAddressOf());
    context->PSSetConstantBuffers(3, 1, m_sceneBufferGPU.GetAddressOf());

    // Enable main m_menu
    m_menuState = MainMenu;

    return S_OK;
}

// Main rendering routine
HRESULT Scene::renderScene() {
    ID3D11DeviceContext* context = m_system->getDeviceContext();

    // Validate window and D3D context
if (isMinimised() || !context)
        return E_FAIL;

    if (m_shouldRenderFrame == false)
        return S_OK;

    // Shadow mapping pass
    if (true)
    {
        // Disable shadow map for shadow pass
        m_sceneBufferCPU->USE_SHADOW_MAP = false;
        mapCbuffer(context, m_sceneBufferCPU.get(), m_sceneBufferGPU.Get(), sizeof(CBufferScene));

        m_shadowMap->update(context);

        // Disable alpha to coverage for shadow map (not multisampled)
        m_treeTemplate->getEffect()->setAlphaToCoverage(m_system->getDevice(), FALSE);
        m_sceneModels[1]->getEffect()->setAlphaToCoverage(m_system->getDevice(), FALSE);

        // Render objects to shadow map
        m_shadowMap->render(m_system.get(), std::bind(&Scene::renderShadowObjects, this, std::placeholders::_1));

        // Re-enable shadow map
        m_sceneBufferCPU->USE_SHADOW_MAP = true;
        mapCbuffer(context, m_sceneBufferCPU.get(), m_sceneBufferGPU.Get(), sizeof(CBufferScene));

        // Re-enable alpha to coverage
        m_treeTemplate->getEffect()->setAlphaToCoverage(m_system->getDevice(), TRUE);
        m_sceneModels[1]->getEffect()->setAlphaToCoverage(m_system->getDevice(), TRUE);

        // Update dynamic cube map
        m_dynamicCubeMap->updateCubeCameras(context,
            XMVectorSet(XMVectorGetX(m_mainCamera->getPos()),
                -XMVectorGetY(m_mainCamera->getPos()) + 1.8,
                XMVectorGetZ(m_mainCamera->getPos()), 1));

        m_dynamicCubeMap->render(m_system.get(), std::bind(&Scene::renderDynamicObjects, this, std::placeholders::_1));

        // Restore cube environment texture
        ID3D11ShaderResourceView* cubeDayTextureSRV = m_cubeDayTexture->getShaderResourceView();
        context->PSSetShaderResources(6, 1, &cubeDayTextureSRV);

        // Restore main camera
        m_mainCamera->update(context);
    }
    else
    {
        m_sceneBufferCPU->USE_SHADOW_MAP = true;
        mapCbuffer(context, m_sceneBufferCPU.get(), m_sceneBufferGPU.Get(), sizeof(CBufferScene));
    }

    // Clear the back buffer
    static const FLOAT clearColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    context->ClearRenderTargetView(m_system->getBackBufferRTV(), clearColor);
    context->ClearDepthStencilView(m_system->getDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Render scene objects
    renderSceneObjects(context);

    // Apply m_bloomUtility effect to glow objects
    m_bloomEffect->blurModels(m_system.get(), std::bind(&Scene::renderGlowObjects, this, std::placeholders::_1));

    // Draw lens flare
    renderGlowObjects(context);

    // Draw flare based on time of day
    if (sin(m_mainClock->gameTimeElapsed() / 20.0f) > 0.5)
        DrawFlare(context);

    // Draw HUD
    m_menu->renderGUIMenu(&m_menuState, m_allKarts.data(), NUM_VEHICLES);

    // Present to screen
    HRESULT hr = m_system->presentBackBuffer();

    return S_OK;
}

// Render glow objects for m_bloomUtility effect
void Scene::renderGlowObjects(ID3D11DeviceContext* context)
{
    m_debugOrb->renderInstances(context);
    m_aiKarts[NUM_AI_VEHICLES - 1]->render(context);
}

// Render dynamic objects for cube map
void Scene::renderDynamicObjects(ID3D11DeviceContext* context)
{
    ID3D11ShaderResourceView* cubeDayTextureSRV = m_cubeDayTexture->getShaderResourceView();
    context->PSSetShaderResources(6, 1, &cubeDayTextureSRV);

    m_skyBox->render(context);
    m_sceneModels[7]->renderInstances(context);//ducks
    m_sceneModels[9]->renderInstances(context);//building

    m_playerKart->render(context, m_playerKart->camMode);

    // Render ground base
    m_sceneBufferCPU->grassHeight = 0.0f;
    mapCbuffer(context, m_sceneBufferCPU.get(), m_sceneBufferGPU.Get(), sizeof(CBufferScene));
    m_terrain->getEffect()->setDepthWriteMask(m_system->getDevice(), D3D11_DEPTH_WRITE_MASK_ALL);
    m_terrain->render(context);
}

// Render objects to shadow map
void Scene::renderShadowObjects(ID3D11DeviceContext* context)
{
    if (m_sceneBufferCPU->USE_SHADOW_MAP == false)
    {
        m_physicsBox->getEffect()->setCullMode(m_system->getDevice(), D3D11_CULL_BACK);
        m_playerKart->getFarings()->getEffect()->setCullMode(m_system->getDevice(), D3D11_CULL_BACK);
    }

    // Render PhysX boxes
    if (m_physicsBox)
        for (int i = 0; i < NUM_PHYSICS_BOXS; i++)
        {
            m_physicsBox->setWorldMatrix(m_boxTransforms[i]);
            m_physicsBox->update(context);
            m_physicsBox->render(context);
        }

    // Render karts
    m_playerKart->render(context, m_playerKart->camMode);
    for (int i = 0; i < NUM_AI_VEHICLES - 1; i++)
        m_aiKarts[i]->render(context);

    // Render m_treeTemplate instances
    if (m_treeTemplate)
        m_treeTemplate->renderInstances(context);

    // Render loaded m_sceneModels
    for (int i = 1; i < m_sceneModels.size(); i++)
        m_sceneModels[i]->renderInstances(context);

    // Render animated characters
    m_dragonModel->render(context);
    m_nathanModel->renderInstances(context);
    m_sophiaModel->renderInstances(context);

    if (m_sceneBufferCPU->USE_SHADOW_MAP == false)
    {
        m_physicsBox->getEffect()->setCullMode(m_system->getDevice(), D3D11_CULL_BACK);
        m_playerKart->getFarings()->getEffect()->setCullMode(m_system->getDevice(), D3D11_CULL_BACK);
    }
}

// Render main scene objects
void Scene::renderSceneObjects(ID3D11DeviceContext* context)
{
    // Render skybox
    if (m_skyBox)
        m_skyBox->render(context);

    renderShadowObjects(context);

    // Render ground base
    m_sceneBufferCPU->grassHeight = 0.0f;
    mapCbuffer(context, m_sceneBufferCPU.get(), m_sceneBufferGPU.Get(), sizeof(CBufferScene));
    m_terrain->getEffect()->setDepthWriteMask(m_system->getDevice(), D3D11_DEPTH_WRITE_MASK_ALL);
    m_terrain->render(context);

    // Render racing line
    if (m_sceneModels.size() > 0)
        m_sceneModels[0]->renderInstances(context);

    // Render remaining m_terrain shells with alpha blending
    m_terrain->getEffect()->setAlphaBlendEnable(m_system->getDevice(), true);
    m_terrain->getEffect()->setDepthWriteMask(m_system->getDevice(), D3D11_DEPTH_WRITE_MASK_ZERO);

    // Render m_terrain layers from base to tip
    for (int i = 1; i < m_grassRenderPasses; i++)
    {
        m_sceneBufferCPU->grassHeight = (m_grassShellHeight / m_grassRenderPasses) * i;
        mapCbuffer(context, m_sceneBufferCPU.get(), m_sceneBufferGPU.Get(), sizeof(CBufferScene));
        m_terrain->render(context);
    }

    //Render Face
    if (g_useTSD > 0)
        m_subsurfaceScattering->blurModel(m_faceModel.get(), m_system->getDepthStencilSRV());
    else
        m_faceModel->render(context);

    // Render m_water with dynamic cube map
    ID3D11ShaderResourceView* dunamicCubeTextureSRV = m_dynamicCubeMap->getSRV();
    context->PSSetShaderResources(6, 1, &dunamicCubeTextureSRV);

    if (m_water)
        m_water->render(context);

    // Render particle effects from kart tires
    if (m_playerKart->getWheelSpin())
    {
        if (m_playerKart->getOnGrass())
        {
            m_dirtParticles->setWorldMatrix(m_playerKart->getLSmokeMat());
            m_dirtParticles->update(context);
            m_dirtParticles->render(context);

            m_dirtParticles->setWorldMatrix(m_playerKart->getRSmokeMat());
            m_dirtParticles->update(context);
            m_dirtParticles->render(context);
        }
        else
        {
            m_smokeParticles->setWorldMatrix(m_playerKart->getLSmokeMat());
            m_smokeParticles->update(context);
            m_smokeParticles->render(context);

            m_smokeParticles->setWorldMatrix(m_playerKart->getRSmokeMat());
            m_smokeParticles->update(context);
            m_smokeParticles->render(context);
        }
    }
}

// Lens flare rendering implementation
void Scene::DrawFlare(ID3D11DeviceContext* context)
{
    if (m_lensFlare) {
        // Set NULL depth buffer to use Depth Buffer as shader resource
        ID3D11RenderTargetView* tempRT = m_system->getBackBufferRTV();
        context->OMSetRenderTargets(1, &tempRT, NULL);

        ID3D11ShaderResourceView* depthSRV = m_system->getDepthStencilSRV();
        context->VSSetShaderResources(5, 1, &depthSRV);

        m_lensFlare->renderInstances(context);

        // Release depth shader resource
        ID3D11ShaderResourceView* nullSRV = NULL;
        context->VSSetShaderResources(5, 1, &nullSRV);

        // Restore default depth buffer view
        tempRT = m_system->getBackBufferRTV();
        context->OMSetRenderTargets(1, &tempRT, m_system->getDepthStencil());
    }
}


// DirectInput setup for wheel controller (legacy input)
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib,"Dinput8.lib")
#pragma comment(lib,"Dxguid.lib")

LPDIRECTINPUT8 di;
const GUID LOGITECH_WHEEL_GUID = { 0x046d, 0xc29b };
const GUID LOGITECH_PEDALS_GUID = { 0x046d, 0xc283 };

LPDIRECTINPUTDEVICE8 wheel;
LPDIRECTINPUTDEVICE8 pedals;

BOOL CALLBACK DeviceEnumCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
    // Device enumeration for input devices
    WCHAR destString[9];
    const char sourceString[9] = "00CFEF40";
    const int sourceLength = strlen(sourceString) + 1;
    const int destLength = sourceLength;

    MultiByteToWideChar(CP_ACP, 0, sourceString, sourceLength, destString, destLength);

    return DIENUM_CONTINUE;
}

bool IsXboxOneController(LPCDIDEVICEINSTANCE lpddi)
{
    // Check for Xbox One controller
    cout << lpddi->tszProductName << endl;
    return false;
}

BOOL CALLBACK EnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
    // Enumerate devices and check for Xbox One controller
    if (IsXboxOneController(lpddi))
    {
        if (FAILED(di->CreateDevice(lpddi->guidInstance, &wheel, NULL)))
        {
            return DIENUM_STOP;
        }
    }
    return DIENUM_CONTINUE;
}

void Scene::setupWheelController()
{
    // Initialize DirectInput
    DirectInput8Create(m_hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&di, nullptr);

    // Find wheel device
    di->EnumDevices(DI8DEVCLASS_GAMECTRL, DeviceEnumCallback, nullptr, DIEDFL_ATTACHEDONLY);

    // Create device and set up data format
    if (wheel)
    {
        wheel->SetDataFormat(&c_dfDIJoystick2);
        wheel->Acquire();
    }
}