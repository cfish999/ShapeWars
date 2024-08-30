#pragma once

#include "Component.h"
#include <memory>
#include <string>
using namespace std;


class Entity {

	friend class EntityManager;

	bool m_active = true;
	size_t m_id = 0;
	string m_tag = "default";

	// constructor and destructor
	Entity(const size_t id, const string& tag);

public:

	// component pointers
	shared_ptr<CTransform> cTransform;
	shared_ptr<CShape> cShape;
	shared_ptr<CCollision> cCollision;
	shared_ptr<CInput> cInput;
	shared_ptr<CScore> cScore;
	shared_ptr<CLifespan> cLifeSpan;
	shared_ptr<CVertices> cVertices;
	shared_ptr<CRandColour> cRandColour;


	// private member access functions
	bool isActive() const;
	const string& tag() const;
	const size_t& id() const;
	void destroy();
 
};