#include "ECS/ECS.hpp"

axm::Entity axm::Scene::CreateEntity()
{
	return Entity(*this);
}

void axm::Scene::DestroyEntity(const Entity& entity)
{
	m_World.delete_with(entity.m_Entity);
}

flecs::entity axm::Scene::GenerateEntityHandle()
{
	return m_World.entity();
}