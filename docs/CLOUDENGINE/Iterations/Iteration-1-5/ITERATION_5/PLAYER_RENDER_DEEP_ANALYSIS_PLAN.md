# Player Rendering Deep Analysis Plan

## Problem Summary

**3 Sessions (2026-04-20 to 2026-04-21) failed to make player visible in CLOUDENGINE**

### What We Know
- Logs confirm: "PlayerEntities: rendering 1 entities" ✅
- PrimitiveMesh sphere is being rendered ✅
- Build successful, FPS stable (~59-62) ✅
- **BUT: Player NOT visible in window** ❌

### Changes Already Attempted
1. ✅ Render order: Sphere before clouds
2. ✅ Player position: In front of camera (+20 units forward)
3. ✅ Cloud shader: alpha=0 (transparent sky/clouds)
4. ✅ Depth mask: glDepthMask(GL_FALSE)
5. ✅ Sphere size: 50 units (big)
6. ✅ Sphere color: Bright red (1.0, 0.2, 0.2)
7. ✅ View/Projection matrices from Camera class

---

## Sub-Agent Analysis Plan

### Agent 1: OpenGL State Debugging
**Focus:** Verify actual OpenGL calls being made

```cpp
// Need to add GL debug logging:
glGetError() checks after every GL call
GL_VIEWPORT verification
GL_DEPTH_FUNC verification
GL_BLEND_FUNC verification
GL_COLOR_BUFFER_BIT verification
```

### Agent 2: Shader Compilation Verification
**Focus:** Confirm PrimitiveMesh shader compiled and used

```cpp
// Add verification:
GLint compileStatus, linkedStatus;
glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
glGetProgramiv(program, GL_LINK_STATUS, &linkedStatus);
glGetShaderInfoLog() / glGetProgramInfoLog()

// Verify uniforms are actually set:
glGetUniformLocation() should not be -1
```

### Agent 3: Coordinate Transformation Debugging
**Focus:** Verify sphere position relative to camera

```cpp
// Log actual matrices:
glm::mat4 view = camera.getViewMatrix();
glm::mat4 proj = camera.getProjectionMatrix();
glm::mat4 model = glm::translate(glm::mat4(1), spherePos);

// Log world vs camera positions
// Check if sphere is actually in view frustum
```

### Agent 4: Fullscreen Quad vs Sphere Conflict
**Focus:** CloudRenderer and PrimitiveMesh both using GL

```cpp
// Issues to investigate:
// 1. CloudRenderer::_quad - what is this?
// 2. Does _quad.disableDepthWrite() affect subsequent rendering?
// 3. VAO state conflict between quad and sphere
// 4. Shader program state conflict
```

---

## Diagnostic Code to Add

```cpp
// In PrimitiveMesh::render() - ADD THIS:
void PrimitiveMesh::render(...) {
    // ... matrix setup ...
    
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        RENDER_LOG_ERROR("GL Error before draw: {}", err);
    }
    
    // Log current state
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    RENDER_LOG_DEBUG("Viewport: {}x{} at {},{}", 
        viewport[2], viewport[3], viewport[0], viewport[1]);
    
    GLboolean depthTest;
    glGetBooleanv(GL_DEPTH_TEST, &depthTest);
    RENDER_LOG_DEBUG("Depth test: {}", depthTest ? "ON" : "OFF");
    
    GLfloat clearDepth;
    glGetFloatv(GL_DEPTH_CLEAR_VALUE, &clearDepth);
    RENDER_LOG_DEBUG("Clear depth: {}", clearDepth);
    
    // Log shader being used
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    RENDER_LOG_DEBUG("Current shader program: {}", currentProgram);
    
    // Draw
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    
    while ((err = glGetError()) != GL_NO_ERROR) {
        RENDER_LOG_ERROR("GL Error after draw: {}", err);
    }
}
```

---

## Files to Create for Next Session

1. `src/rendering/debug_renderer.h/cpp` - OpenGL state debugging utilities
2. `src/rendering/shader_verifier.h/cpp` - Shader compilation verification
3. Update `primitive_mesh.cpp` with diagnostic code

## Next Session Command

```
Use subagents to analyze:
1. OpenGL state during render
2. Shader compilation logs
3. Coordinate transformations
4. Fullscreen quad interaction
```

---

## Key Question for Next Session

**Why is a 50-unit bright red sphere at position (camera + 20 forward) not visible?**

Possible answers:
1. Shader outputting wrong colors (black?)
2. Depth test rejecting all fragments
3. Viewport is 0x0
4. Camera looking away from sphere
5. VAO not bound correctly
6. glDrawElements not being called