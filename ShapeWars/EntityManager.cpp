#include "EntityManager.h"
#include <iostream>

EntityManager::EntityManager() {

}

void EntityManager::update() {

	// add them to the vector of all entities
	// add them to the vector inside the map, with the tag as a key

	for (auto e : m_entitiesToAdd) {
		m_entities.push_back(e);
		// Added to map
		m_entityMap[e->tag()].push_back(e);
	}

	// removes dead entities from the vector
	removeDeadEntities(m_entities);

	// Removes dead entities from the entity map
	for (auto& pair : m_entityMap) {
		removeDeadEntities(pair.second);
	}

	// clears the entitiesToAdd as we have just added them
	m_entitiesToAdd.clear();

}

void EntityManager::removeDeadEntities(EntityVec& vec) {
	
	vec.erase(std::remove_if(vec.begin(), vec.end(),
		[](std::shared_ptr<Entity> const& e) {
			return !e->isActive();
		}), vec.end());
}

std::shared_ptr<Entity> EntityManager::addEntity(const std::string & tag) {

	auto entity = std::shared_ptr<Entity>(new Entity(m_totalEntities++, tag));

	m_entitiesToAdd.push_back(entity);
	m_entityMap[tag].push_back(entity);

	return entity;

}

const EntityVec& EntityManager::getEntities() {

	return m_entities;
}

const EntityVec& EntityManager::getEntities(const std::string& tag) {

	// might be wrong
	return m_entityMap[tag];
 }