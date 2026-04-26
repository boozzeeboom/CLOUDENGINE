#include "cloud_renderer.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#define __gl_h_
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <core/logger.h>

namespace Core { namespace Rendering {

DepthFBO::DepthFBO() : _fbo(0), _depthTexture(0), _width(0), _height(0) {}

DepthFBO::~DepthFBO() {
    shutdown();
}

bool DepthFBO::init(int width, int height) {
    if (_fbo != 0) {
        RENDER_LOG_WARN("DepthFBO::init() - Already initialized!");
        return true;
    }

    _width = width;
    _height = height;

    // Create depth texture - use 24-bit for better compatibility
    glGenTextures(1, &_depthTexture);
    glBindTexture(GL_TEXTURE_2D, _depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create FBO with depth attachment
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture, 0);

    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        RENDER_LOG_ERROR("DepthFBO::init() - FBO incomplete: {}", status);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    RENDER_LOG_INFO("DepthFBO::init() - Created depth FBO ({}x{})", width, height);
    return true;
}

void DepthFBO::shutdown() {
    if (_depthTexture) {
        glDeleteTextures(1, &_depthTexture);
        _depthTexture = 0;
    }
    if (_fbo) {
        glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }
    _width = 0;
    _height = 0;
}

void DepthFBO::bindForWriting() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    glViewport(0, 0, _width, _height);
}

void DepthFBO::bindForReading() {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _fbo);
}

}} // namespace Core::Rendering