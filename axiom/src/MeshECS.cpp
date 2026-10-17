#include "MeshECS.hpp"

void axm::MeshSystem::InitSystem() {}

void axm::MeshSystem::InitScene(Scene& scene) {}

void axm::MeshSystem::Update(Scene& scene) {}

void axm::MeshSystem::Render(Scene& scene) {}

void axm::MeshSystem::CleanupScene(Scene& scene) {}

void axm::MeshSystem::Cleanup() {}

json axm::MeshSystem::Serialize(const Scene& scene)
{
	return json();
}

bool axm::MeshSystem::Deserialize(const json& json)
{
	return false;
}
