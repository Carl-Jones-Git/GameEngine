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


#pragma once
 // System includes
#include <Windows.h>
#include <memory>
#include <vector>
#include <string>
#include <array>
#include <functional>
#include <filesystem>

#include <CGDClock.h>
#include <Camera.h>
#include <FirstPersonCamera.h>
#include <Model.h>
#include <Kart.h>
#include <AIKart.h>
#include <PlayerKart.h>
#include <Box.h>
#include <Grid.h>
#include <CBufferStructures.h>
#include <ParticleSystem.h>
#include <Flare.h>
#include <Terrain.h>
#include <ShadowMap.h>
#include <DynamicCube.h>
#include <BlurUtility.h>
#include <SkinUtility.h>
#include <Xinput.h>
#include <PhysXVehicleController.h>
#include <PhysXKarting.h>
#include <NetMgr.h>
#include <SkinnedModel.h>
#include <Menu.h>

// C++ libraries for file handling and XML parsing
#include <tinyxml2.h>
namespace ti = tinyxml2;
namespace fs = std::filesystem;

/// @brief Physics behavior types for scene objects
enum class PhysXMode {
    Static,      ///< Immovable objects (walls, buildings)
    Dynamic,     ///< Moving objects (boxes, debris)
    Driveable    ///< Surface that vehicles can drive on
};

/// @brief Model Data loaded from XML file
struct ModelData
{
    XMVECTOR position;
    XMVECTOR rotation;
    XMVECTOR scale;
    string path;
};


// ========================================================================
 // Configuration Constants
 // ========================================================================
 // Physics & Scene constants
static constexpr float PHYSICS_BOX_HALF_SIZE = 0.5f;
static constexpr physx::PxU32 PHYSICS_BOX_PYRAMID_BASE_COUNT = 5;
static constexpr int NUM_PHYSICS_BOXS = (PHYSICS_BOX_PYRAMID_BASE_COUNT * (PHYSICS_BOX_PYRAMID_BASE_COUNT + 1.0)) / 2;
static constexpr int NUM_TREES = 30;
static constexpr int MAX_LIGHTS = 10;
static constexpr int NUM_FLARES = 6;



/// @brief Main scene manager class handling rendering, physics, and game logic
/// 
/// This class follows the Facade pattern, providing a simplified interface
/// to the complex subsystems (rendering, physics, audio, input).
/// Uses RAII principles with smart pointers for automatic resource management.
class Scene {
public:
public:
    // ========================================================================
    // Factory Method Pattern - Ensures single instance creation
    // ========================================================================

    /// @brief Creates a scene instance (singleton pattern)
    /// @param width Window width in pixels
    /// @param height Window height in pixels
    /// @param wndClassName Windows class name
    /// @param wndTitle Window title
    /// @param nCmdShow Window show command
    /// @param hInstance Application instance handle
    /// @param WndProc Window procedure callback
    /// @return Pointer to created scene or nullptr if already exists
    static Scene* CreateScene(
        LONG width,
        LONG height,
        const wchar_t* wndClassName,
        const wchar_t* wndTitle,
        int nCmdShow,
        HINSTANCE hInstance,
        WNDPROC WndProc
    );

    // ========================================================================
    // Core Game Loop Methods
    // ========================================================================

    /// @brief Main update and render loop - called each frame
    /// @return S_OK on success, error code otherwise
    HRESULT updateAndRenderScene();

    /// @brief Updates game logic, physics, and animations
    /// @param context DirectX device context
    /// @param camera Active camera for the scene
    /// @return S_OK on success, error code otherwise
    HRESULT updateScene(ID3D11DeviceContext* context, FirstPersonCamera* camera);

    /// @brief Renders all scene objects to screen
    /// @return S_OK on success, error code otherwise
    HRESULT renderScene();

    // ========================================================================
    // Rendering Passes - Multi-pass rendering architecture
    // ========================================================================

    /// @brief Renders objects that cast shadows (shadow map pass)
    void renderShadowObjects(ID3D11DeviceContext* context);

    /// @brief Renders objects visible in dynamic reflections (cube map pass)
    void renderDynamicObjects(ID3D11DeviceContext* context);

    /// @brief Renders emissive objects for m_bloomUtility effect
    void renderGlowObjects(ID3D11DeviceContext* context);

    /// @brief Renders main scene geometry
    void renderSceneObjects(ID3D11DeviceContext* context);

    /// @brief Renders lens flare effect
    void DrawFlare(ID3D11DeviceContext* context);

    // ========================================================================
    // Input Handling - Delegate pattern for input events
    // ========================================================================

    /// @brief Handles mouse drag events
    /// @param disp Displacement vector from last position
    void handleMouseLDrag(const POINT& disp);

    /// @brief Handles mouse wheel scrolling
    /// @param zDelta Scroll amount (positive = up, negative = down)
    void handleMouseWheel(short zDelta);

    /// @brief Handles key press events
    /// @param keyCode Virtual key code
    /// @param extKeyFlags Extended key information
    void handleKeyDown(WPARAM keyCode, LPARAM extKeyFlags);

    /// @brief Handles key release events
    /// @param keyCode Virtual key code
    /// @param extKeyFlags Extended key information
    void handleKeyUp(WPARAM keyCode, LPARAM extKeyFlags);


    // ========================================================================
    // Resource Management
    // ========================================================================

    /// @brief Initializes all scene resources (textures, m_sceneModels, shaders)
    /// @return S_OK on success, error code otherwise
    HRESULT initialiseSceneResources();

    /// @brief Handles window resize events and recreates render targets
    /// @return S_OK on success, error code otherwise
    HRESULT resizeResources();

    /// @brief Rebuilds m_viewport for current window dimensions
    /// @return S_OK on success, error code otherwise
    HRESULT rebuildViewport();

    // ========================================================================
    // Scene Loading - XML-based asset loading
    // ========================================================================

    /// @brief Loads scene from XML configuration file
    /// @param path Path to XML file
    /// @param effect Default shader effect for loaded m_sceneModels
    /// @param mapScale Scaling factor for positions
    /// @param LHCoords Coordinate m_system handedness (-1 for left-handed)

    void loadScene(
        const std::string& path,
        std::shared_ptr<Effect> effect,
        float mapScale = 2.0f,
        float LHCoords = -1.0f
    );
    ModelData parseModelXML(ti::XMLElement* element, float mapScale);

    std::string getXMLText(
        ti::XMLElement* element,
        const char* childName,
        const char* defaultValue) const;

    std::shared_ptr<Model> Scene::createModel(
        ID3D11DeviceContext* context,
        ID3D11Device* device,
        const ModelData& data,
        std::shared_ptr<Effect> effect,
        PhysXMode physicsMode);


    // ========================================================================
    // Timing System
    // ========================================================================

    /// @brief Starts the game clock
    void startClock();

    /// @brief Stops the game clock
    void stopClock();

    /// @brief Outputs timing statistics to console
    void reportTimingData();


    // ========================================================================
     // Utility Methods
     // ========================================================================

     /// @brief Checks if window is minimized
     /// @return TRUE if minimized, FALSE otherwise
    BOOL isMinimised() const;

    /// @brief Destroys the window
    void destroyWindow();

    /// @brief Updates render quality settings based on user preference
    void updateRenderQuality();

    /// @brief Initializes DirectInput wheel controller support
    void setupWheelController();

    /// @brief  Register window class
    void registerWindowClass(const wchar_t* className, WNDPROC wndProc);

    /// @brief Create window
    void createWindow(const wchar_t* className, const wchar_t* title, LONG width, LONG height, int nCmdShow);

    /// @brief Initialize DirectX
    void initializeDirectX();

    /// @brief Initialize timing m_system
    void initializeClock();

    /// @brief Return m_menu state
    MenuState getMenuState() {
        return m_menuState;
    }

    // ========================================================================
    // Destructor - RAII cleanup
    // ========================================================================

    /// @brief Cleans up all resources in reverse order of creation
    ~Scene();

    // Non-copyable and non-movable (singleton pattern)
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;
    // Clean up in reverse order of creation
    void cleanup();


private:
    // ========================================================================
// Private Constructor - Factory Method Pattern
// ========================================================================

/// @brief Private constructor - use CreateScene() factory method
    Scene(
        LONG width,
        LONG height,
        const wchar_t* wndClassName,
        const wchar_t* wndTitle,
        int nCmdShow,
        HINSTANCE hInstance,
        WNDPROC WndProc
    );

    // ========================================================================
    // Private Helper Methods
    // ========================================================================
    
    /// @brief Camera m_system implementation for tracking kart
    void updatePlayerCamera(ID3D11DeviceContext* context, FirstPersonCamera* camera);



    /// @brief Loads individual model from XML element
    void loadModel(
        ti::XMLElement* modelElement,
        std::vector<std::shared_ptr<Model>>* modelList,
        std::shared_ptr<Effect> effect,
        float mapScale = 1.0f,
        float LHCoords = -1.0f
    );

    /// @brief Converts model geometry to PhysX collision shapes
    /// @param model Model to convert
    /// @param rotation Initial rotation quaternion
    /// @param translation Initial position
    /// @param type Physics behavior type
    /// @param instance Instance index for instanced m_sceneModels
    void convertModelToPhysX(
        std::shared_ptr<Model> model,
        DirectX::XMVECTOR rotation,
        DirectX::XMVECTOR translation,
        PhysXMode type = PhysXMode::Static,
        int instance = 0
    );
    
    /// @brief Setup light constant buffers
    HRESULT setupLightsConstantBuffers(ID3D11DeviceContext* context,
        ID3D11Device* device);

    /// @brief Scene constant buffer setup
    HRESULT setupSceneConstantBuffers(ID3D11DeviceContext* context,
        ID3D11Device* device);

    /// @brief Initialize PhysX karts and scene
    HRESULT initKartsAndPhysXScene(ID3D11DeviceContext* context, ID3D11Device* device, shared_ptr <Effect> perPixelLightingEffect, shared_ptr <Effect> emissiveEffect);

    /// @brief Load Skinned models and skinning effect
    HRESULT loadSkinnedModels(ID3D11DeviceContext* context, ID3D11Device* device);
    
    /// @brief Setup lens flares and flare effect
    HRESULT setupLensFlaresAndFlareEffect(ID3D11Device* device);

    /// @brief Setup random trees and tree effect
    HRESULT setupRandomTreesAndTreeEffect(ID3D11DeviceContext* context, ID3D11Device* device);

    /// @brief Setup particle systems and effects for kart wheels
    HRESULT setupParticleSystemsAndEffects(ID3D11DeviceContext* context, ID3D11Device* device);

    /// @brief Setup terrain and terrain effect
    HRESULT Scene::setupTerrainAndTerrainEffect(ID3D11DeviceContext* context, ID3D11Device* device);

    /// @brief Load orb template for rendering at origin of point lights
    HRESULT Scene::loadOrbTemplate(ID3D11DeviceContext* context, ID3D11Device* device, shared_ptr <Effect> emissiveEffect);

    /// @brief Setup subsurface scattering and load face model
    HRESULT setupSkinEffectAndLoadFaceModel(ID3D11DeviceContext* context, ID3D11Device* device);

    /// @brief Setup water and water effect (based on Nvidia Ocean shaders)
    HRESULT Scene::setupWaterAndWaterEffect(ID3D11DeviceContext* context, ID3D11Device* device);

    /// @brief Setup skyBox and skyBoxEffect
    HRESULT setupSkyBoxAndSkyBoxEffect(ID3D11DeviceContext* context, ID3D11Device* device);


    // ========================================================================
    // Window and DirectX Core
    // ========================================================================

    // Windows API handles needed for DirectX applications
    HINSTANCE m_hInstance;
    HWND m_wndHandle;

    // Core DirectX system abstraction
    std::unique_ptr<System> m_system;
    D3D11_VIEWPORT m_viewport;
    
    
    // ========================================================================
    // Timing System - Fixed timestep physics
    // ========================================================================

    std::unique_ptr<CGDClock> m_mainClock;        // Game timing implementation
    // Physics timing variables for fixed timestep implementation
    float m_accumulator;        // Accumulator for fixed timestep
    float m_fixedTimeStep;      // Fixed physics timestep (60Hz)
    int m_numPhysXUpdates;      // Physics update multiplier for stability
    int m_numScenePhysXUpdates;
    int m_numKartPhysXUpdates;
    
    // Frame rendering flag
    bool m_shouldRenderFrame;   
    
    // ========================================================================
    // Camera System
    // ========================================================================

    std::unique_ptr<FirstPersonCamera> m_mainCamera; ///< Player camera

    // ========================================================================
    // Audio System - FMOD integration
    // ========================================================================

    FMOD::System* m_audioSystem;                    ///< FMOD audio m_system (raw pointer from library)
    
    // ========================================================================
    // Menu and UI System
    // ========================================================================

    std::unique_ptr<Menu> m_menu;                   ///< Manages the menu sysetm
    MenuState m_menuState;                          ///< Current m_menu state
    float m_menuKartRotation;                       ///< Kart rotation for m_menu display
    NetMgr m_networkManager;                        ///< Network manager for multiplayer

    // ========================================================================
    // Physics System - PhysX integration
    // ========================================================================

    std::unique_ptr<PhysXKarting> m_physXScene;   ///< PhysX scene wrapper
    std::unique_ptr<PhysXVehicleController> m_physXKartController; ///< Vehicle physics controller
    std::unique_ptr<Box> m_physXBox;///< Template box for PhysX visualization
    std::array<DirectX::XMMATRIX, NUM_PHYSICS_BOXS> m_boxTransforms; ///< Transform matrices for physics boxes

    // ========================================================================
    // Kart Management - Game entities
    // ========================================================================

    std::array<Kart*, NUM_VEHICLES> m_allKarts;     ///< All karts (player + AI)
    std::array<std::unique_ptr<AIKart>, NUM_AI_VEHICLES> m_aiKarts; ///< AI-controlled karts
    std::unique_ptr<PlayerKart> m_playerKart;       ///< Player-controlled kart
    std::unique_ptr<NavPoints> m_navigationPoints;  ///< AI navigation waypoints
    
    // ========================================================================
    // Kart Customization Resources
    // ========================================================================

    std::vector<std::string> m_kartWrapNames;       ///< Kart skin texture names
    std::vector<std::shared_ptr<Texture>> m_kartTextures; ///< Kart skin textures

    // ========================================================================
    // Lighting & Scene - Constant buffers for GPU
    // ========================================================================

    std::unique_ptr<CBufferScene, AlignedDeleter> m_sceneBufferCPU; ///< CPU scene data
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_sceneBufferGPU;          ///< GPU scene buffer
    std::unique_ptr<CBufferLight[], AlignedDeleter> m_lightBufferCPU; ///< CPU light data
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_lightBufferGPU;          ///< GPU light buf    Microsoft::WRL::Cofer


    // ========================================================================
    // Environment Textures
    // ========================================================================

    std::unique_ptr<Texture> m_cubeDayTexture;      ///< Day sky cube map
    std::unique_ptr<DynamicCube> m_dynamicCubeMap;  ///< Dynamic reflection cube map

    // ========================================================================
    // Scene Geometry
    // ========================================================================
    std::unique_ptr<Box> m_skyBox;                  ///< Sky box
    std::unique_ptr<Grid> m_water;                  ///< Water surface
    std::vector<std::shared_ptr<Model>> m_sceneModels; ///< Static scene m_sceneModels

    // ========================================================================
     // Animated Characters
     // ========================================================================

    std::unique_ptr <SkinnedModel> m_dragonModel;    ///< Animated m_dragonModel
    std::unique_ptr<SkinnedModel> m_nathanModel;    ///< Animated character Nathan
    std::unique_ptr<SkinnedModel> m_sophiaModel;    ///< Animated character Sophia
    
    
    // ========================================================================
    // Terrain System - Heightmap-based terrain with foliage
    // ========================================================================

    std::unique_ptr<Terrain> m_terrain;             ///< Terrain mesh
    std::unique_ptr<Model> m_treeTemplate;          ///< Tree model for instancing
    int m_terrainResolution;                        ///< Terrain grid size
    float m_terrainScaleXZ;                         ///< Horizontal terrain scale
    float m_terrainScaleY;                          ///< Vertical terrain scale
    float m_grassShellHeight;                       ///< Height per m_terrain shell layer
    int m_grassRenderPasses;                        ///< Number of m_terrain shells (quality)


    // ========================================================================
    // Visual Effects
    // ========================================================================

    std::unique_ptr<ParticleSystem> m_dirtParticles;    ///< Dirt spray particles
    std::unique_ptr<ParticleSystem> m_smokeParticles;   ///< Smoke particles
    std::unique_ptr<Flare> m_lensFlare;                 ///< Lens flare m_system
    int m_numLightsActive;
    std::unique_ptr<Model> m_lightOrbTemplate;                  ///< Debug visualization orb

    
    // ========================================================================
// Post-Processing Effects
// ========================================================================

    std::unique_ptr<ShadowMap> m_shadowMap;         ///< Shadow mapping m_system
    std::unique_ptr<BlurUtility> m_bloomUtility;     ///< Bloom post-process
    std::unique_ptr<SkinUtility> m_skinSimulationUtility; ///< Skin rendering with subsurface scattering
    std::unique_ptr<Model> m_faceModel;             ///< Face model for SSS demo
    std::shared_ptr<Effect> m_skinEffect;           ///< Skin rendering shader

};
