#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>

struct Particle {
    float x, y, z;
    float vx, vy, vz;
    std::uint32_t color;
};

struct Particles {
    std::vector<float> x, y, z;
    std::vector<float> vx, vy, vz;
    std::vector<std::uint32_t> color;

    explicit Particles(std::size_t n) : x(n), y(n), z(n), vx(n), vy(n), vz(n), color(n) {}
};

using Clock = std::chrono::steady_clock;

[[gnu::noipa]] void step_aos(Particle* particles, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        particles[i].z += particles[i].vz;
    }
}

[[gnu::noipa]] void step_soa(float* __restrict__ x, float* __restrict__ y, float* __restrict__ z,
                           const float* __restrict__ vx, const float* __restrict__ vy,
                           const float* __restrict__ vz, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        x[i] += vx[i];
        y[i] += vy[i];
        z[i] += vz[i];
    }
}

Particle initial_particle(std::size_t i) {
    float value = static_cast<float>(i % 1024);
    return {value, value + 1, value + 2, 1, 2, 3, static_cast<std::uint32_t>(i)};
}

void run_aos(std::size_t n) {
    std::vector<Particle> particles(n);
    for (std::size_t i = 0; i < n; ++i) {
        particles[i] = initial_particle(i);
    }

    auto start = Clock::now();
    step_aos(particles.data(), n);
    auto end = Clock::now();

    std::uint64_t checksum = 0;
    for (const auto& p : particles) {
        checksum += static_cast<std::uint64_t>(p.x + p.y + p.z);
    }
    std::cout << "AoS: " << std::chrono::duration<double, std::milli>(end - start).count()
              << " ms, checksum " << checksum << '\n';
}

void run_soa(std::size_t n) {
    Particles particles(n);
    for (std::size_t i = 0; i < n; ++i) {
        Particle p = initial_particle(i);
        particles.x[i] = p.x;
        particles.y[i] = p.y;
        particles.z[i] = p.z;
        particles.vx[i] = p.vx;
        particles.vy[i] = p.vy;
        particles.vz[i] = p.vz;
        particles.color[i] = p.color;
    }

    auto start = Clock::now();
    step_soa(particles.x.data(), particles.y.data(), particles.z.data(),
             particles.vx.data(), particles.vy.data(), particles.vz.data(), n);
    auto end = Clock::now();

    std::uint64_t checksum = 0;
    for (std::size_t i = 0; i < n; ++i) {
        checksum += static_cast<std::uint64_t>(particles.x[i] + particles.y[i] + particles.z[i]);
    }
    std::cout << "SoA: " << std::chrono::duration<double, std::milli>(end - start).count()
              << " ms, checksum " << checksum << '\n';
}

int main(int argc, char** argv) {
    constexpr std::size_t n = 10'000'000;
    std::cout << std::fixed << std::setprecision(3);
    if (argc == 2 && std::string_view(argv[1]) == "aos") {
        run_aos(n);
    } else if (argc == 2 && std::string_view(argv[1]) == "soa") {
        run_soa(n);
    } else {
        std::cerr << "Usage: " << argv[0] << " aos|soa\n";
        return 1;
    }
}
