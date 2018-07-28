#include "Boids.h"

using namespace Urho3D;


float Boids::Range_FAttract = 75.0f;
float Boids::Range_FRepel = 20.0f;
float Boids::Range_FAlign = 5.0f;
float Boids::FAttract_Vmax = 2.0f;
float Boids::FAttract_Factor = 5.0f;
float Boids::FRepel_Factor = 7.0f;
float Boids::FAlign_Factor = 4.0f;

Vector3 Player_pos;
bool isRunning;

Boids::Boids()
{

}


Boids::~Boids()
{
}

void Boids::Initialise(ResourceCache * pRes, Scene * pScene)
{


	pNode = pScene->CreateChild("Boid");

	pNode->SetScale(Vector3(1.0f, 1.0f, 1.0f));

	pStaticmodel = pNode->CreateComponent<StaticModel>();
	pStaticmodel->SetModel(pRes->GetResource<Model>("Models/tna_body.mdl"));
	pStaticmodel->SetMaterial(pRes->GetResource<Material>("Materials/Fishy.xml"));

	pStaticmodel->SetCastShadows(true);
	pRigidbody = pNode->CreateComponent<RigidBody>();

	pRigidbody->SetMass(1.0f);
	pRigidbody->SetUseGravity(false);
	pRigidbody->SetPosition(Vector3(Random(60.0f) - 30.0f, Random(10.0f) + 20, Random(60.0f) - 30.0f));
	//pRigidbody->SetRotation(Quaternion(0.0f, 0.0f, 0.0f));
	pRigidbody->SetLinearVelocity(Vector3(Random(20.0f) - 20.0f, 0.0f, Random(20.0f) - 20.0f));


	// The Trigger mode makes the rigid body only detect collisions, but impart no forces on the
	// colliding objects
	pRigidbody->SetTrigger(false);
	pRigidbody->SetCollisionLayer(2);

	pCollisionshape = pNode->CreateComponent<CollisionShape>();
	//pCollisionshape->SetModel(pRes->GetResource<Model>("Models/tna_body.mdl"));
	pCollisionshape->SetCapsule(3.0f, 1.0f, Vector3(0.0f, 1.0f, 0.0f));
	
}

void Boids::Computeforce(Boids * Boid)
{


	Vector3 CoM; //centre of mass, accumulated total
	float n = 0.0f; //count number of neigbours
	float a = 0.0f; //alignment
	float r = 0.0f;
	Vector3 sep_Distance;
	//set the force member variable to zero 


	force = Vector3(0, 0, 0);
	//Search Neighbourhood
	for (int i = 0; i < NumBoids; i++)
	{
		//the current boid?
		if (this == &Boid[i]) continue;
		//sep = vector position of this boid from current oid
		Vector3 sep = pRigidbody->GetPosition() -
			Boid[i].pRigidbody->GetPosition();
		float d = sep.Length(); //distance of boid


		if (d < Range_FAttract)
		{
			//with range, so is a neighbour
			CoM += Boid[i].pRigidbody->GetPosition();
			n++;
		}

		if (d < Range_FRepel)
		{
			float sep_Distancefloat = sep.Length();
			sep_Distance += sep / (sep_Distancefloat*sep_Distancefloat);
			r++;
		}


		if (d < Range_FAlign)
		{
			a++;
		}

	}

	//Attractive force component
	if (n > 0)
	{
		CoM /= n;
		Vector3 dir = (CoM - pRigidbody->GetPosition()).Normalized();
		Vector3 vDesired = dir*FAttract_Vmax;
		force += (vDesired - pRigidbody->GetLinearVelocity())*FAttract_Factor;
	}
	//Repulsive force component
	if (r > 0)
	{
		force += (sep_Distance * FRepel_Factor);

	}

	//Alignment force component
	if (a > 0)
	{
		Vector3 dir = (CoM - pRigidbody->GetPosition()).Normalized();
		force += (dir - FAlign_Factor * pRigidbody->GetLinearVelocity());
	}

	Vector3 Boid_Loc = pRigidbody->GetPosition();


	if (isRunning == true)
	{

		if (Boid_Loc.x_ <= (Player_pos.x_ + 5) & Boid_Loc.x_ >= (Player_pos.x_ - 5) & Boid_Loc.y_ <= (Player_pos.y_ + 5) & Boid_Loc.y_ >= (Player_pos.y_ - 5) & Boid_Loc.z_ <= (Player_pos.z_ + 5) & Boid_Loc.z_ >= (Player_pos.z_ - 5))
		{
			Log::WriteRaw("A Boid has been Captured!");
			pRigidbody->SetPosition(Vector3(0, -1000, 0));
		}
	}
}

void Boids::Update(float tm)
{
	pRigidbody->ApplyForce(force);
	Vector3 vel = pRigidbody->GetLinearVelocity();
	float d = vel.Length();
	if (d < 10.0f)
	{
		d = 10.0f;
		pRigidbody->SetLinearVelocity(vel.Normalized()*d);
	}
	else if (d > 50.0f)
	{
		d = 50.0f;
		pRigidbody->SetLinearVelocity(vel.Normalized()*d);
	}
	Vector3 vn = vel.Normalized();
	Vector3 cp = -vn.CrossProduct(Vector3(0.0f, 1.0f, 0.0f));
	float dp = cp.DotProduct(vn);
	pRigidbody->SetRotation(Quaternion(Acos(dp), cp));
	Vector3 p = pRigidbody->GetPosition();
	if (p.y_ < 10.0f)
	{
		p.y_ = 10.0f;
		pRigidbody->SetPosition(p);
	}
	else if (p.y_ > 50.0f)
	{
		p.y_ = 50.0f;
		pRigidbody->SetPosition(p);
	}


}

void Boids::GetPlayerPos(Vector3 coords, bool Game_Running)
{
	Player_pos = coords;
	isRunning = true;
}

void BoidSet::Initialise(ResourceCache * pRes, Scene * pScene)
{
	for (int i = 0; i < NumBoids; i++)
	{
		boidList1[i].Initialise(pRes, pScene);
		boidList2[i].Initialise(pRes, pScene);
		boidList3[i].Initialise(pRes, pScene);
		boidList4[i].Initialise(pRes, pScene);
		boidList5[i].Initialise(pRes, pScene);
	}
}

void BoidSet::Update(float tm)
{
	for (int i = 0; i < NumBoids; i++)
	{
		boidList1[i].Computeforce(&boidList1[0]);
		boidList1[i].Update(tm);

		boidList2[i].Computeforce(&boidList2[0]);
		boidList2[i].Update(tm);

		boidList3[i].Computeforce(&boidList3[0]);
		boidList3[i].Update(tm);

		boidList4[i].Computeforce(&boidList4[0]);
		boidList4[i].Update(tm);

		boidList5[i].Computeforce(&boidList5[0]);
		boidList5[i].Update(tm);
	}
}
