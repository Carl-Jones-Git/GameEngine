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
// DirectX interfaces and scene variables (model)
#include <Windows.h>
#include <memory>
#include <vector>
#include <string>
#include <array>
#include <functional>


 // System includes
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
#include <PhysXVehicleController.h>
#include <PhysXKarting.h>
#include <NetMgr.h>
#include <SkinnedModel.h>
#include <Menu.h>

// Third-party libraries
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

/// @brief Main scene manager class handling rendering, physics, and game logic
/// 
/// This class follows the Facade pattern, providing a simplified interface
/// to the complex subsystems (rendering, physics, audio, input).
/// Uses RAII principles with smart pointers for automatic resource management.
class Scene {
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
    // Getter Setter Methods
    // ========================================================================
    MenuState getMenuState() {
        return m_menuState;
    }

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
    // Timing System
    // ========================================================================

    /// @brief Starts the game clock
    void startClock();

    /// @brief Stops the game clock
    void stopClock();

    /// @brief Outputs timing statistics to console
    void reportTimingData();

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

    void createStaticPhysicsObject(
        std::shared_ptr<Model> model, 
        PxMaterial* material, 
        PhysXMode type);

    void createDynamicPhysicsObject(
        std::shared_ptr<Model> model,
        PxMaterial* material,
        XMVECTOR rotation,
        XMVECTOR translation,
        int instance);

    /// @brief Updates camera to follow player kart
    void updatePlayerCamera(ID3D11DeviceContext* context, FirstPersonCamera* camera);

    /// @brief Apply special configurations to specific m_sceneModels
    void applySpecialModelConfigurations(std::shared_ptr<Effect> effect);

 
    // ========================================================================
    // Window and DirectX Core
    // ========================================================================

    HINSTANCE m_hInstance;                          ///< Application instance handle
    HWND m_windowHandle;                            ///< Main window handle
    std::unique_ptr<System> m_system;               ///< DirectX m_system wrapper
    D3D11_VIEWPORT m_viewport;                      ///< Rendering m_viewport

    // ========================================================================
    // Timing System - Fixed timestep physics
    // ========================================================================

    std::unique_ptr<CGDClock> m_mainClock;          ///< Game clock
    float m_accumulator;                            ///< Time accumulator for fixed timestep
    float m_fixedTimeStep;                          ///< Physics timestep (default 1/60s)
    int m_physicsUpdatesPerFrame;                   ///< Physics substeps for stability
    int m_scenePhysicsUpdates;                      ///< Scene physics update count
    int m_kartPhysicsUpdates;                       ///< Kart physics update count
    bool m_shouldRenderFrame;                       ///< Frame rendering flag

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

    std::unique_ptr<Menu> m_menu;                   ///< Menu m_system
    MenuState m_menuState;                          ///< Current m_menu state
    float m_menuKartRotation;                       ///< Kart rotation for m_menu display
    NetMgr m_networkManager;                        ///< Network manager for multiplayer

    // ========================================================================
    // Physics System - PhysX integration
    // ========================================================================

    std::unique_ptr<PhysXKarting> m_physicsScene;   ///< PhysX scene wrapper
    std::unique_ptr<PhysXVehicleController> m_vehicleController; ///< Vehicle physics controller
    std::unique_ptr<Box> m_physicsBox;              ///< Template box for PhysX visualization
    // Use the aligned allocator provided by DirectXMath

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

 
    // ============================================================================
    // Custom Deleters for Aligned Memory (SIMD requirements)
    // ============================================================================

    /// @brief Deleter for 16-byte aligned memory (required for DirectXMath types)
    struct AlignedDeleter {
        void operator()(void* ptr) const {
            if (ptr) {
                _aligned_free(ptr);
            }
        }
    };

    // ========================================================================
    // Lighting & Scene - Constant buffers for GPU
    // ========================================================================

    std::unique_ptr<CBufferScene, AlignedDeleter> m_sceneBufferCPU; ///< CPU scene data
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_sceneBufferGPU;          ///< GPU scene buffer
    std::unique_ptr<CBufferLight[], AlignedDeleter> m_lightBufferCPU; ///< CPU light data
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_lightBufferGPU;          ///< GPU light buffer

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
    std::unique_ptr<Model> m_debugOrb;                  ///< Debug visualization orb

    // ========================================================================
    // Post-Processing Effects
    // ========================================================================

    std::unique_ptr<ShadowMap> m_shadowMap;         ///< Shadow mapping m_system
    std::unique_ptr<BlurUtility> m_bloomEffect;     ///< Bloom post-process
    std::unique_ptr<SkinUtility> m_subsurfaceScattering; ///< Skin rendering
    std::unique_ptr<Model> m_faceModel;             ///< Face model for SSS demo
    std::shared_ptr<Effect> m_skinEffect;           ///< Skin rendering shader
};