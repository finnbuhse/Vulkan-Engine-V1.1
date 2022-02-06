#include "Gun.h"
#include "SceneManager.h"

void GunItem::initialize(const char* weaponFilename)
{
	*gunWeaponFilename = weaponFilename;
	std::vector<char> vecData = readFile(weaponFilename);
	serializedGunWeapon.resize(vecData.size());
	memcpy(serializedGunWeapon.data, vecData.data(), vecData.size());
}

void GunWeapon::initialize(const char* itemFilename, const char* bulletFilename)
{
	*gunItemFilename = itemFilename;
	std::vector<char> vecData = readFile(itemFilename);
	serializedGunItem.resize(vecData.size());
	memcpy(serializedGunItem.data, vecData.data(), vecData.size());

	*(this->bulletFilename) = bulletFilename;
	vecData = readFile(bulletFilename);
	serializedBullet.resize(vecData.size());
	memcpy(serializedBullet.data, vecData.data(), vecData.size());
}

template <>
std::vector<char> serialize(const GunItem& gunItem)
{
	std::vector<char> serializedGunItem;

	std::vector<char> vecData = serialize(*gunItem.gunWeaponFilename);
	serializedGunItem.insert(serializedGunItem.end(), vecData.begin(), vecData.end());

	vecData = serialize(gunItem.instantiatePosition);
	serializedGunItem.insert(serializedGunItem.end(), vecData.begin(), vecData.end());

	vecData = serialize(gunItem.instantiateRotation);
	serializedGunItem.insert(serializedGunItem.end(), vecData.begin(), vecData.end());

	vecData = serialize(gunItem.instantiateScale);
	serializedGunItem.insert(serializedGunItem.end(), vecData.begin(), vecData.end());

	return serializedGunItem;
}

template <>
void deserialize(const std::vector<char>& vecData, GunItem& gunItem)
{
	unsigned int begin = 0;
	unsigned int size = sizeof(unsigned int);
	unsigned int fileNameLength;
	deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), fileNameLength);
	begin += size;

	size = fileNameLength;
	std::string filename;
	deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), filename);
	gunItem.initialize(filename.c_str());
	begin += size;

	size = sizeof(glm::vec3);
	deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), gunItem.instantiatePosition);
	begin += size;

	size = sizeof(glm::quat);
	deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), gunItem.instantiateRotation);
	begin += size;

	size = sizeof(glm::vec3);
	deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), gunItem.instantiateScale);
}

template <>
std::vector<char> serialize(const GunWeapon& gunWeapon)
{
	std::vector<char> serializedGunWeapon;

	std::vector<char> vecData = serialize(*gunWeapon.gunItemFilename);
	serializedGunWeapon.insert(serializedGunWeapon.end(), vecData.begin(), vecData.end());

	vecData = serialize(*gunWeapon.bulletFilename);
	serializedGunWeapon.insert(serializedGunWeapon.end(), vecData.begin(), vecData.end());

	vecData = serialize(gunWeapon.instantiatePosition);
	serializedGunWeapon.insert(serializedGunWeapon.end(), vecData.begin(), vecData.end());

	vecData = serialize(gunWeapon.instantiateRotation);
	serializedGunWeapon.insert(serializedGunWeapon.end(), vecData.begin(), vecData.end());

	vecData = serialize(gunWeapon.instantiateScale);
	serializedGunWeapon.insert(serializedGunWeapon.end(), vecData.begin(), vecData.end());

	vecData = serialize(gunWeapon.instantiateForce);
	serializedGunWeapon.insert(serializedGunWeapon.end(), vecData.begin(), vecData.end());

	vecData = serialize(gunWeapon.muzzlePosition);
	serializedGunWeapon.insert(serializedGunWeapon.end(), vecData.begin(), vecData.end());

	return serializedGunWeapon;
}

template <>
void deserialize(const std::vector<char>& vecData, GunWeapon& gunWeapon)
{
	unsigned int begin = 0;
	unsigned int size = sizeof(unsigned int);
	unsigned int itemFilenameLength;
	deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), itemFilenameLength);
	begin += size;

	size = itemFilenameLength;
	std::string itemFilename;
	deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), itemFilename);
	begin += size;

	size = sizeof(unsigned int);
	unsigned int bulletFilenameLength;
	deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), bulletFilenameLength);
	begin += size;

	size = bulletFilenameLength;
	std::string bulletFilename;
	deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), bulletFilename);
	begin += size;

	gunWeapon.initialize(itemFilename.c_str(), bulletFilename.c_str());

	size = sizeof(glm::vec3);
	deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), gunWeapon.instantiatePosition);
	begin += size;

	size = sizeof(glm::quat);
	deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), gunWeapon.instantiateRotation);
	begin += size;

	size = sizeof(glm::vec3);
	deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), gunWeapon.instantiateScale);
	begin += size;

	deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), gunWeapon.instantiateForce);
	begin += size;

	deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), gunWeapon.muzzlePosition);
}

GunSystem::GunSystem()
{
	mTransformManager.subscribeAddedEvent(&mComponentAddedCallback);
	mTransformManager.subscribeRemovedEvent(&mComponentRemovedCallback);
	mRigidBodyManager.subscribeAddedEvent(&mComponentAddedCallback);
	mRigidBodyManager.subscribeRemovedEvent(&mComponentRemovedCallback);
	mInteractableManager.subscribeAddedEvent(&mComponentAddedCallback);
	mInteractableManager.subscribeRemovedEvent(&mComponentRemovedCallback);
	mGunItemManager.subscribeAddedEvent(&mComponentAddedCallback);
	mGunItemManager.subscribeRemovedEvent(&mComponentRemovedCallback);

	mTransformManager.subscribeAddedEvent(&mWeaponComponentAddedCallback);
	mTransformManager.subscribeRemovedEvent(&mWeaponComponentRemovedCallback);
	mGunWeaponManager.subscribeAddedEvent(&mWeaponComponentAddedCallback);
	mGunWeaponManager.subscribeRemovedEvent(&mWeaponComponentRemovedCallback);

	mGunItemComposition = mTransformManager.bit | mRigidBodyManager.bit | mInteractableManager.bit | mGunItemManager.bit;
	mGunWeaponComposition = mTransformManager.bit | mGunWeaponManager.bit;

	mWindowManager.subscribeKeyPressEvent(F, &mThrowCallback);
	mWindowManager.subscribeMouseButtonEvent(&mShootCallback);
}

GunSystem::~GunSystem()
{
	mTransformManager.subscribeAddedEvent(&mComponentAddedCallback);
	mTransformManager.subscribeRemovedEvent(&mComponentRemovedCallback);
	mRigidBodyManager.subscribeAddedEvent(&mComponentAddedCallback);
	mRigidBodyManager.subscribeRemovedEvent(&mComponentRemovedCallback);
	mInteractableManager.subscribeAddedEvent(&mComponentAddedCallback);
	mInteractableManager.subscribeRemovedEvent(&mComponentRemovedCallback);
	mGunItemManager.subscribeAddedEvent(&mComponentAddedCallback);
	mGunItemManager.subscribeRemovedEvent(&mComponentRemovedCallback);

	mTransformManager.unsubscribeAddedEvent(&mWeaponComponentAddedCallback);
	mTransformManager.unsubscribeRemovedEvent(&mWeaponComponentRemovedCallback);
	mGunWeaponManager.unsubscribeAddedEvent(&mWeaponComponentAddedCallback);
	mGunWeaponManager.unsubscribeRemovedEvent(&mWeaponComponentRemovedCallback);

	mWindowManager.unsubscribeKeyPressEvent(F, &mThrowCallback);
	mWindowManager.unsubscribeMouseButtonEvent(&mShootCallback);
}

void GunSystem::componentAdded(const Entity& entity)
{
	if ((entity.composition() & mGunItemComposition) == mGunItemComposition)
	{
		Interactable& interactable = entity.getComponent<Interactable>();
		interactable.subscribeInteractEvent(&mInteractCallback);

		GunItem& gunItem = entity.getComponent<GunItem>();
		gunItem.gunWeaponFilename = new std::string();
		gunItem.serializedGunWeapon.initialize();
	}
}

void GunSystem::componentRemoved(const Entity& entity)
{
	if ((entity.composition() & mGunItemComposition) == mGunItemComposition)
	{
		Interactable& interactable = entity.getComponent<Interactable>();
		if(interactable.interactCallbacks.data)
			interactable.unsubscribeInteractEvent(&mInteractCallback);

		GunItem& gunItem = entity.getComponent<GunItem>();
		delete gunItem.gunWeaponFilename;
		gunItem.serializedGunWeapon.free();
	}
}

void GunSystem::weaponComponentAdded(const Entity& entity)
{
	if ((entity.composition() & mGunWeaponComposition) == mGunWeaponComposition)
	{
		mEquipedGun = entity.ID();
		GunWeapon& gunWeapon = mGunWeaponManager.getComponent(mEquipedGun);
		gunWeapon.gunItemFilename = new std::string();
		gunWeapon.serializedGunItem.initialize();

		gunWeapon.bulletFilename = new std::string();
		gunWeapon.serializedBullet.initialize();

		gunWeapon.bullets.initialize();
	}
}

void GunSystem::weaponComponentRemoved(const Entity& entity)
{
	static SceneManager& sceneManager = SceneManager::instance();
	if ((entity.composition() & mGunWeaponComposition) == mGunWeaponComposition)
	{
		GunWeapon& gunWeapon = mGunWeaponManager.getComponent(mEquipedGun);
		delete gunWeapon.gunItemFilename;
		gunWeapon.serializedGunItem.free();
		delete gunWeapon.bulletFilename;

		gunWeapon.serializedBullet.free();

		for (unsigned int i = 0; i < gunWeapon.bullets.length; i++)
		{
			Entity bullet(gunWeapon.bullets[i]);
			destroyChildren(bullet);
			bullet.destroy();
		}
		gunWeapon.bullets.free();

		mEquipedGun = NULL;
	}
}

void GunSystem::interact(const Entity& entity)
{
	static SceneManager& sceneManager = SceneManager::instance();

	GunItem& gunItem = entity.getComponent<GunItem>();

	Entity gunWeapon;
	deserialize(std::vector<char>(gunItem.serializedGunWeapon.data, gunItem.serializedGunWeapon.data + gunItem.serializedGunWeapon.length), gunWeapon);
	sceneManager.addEntity(gunWeapon);

	Transform& transform = gunWeapon.getComponent<Transform>();
	transform.position = gunItem.instantiatePosition;
	transform.rotation = gunItem.instantiateRotation;
	transform.scale = gunItem.instantiateScale;

	mTransformManager.getComponent(InteractSystem::instance().getInteractor()).addChild(gunWeapon);
	
	sceneManager.destroyEntity(entity);
}

void GunSystem::throwEquipedGun()
{
	static SceneManager& sceneManager = SceneManager::instance();
	if (mEquipedGun)
	{
		Entity gun(mEquipedGun);
		GunWeapon& gunWeapon = mGunWeaponManager.getComponent(mEquipedGun);

		Entity gunItem;
		deserialize(std::vector<char>(gunWeapon.serializedGunItem.data, gunWeapon.serializedGunItem.data + gunWeapon.serializedGunItem.length), gunItem);
		sceneManager.addEntity(gunItem);

		Transform& interactorTransform = mTransformManager.getComponent(InteractSystem::instance().getInteractor());

		RigidBody& rigidBody = gunItem.getComponent<RigidBody>();
		rigidBody.setTransform(interactorTransform.worldPosition + interactorTransform.worldRotation * gunWeapon.instantiatePosition, interactorTransform.worldRotation * gunWeapon.instantiateRotation);
		rigidBody.setLinearVelocity(mControllerManager.getComponent(interactorTransform.parentID).getLinearVelocity());
		rigidBody.applyForce(interactorTransform.worldRotation * gunWeapon.instantiateForce);
		
		Transform& transform = gunItem.getComponent<Transform>();
		transform.scale = gunWeapon.instantiateScale;

		sceneManager.destroyEntity(gun);
	}
}

void GunSystem::shoot()
{
	static SceneManager& sceneManager = SceneManager::instance();
	if (mEquipedGun)
	{
		GunWeapon& gunWeapon = mGunWeaponManager.getComponent(mEquipedGun);

		Entity bullet;
		deserialize(std::vector<char>(gunWeapon.serializedBullet.data, gunWeapon.serializedBullet.data + gunWeapon.serializedBullet.length), bullet);
		gunWeapon.bullets.push(bullet.ID());

		RigidBody& rigidBody = bullet.getComponent<RigidBody>();
		Transform& gunTransform = mTransformManager.getComponent(mEquipedGun);
		Transform& headTransform = mTransformManager.getComponent(InteractSystem::instance().getInteractor());
		rigidBody.setTransform(gunTransform.worldPosition + headTransform.worldRotation * gunWeapon.muzzlePosition, headTransform.worldRotation);

		rigidBody.applyForce(headTransform.worldRotation * bullet.getComponent<Bullet>().fireForce);
	}
}
