---
paths:
  - "shaders/**"
  - "src/Rendering/**"
---

# Shader Code Rules — CLOUDENGINE

**These rules apply to all shader development.**

## Shader Files

- Use `.hlsl` extension for cross-platform shaders
- Document all uniforms in header comment block
- Follow naming: `PascalCase`, descriptive
- One shader concept per file

```hlsl
// Header comment template
//
// Shader: CloudGhibli.shader
// Description: Cel-shaded clouds with Ghibli-inspired softness
// Author: [name]
// Version: 1.0
//
// Uniforms:
//   _CloudColor ("Cloud Color", Color) = (1, 1, 1, 1)
//   _CloudSpeed ("Cloud Speed", Float) = 0.5
//   _CloudScale ("Cloud Scale", Float) = 1.0
//

#ifndef CLOUD_GHIBLI_INCLUDED
#define CLOUD_GHIBLI_INCLUDED
#endif
```

## Shader Structure

```hlsl
// cbuffer for frequently updated data
cbuffer FrameConstants : register(b0) {
    float4x4 ViewProjection;
    float4 CameraPosition;
    float Time;
    float DeltaTime;
};

// Constant buffers for material data
cbuffer MaterialConstants : register(b1) {
    float4 BaseColor;
    float4 EmissiveColor;
    float Roughness;
    float Metallic;
};

// Vertex input
struct VertexInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

// Vertex to pixel
struct PixelInput {
    float4 position : SV_POSITION;
    float3 worldPosition : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    float2 uv : TEXCOORD2;
};
```

## Performance Rules

| Rule | Target |
|------|--------|
| Texture samples | < 16 per pixel |
| ALU instructions | < 128 per pixel |
| Branch divergence | Minimize in loops |
| Memory bandwidth | Profile texture usage |

## CloudGhibli Shader Requirements

### Procedural Generation
- Use FBM (Fractal Brownian Motion) for cloud shapes
- Multiple octaves (4-6) for detail
- Animate via time uniform
- Support wind direction/speed

```hlsl
float GetCloudDensity(float3 position, float time) {
    float noise = 0;
    float amplitude = 1.0;
    float frequency = 1.0;
    
    for (int i = 0; i < 5; i++) {
        noise += amplitude * snoise(position * frequency + time * _WindDirection);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return noise;
}
```

### Cel-Shading
- Stepped diffuse for cartoon effect
- Soft shadow edges (not hard outlines)
- Rim lighting for depth

```hlsl
float3 CelShade(float NdotL, float3 lightColor) {
    float shadow = smoothstep(0.0, 0.1, NdotL);
    float halfShade = smoothstep(0.4, 0.5, NdotL) * 0.25;
    return lightColor * (shadow + halfShade);
}
```

### Color Palette
- Low contrast (avoid pure black/white)
- Warm undertones (Ghibli style)
- Pastel saturation (~70%)

## Mobile Fallback

- Simplify to 2 octaves FBM
- Reduce texture samples
- Disable expensive effects (rim, soft shadows)
- Provide `#ifdef MOBILE` paths

## Anti-Patterns

```hlsl
// VIOLATION: Expensive operations in pixel shader
float4 main(PixelInput input) : SV_TARGET {
    float4 color = tex2D(_MainTex, input.uv);
    // DON'T: Loop inside pixel shader
    for (int i = 0; i < 100; i++) {
        color += complexCalculation();
    }
    return color;
}

// VIOLATION: Undefined behavior
float result = someValue / 0;  // Infinity/NaN

// VIOLATION: Unoptimized texture sampling
float4 GetPixel(float2 uv) {
    float4 sum = float4(0,0,0,0);
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            sum += tex2D(_MainTex, uv + float2(x,y) * 0.01);  // 25 samples!
        }
    }
    return sum / 25;
}
```

## Profiling

- Use RenderDoc for GPU profiling
- Check for memory bandwidth bottlenecks
- Profile on target hardware (mobile/low-end)
- Document frame times for complex shaders

---

**Rules enforced by:** `render-engine-specialist`
