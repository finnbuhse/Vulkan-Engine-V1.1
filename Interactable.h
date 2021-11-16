#pragma once
#include "Entity.h"
#include "Vector.h"
#include <vector>
#include <functional>

typedef std::function<void(Entity&)> InteractCallback;

struct Interactable
{
	Vector<InteractCallback*> interactCallbacks;

	void subscribeInteractEvent(const InteractCallback* callback);
	void unsubscribeChangedEvent(const InteractCallback* callback);
};

struct InteractController
{
	float interactDistance;
};

class InteractSystem
{
private:


	InteractSystem();

	void interact();

public:
	static InteractSystem& instance();

};