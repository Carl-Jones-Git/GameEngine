// Scene.h
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
// DirectX interfaces and scene variables (model)
#pragma once
#include <Windows.h>

//#define PC_BUILD  // Conditional compilation for different input systems
#include <CGDClock.h>
#include <Camera.h>
#include <LookAtCamera.h>
#include <FirstPersonCamera.h>
#include <Triangle.h>
#include <Model.h>
#include <Kart.h>
#include <AIKart.h>
#include <PlayerKart.h>
#include <Box.h>
#include <Grid.h>
#include <Quad.h>
#include <CBufferStructures.h>
#include <ParticleSystem.h>
#include <Flare.h>
#include <Terrain.h>
#include <ShadowMap.h>
#include <DynamicCube.h>
#include <ShadowVolume.h>
#include <BlurUtility.h>
#include <SkinUtility.h>
#include <Xinput.h>
#include <PhysXVehicleController.h>
#include <PhysXKarting.h>
#include <NetMgr.h>
#include <SkinnedModel.h>
#include <Menu.h>

// Modern C++ libraries for file handling and XML parsing
#include <filesystem>
#include <tinyxml2.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>


namespace ti = tinyxml2;
namespace fs = std::filesystem;

// Enumeration for PhysX object types - demonstrates different physics behaviors
enum PhysXMode { STATIC, DYNAMIC, DRIVEABLE };

// Old-style macro constants (pre-constexpr) - shows historical approaches
#define numTrees 30
#define MAX_LIGHTS 10

class Scene {
public:
    Menu* menu = nullptr;  // Raw pointer shows manual memory management
    MenuState MainMenuState = Intro;
    NetMgr netMgr;         // Network manager for multiplayer functionality

    // Windows API handles needed for DirectX applications
    HINSTANCE hInst = NULL;
    HWND wndHandle = NULL;

    // Core DirectX system abstraction
    System* system = nullptr;
    D3D11_VIEWPORT viewport;
    CGDClock* mainClock;      // Game timing implementation
    FirstPersonCamera* mainCamera; // Player camera system

    // FMOD audio system integration
    FMOD::System* FMSystem = nullptr;

    // Physics timing variables for fixed timestep implementation
    float acc = 0.0f;                // Accumulator for fixed timestep
    float stepSize = 1.0f / 60.0f;   // Fixed physics timestep (60Hz)
    int numPXUpdates = 1;            // Physics update multiplier for stability
    int numScenePXUpdates = 1;
    int numKartPXUpdates = 1;

    // PhysX integration - shows how to combine physics engine with rendering
    PhysXKarting* PXScene = nullptr;
    Box* physXBox = nullptr;
    XMMATRIX* boxMatArray = nullptr;

    // Kart management system - demonstrates entity handling in games
    Kart* kartArray[NUM_VEHICLES];           // Fixed array shows static allocation
    AIKart* aiKarts[NUM_AI_VEHICLES + 1];    // AI entities with custom behavior
    PlayerKart* playerKart = nullptr;        // Player-controlled entity
    float menuKartRot = -0.3f;               // Rotation for menu display

    // Resource management with mixed approaches
    vector<std::string> kartWraps;           // String collection for resource names
    vector<shared_ptr<Texture>> kartTextures; // Smart pointers for automatic cleanup

    PhysXVehicleController* PXKartController = nullptr;
    NavPoints* navPoints = nullptr;          // Navigation points for AI pathfinding

    // Constant buffers for shader communication - demonstrates DX11 pattern
    static const int numLights = 9;
    CBufferScene* cBufferSceneCPU = nullptr;  // CPU-side buffer
    ID3D11Buffer* cBufferSceneGPU = nullptr;  // GPU-side buffer
    CBufferLight* cBufferLightCPU = nullptr;
    ID3D11Buffer* cBufferLightGPU = nullptr;

    // Texture resources - raw pointer shows manual management
    Texture* cubeDayTexture = nullptr;

    // Scene objects demonstrating different rendering techniques
    Box* skyBox = nullptr;                    // Skybox implementation
    Model* orb3 = nullptr;                    // Basic model
    vector<shared_ptr<Model>> models;         // Model collection with smart pointers
    SkinnedModel* dragon = nullptr;           // Animated model with skinning
    SkinnedModel* nathan = nullptr;
    SkinnedModel* sophia = nullptr;

    // Water rendering system
    Grid* water = nullptr;
    DynamicCube* dynamicCubeMap = nullptr;    // Dynamic environment mapping

    // Terrain and foliage system
    int terrainSizeWH = 200;
    float terrainScaleXZ = 1.0f;
    float terrainScaleY = 2.5;
    float grassLength = 0.001666f / terrainScaleY;
    int numGrassPasses = 0;                   // Quality-based grass rendering
    Model* tree = nullptr;
    Terrain* grass = nullptr;

    // Particle systems for special effects
    ParticleSystem* dirt;
    ParticleSystem* smoke;

    // Lens flare rendering system
    static const int numFlares = 6;
    Flare* flares;

    // Shadow mapping implementation
    ShadowMap* shadowMap = nullptr;

    // Utility to add bloom to bright objects
    BlurUtility* bloom = nullptr;

    // Skin rendering utility
    SkinUtility* skinSim;
    Model* face = nullptr;
    shared_ptr<Effect> skinEffect = nullptr;

    // Private constructor - demonstrates factory pattern
    Scene(const LONG _width, const LONG _height, const wchar_t* wndClassName,
        const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc);

    // Window state utility
    BOOL isMinimised();

public:
    // Factory method pattern for object creation
    static Scene* CreateScene(const LONG _width, const LONG _height, const wchar_t* wndClassName,
        const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc);

    // DirectX resource management
    HRESULT rebuildViewport();

    // XML loading demonstrates external asset configuration
    void load(string path, shared_ptr<Effect> effect, float mapScale = 2.0f, float LHCoords = -1.0f);
    void loadModel(ti::XMLElement* model, vector<shared_ptr<Model>>* modelList,
        shared_ptr<Effect> effect, float mapScale = 1.0f, float LHCoords = -1.0f);

    // Comprehensive resource initialization
    HRESULT initialiseSceneResources();

    // Game loop methods
    HRESULT updateScene(ID3D11DeviceContext* context, FirstPersonCamera* camera);
    HRESULT renderScene();

    // Multi-pass rendering methods
    void renderShadowObjects(ID3D11DeviceContext* context);
    void renderDynamicObjects(ID3D11DeviceContext* context);
    void renderGlowObjects(ID3D11DeviceContext* context);
    void renderSceneObjects(ID3D11DeviceContext* context);

    // Quality settings adjustment
    void UpdateRenderQuality();

    // Camera system implementation
    void updatePlayerCamera(ID3D11DeviceContext* context, FirstPersonCamera* camera);

    // Input device setup
    void SetUpWheel();

    // Physics integration methods
    void toPhysX(shared_ptr<Model> model, XMVECTOR R, XMVECTOR T,
        PhysXMode type = STATIC, int instance = 0);

    // Special effects rendering
    void DrawFlare(ID3D11DeviceContext* context);

    // Timing system
    void startClock();
    void stopClock();
    void reportTimingData();

    // Input handling demonstrates different input processing approaches
    void handleMouseLDrag(const POINT& disp);
    void handleMouseWheel(const short zDelta);
    void handleKeyDown(const WPARAM keyCode, const LPARAM extKeyFlags);
    void handleKeyUp(const WPARAM keyCode, const LPARAM extKeyFlags);

    // Main game loop implementation
    HRESULT updateAndRenderScene();

    // Destructor with comprehensive cleanup
    ~Scene();

    // Window management
    void destoryWindow();

    // Resource handling for window resize events
    HRESULT resizeResources();
};
