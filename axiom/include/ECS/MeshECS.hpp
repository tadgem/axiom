#pragma once
#include "Assets/Assets.hpp"
#include "ECS.hpp"
#include "Render/Mesh.hpp"

namespace axm
{

struct MeshComponent
{
	Mesh        m_Mesh;
	AssetHandle m_Handle;
};
class MeshSystem : public ISystem
{
public:
	void InitSystem() override;
	void InitScene(Scene& scene) override;
	void Update(Scene& scene) override;
	void Render(Scene& scene) override;
	void CleanupScene(Scene& scene) override;
	void Cleanup() override;
	json Serialize(const Scene& scene) override;
	bool Deserialize(const json& json) override;
};
}  // namespace axm
