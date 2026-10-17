#pragma once
#include "Core/JSON.hpp"
#include "flecs.h"
namespace axm
{
class Scene;

class Entity
{
public:
	flecs::entity m_Entity;

private:
	Entity(Scene& scene);

	Scene& m_ParentScene;

	friend class Scene;
};

class Scene
{
public:
	flecs::world m_World;

	Entity       CreateEntity();
	void         DestroyEntity(const Entity& entity);

private:
	friend class Entity;

	flecs::entity GenerateEntityHandle();
};

class ISystem
{
public:
	virtual void Init()        = 0;
	virtual void Update()      = 0;
	virtual void Render()      = 0;
	virtual void Quit()        = 0;

	virtual json Serialize()   = 0;
	virtual bool Deserialize() = 0;
};

class ECSDispatcher
{
private:
	DynArray<Unique<ISystem>> m_Systems;

public:
	template <class ISystem, typename... Args>
	ISystem& AddSystem(Args&&... args)
	{
		m_Systems.push_back(MakeUnique<ISystem>(std::forward<Args>(args)...));

		return *m_Systems.back().get();
	}
};
}  // namespace axm
