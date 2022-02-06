#pragma once
#include "Interactable.h"
#include "Vector.h"

struct GunItem
{
	glm::vec3 instantiatePosition;
	glm::quat instantiateRotation;
	glm::vec3 instantiateScale;
	std::string* gunWeaponFilename;
	Vector<char> serializedGunWeapon;

	void initialize(const char* weaponFilename);
};

struct GunWeapon
{
	glm::vec3 instantiatePosition;
	glm::quat instantiateRotation;
	glm::vec3 instantiateScale;
	glm::vec3 instantiateForce;
	glm::vec3 muzzlePosition;
	std::string* gunItemFilename;
	Vector<char> serializedGunItem;
	std::string* bulletFilename;
	Vector<char> serializedBullet;

	Vector<EntityID> bullets;

	void initialize(const char* itemFilename, const char* bulletFilename);
};

template <>
std::vector<char> serialize(const GunItem& gunItem);

template <>
void deserialize(const std::vector<char>& vecData, GunItem& gunItem);

template <>
std::vector<char> serialize(const GunWeapon& gunWeapon);

template <>
void deserialize(const std::vector<char>& vecData, GunWeapon& gunWeapon);

struct Bullet
{
	glm::vec3 fireForce;
};

class GunSystem
{
private:
	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();
	ComponentManager<RigidBody>& mRigidBodyManager = ComponentManager<RigidBody>::instance();
	ComponentManager<Interactable>& mInteractableManager = ComponentManager<Interactable>::instance();
	ComponentManager<GunItem>& mGunItemManager = ComponentManager<GunItem>::instance();
	ComponentManager<GunWeapon>& mGunWeaponManager = ComponentManager<GunWeapon>::instance();
	ComponentManager<CharacterController>& mControllerManager = ComponentManager<CharacterController>::instance();
	WindowManager& mWindowManager = WindowManager::instance();

	const ComponentAddedCallback mComponentAddedCallback = std::bind(&GunSystem::componentAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mComponentRemovedCallback = std::bind(&GunSystem::componentRemoved, this, std::placeholders::_1);
	const ComponentAddedCallback mWeaponComponentAddedCallback = std::bind(&GunSystem::weaponComponentAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mWeaponComponentRemovedCallback = std::bind(&GunSystem::weaponComponentRemoved, this, std::placeholders::_1);
	const InteractCallback mInteractCallback = std::bind(&GunSystem::interact, this, std::placeholders::_1);
	const KeyCallback mThrowCallback = std::bind(&GunSystem::throwEquipedGun, this);
	const KeyCallback mShootCallback = std::bind(&GunSystem::shoot, this);

	Composition mGunItemComposition;
	Composition mGunWeaponComposition;

	EntityID mEquipedGun = NULL;

	GunSystem();

public:
	static GunSystem& instance()
	{
		static GunSystem instance;
		return instance;
	}

	GunSystem(const GunSystem& copy) = delete;
	~GunSystem();

	void componentAdded(const Entity& entity);
	void componentRemoved(const Entity& entity);

	void weaponComponentAdded(const Entity& entity);
	void weaponComponentRemoved(const Entity& entity);

	void interact(const Entity& entity);

	void throwEquipedGun();

	void shoot();
};
