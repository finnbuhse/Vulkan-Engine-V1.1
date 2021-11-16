#include "Interactable.h"

void Interactable::subscribeInteractEvent(const InteractCallback* callback)
{
	interactCallbacks.push((InteractCallback*)callback);
}

void Interactable::unsubscribeChangedEvent(const InteractCallback* callback)
{
	interactCallbacks.remove(interactCallbacks.find((InteractCallback*)callback));
}

InteractSystem::InteractSystem()
{

}
