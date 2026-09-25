

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

