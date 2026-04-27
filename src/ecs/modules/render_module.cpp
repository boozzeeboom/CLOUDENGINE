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
    std::unordered_map<std::string, std::unique_ptr<GltfMesh>> _gltfMeshes;
    unsigned int _gltfShaderProgram = 0;
    const Rendering::Camera* _camera = nullptr;

    void setCamera(const Rendering::Camera* camera) { _camera = camera; }

    GltfMesh* getOrLoadGltf(const std::string& path) {
        auto it = _gltfMeshes.find(path);
        if (it != _gltfMeshes.end()) {
            CE_LOG_DEBUG("RenderModule: Using cached glTF mesh for {}", path);
            return it->second.get();
        }

        CE_LOG_DEBUG("RenderModule: Loading glTF mesh from {}", path);
        MeshData* meshData = AssetManager::get().loadModel(path);
        if (!meshData) {
            CE_LOG_ERROR("RenderModule: Failed to load model: {}", path);
            return nullptr;
        }

        auto mesh = std::make_unique<GltfMesh>();
        if (!mesh->loadFromMeshData(meshData)) {
            CE_LOG_ERROR("RenderModule: Failed to create GLTF mesh: {}", path);
            return nullptr;
        }

        GltfMesh* result = mesh.get();
        _gltfMeshes.emplace(path, std::move(mesh));
        CE_LOG_DEBUG("RenderModule: Created new glTF mesh for {}", path);
        return result;
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
            CE_LOG_TRACE("getGltfShader: Creating new shader");
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
                if (!_camera) {
                    CE_LOG_DEBUG("RenderGltfModels: No camera set, skipping");
                    return;
                }

                unsigned int shader = getGltfShader();
                glUseProgram(shader);
                CE_LOG_TRACE("RenderGltfModels: Using shader id={}", shader);

                CE_LOG_DEBUG("RenderGltfModels: Processing {} entities", it.count());

                int width = 1280, height = 720;
                GLFWwindow* win = glfwGetCurrentContext();
                if (win) glfwGetWindowSize(win, &width, &height);
                float aspect = static_cast<float>(width) / static_cast<float>(height > 0 ? height : 1);

                glm::mat4 view = _camera->getViewMatrix();
                glm::mat4 proj = _camera->getProjectionMatrix(aspect);

                glUniformMatrix4fv(glGetUniformLocation(shader, "uViewMatrix"), 1, GL_FALSE, &view[0][0]);
                glUniformMatrix4fv(glGetUniformLocation(shader, "uProjectionMatrix"), 1, GL_FALSE, &proj[0][0]);

                int renderedCount = 0;
                for (auto i : it) {
                    flecs::entity e = it.entity(i);
                    const auto* transform = e.get<Transform>();
                    const auto* modelAsset = e.get<ModelAsset>();
                    const auto* color = e.get<PlayerColor>();

                    if (transform && modelAsset && color) {
                        CE_LOG_DEBUG("RenderGltfModels: Entity {} has ModelAsset path={}", e.name().c_str(), modelAsset->path.c_str());
                        GltfMesh* gltf = getOrLoadGltf(modelAsset->path);
                        if (gltf && gltf->isLoaded()) {
CE_LOG_DEBUG("RenderGltfModels: Rendering entity {} at ({},{},{}) scale=100 color=(1,0,0)",
                                e.name().c_str(),
                                transform->position.x, transform->position.y, transform->position.z);
                            glm::mat4 model = glm::translate(glm::mat4(1.0f), transform->position);
                            model = model * glm::mat4_cast(transform->rotation);
                            model = glm::scale(model, glm::vec3(100.0f, 100.0f, 100.0f));
                            
                            CE_LOG_DEBUG("RenderGltfModels: model row0=({:.2f},{:.2f},{:.2f},{:.2f})",
                                model[0][0], model[0][1], model[0][2], model[0][3]);
                            CE_LOG_DEBUG("RenderGltfModels: model row3(pos)=({:.2f},{:.2f},{:.2f},{:.2f})",
                                model[3][0], model[3][1], model[3][2], model[3][3]);

                            glUseProgram(shader);
                            
                            GLint viewport[4];
                            glGetIntegerv(GL_VIEWPORT, viewport);
                            CE_LOG_DEBUG("RenderGltfModels: viewport={},{} {}x{}", 
                                viewport[0], viewport[1], viewport[2], viewport[3]);
                            
                            glUniformMatrix4fv(glGetUniformLocation(shader, "uModelMatrix"), 1, GL_FALSE, &model[0][0]);
                            glm::vec3 testColor(1.0f, 0.0f, 0.0f);
                            glUniform3fv(glGetUniformLocation(shader, "uColor"), 1, &testColor[0]);
                            
                            glBindVertexArray(gltf->getVAO());
                            GLint boundVAO = 0;
                            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &boundVAO);
                            
                            GLint posAttribIdx = glGetAttribLocation(shader, "aPosition");
                            CE_LOG_DEBUG("RenderGltfModels: aPosition attrib idx={}, VAO={}, indices={}", 
                                posAttribIdx, gltf->getVAO(), gltf->getIndexCount());
                            
                            glm::vec4 clipPos = proj * view * model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                            CE_LOG_DEBUG("RenderGltfModels: model origin NDC=({:.2f},{:.2f},{:.2f},{:.2f})",
                                clipPos.x, clipPos.y, clipPos.z, clipPos.w);
                            
                            GLint prevViewport[4];
                            glGetIntegerv(GL_VIEWPORT, prevViewport);
                            CE_LOG_DEBUG("RenderGltfModels: GL_VIEWPORT before={},{},{},{}", 
                                prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
                            
                            glDrawElements(GL_TRIANGLES, gltf->getIndexCount(), GL_UNSIGNED_INT, nullptr);
                            
GLint afterViewport[4];
                            glGetIntegerv(GL_VIEWPORT, afterViewport);
                            GLenum drawErr = glGetError();
                            if (drawErr != GL_NO_ERROR) {
                                CE_LOG_ERROR("RenderGltfModels: GL error {} after draw", drawErr);
                            } else {
                                CE_LOG_DEBUG("RenderGltfModels: draw call succeeded");
                            }
                            renderedCount++;
                        } else {
                            CE_LOG_DEBUG("RenderGltfModels: GltfMesh not loaded for path {}", modelAsset->path.c_str());
                        }
                    }
                }
                if (renderedCount > 0) {
                    CE_LOG_DEBUG("RenderGltfModels: Rendered {} entities", renderedCount);
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
