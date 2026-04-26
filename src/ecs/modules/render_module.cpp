#include "render_module.h"
#include "rendering/primitive_mesh.h"
#include "rendering/asset_manager.h"
#include "rendering/gltf_mesh.h"
#include "rendering/camera.h"
#include <GLFW/glfw3.h>
#include "core/logger.h"
#include <unordered_map>

using namespace Core::Rendering;

namespace Core { namespace ECS {

namespace detail {
struct RenderModuleImpl {
    bool _meshesInit = false;
    std::unordered_map<std::string, GltfMesh> _gltfMeshes;
    unsigned int _gltfShaderProgram = 0;
    const Rendering::Camera* _camera = nullptr;
    
    void setCamera(const Rendering::Camera* camera) { _camera = camera; }
    
    GltfMesh* getOrLoadGltf(const std::string& path) {
        auto it = _gltfMeshes.find(path);
        if (it != _gltfMeshes.end()) {
            return &it->second;
        }
        
        MeshData* meshData = AssetManager::get().loadModel(path);
        if (!meshData) {
            CE_LOG_ERROR("RenderModule: Failed to load model: {}", path);
            return nullptr;
        }
        
        GltfMesh gltfMesh;
        if (!gltfMesh.loadFromMeshData(meshData)) {
            CE_LOG_ERROR("RenderModule: Failed to create GLTF mesh: {}", path);
            return nullptr;
        }
        
        auto [insertIt, inserted] = _gltfMeshes.emplace(path, std::move(gltfMesh));
        return &insertIt->second;
    }
    
    void initMeshes() {
        if (_meshesInit) return;
        
        // FIX: Generate both Sphere and Cube VAOs
        // Platforms use Cube, remote players use Sphere
        GetPrimitiveMesh().generateSphere(0.5f, 12);
        GetPrimitiveMesh().generateCube(0.5f);
        
        _meshesInit = true;
        CE_LOG_INFO("RenderModule: Primitive meshes initialized (Sphere + Cube)");
    }
    
    unsigned int getGltfShader() {
        if (_gltfShaderProgram == 0) {
            const char* vs = R"(
                #version 330 core
                layout(location = 0) in vec3 aPosition;
                uniform mat4 uModelMatrix;
                uniform mat4 uViewMatrix;
                uniform mat4 uProjectionMatrix;
                void main() {
                    gl_Position = uProjectionMatrix * uViewMatrix * uModelMatrix * vec4(aPosition, 1.0);
                }
            )";
            const char* fs = R"(
                #version 330 core
                uniform vec3 uColor;
                out vec4 fragColor;
                void main() {
                    fragColor = vec4(uColor, 1.0);
                }
            )";
            _gltfShaderProgram = GetPrimitiveMesh().createShaderProgram(vs, fs);
            CE_LOG_INFO("RenderModule: Created glTF shader (id={})", _gltfShaderProgram);
        }
        return _gltfShaderProgram;
    }
    
    void registerSystems(flecs::world& world) {
        initMeshes();
        
        world.system("RenderGltfModels")
            .kind(flecs::PostUpdate)
            .with<Transform>()
            .with<ModelAsset>()
            .with<PlayerColor>()
        .iter([this](flecs::iter& it) {
                if (!_camera) return;
                
                unsigned int shader = getGltfShader();
                glUseProgram(shader);
                
                int width = 1280, height = 720;
                GLFWwindow* win = glfwGetCurrentContext();
                if (win) glfwGetWindowSize(win, &width, &height);
                float aspect = static_cast<float>(width) / static_cast<float>(height > 0 ? height : 1);
                
                glm::mat4 view = _camera->getViewMatrix();
                glm::mat4 proj = _camera->getProjectionMatrix(aspect);
                
                glUniformMatrix4fv(glGetUniformLocation(shader, "uViewMatrix"), 1, GL_FALSE, &view[0][0]);
                glUniformMatrix4fv(glGetUniformLocation(shader, "uProjectionMatrix"), 1, GL_FALSE, &proj[0][0]);
                
                for (auto i : it) {
                    flecs::entity e = it.entity(i);
                    const auto* transform = e.get<Transform>();
                    const auto* modelAsset = e.get<ModelAsset>();
                    const auto* color = e.get<PlayerColor>();
                    
                    if (transform && modelAsset && color) {
                        GltfMesh* gltf = getOrLoadGltf(modelAsset->path);
                        if (gltf && gltf->isLoaded()) {
                            glm::mat4 model = glm::translate(glm::mat4(1.0f), transform->position);
                            model = model * glm::mat4_cast(transform->rotation);
                            glUniformMatrix4fv(glGetUniformLocation(shader, "uModelMatrix"), 1, GL_FALSE, &model[0][0]);
                            glUniform3fv(glGetUniformLocation(shader, "uColor"), 1, &color->color[0]);
                            gltf->render(nullptr, model);
                        }
                    }
                }
            });
        
        CE_LOG_INFO("RenderModule: Render systems registered");
    }
};

static RenderModuleImpl s_impl;
} // namespace detail

void registerRenderComponents(flecs::world& world) {
    // Register IsBillboard tag
    world.component<IsBillboard>("IsBillboard");
    
    CE_LOG_INFO("RenderModule: Components registered (IsBillboard)");
}

void registerRemotePlayerRenderSystem(flecs::world& world) {
    detail::s_impl.registerSystems(world);
}

void setRenderModuleCamera(const Rendering::Camera* camera) {
    detail::s_impl.setCamera(camera);
}

}} // namespace Core::ECS
