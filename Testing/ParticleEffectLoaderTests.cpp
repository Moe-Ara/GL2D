#include <boost/test/unit_test.hpp>

#include "Exceptions/SubsystemExceptions.hpp"
#include "ParticleSystem/ParticleEffectLoader.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace {
std::filesystem::path writeParticleDocument(std::string_view document,
                                            std::string_view suffix) {
    const auto path = std::filesystem::temp_directory_path() /
        ("gl2d_particles_" + std::string(suffix) + ".json");
    std::ofstream output(path);
    output << document;
    return path;
}
}

BOOST_AUTO_TEST_SUITE(ParticleEffectLoaderTests)

BOOST_AUTO_TEST_CASE(loads_deterministic_curve_and_rotation_fields) {
    const auto path = writeParticleDocument(R"({"effects":[{
        "name":"glow", "maxParticles":32, "spawnRate":12,
        "minLifeTime":0.2, "maxLifeTime":0.4,
        "minSpeed":1, "maxSpeed":2, "minSize":3, "maxSize":6,
        "endSizeMultiplier":0.25, "minAngularVelocity":-2,
        "maxAngularVelocity":3, "randomSeed":1234,
        "startColor":[0.2,0.8,2.0,0.7],
        "endColor":[0.1,0.2,1.0,0.0]
    }]})", "valid");

    const auto effects = ParticleEffectLoader::loadFromFile(path.string());
    std::filesystem::remove(path);

    BOOST_REQUIRE_EQUAL(effects.size(), 1u);
    BOOST_TEST(effects[0].maxParticles == 32u);
    BOOST_TEST(effects[0].config.endSizeMultiplier == 0.25f);
    BOOST_TEST(effects[0].config.minAngularVelocity == -2.0f);
    BOOST_TEST(effects[0].config.randomSeed == 1234u);
    BOOST_TEST(effects[0].config.startColor.z == 2.0f);
}

BOOST_AUTO_TEST_CASE(rejects_negative_capacity_before_allocation) {
    const auto path = writeParticleDocument(
        R"([{"name":"bad","maxParticles":-1}])", "negative_capacity");
    BOOST_CHECK_THROW(ParticleEffectLoader::loadFromFile(path.string()),
                      Engine::ParticleException);
    std::filesystem::remove(path);
}

BOOST_AUTO_TEST_CASE(rejects_wrong_vector_shapes_instead_of_hiding_them) {
    const auto path = writeParticleDocument(
        R"([{"name":"bad","gravity":[0]}])", "bad_vector");
    BOOST_CHECK_THROW(ParticleEffectLoader::loadFromFile(path.string()),
                      Engine::ParticleException);
    std::filesystem::remove(path);
}

BOOST_AUTO_TEST_SUITE_END()
