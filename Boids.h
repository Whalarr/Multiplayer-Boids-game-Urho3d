#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>

#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/Terrain.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Network/Connection.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>

#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Physics/Constraint.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Window.h>
#include <Urho3D/UI/CheckBox.h>




namespace Urho3D
{
	class Node;
	class Scene;
	class RigidBody;
	class CollisionShape;
	class ResourceCache;
}
// All Urho3D classes reside in namespace Urho3D
using namespace Urho3D;
const static int NumBoids = 20;


class Boids
{
public:
	Vector3 force;

	static float Range_FRepel;
	static float Range_FAlign;
	static float FAttract_Factor;
	static float FRepel_Factor;
	static float FAlign_Factor;
	static float FAttract_Vmax;
	static float Range_FAttract;

	Node *pNode;
	RigidBody *pRigidbody;
	CollisionShape *pCollisionshape;
	Object *pObject;
	StaticModel *pStaticmodel;

	Boids();
	~Boids();

	void Initialise(ResourceCache *pRes, Scene *pScene);



	void Computeforce(Boids *Boid);
	void Update(float tm);

	void GetPlayerPos(Vector3 coords, bool Game_Running);

};

class BoidSet
{
public:
	Boids boidList1[NumBoids];
	Boids boidList2[NumBoids];
	Boids boidList3[NumBoids];
	Boids boidList4[NumBoids];
	Boids boidList5[NumBoids];

	BoidSet() {};
	void Initialise(ResourceCache *pRes, Scene *pScene);
	void Update(float tm);
};