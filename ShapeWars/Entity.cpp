#include "Entity.h"
#include <string>
#include <iostream>
using namespace std;

Entity::Entity(const size_t id, const string & tag)
	: m_id(id)
	, m_tag(tag)
	{
	}


bool Entity::isActive() const
{
	return m_active;
}

const string & Entity::tag() const
{
	return m_tag;
}

const size_t & Entity::id() const
{
	return m_id;
}

void Entity::destroy()
{
	m_active = false;
}
