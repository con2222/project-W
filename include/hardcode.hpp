#pragma once

namespace c2::hard {

constexpr const char* SHADER_TRIANGLE = R"(
        @vertex fn vs(@builtin(vertex_index) VertexIndex : u32)
                            -> @builtin(position) vec4f {
            var pos = array(
                vec2f( 0.0,  0.5),
                vec2f(-0.5, -0.5),
                vec2f( 0.5, -0.5)
            );
            return vec4f(pos[VertexIndex], 0, 1);
        }

        @fragment fn fs() -> @location(0) vec4f {
            return vec4f(1, 0, 0, 1);
        }
    )";

constexpr int WINDOW_WIDTH = 1920;
constexpr int WINDOW_HEIGHT = 1080;

inline const char* shader = R"(

struct SceneVSOutput {
    @builtin(position) position : vec4f,
};

// ---------------------------------------------------------
// Scene pass
//
// A fullscreen triangle is generated directly from vertex_index.
// No vertex buffer is needed.
//
// The triangle is intentionally larger than the viewport,
// so after clipping it covers the entire render target.
// ---------------------------------------------------------
@vertex
fn scene_vs(@builtin(vertex_index) idx: u32) -> SceneVSOutput {
    var positions = array<vec2f, 3>(
        vec2f(-1.0, -1.0),
        vec2f( 3.0, -1.0),
        vec2f(-1.0,  3.0)
    );

    var out: SceneVSOutput;
    out.position = vec4f(positions[idx], 0.0, 1.0);
    return out;
}

// Generate a procedural color for every fragment.
//
// @builtin(position) in a fragment shader contains the
// fragment position in framebuffer coordinates.
//
// Dividing by the render target resolution converts
// pixel coordinates into normalized coordinates [0, 1].
@fragment
fn scene_fs(
    @builtin(position) fragCoord: vec4f
) -> @location(0) vec4f {

    let resolution = vec2f(800.0, 600.0);

    let uv = fragCoord.xy / resolution;

    


    return vec4f(
        uv.x,
        uv.y,
        0.0f,
        1.0
    );
}
    
)";

inline const char* shader1 = R"(
struct Uniforms {
    pcmFrames: u32,
};

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

struct SceneVSOutput {
    @builtin(position) position : vec4f,
};

@vertex
fn scene_vs(@builtin(vertex_index) idx: u32) -> SceneVSOutput {
    var positions = array<vec2f, 3>(
        vec2f(-1.0, -1.0),
        vec2f( 3.0, -1.0),
        vec2f(-1.0,  3.0)
    );

    var out: SceneVSOutput;
    out.position = vec4f(positions[idx], 0.0, 1.0);
    return out;
}


@fragment
fn scene_fs(
    @builtin(position) fragCoord: vec4f
) -> @location(0) vec4f {

    let resolution = vec2f(800.0, 600.0);

    let uv = fragCoord.xy / resolution;

    return vec4f(
        sin(uv.x * f32(uniforms.pcmFrames)),
        sin(uv.y * f32(uniforms.pcmFrames)),
        0.0f,
        1.0
    );
}
)";

}  // namespace c2::hard
