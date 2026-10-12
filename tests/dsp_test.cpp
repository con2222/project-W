#include <audio.hpp>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <complex>
#include <cstddef>
#include <numbers>
#include <numeric>
#include <random>
#include <vector>

using namespace c2::audio;

namespace {

constexpr double testPi = std::numbers::pi_v<double>;

void requireNear(float actual, double expected, double absolute = 1e-5,
                 double relative = 2e-5) {
    INFO("actual = " << actual << ", expected = " << expected);

    REQUIRE(std::isfinite(actual));
    REQUIRE_THAT(static_cast<double>(actual),
                 Catch::Matchers::WithinAbs(expected, absolute) ||
                     Catch::Matchers::WithinRel(expected, relative));
}

void requireComplexNear(std::complex<float> actual,
                        std::complex<double> expected, double absolute = 1e-5,
                        double relative = 2e-5) {
    requireNear(actual.real(), expected.real(), absolute, relative);
    requireNear(actual.imag(), expected.imag(), absolute, relative);
}

std::vector<float> makeCosine(std::size_t count, double bin, double amplitude,
                              double phase = 0.0) {
    std::vector<float> samples(count);

    for (std::size_t n = 0; n < count; ++n) {
        const double angle = 2.0 * testPi * bin * static_cast<double>(n) /
                                 static_cast<double>(count) +
                             phase;

        samples[n] = static_cast<float>(amplitude * std::cos(angle));
    }

    return samples;
}

std::vector<float> makeRandomSignal(std::size_t count) {
    std::mt19937 generator(0xC2D5u);
    std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);

    std::vector<float> samples(count);

    for (float& sample : samples) {
        sample = distribution(generator);
    }

    return samples;
}

std::vector<std::complex<double>> referenceDft(
    const std::vector<float>& samples) {
    const std::size_t count = samples.size();
    std::vector<std::complex<double>> result(count);

    for (std::size_t k = 0; k < count; ++k) {
        for (std::size_t n = 0; n < count; ++n) {
            const double angle = -2.0 * testPi * static_cast<double>(k) *
                                 static_cast<double>(n) /
                                 static_cast<double>(count);

            result[k] += static_cast<double>(samples[n]) *
                         std::complex<double>(std::cos(angle), std::sin(angle));
        }
    }

    return result;
}

}  // namespace

TEST_CASE("DFT handles empty and single-sample input", "[dsp][dft]") {
    SECTION("Empty input") { REQUIRE(dft(std::vector<float>{}).empty()); }

    SECTION("One sample") {
        const auto spectrum = dft(std::vector<float>{-0.75f});

        REQUIRE(spectrum.size() == 1);
        requireComplexNear(spectrum[0], {-0.75, 0.0});
    }
}

TEST_CASE("DFT supports odd lengths", "[dsp][dft]") {
    const auto samples = makeCosine(5, 1.0, 0.5);
    const auto spectrum = dft(samples);

    REQUIRE(spectrum.size() == 5);

    for (std::size_t k = 0; k < spectrum.size(); ++k) {
        CAPTURE(k);

        const double expected = (k == 1 || k == 4) ? 1.25 : 0.0;
        requireComplexNear(spectrum[k], {expected, 0.0});
    }
}

TEST_CASE("FFT rejects empty and non-power-of-two input", "[dsp][fft]") {
    for (std::size_t count : {0u, 3u, 6u, 10u}) {
        CAPTURE(count);

        REQUIRE(fft(std::vector<float>(count, 1.0f)).empty());
        REQUIRE(
            fft(std::vector<std::complex<float>>(count, {1.0f, 0.0f})).empty());
    }
}

TEST_CASE("FFT handles a single real or complex sample", "[dsp][fft]") {
    SECTION("Real sample") {
        const auto spectrum = fft(std::vector<float>{-0.75f});

        REQUIRE(spectrum.size() == 1);
        requireComplexNear(spectrum[0], {-0.75, 0.0});
    }

    SECTION("Complex sample") {
        const auto spectrum =
            fft(std::vector<std::complex<float>>{{0.25f, -0.5f}});

        REQUIRE(spectrum.size() == 1);
        requireComplexNear(spectrum[0], {0.25, -0.5});
    }
}

TEST_CASE("DFT and FFT match known four-point transforms", "[dsp][dft][fft]") {
    std::vector<float> samples;
    std::vector<std::complex<double>> expected;

    SECTION("Silence") {
        samples = {0, 0, 0, 0};
        expected = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
    }

    SECTION("Constant signal has only DC") {
        samples = {1, 1, 1, 1};
        expected = {{4, 0}, {0, 0}, {0, 0}, {0, 0}};
    }

    SECTION("Impulse at sample zero") {
        samples = {1, 0, 0, 0};
        expected = {{1, 0}, {1, 0}, {1, 0}, {1, 0}};
    }

    SECTION("Shifted impulse checks the imaginary sign") {
        samples = {0, 1, 0, 0};
        expected = {{1, 0}, {0, -1}, {-1, 0}, {0, 1}};
    }

    const auto direct = dft(samples);
    const auto fast = fft(samples);

    REQUIRE(direct.size() == expected.size());
    REQUIRE(fast.size() == expected.size());

    for (std::size_t k = 0; k < expected.size(); ++k) {
        CAPTURE(k);

        requireComplexNear(direct[k], expected[k]);
        requireComplexNear(fast[k], expected[k]);
    }
}

TEST_CASE("Complex FFT keeps the imaginary input component", "[dsp][fft]") {
    const std::vector<std::complex<float>> samples = {
        {0, 0}, {0, 1}, {0, 0}, {0, 0}};

    const std::vector<std::complex<double>> expected = {
        {0, 1}, {1, 0}, {0, -1}, {-1, 0}};

    const auto spectrum = fft(samples);

    REQUIRE(spectrum.size() == expected.size());

    for (std::size_t k = 0; k < expected.size(); ++k) {
        CAPTURE(k);
        requireComplexNear(spectrum[k], expected[k]);
    }
}

TEST_CASE("FFT overloads agree and transforms preserve input",
          "[dsp][fft][dft]") {
    const auto samples = makeRandomSignal(32);
    const auto original = samples;

    std::vector<std::complex<float>> complexSamples;
    for (float sample : samples) {
        complexSamples.emplace_back(sample, 0.0f);
    }

    const auto originalComplex = complexSamples;

    const auto realSpectrum = fft(samples);
    const auto complexSpectrum = fft(complexSamples);
    const auto direct = dft(samples);

    REQUIRE(realSpectrum.size() == samples.size());
    REQUIRE(complexSpectrum.size() == samples.size());
    REQUIRE(direct.size() == samples.size());

    REQUIRE(samples == original);
    REQUIRE(complexSamples == originalComplex);

    for (std::size_t k = 0; k < samples.size(); ++k) {
        CAPTURE(k);

        requireComplexNear(realSpectrum[k],
                           std::complex<double>(complexSpectrum[k]));
    }
}

TEST_CASE("DFT agrees with a double reference", "[dsp][dft]") {
    for (std::size_t count : {3u, 5u, 8u, 16u, 32u, 64u}) {
        CAPTURE(count);

        const auto samples = makeRandomSignal(count);
        const auto expected = referenceDft(samples);
        const auto actual = dft(samples);

        REQUIRE(actual.size() == expected.size());

        for (std::size_t k = 0; k < count; ++k) {
            CAPTURE(k);
            requireComplexNear(actual[k], expected[k], 5e-6 * count, 5e-5);
        }
    }
}

TEST_CASE("FFT agrees with a double DFT reference", "[dsp][fft]") {
    for (std::size_t count : {8u, 32u, 128u, 256u}) {
        CAPTURE(count);

        const auto samples = makeRandomSignal(count);
        const auto expected = referenceDft(samples);
        const auto actual = fft(samples);

        REQUIRE(actual.size() == expected.size());

        for (std::size_t k = 0; k < count; ++k) {
            CAPTURE(k);
            requireComplexNear(actual[k], expected[k], 1e-4, 2e-5);
        }
    }
}

TEST_CASE("FFT resolves a 2048-sample sine", "[dsp][fft]") {
    constexpr std::size_t count = 2048;
    constexpr std::size_t bin = 37;

    std::vector<float> samples(count);

    for (std::size_t n = 0; n < count; ++n) {
        samples[n] =
            static_cast<float>(0.5 * std::sin(2.0 * testPi * bin *
                                              static_cast<double>(n) / count));
    }

    const auto spectrum = fft(samples);

    REQUIRE(spectrum.size() == count);

    for (std::size_t k = 0; k < count; ++k) {
        CAPTURE(k);

        double imaginary = 0.0;

        if (k == bin) {
            imaginary = -512.0;
        } else if (k == count - bin) {
            imaginary = 512.0;
        }

        requireComplexNear(spectrum[k], {0.0, imaginary}, 1e-3, 2e-5);
    }
}

TEST_CASE("Amplitude spectrum handles empty input", "[dsp][amplitude]") {
    REQUIRE(amplitudeSpectrum({}, 1.0f).empty());
}

TEST_CASE("Even amplitude spectrum uses magnitudes and normalization",
          "[dsp][amplitude]") {
    const std::vector<std::complex<float>> spectrum = {
        {8, 0},  {3, 4},     {0, -4},    {-4, 0},
        {-8, 0}, {100, 100}, {100, 100}, {100, 100}};

    const auto amplitudes = amplitudeSpectrum(spectrum, 4.0f);
    const std::vector<double> expected = {2.0, 2.5, 2.0, 2.0, 2.0};

    REQUIRE(amplitudes.size() == expected.size());

    for (std::size_t k = 0; k < expected.size(); ++k) {
        CAPTURE(k);
        requireNear(amplitudes[k], expected[k]);
    }
}

TEST_CASE("Odd amplitude spectrum doubles its last positive bin",
          "[dsp][amplitude]") {
    std::vector<std::complex<float>> spectrum(9);
    spectrum[0] = {9.0f, 0.0f};
    spectrum[4] = {0.0f, 4.5f};

    const auto amplitudes = amplitudeSpectrum(spectrum, 9.0f);

    REQUIRE(amplitudes.size() == 5);
    requireNear(amplitudes[0], 1.0);
    requireNear(amplitudes[4], 1.0);

    for (std::size_t k = 1; k < 4; ++k) {
        CAPTURE(k);
        requireNear(amplitudes[k], 0.0);
    }
}

TEST_CASE("Amplitude spectrum handles sizes one and two", "[dsp][amplitude]") {
    SECTION("One point contains only DC") {
        const auto amplitudes = amplitudeSpectrum({{3.0f, 4.0f}}, 1.0f);

        REQUIRE(amplitudes.size() == 1);
        requireNear(amplitudes[0], 5.0);
    }

    SECTION("Two points contain only DC and Nyquist") {
        const auto amplitudes =
            amplitudeSpectrum({{2.0f, 0.0f}, {0.0f, -2.0f}}, 2.0f);

        REQUIRE(amplitudes.size() == 2);
        requireNear(amplitudes[0], 1.0);
        requireNear(amplitudes[1], 1.0);
    }
}

TEST_CASE("Amplitude conversion preserves its input", "[dsp][amplitude]") {
    const auto spectrum = fft(makeRandomSignal(16));
    const auto original = spectrum;

    const auto amplitudes = amplitudeSpectrum(spectrum, 16.0f);

    REQUIRE(amplitudes.size() == 9);
    REQUIRE(spectrum == original);
}

TEST_CASE("Periodic Hann window scales samples correctly", "[dsp][hann]") {
    SECTION("Empty array remains empty") {
        std::vector<float> samples;
        applyHannWindow(samples);
        REQUIRE(samples.empty());
    }

    SECTION("One sample remains unchanged") {
        std::vector<float> samples = {0.5f};
        applyHannWindow(samples);
        REQUIRE(samples.size() == 1);
        requireNear(samples[0], 0.5);
    }

    SECTION("Four ones are shaped into a Hann window") {
        std::vector<float> samples = {1.0f, 1.0f, 1.0f, 1.0f};
        applyHannWindow(samples);

        REQUIRE(samples.size() == 4);
        requireNear(samples[0], 0.0);
        requireNear(samples[1], 0.5);
        requireNear(samples[2], 1.0);
        requireNear(samples[3], 0.5);
    }

    SECTION("Window multiplies existing samples") {
        std::vector<float> samples = {2.0f, -2.0f, 3.0f, -4.0f};
        applyHannWindow(samples);

        REQUIRE(samples.size() == 4);
        requireNear(samples[0], 0.0);
        requireNear(samples[1], -1.0);
        requireNear(samples[2], 3.0);
        requireNear(samples[3], -2.0);
    }

    SECTION("Sum of coefficients is approximately N / 2") {
        constexpr std::size_t N = 256;
        std::vector<float> samples(N, 1.0f);
        applyHannWindow(samples);

        const float sum = std::accumulate(samples.begin(), samples.end(), 0.0f);
        requireNear(sum, static_cast<double>(N) / 2.0);
    }
}

TEST_CASE("Amplitude spectrum scales with normalization factor",
          "[dsp][amplitude]") {
    const std::vector<std::complex<float>> spectrum = {
        {0.0f, 0.0f}, {3.0f, 4.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}};

    const auto amplitudesBase = amplitudeSpectrum(spectrum, 10.0f);
    REQUIRE(amplitudesBase.size() == 3);
    requireNear(amplitudesBase[1], 1.0);

    const auto amplitudesDoubled = amplitudeSpectrum(spectrum, 20.0f);
    REQUIRE(amplitudesDoubled.size() == 3);
    requireNear(amplitudesDoubled[1], 0.5);
}

TEST_CASE("Full DSP chain resolves signal amplitudes", "[dsp][chain]") {
    constexpr std::size_t N = 256;
    constexpr float normalization = static_cast<float>(N) / 2.0f;

    SECTION("Silence results in zero amplitudes") {
        std::vector<float> signal(N, 0.0f);
        applyHannWindow(signal);

        const auto spectrum = fft(signal);
        const auto amplitudes = amplitudeSpectrum(spectrum, normalization);

        REQUIRE(amplitudes.size() == N / 2 + 1);
        for (std::size_t k = 0; k < amplitudes.size(); ++k) {
            CAPTURE(k);
            requireNear(amplitudes[k], 0.0);
        }
    }

    SECTION("Cosine at bin 10 is broadened by the Hann window") {
        auto signal = makeCosine(N, 10.0, 0.5);
        applyHannWindow(signal);

        const auto spectrum = fft(signal);
        const auto amplitudes = amplitudeSpectrum(spectrum, normalization);

        REQUIRE(amplitudes.size() == N / 2 + 1);

        requireNear(amplitudes[10], 0.5, 1e-4);
        requireNear(amplitudes[9], 0.25, 1e-4);
        requireNear(amplitudes[11], 0.25, 1e-4);
    }

    SECTION("Sum of two cosines resolves into two peaks") {
        const auto cosine1 = makeCosine(N, 10.0, 0.5);
        const auto cosine2 = makeCosine(N, 37.0, 0.25);

        std::vector<float> signal(N);
        for (std::size_t i = 0; i < N; ++i) {
            signal[i] = cosine1[i] + cosine2[i];
        }

        applyHannWindow(signal);

        const auto spectrum = fft(signal);
        const auto amplitudes = amplitudeSpectrum(spectrum, normalization);

        REQUIRE(amplitudes.size() == N / 2 + 1);
        requireNear(amplitudes[10], 0.5, 1e-4);
        requireNear(amplitudes[37], 0.25, 1e-4);
    }

    SECTION("Constant signal peak is preserved at DC") {
        std::vector<float> signal(N, 0.25f);
        applyHannWindow(signal);

        const auto spectrum = fft(signal);
        const auto amplitudes = amplitudeSpectrum(spectrum, normalization);

        REQUIRE(amplitudes.size() == N / 2 + 1);
        requireNear(amplitudes[0], 0.25, 1e-4);
    }

    SECTION("Alternating signal peak is preserved at Nyquist frequency") {
        std::vector<float> signal(N);
        for (std::size_t i = 0; i < N; ++i) {
            signal[i] = (i % 2 == 0) ? 0.5f : -0.5f;
        }

        applyHannWindow(signal);

        const auto spectrum = fft(signal);
        const auto amplitudes = amplitudeSpectrum(spectrum, normalization);

        REQUIRE(amplitudes.size() == N / 2 + 1);
        requireNear(amplitudes[N / 2], 0.5, 1e-4);
    }
}