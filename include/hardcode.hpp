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
constexpr unsigned int RG_BUFFER_SIZE = 8192;  // size in frames

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

struct FrequencyBands {
    bass: f32,
    mid: f32,
    treble: f32,
};

struct Uniforms {
    time: f32,
    audioIntensity: f32,
    resolution: vec2f,
    bands: FrequencyBands,
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
    
    var uv = (fragCoord.xy / uniforms.resolution.xy) * 2.0 - vec2f(1.0, 1.0);    
    uv.x *= uniforms.resolution.x / uniforms.resolution.y;
    let time = uniforms.time;
    let d = length(uv);

    let dAudio = d - uniforms.audioIntensity * 0.15;
    
    // Вычисляем угол для создания спиральных или радиальных искажений
    let angle = atan2(uv.y, uv.x);

    // Создаем расходящиеся кольца
    // let rings = sin(d * 10.0 - time * 4.0);

    // let rings = sin(d * (10.0 + uniforms.audioIntensity * 12.0) - time * 4.0);

    let rings = sin(dAudio * 10.0 - time * 4.0);
    
    // Добавляем искажение волной по кругу
    //let wave = sin(angle * 4.0 + time * 2.0) * 0.2;

    let wave =
    sin(angle * 4.0 + time * 2.0) * (0.2 + uniforms.audioIntensity * 0.5);
    
    // Высчитываем толщину и яркость колец (эффект свечения)
   let glow = (0.05 + uniforms.audioIntensity * 0.15) / abs(rings + wave);

    // Динамически меняем цвета с течением времени и расстояния
    let r = 0.5 + 0.5 * sin(time + d * 3.0);

    //let r = (0.5 + 0.5 * sin(time + d * 3.0)) * (0.5 + uniforms.audioIntensity);

    let g = 0.2 + 0.2 * sin(time * 1.5 + angle);
    let b = 0.5 + 0.5 * cos(time * 0.8 - d * 5.0);

    // Умножаем базовый цвет на силу свечения
    return vec4f(
        r * glow,
        g * glow,
        b * glow + 0.2, // Немного синего фона
        1.0
    );
}
)";

inline const char* shader2 = R"(

struct FrequencyBands {
    bass: f32,
    mid: f32,
    treble: f32,
};

struct Uniforms {
    time: f32,
    audioIntensity: f32,
    resolution: vec2f,
    bands: FrequencyBands,
};

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;

struct SceneVSOutput {
    @builtin(position) position: vec4f,
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

// Simple pseudo-random hash
fn hash21(p: vec2f) -> f32 {
    return fract(
        sin(dot(p, vec2f(127.1, 311.7))) * 43758.5453
    );
}

@fragment
fn scene_fs(
    @builtin(position) fragCoord: vec4f
) -> @location(0) vec4f {

    // -----------------------------------------
    // 1. Coordinates and audio
    // -----------------------------------------

    let res = max(uniforms.resolution, vec2f(1.0));
    let uv = (fragCoord.xy - 0.5 * res) / res.y * 2.0;

    let t = uniforms.time;
    let r = length(uv);
    let a = atan2(uv.y, uv.x);

    let bass = clamp(uniforms.bands.bass * 50.0, 0.0, 1.0);
    let mid = clamp(uniforms.bands.mid * 50.0, 0.0, 1.0);
    let treble = clamp(uniforms.bands.treble * 50.0, 0.0, 1.0);
    let energy = clamp(uniforms.audioIntensity, 0.0, 1.0);

    let cyan = vec3f(0.05, 0.85, 1.8);
    let purple = vec3f(0.55, 0.12, 1.4);
    let pink = vec3f(1.5, 0.06, 0.65);

    var color = vec3f(0.003, 0.005, 0.018);

    // -----------------------------------------
    // 2. Background nebula
    // -----------------------------------------

    let nebula = 0.5 + 0.5 * sin(
        uv.x * 3.7 +
        sin(uv.y * 4.2 - t * 0.10)
    );

    let haze = nebula * exp(-r * 1.3);

    color += vec3f(0.028, 0.012, 0.08) * haze;

    // -----------------------------------------
    // 3. Procedural star field
    // -----------------------------------------

    let gridUV = uv * 34.0;
    let cell = floor(gridUV);
    let local = fract(gridUV) - 0.5;

    let seed = hash21(cell);

    let offset = vec2f(
        hash21(cell + vec2f(2.1, 4.7)),
        hash21(cell + vec2f(4.3, 8.2))
    ) - 0.5;

    let starDist = length(local - offset * 0.65);

    let star = (1.0 - smoothstep(0.015, 0.055, starDist))
             * step(0.978, seed);

    let twinkle = 0.65 + 0.35 * sin(t * 1.7 + seed * 60.0);

    color += vec3f(0.3, 0.55, 1.0)
           * star
           * twinkle
           * (0.7 + treble * 1.6);

    // -----------------------------------------
    // 4. Swirling plasma filaments
    // -----------------------------------------

    let phase =
        a * 6.0 +
        r * 16.0 -
        t * 0.55 +
        1.4 * sin(a * 2.0 - r * 7.0 + t * 0.2);

    let filament = pow(
        1.0 - abs(sin(phase)),
        18.0
    );

    let filamentGlow = pow(
        1.0 - abs(sin(phase)),
        3.0
    );

    let region = exp(-pow((r - 0.66) * 2.3, 2.0));

    let plasmaColor = mix(
        purple,
        cyan,
        0.5 + 0.5 * sin(a * 2.0 + t * 0.15)
    );

    color += plasmaColor
           * region
           * (filament * 0.85 + filamentGlow * 0.09)
           * (0.6 + mid * 1.7);

    // -----------------------------------------
    // 5. Audio reactive orbital rings
    // -----------------------------------------

    let distortion =
        0.013 * sin(a * 11.0 + t * 0.5) * (1.0 + mid * 2.0) +
        0.007 * sin(a * 29.0 - t * 0.7) * treble;

    let distortedRadius = r + distortion;

    let ringRadius1 = 0.48 + bass * 0.055;
    let ringRadius2 = 0.84 + bass * 0.025;

    let d1 = abs(distortedRadius - ringRadius1);
    let d2 = abs(distortedRadius - ringRadius2);

    let ring1 = exp(-d1 * 110.0);
    let ring2 = exp(-d2 * 90.0);

    let ringGlow1 = exp(-d1 * 16.0);
    let ringGlow2 = exp(-d2 * 12.0);

    let arc = 0.45 + 0.55 * pow(
        0.5 + 0.5 * sin(a * 5.0 + t * 0.45),
        3.0
    );

    color += cyan
           * (ring1 * 0.9 + ringGlow1 * 0.12)
           * (0.6 + bass * 1.5);

    color += pink
           * (ring2 * 0.7 + ringGlow2 * 0.09)
           * arc
           * (0.6 + energy);

    // -----------------------------------------
    // 6. High frequency electric sparks
    // -----------------------------------------

    let electricPhase =
        a * 16.0 +
        r * 24.0 -
        t * 0.9;

    let electric = pow(
        1.0 - abs(sin(electricPhase)),
        34.0
    );

    let electricRegion = exp(
        -pow((r - 0.58) * 4.0, 2.0)
    );

    color += vec3f(0.4, 0.9, 2.0)
           * electric
           * electricRegion
           * (0.08 + treble * 1.5);

    // -----------------------------------------
    // 7. Central energy core
    // -----------------------------------------

    let coreRadius = 0.14 + bass * 0.055;

    let coreHalo = exp(-r * (9.0 - bass * 2.0));

    let coreShell = exp(
        -abs(r - coreRadius) * 100.0
    );

    let coreBody = 1.0 - smoothstep(
        0.02,
        coreRadius,
        r
    );

    let coreHot = exp(-r * r * 190.0);

    color += cyan
           * coreHalo
           * (0.5 + bass * 1.8);

    color += mix(
        cyan,
        pink,
        0.5 + 0.5 * sin(a * 6.0 - t * 0.45)
    ) * coreShell * (0.7 + bass);

    color += vec3f(0.08, 0.22, 0.6) * coreBody;

    color += vec3f(1.2, 2.0, 3.0)
           * coreHot
           * (0.8 + bass * 1.2);

    // -----------------------------------------
    // 8. Vignette and tone mapping
    // -----------------------------------------

    let vignette = 1.0 - 0.30 * smoothstep(
        0.5,
        1.8,
        r
    );

    color *= vignette;

    // Compress bright values into display range
    color = vec3f(1.0) - exp(-color * 1.25);

    return vec4f(color, 1.0);
}

)";

inline const char* shader3 = R"(

struct FrequencyBands {
    bass: f32,
    mid: f32,
    treble: f32,
};

struct Uniforms {
    time: f32,
    audioIntensity: f32,
    resolution: vec2f,
    bands: FrequencyBands,
};

@group(0) @binding(0)
var<uniform> uniforms: Uniforms;

struct VSOutput {
    @builtin(position) position: vec4f,
};

@vertex
fn scene_vs(@builtin(vertex_index) idx: u32) -> VSOutput {
    var positions = array<vec2f, 3>(
        vec2f(-1.0, -1.0),
        vec2f( 3.0, -1.0),
        vec2f(-1.0,  3.0)
    );

    var out: VSOutput;
    out.position = vec4f(positions[idx], 0.0, 1.0);
    return out;
}

// --------------------------------------------------
// Helpers
// --------------------------------------------------

fn hash21(p: vec2f) -> f32 {
    return fract(sin(dot(p, vec2f(127.1, 311.7))) * 43758.5453);
}

// Increase sensitivity for small FFT amplitudes.
// Adjust 50.0 depending on your audio levels.
fn audioLevel(value: f32) -> f32 {
    return sqrt(clamp(value * 50.0, 0.0, 1.0));
}

// A flowing neon ribbon.
// Each ribbon bends in response to the music.
fn neonRibbon(
    uv: vec2f,
    t: f32,
    phase: f32,
    offset: f32,
    bass: f32,
    mid: f32,
    treble: f32
) -> f32 {

    let slowWave =
        sin(uv.x * 2.4 + t * 0.45 + phase) * 0.12;

    let audioWave =
        sin(uv.x * 6.0 - t * 1.4 + phase) *
        (0.035 + mid * 0.22);

    let detailWave =
        sin(uv.x * 17.0 + t * 2.6 + phase) *
        (0.006 + treble * 0.055);

    let bassWave =
        sin(uv.x * 3.5 - t * 0.9 + phase) *
        bass * 0.11;

    let center =
        offset +
        slowWave +
        audioWave +
        detailWave +
        bassWave;

    let distanceToLine = abs(uv.y - center);

    let core = exp(-distanceToLine * 140.0);
    let innerGlow = exp(-distanceToLine * 32.0);
    let outerGlow = exp(-distanceToLine * 8.0);

    return core * 1.5 +
           innerGlow * 0.45 +
           outerGlow * 0.12;
}

// Interpolate the three audio bands across X.
fn frequencyLevel(
    x: f32,
    bass: f32,
    mid: f32,
    treble: f32
) -> f32 {

    if (x < 0.5) {
        return mix(
            bass,
            mid,
            smoothstep(0.0, 0.5, x)
        );
    }

    return mix(
        mid,
        treble,
        smoothstep(0.5, 1.0, x)
    );
}

// --------------------------------------------------
// Fragment shader
// --------------------------------------------------

@fragment
fn scene_fs(
    @builtin(position) fragCoord: vec4f
) -> @location(0) vec4f {

    let res = max(uniforms.resolution, vec2f(1.0));

    let uv = (fragCoord.xy - res * 0.5) * 2.0 / res.y;
    let screenUV = fragCoord.xy / res;

    let t = uniforms.time;

    let bass = audioLevel(uniforms.bands.bass);
    let mid = audioLevel(uniforms.bands.mid);
    let treble = audioLevel(uniforms.bands.treble);
    let energy = audioLevel(uniforms.audioIntensity);

    let cyan = vec3f(0.05, 0.9, 2.0);
    let violet = vec3f(0.65, 0.15, 1.5);
    let pink = vec3f(1.7, 0.08, 0.6);

    var color = vec3f(0.004, 0.006, 0.02);

    // --------------------------------------------------
    // 1. Atmospheric background
    // --------------------------------------------------

    let backgroundWave =
        sin(uv.x * 2.0 + t * 0.15) *
        sin(uv.y * 3.0 - t * 0.12);

    let atmosphere =
        0.5 + 0.5 * backgroundWave;

    color += vec3f(0.015, 0.008, 0.055) *
             atmosphere;

    // Subtle vertical glow.
    let verticalGlow = exp(-abs(uv.y) * 1.8);

    color += vec3f(0.01, 0.025, 0.07) *
             verticalGlow *
             (0.3 + energy);

    // --------------------------------------------------
    // 2. Three independent flowing energy ribbons
    // --------------------------------------------------

    let ribbon1 = neonRibbon(
        uv, t, 0.0, -0.34, bass, mid, treble
    );

    let ribbon2 = neonRibbon(
        uv, t, 2.1, -0.08, bass, mid, treble
    );

    let ribbon3 = neonRibbon(
        uv, t, 4.2, 0.20, bass, mid, treble
    );

    color += cyan *
             ribbon1 *
             (0.12 + bass * 1.6);

    color += violet *
             ribbon2 *
             (0.12 + mid * 1.6);

    color += pink *
             ribbon3 *
             (0.12 + treble * 1.6);

    // --------------------------------------------------
    // 3. Digital audio columns
    // --------------------------------------------------

    let numberOfBars = 64.0;

    let barPosition = screenUV.x * numberOfBars;
    let barIndex = floor(barPosition);

    let localX = fract(barPosition) - 0.5;

    let barShape =
        1.0 - smoothstep(0.28, 0.43, abs(localX));

    let xNormalized =
        (barIndex + 0.5) / numberOfBars;

    let band = frequencyLevel(
        xNormalized, bass, mid, treble
    );

    // Give each column a slightly different height.
    let randomness =
        0.65 + 0.35 *
        hash21(vec2f(barIndex, 7.2));

    let movement =
        0.85 + 0.15 *
        sin(t * 4.0 + barIndex * 1.7);

    let barHeight =
        0.035 +
        band * 0.78 * randomness * movement;

    // Bars grow upward from the bottom.
    let heightFromBottom = 0.87 - uv.y;

    let insideBar =
        step(0.0, heightFromBottom) *
        (1.0 - smoothstep(
            barHeight - 0.015,
            barHeight + 0.015,
            heightFromBottom
        ));

    // Segmented neon columns.
    let segments =
        step(0.20, fract(heightFromBottom * 33.0));

    let bar =
        barShape *
        insideBar *
        segments;

    var barColor = mix(
        cyan,
        violet,
        smoothstep(0.0, 0.5, xNormalized)
    );

    barColor = mix(
        barColor,
        pink,
        smoothstep(0.5, 1.0, xNormalized)
    );

    color += barColor *
             bar *
             (0.25 + band * 1.8);

    // Soft glow surrounding columns.
    let barGlow =
        exp(-abs(localX) * 5.0) *
        step(0.0, heightFromBottom) *
        (1.0 - smoothstep(
            barHeight,
            barHeight + 0.15,
            heightFromBottom
        ));

    color += barColor *
             barGlow *
             band * 0.11;

    // --------------------------------------------------
    // 4. Floating electric sparks
    // --------------------------------------------------

    let sparkUV =
        uv * vec2f(29.0, 21.0) +
        vec2f(
            t * 0.45,
            -t * (0.8 + treble * 2.5)
        );

    let sparkCell = floor(sparkUV);
    let sparkLocal = fract(sparkUV) - 0.5;

    let seed = hash21(sparkCell);

    let sparkOffset = vec2f(
        hash21(sparkCell + vec2f(2.3, 5.7)),
        hash21(sparkCell + vec2f(8.1, 3.2))
    ) - 0.5;

    let sparkDistance =
        length(sparkLocal - sparkOffset * 0.6);

    let sparkMask =
        step(0.982 - treble * 0.045, seed);

    let sparkCore =
        1.0 - smoothstep(
            0.015,
            0.095,
            sparkDistance
        );

    let sparkle =
        0.65 + 0.35 *
        sin(t * 8.0 + seed * 50.0);

    color += vec3f(0.5, 0.9, 2.0) *
             sparkMask *
             sparkCore *
             sparkle *
             (0.18 + treble * 2.5);

    // --------------------------------------------------
    // 5. Bass-driven illumination
    // --------------------------------------------------

    // A broad glow at the base of the visualization.
    let bassGlow =
        exp(-abs(uv.y - 0.84) * 13.0);

    color += mix(cyan, pink, 0.35) *
             bassGlow *
             bass * 0.32;

    // Global illumination responds to audio energy.
    color *= 0.8 + energy * 0.45;

    // --------------------------------------------------
    // 6. Vignette and tone mapping
    // --------------------------------------------------

    let centeredUV = screenUV * 2.0 - 1.0;

    let vignette =
        1.0 - 0.28 *
        smoothstep(
            0.3,
            1.5,
            length(centeredUV)
        );

    color *= vignette;

    color = vec3f(1.0) - exp(-color * 1.3);

    return vec4f(color, 1.0);
}

)";

}  // namespace c2::hard
