#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Context.h>
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
#include <Urho3D/IO/MemoryBuffer.h>

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

#include "Character.h"
#include "CharacterDemo.h"
#include "Touch.h"

#include <Urho3D/DebugNew.h>
#include <Urho3D/Engine/DebugHud.h>

// Custom remote event we use to tell the client which object they control
static const StringHash E_CLIENTOBJECTAUTHORITY("ClientObjectAuthority");
// Identifier for the node ID parameter in the event data
static const StringHash PLAYER_ID("IDENTITY");
// Custom event on server, client has pressed button that it wants to start game
static const StringHash E_CLIENTISREADY("ClientReadyToStart");

static const String INSTRUCTION("instructionText");
Boids boids;
bool Game_Running;


URHO3D_DEFINE_APPLICATION_MAIN(CharacterDemo)

CharacterDemo::CharacterDemo(Context* context) :
	Sample(context),
	firstPerson_(false)
{
	//TUTORIAL: TODO

}

CharacterDemo::~CharacterDemo()
{
}

void CharacterDemo::Start()
{
	// Execute base class startup
	Sample::Start();
	if (touchEnabled_)
		touch_ = new Touch(context_, TOUCH_SENSITIVITY);
	//TUTORIAL: TODO

	// Create static scene content
	CreateScene();
	CreateMainMenu();

	// Create the UI content
	CreateInstructions();
	// Subscribe to necessary events
	SubscribeToEvents();
	// Set the mouse mode to use in the sample
	Sample::InitMouseMode(MM_RELATIVE);

}

void CharacterDemo::CreateScene()
{
	//Hashmap Pointer
	HashMap<Connection*, WeakPtr<Node> > serverObjects_;
	//so we can access resources
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	scene_ = new Scene(context_);
	// Create scene subsystem components
	scene_->CreateComponent<Octree>(LOCAL);
	scene_->CreateComponent<PhysicsWorld>(LOCAL);

	//Create camera node and component
	cameraNode_ = new Node(context_);
	Camera* camera = cameraNode_->CreateComponent<Camera>(LOCAL);
	cameraNode_->SetPosition(Vector3(0.0f, 20.0f, 0.0f));
	camera->SetFarClip(600.0f);

	GetSubsystem<Renderer>()->SetViewport(0, new Viewport(context_,
		scene_, camera));

	// Create static scene content. First create a zone for ambient
	//lighting and fog control
	Node* zoneNode = scene_->CreateChild("Zone",LOCAL);
	Zone* zone = zoneNode->CreateComponent<Zone>(LOCAL);
	zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
	zone->SetFogColor(Color(0.2f, 0.5f, 0.7f));
	zone->SetFogStart(40.0f);
	zone->SetFogEnd(150.0f);
	zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));

	// Create a directional light with cascaded shadow mapping
	Node* lightNode = scene_->CreateChild("DirectionalLight",LOCAL);
	lightNode->SetDirection(Vector3(0.3f, -0.5f, 0.425f));
	Light* light = lightNode->CreateComponent<Light>(LOCAL);
	light->SetLightType(LIGHT_DIRECTIONAL);
	light->SetCastShadows(true);
	light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
	light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
	//alter brightness values
	light->SetSpecularIntensity(1.0f);
	light->SetBrightness(0.6f);

	// Create skybox. The Skybox component is used like StaticModel, but it will be always located at the camera, giving the
	// illusion of the box planes being far away. Use just the ordinary Box model and a suitable material, whose shader will
	// generate the necessary 3D texture coordinates for cube mapping
	Node* skyNode = scene_->CreateChild("Sky",LOCAL);
	skyNode->SetScale(80.0f); // The scale actually does not matter
	Skybox* skybox = skyNode->CreateComponent<Skybox>(LOCAL);
	skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

	// Create heightmap terrain
	Node* terrainNode = scene_->CreateChild("Terrain",LOCAL);
	terrainNode->SetPosition(Vector3(20.0f, -10.0f, 0.0f));
	Terrain* terrain = terrainNode->CreateComponent<Terrain>(LOCAL);
	terrain->SetPatchSize(64);
	terrain->SetSpacing(Vector3(0.6f, 0.3f, 0.6f)); // Spacing between vertices and vertical resolution of the height map
	terrain->SetSmoothing(true);
	terrain->SetHeightMap(cache->GetResource<Image>("Textures/HeightMap.png"));
	terrain->SetMaterial(cache->GetResource<Material>("Materials/Terrain.xml"));
	// The terrain consists of large triangles, which fits well for occlusion rendering, as a hill can occlude all
	// terrain patches and other objects behind it
	terrain->SetOccluder(true);

	RigidBody* Terrainbody = terrainNode->CreateComponent<RigidBody>(LOCAL);
	Terrainbody->SetCollisionLayer(2); // Use layer bitmask 2 for static geometry

	CollisionShape* Terrainshape = terrainNode->CreateComponent<CollisionShape>(LOCAL);
	Terrainshape->SetTerrain();
	

	// Create a water plane object that is as large as the terrain
	waterNode_ = scene_->CreateChild("Water",LOCAL);
	waterNode_->SetScale(Vector3(2048.0f, 1.0f, 2048.0f));
	waterNode_->SetPosition(Vector3(0.0f, 45.0f, 0.0f));

	waterNode_->SetRotation(Quaternion(0.0f, 0.0f, 180.0f));

	StaticModel* water = waterNode_->CreateComponent<StaticModel>(LOCAL);
	water->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
	water->SetMaterial(cache->GetResource<Material>("Materials/Water.xml"));
	// Set a different viewmask on the water plane to be able to hide it from the reflection camera
	water->SetViewMask(0x80000000);

	// Create a mathematical plane to represent the water in calculations
	waterPlane_ = Plane(waterNode_->GetWorldRotation() * Vector3(0.0f, 1.0f, 0.0f), waterNode_->GetWorldPosition());
	//Create a downward biased plane for reflection view clipping. Biasing is necessary to avoid too aggressive //clipping
	waterClipPlane_ = Plane(waterNode_->GetWorldRotation() * Vector3(0.0f, 1.0f, 0.0f),
		waterNode_->GetWorldPosition() - Vector3(0.0f, 0.01f, 0.0f));
	
	RigidBody* Waterbody = waterNode_->CreateComponent<RigidBody>(LOCAL);
	Waterbody->SetCollisionLayer(2); // Use layer bitmask 2 for static geometry
	CollisionShape* Watershape = waterNode_->CreateComponent<CollisionShape>(LOCAL);
	Watershape->SetTerrain();

	// Create Plants of varying sizes
	const unsigned NUM_PLANTS = 40;
	for (unsigned i = 0; i < NUM_PLANTS; ++i)
	{
		Node* objectNode = scene_->CreateChild("Plant");

		Vector3 position(Random(100.0f) - 30.0f, 0.0f, Random(100.0f) - 30.0f);
		position.y_ = terrain->GetHeight(position);
		objectNode->SetPosition(position);

		objectNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
		objectNode->SetScale(0.05f + Random(0.05f));
		StaticModel* object = objectNode->CreateComponent<StaticModel>();
		object->SetModel(cache->GetResource<Model>("Models/plant_002a.mdl"));
		object->SetMaterial(cache->GetResource<Material>("Materials/Foliage.xml"));
		object->SetCastShadows(true);

		RigidBody* body = objectNode->CreateComponent<RigidBody>();
		body->SetCollisionLayer(2);
		CollisionShape* shape = objectNode->CreateComponent<CollisionShape>();
		shape->SetTriangleMesh(object->GetModel(), 0);
	}

	// Create Plants of varying sizes, replicated code for easy alterations
	const unsigned NUM_BAMBOO = 60;
	for (unsigned i = 0; i < NUM_BAMBOO; ++i)
	{
		Node* objectNode = scene_->CreateChild("Bamboo");

		Vector3 position(Random(150.0f) - 30.0f, 0.0f, Random(150.0f) - 30.0f);
		position.y_ = terrain->GetHeight(position);
		objectNode->SetPosition(position);

		objectNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
		objectNode->SetScale(0.02f + Random(0.03f));
		StaticModel* object = objectNode->CreateComponent<StaticModel>();
		object->SetModel(cache->GetResource<Model>("Models/Bamboo.mdl"));
		object->SetMaterial(cache->GetResource<Material>("Materials/Foliage.xml"));
		object->SetCastShadows(true);

		RigidBody* body = objectNode->CreateComponent<RigidBody>();
		body->SetCollisionLayer(2);
		CollisionShape* shape = objectNode->CreateComponent<CollisionShape>();
		shape->SetTriangleMesh(object->GetModel(), 0);
	}
	// Create camera for water reflection
	// It will have the same farclip and position as the main viewport camera, but uses a reflection plane to modify
	// its position when rendering
	reflectionCameraNode_ = cameraNode_->CreateChild();
	Camera* reflectionCamera = reflectionCameraNode_->CreateComponent<Camera>();
	reflectionCamera->SetFarClip(50.0);
	reflectionCamera->SetViewMask(0x7fffffff); // Hide objects with only bit 31 in the viewmask (the water plane)
	reflectionCamera->SetAutoAspectRatio(true);
	reflectionCamera->SetUseReflection(true);
	reflectionCamera->SetReflectionPlane(waterPlane_);
	reflectionCamera->SetUseClipping(true); // Enable clipping of geometry behind water plane
	reflectionCamera->SetClipPlane(waterClipPlane_);

	//// The water reflection texture is rectangular. Set reflection camera aspect ratio to match
	//reflectionCamera->SetAspectRatio((float)graphics->GetWidth() / (float)graphics->GetHeight());

	// View override flags could be used to optimize reflection rendering. For example disable shadows
	//reflectionCamera->SetViewOverrideFlags(VO_DISABLE_SHADOWS);
	// Create a texture and setup viewport for water reflection. Assign the reflection texture to the diffuse
	// texture unit of the water material
	int texSize = 1024;
	SharedPtr<Texture2D> renderTexture(new Texture2D(context_));
	renderTexture->SetSize(texSize, texSize, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
	renderTexture->SetFilterMode(FILTER_BILINEAR);
	RenderSurface* surface = renderTexture->GetRenderSurface();
	SharedPtr<Viewport> rttViewport(new Viewport(context_, scene_, reflectionCamera));
	surface->SetViewport(0, rttViewport);
	Material* waterMat = cache->GetResource<Material>("Materials/Water.xml");
	waterMat->SetTexture(TU_DIFFUSE, renderTexture);

}

void CharacterDemo::CreateClientScene() 
{
	//Same as the create scene for server, alterations for what the client handles
	//so we can access resources
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	scene_ = new Scene(context_);
	// Create scene subsystem components
	scene_->CreateComponent<Octree>(LOCAL);
	scene_->CreateComponent<PhysicsWorld>(LOCAL);

	//Create camera node and component
	cameraNode_ = new Node(context_);
	Camera* camera = cameraNode_->CreateComponent<Camera>(LOCAL);
	cameraNode_->SetPosition(Vector3(0.0f, 20.0f, 0.0f));
	camera->SetFarClip(600.0f);

	GetSubsystem<Renderer>()->SetViewport(0, new Viewport(context_,
		scene_, camera));

	// Create static scene content. First create a zone for ambient
	//lighting and fog control
	Node* zoneNode = scene_->CreateChild("Zone", LOCAL);
	Zone* zone = zoneNode->CreateComponent<Zone>(LOCAL);
	zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
	zone->SetFogColor(Color(0.2f, 0.5f, 0.7f));
	zone->SetFogStart(40.0f);
	zone->SetFogEnd(150.0f);
	zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));

	// Create a directional light with cascaded shadow mapping
	Node* lightNode = scene_->CreateChild("DirectionalLight", LOCAL);
	lightNode->SetDirection(Vector3(0.3f, -0.5f, 0.425f));
	Light* light = lightNode->CreateComponent<Light>(LOCAL);
	light->SetLightType(LIGHT_DIRECTIONAL);
	light->SetCastShadows(true);

	light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
	light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
	light->SetSpecularIntensity(1.0f);
	light->SetBrightness(0.6f);

	// Create skybox. The Skybox component is used like StaticModel, but it will be always located at the camera, giving the
	// illusion of the box planes being far away. Use just the ordinary Box model and a suitable material, whose shader will
	// generate the necessary 3D texture coordinates for cube mapping
	Node* skyNode = scene_->CreateChild("Sky", LOCAL);
	skyNode->SetScale(80.0f); // The scale actually does not matter
	Skybox* skybox = skyNode->CreateComponent<Skybox>(LOCAL);
	skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
	skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

	// Create heightmap terrain
	Node* terrainNode = scene_->CreateChild("Terrain", LOCAL);
	terrainNode->SetPosition(Vector3(20.0f, -10.0f, 0.0f));
	Terrain* terrain = terrainNode->CreateComponent<Terrain>(LOCAL);
	terrain->SetPatchSize(64);
	terrain->SetSpacing(Vector3(0.6f, 0.3f, 0.6f)); // Spacing between vertices and vertical resolution of the height map
	terrain->SetSmoothing(true);
	terrain->SetHeightMap(cache->GetResource<Image>("Textures/HeightMap.png", LOCAL));
	terrain->SetMaterial(cache->GetResource<Material>("Materials/Terrain.xml", LOCAL));
	// The terrain consists of large triangles, which fits well for occlusion rendering, as a hill can occlude all
	// terrain patches and other objects behind it
	terrain->SetOccluder(true);

	RigidBody* Terrainbody = terrainNode->CreateComponent<RigidBody>(LOCAL);
	Terrainbody->SetCollisionLayer(2); // Use layer bitmask 2 for static geometry

	CollisionShape* Terrainshape = terrainNode->CreateComponent<CollisionShape>(LOCAL);
	Terrainshape->SetTerrain();


	// Create a water plane object that is as large as the terrain
	waterNode_ = scene_->CreateChild("Water", LOCAL);
	waterNode_->SetScale(Vector3(2048.0f, 1.0f, 2048.0f));
	waterNode_->SetPosition(Vector3(0.0f, 45.0f, 0.0f));

	waterNode_->SetRotation(Quaternion(0.0f, 0.0f, 180.0f));

	StaticModel* water = waterNode_->CreateComponent<StaticModel>(LOCAL);
	water->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
	water->SetMaterial(cache->GetResource<Material>("Materials/Water.xml"));
	// Set a different viewmask on the water plane to be able to hide it from the reflection camera
	water->SetViewMask(0x80000000);

	// Create a mathematical plane to represent the water in calculations
	waterPlane_ = Plane(waterNode_->GetWorldRotation() * Vector3(0.0f, 1.0f, 0.0f), waterNode_->GetWorldPosition());
	//Create a downward biased plane for reflection view clipping. Biasing is necessary to avoid too aggressive //clipping
	waterClipPlane_ = Plane(waterNode_->GetWorldRotation() * Vector3(0.0f, 1.0f, 0.0f),
		waterNode_->GetWorldPosition() - Vector3(0.0f, 0.01f, 0.0f));

	RigidBody* Waterbody = waterNode_->CreateComponent<RigidBody>(LOCAL);
	Waterbody->SetCollisionLayer(2); // Use layer bitmask 2 for static geometry
	CollisionShape* Watershape = waterNode_->CreateComponent<CollisionShape>(LOCAL);
	Watershape->SetTerrain();


	// Create Plants of varying sizes
	const unsigned NUM_PLANTS = 40;
	for (unsigned i = 0; i < NUM_PLANTS; ++i)
	{
		Node* objectNode = scene_->CreateChild("Plant",LOCAL);

		Vector3 position(Random(100.0f) - 30.0f, 0.0f, Random(100.0f) - 30.0f);
		position.y_ = terrain->GetHeight(position);
		objectNode->SetPosition(position);

		objectNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
		objectNode->SetScale(0.05f + Random(0.05f));
		StaticModel* object = objectNode->CreateComponent<StaticModel>(LOCAL);
		object->SetModel(cache->GetResource<Model>("Models/plant_002a.mdl"));
		object->SetMaterial(cache->GetResource<Material>("Materials/Foliage.xml"));
		object->SetCastShadows(true);
		RigidBody* body = objectNode->CreateComponent<RigidBody>(LOCAL);
		body->SetCollisionLayer(2);
		//body->SetMass(2.0f);
		CollisionShape* shape = objectNode->CreateComponent<CollisionShape>(LOCAL);
		shape->SetTriangleMesh(object->GetModel(), 0);
	}

	// Create Plants of varying sizes
	const unsigned NUM_BAMBOO = 60;
	for (unsigned i = 0; i < NUM_BAMBOO; ++i)
	{
		Node* objectNode = scene_->CreateChild("Bamboo", LOCAL);

		Vector3 position(Random(150.0f) - 30.0f, 0.0f, Random(150.0f) - 30.0f);
		position.y_ = terrain->GetHeight(position);
		objectNode->SetPosition(position);

		objectNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
		objectNode->SetScale(0.02f + Random(0.03f));
		StaticModel* object = objectNode->CreateComponent<StaticModel>(LOCAL);
		object->SetModel(cache->GetResource<Model>("Models/Bamboo.mdl"));
		object->SetMaterial(cache->GetResource<Material>("Materials/Foliage.xml"));
		object->SetCastShadows(true);
		RigidBody* body = objectNode->CreateComponent<RigidBody>(LOCAL);
		body->SetCollisionLayer(2);
		//body->SetMass(2.0f);
		CollisionShape* shape = objectNode->CreateComponent<CollisionShape>(LOCAL);
		shape->SetTriangleMesh(object->GetModel(), 0);
	}



	// Create camera for water reflection
	// It will have the same farclip and position as the main viewport camera, but uses a reflection plane to modify
	// its position when rendering
	reflectionCameraNode_ = cameraNode_->CreateChild();
	Camera* reflectionCamera = reflectionCameraNode_->CreateComponent<Camera>();
	reflectionCamera->SetFarClip(50.0);
	reflectionCamera->SetViewMask(0x7fffffff); // Hide objects with only bit 31 in the viewmask (the water plane)
	reflectionCamera->SetAutoAspectRatio(true);
	reflectionCamera->SetUseReflection(true);
	reflectionCamera->SetReflectionPlane(waterPlane_);
	reflectionCamera->SetUseClipping(true); // Enable clipping of geometry behind water plane
	reflectionCamera->SetClipPlane(waterClipPlane_);


	//// The water reflection texture is rectangular. Set reflection camera aspect ratio to match
	//reflectionCamera->SetAspectRatio((float)graphics->GetWidth() / (float)graphics->GetHeight());

	// View override flags could be used to optimize reflection rendering. For example disable shadows
	//reflectionCamera->SetViewOverrideFlags(VO_DISABLE_SHADOWS);
	// Create a texture and setup viewport for water reflection. Assign the reflection texture to the diffuse
	// texture unit of the water material
	int texSize = 1024;
	SharedPtr<Texture2D> renderTexture(new Texture2D(context_));
	renderTexture->SetSize(texSize, texSize, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
	renderTexture->SetFilterMode(FILTER_BILINEAR);
	RenderSurface* surface = renderTexture->GetRenderSurface();
	SharedPtr<Viewport> rttViewport(new Viewport(context_, scene_, reflectionCamera));
	surface->SetViewport(0, rttViewport);
	Material* waterMat = cache->GetResource<Material>("Materials/Water.xml");
	waterMat->SetTexture(TU_DIFFUSE, renderTexture);
}

void CharacterDemo::CreateMainMenu()
{
	// Set the mouse mode to use in the sample
	InitMouseMode(MM_RELATIVE);
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();
	UIElement* root = ui->GetRoot();
	XMLFile* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
	root->SetDefaultStyle(uiStyle); //need to set default ui style
									//Create a Cursor UI element.We want to be able to hide and show the main menu at will.When hidden, 
									//the mouse will control the camera, and when visible, the
									//mouse will be able to interact with the GUI.
	SharedPtr<Cursor> cursor(new Cursor(context_));
	cursor->SetStyleAuto(uiStyle);
	ui->SetCursor(cursor);
	// Create the Window and add it to the UI's root node
	window_ = new Window(context_);
	root->AddChild(window_);
	// Set Window size and layout settings
	window_->SetMinWidth(512);
	window_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
	window_->SetAlignment(HA_CENTER, VA_CENTER);
	window_->SetName("Window");
	window_->SetStyleAuto();

	//allow users to input IP Address, otherwise default to localhosting for server init
	serverAddressLineEdit_ = window_->CreateChild<LineEdit>();

	//create objects of type button
	Button* Button_Connect = window_->CreateChild<Button>();
	Button* Button_Disconnect = window_->CreateChild<Button>();
	Button* Button_ServerStartup = window_->CreateChild<Button>();
	Button* Button_StartClientGame = window_->CreateChild<Button>();
	Button* Button_Resume = window_->CreateChild<Button>();
	Button* Button_Quit = window_->CreateChild<Button>();

	//create buttons w/display text
	CreateLineEdit("IPAddress:", 24, window_, serverAddressLineEdit_);
	CreateButton("Connect", 24, window_, Button_Connect);
	CreateButton("Disconnect", 24, window_, Button_Disconnect);
	CreateButton("Start Server", 24, window_, Button_ServerStartup);
	CreateButton("Start Client Game", 24, window_, Button_StartClientGame);
	CreateButton("Resume", 24, window_, Button_Resume);
	CreateButton("Quit", 24, window_, Button_Quit);
	//subscribe to event, links buttons to the functions
	SubscribeToEvent(Button_Quit, E_RELEASED, URHO3D_HANDLER(CharacterDemo, HandleQuit));
	SubscribeToEvent(Button_ServerStartup, E_RELEASED, URHO3D_HANDLER(CharacterDemo, HandleStartServer));
	SubscribeToEvent(Button_StartClientGame, E_RELEASED, URHO3D_HANDLER(CharacterDemo, HandleClientStartGame));
	SubscribeToEvent(Button_Connect, E_RELEASED, URHO3D_HANDLER(CharacterDemo, HandleConnect));
	SubscribeToEvent(Button_Disconnect, E_RELEASED, URHO3D_HANDLER(CharacterDemo, HandleDisconnect));
	SubscribeToEvent(Button_Resume, E_RELEASED, URHO3D_HANDLER(CharacterDemo, HandleResume));

}

Button* CharacterDemo::CreateButton(const String& text, int pHeight, Urho3D::Window*whichWindow, Button* button)
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	Font* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");

	button->SetMinHeight(pHeight);
	button->SetStyleAuto();
	Text* buttonText = button->CreateChild<Text>();
	buttonText->SetFont(font, 12);
	buttonText->SetAlignment(HA_CENTER, VA_CENTER);
	buttonText->SetText(text);
	whichWindow->AddChild(button);
	return button;
}

LineEdit* CharacterDemo::CreateLineEdit(const String& text, int pHeight, Urho3D::Window*whichWindow, LineEdit* lineedit)
{
	LineEdit* lineEdit = whichWindow->CreateChild<LineEdit>();
	lineEdit->SetMinHeight(pHeight);
	lineEdit->SetAlignment(HA_CENTER, VA_CENTER);
	lineEdit->SetText(text);
	whichWindow->AddChild(lineEdit);
	lineEdit->SetStyleAuto();
	return lineEdit;
};

void CharacterDemo::CreateInstructions()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	UI* ui = GetSubsystem<UI>();

	// Construct new Text object, set string to display and font to use
	Text* instructionText = ui->GetRoot()->CreateChild<Text>(INSTRUCTION);
	instructionText->SetText(

		"Use WASD keys to move and mouse to look around\n"
		"Use Spacebar to ascend, CTRL to descend\n"
		"Toggle this Text by Pressing T \n"
		"Toggle the Main Menu by Pressing M\n"

	);
	instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
	// The text has multiple rows. Center them in relation to each other
	instructionText->SetTextAlignment(HA_CENTER);

	// Position the text relative to the screen center
	instructionText->SetHorizontalAlignment(HA_CENTER);
	instructionText->SetVerticalAlignment(VA_CENTER);
	instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void CharacterDemo::SubscribeToEvents()
{
	//TUTORIAL: TODO
	// Subscribe to Update event for setting the character controls
	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(CharacterDemo, HandleUpdate));

	// Subscribe to Update event for setting the character controls
	SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(CharacterDemo, HandlePostUpdate));

	// Setting or applying controls
	SubscribeToEvent(E_PHYSICSPRESTEP, URHO3D_HANDLER(CharacterDemo, HandlePhysicsPreStep));

	// server: what happens when a client is connected
	SubscribeToEvent(E_CLIENTCONNECTED, URHO3D_HANDLER(CharacterDemo, HandleClientConnected));
	// server: what happens when a client is disconnected
	SubscribeToEvent(E_CLIENTDISCONNECTED, URHO3D_HANDLER(CharacterDemo, HandleClientDisconnected));

	//Server: This is called when a client has connected to a Server
	SubscribeToEvent(E_CLIENTSCENELOADED, URHO3D_HANDLER(CharacterDemo, HandleClientFinishedLoading));


	SubscribeToEvent(E_CLIENTISREADY, URHO3D_HANDLER(CharacterDemo, HandleClientToServerReadyToStart));
	GetSubsystem<Network>()->RegisterRemoteEvent(E_CLIENTISREADY);
	SubscribeToEvent(E_CLIENTOBJECTAUTHORITY, URHO3D_HANDLER(CharacterDemo, HandleServerToClientObjectID));
	GetSubsystem<Network>()->RegisterRemoteEvent(E_CLIENTOBJECTAUTHORITY);

}

void CharacterDemo::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
	//TUTORIAL: TODO
	using namespace Update;
	UI* ui = GetSubsystem<UI>();
	// Take the frame time step, which is stored as a float
	float timeStep = eventData[P_TIMESTEP].GetFloat();
	// Do not move if the UI has a focused element (the console)
	if (GetSubsystem<UI>()->GetFocusElement()) return;
	Input* input = GetSubsystem<Input>();

	// Movement speed as world units per second
	const float MOVE_SPEED = 20.0f;
	// Mouse sensitivity as degrees per pixel
	const float MOUSE_SENSITIVITY = 0.1f;
	// Use this frame's mouse motion to adjust camera node yaw and pitch.
	// Clamp the pitch between -90 and 90 degrees
	IntVector2 mouseMove = input->GetMouseMove();
	yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
	pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
	pitch_ = Clamp(pitch_, -90.0f, 90.0f);
	// Construct new orientation for the camera scene node from
	// yaw and pitch. Roll is fixed to zero
	cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
	// Read WASD keys and move the camera scene node to the corresponding
	// direction if they are pressed, use the Translate() function
	// (default local space) to move relative to the node's orientation.
	if (input->GetKeyDown(KEY_W))
		cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED *
			timeStep);
	if (input->GetKeyDown(KEY_S))
		cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
	if (input->GetKeyDown(KEY_A))
		cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
	if (input->GetKeyDown(KEY_D))
		cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
	//Switch between menu and not menu
	if (input->GetKeyPress(KEY_M))
		menuVisible = !menuVisible;

	ui->GetCursor()->SetVisible(menuVisible);
	window_->SetVisible(menuVisible);

	if (input->GetKeyPress(KEY_T))
	{
		UIElement* instruction = ui->GetRoot()->GetChild(INSTRUCTION);
		instruction ->SetVisible(!instruction->IsVisible());
	}

}

void CharacterDemo::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    //update the camera 
	MoveCamera();
}

void CharacterDemo::HandlePhysicsPreStep(StringHash eventType, VariantMap & eventData)
{
	using namespace Update;
	// Take the frame time step, which is stored as a float
	float timeStep = eventData[P_TIMESTEP].GetFloat();

	Network* network = GetSubsystem<Network>();
	Connection* serverConnection = network->GetServerConnection();
	// Client: collect controls
	if (serverConnection)
	{
		serverConnection->SetPosition(cameraNode_->GetPosition()); // send camera position too
		serverConnection->SetControls(FromClientToServerControls()); // send controls to serve
		VariantMap remoteEventData;
		remoteEventData["aValueRemoteValue"] = 0;
	}
	// Server: Read Controls, Apply them if needed
	else if (network->IsServerRunning())
	{
		ProcessClientControls(); // take data from clients, process it
		//update boids
		boidSet.Update(timeStep);
		//if a client object is active, run collision code to check for collisions with boids.
		if (ballNode)
		{
			Game_Running = true;
			Vector3 Player_pos = ballNode->GetPosition();
			boids.GetPlayerPos(Player_pos, Game_Running);
		}
	}
	
	
}
////////////////////////////////////////////////////////////////////////////////////////////////////
Node* CharacterDemo::CreateControllableObject()
{

	ResourceCache* cache = GetSubsystem<ResourceCache>();
	// Create the scene node & visual representation. This will be a replicated object
	ballNode = scene_->CreateChild("AClientBall");
	ballNode->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
	ballNode->SetScale(2.0f);
	StaticModel* ballObject = ballNode->CreateComponent<StaticModel>();
	//set model 
	ballObject->SetModel(cache->GetResource<Model>("Models/3Shark.mdl")); //because 1 wasnt enough..you should see his brother (try changing the model to "2Shark")																     
	ballObject->SetMaterial(cache->GetResource<Material>("Materials/Player.xml"));
	ballNode->SetRotation(Quaternion(0.0f, 0.0f, 0.0f));

	// Create the physics components	
	RigidBody* body = ballNode->CreateComponent<RigidBody>();
	body->SetMass(3.0f);
	body->SetUseGravity(false);
	body->SetTrigger(false);
	body->SetFriction(1.0f);
	body->SetAngularFactor(Vector3::ZERO);
	body->SetCollisionLayer(2);
	body->SetLinearDamping(0.95f);
	body->SetAngularDamping(0.95f);

	CollisionShape* shape = ballNode->CreateComponent<CollisionShape>();
	//shape->SetModel(cache->GetResource<Model>("Models/3Shark.mdl"));
	shape->SetCapsule(5.0f, 4.0f, Vector3(0.0, 1.0, 0.0));
	//returns the node
	return ballNode;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
Controls CharacterDemo::FromClientToServerControls()
{
	Input* input = GetSubsystem<Input>();
	UI* ui = GetSubsystem<UI>();
	Controls controls;
	// mouse yaw to server
	//camera adjust player rotation
	Quaternion rotation(0.0f, controls.yaw_, 0.0f);
	//Check which button has been pressed, keep track
	if (input->GetKeyDown(KEY_W))
		controls.Set(CTRL_FORWARD, true);
	if (input->GetKeyDown(KEY_S))
		controls.Set(CTRL_BACK, true);
	if (input->GetKeyDown(KEY_A))
		controls.Set(CTRL_LEFT, true);
	if (input->GetKeyDown(KEY_D))
		controls.Set(CTRL_RIGHT, true);
	if (input->GetKeyDown(KEY_SPACE))
		controls.Set(CTRL_UP, true);
	if (input->GetKeyDown(KEY_LCTRL))
		controls.Set(CTRL_DOWN, true);
	if (input->GetKeyPress(KEY_M))
		menuVisible = !menuVisible;
	ui->GetCursor()->SetVisible(menuVisible);
	window_->SetVisible(menuVisible);
	if (input->GetKeyPress(KEY_T))
	{
		UIElement* instruction = ui->GetRoot()->GetChild(INSTRUCTION);
		instruction->SetVisible(!instruction->IsVisible());
	}
	controls.yaw_ = yaw_;
	return controls;
}

void CharacterDemo::ProcessClientControls()
{
	Network* network = GetSubsystem<Network>();
	UI* ui = GetSubsystem<UI>();
	const Vector<SharedPtr<Connection> >& connections = network->GetClientConnections();
	// Movement speed as world units per second, for realism/balancing, client has slower movement on movement in non-forward directions
	float MOVE_SPEED = 50.0f;
	const float MOVE_SPEED_SLOW = MOVE_SPEED/2;
	//Server: go through every client connected
	for (unsigned i = 0; i < connections.Size(); ++i)
	{
		Connection* connection = connections[i];
		// Get the object this connection is controlling
		Node* ballNode = serverObjects_[connection];
		// Client has no item connected
		if (!ballNode) continue;
		RigidBody* body = ballNode->GetComponent<RigidBody>();
		// Get the last controls sent by the client
		const Controls& controls = connection->GetControls();
		Quaternion rotation(controls.pitch_, controls.yaw_, 0.0f);
		// yaw and pitch. Roll is fixed to zero
		body->SetRotation(rotation);
		//each movement updates the players location by the movespeed, mouse input moves the camera with the player, movement adjusts acordingly
		//so the client camera is always facing the back end of the player model
		if (controls.buttons_ & CTRL_FORWARD)
		{
			body->ApplyForce(rotation * Vector3::FORWARD * MOVE_SPEED);
		}

		if (controls.buttons_ & CTRL_BACK)
		{
			body->ApplyForce(rotation * Vector3::BACK * MOVE_SPEED);
		}

		if (controls.buttons_ & CTRL_LEFT)
		{
			body->ApplyForce(rotation * Vector3::LEFT * MOVE_SPEED_SLOW);
		}
		if (controls.buttons_ & CTRL_RIGHT)
		{
			body->ApplyForce(rotation * Vector3::RIGHT * MOVE_SPEED_SLOW);
		}
		if (controls.buttons_ & CTRL_UP)
		{
			body->ApplyForce(rotation * Vector3::UP * MOVE_SPEED);
		}
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void CharacterDemo::HandleConnect(StringHash eventType, VariantMap& eventData)
{
	//Clears scene, prepares it for receiving
	CreateClientScene();

	Network* network = GetSubsystem<Network>();
	String address = serverAddressLineEdit_->GetText().Trimmed();
	if (address.Empty())
	{
		address = "localhost";
	} // Default to localhost if nothing else specified

	  // Connect to server, specify scene to use as a client for replication
	network->Connect(address, SERVER_PORT, scene_);
}

void CharacterDemo::HandleDisconnect(StringHash eventType, VariantMap& eventData)
{
	Log::WriteRaw("HandleDisconnect has been pressed. \n");
	Network* network = GetSubsystem<Network>();
	Connection* serverConnection = network->GetServerConnection();
	// Running as Client
	if (serverConnection)
	{
		serverConnection->Disconnect();
		scene_->Clear(true, false);
		clientObjectID_ = 0;
	}
	// Running as a server, stop it
	else if (network->IsServerRunning())
	{
		network->StopServer();
		scene_->Clear(true, false);
	}
}

void CharacterDemo::HandleStartServer(StringHash eventType, VariantMap&eventData)
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();

	Log::WriteRaw("(HandleStartServer called) Server is started!");
	Network* network = GetSubsystem<Network>();
	network->StartServer(SERVER_PORT);
	// code to make your main menu disappear. Boolean value
	menuVisible = !menuVisible;
	//initialise boids upon starting the server
	boidSet.Initialise(cache, scene_);

}

void CharacterDemo::HandleClientConnected(StringHash eventType, VariantMap& eventData)
{
	Log::WriteRaw("(HandleClientConnected) A client has connected!");
	using namespace ClientConnected;

	// When a client connects, assign to a scene
	Connection* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
	newConnection->SetScene(scene_);

	//send an event to the client that has just connected
	VariantMap remoteEventData;
	remoteEventData["aValueRemoteValue"] = 0;
}

void CharacterDemo::HandleClientDisconnected(StringHash eventType, VariantMap& eventData)
{
	using namespace ClientConnected;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
void CharacterDemo::HandleServerToClientObjectID(StringHash eventType, VariantMap & eventData)
{
	clientObjectID_ = eventData[PLAYER_ID].GetUInt();
	printf("Client ID : %i \n", clientObjectID_);
}

void CharacterDemo::HandleClientToServerReadyToStart(StringHash eventType, VariantMap & eventData)
{
	printf("Event sent by the Client and running on Server: Client is ready to start the game \n");
	using namespace ClientConnected;
	Connection* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
	// Create a controllable object for that client
	Node* newObject = CreateControllableObject();
	serverObjects_[newConnection] = newObject;
	// Finally send the object's node ID using a remote event
	VariantMap remoteEventData;
	remoteEventData[PLAYER_ID] = newObject->GetID();
	newConnection->SendRemoteEvent(E_CLIENTOBJECTAUTHORITY, true, remoteEventData);

}

//////////////////////////////////////////////////////////////////////////////////////////////////////
void CharacterDemo::HandleClientStartGame(StringHash eventType, VariantMap & eventData)
{
	printf("Client has pressed START GAME \n");
	if (clientObjectID_ == 0) // Client is still observer
	{
		Network* network = GetSubsystem<Network>();
		Connection* serverConnection = network->GetServerConnection();
		if (serverConnection)
		{
			VariantMap remoteEventData;
			remoteEventData[PLAYER_ID] = 0;
			serverConnection->SendRemoteEvent(E_CLIENTISREADY, true, remoteEventData);
		}
	}
}

void CharacterDemo::HandleClientFinishedLoading(StringHash eventType, VariantMap & eventData)
{
	Log::WriteRaw("Client has finished loading up the scene from the server \n");
}

void CharacterDemo::MoveCamera()
{
	ResourceCache* cache = GetSubsystem<ResourceCache>();
	// Right mouse button controls mouse cursor visibility: hide when pressed
	UI* ui = GetSubsystem<UI>();
	Input* input = GetSubsystem<Input>();

	if (clientObjectID_)
	{
		ballNode = scene_->GetNode(clientObjectID_);
		if (ballNode)
		{
			//maintains the camera distance from the clients playermodel
			const float CAMERA_DISTANCE = 20.0f;
			cameraNode_->SetPosition(ballNode->GetPosition() + cameraNode_->GetRotation() * Vector3::BACK * CAMERA_DISTANCE);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////

void CharacterDemo::HandleResume(StringHash eventType, VariantMap& eventData)
{
	//toggles menu visible on button press
	UI* ui = GetSubsystem<UI>();
	Input* input = GetSubsystem<Input>();

	menuVisible = !menuVisible;

	ui->GetCursor()->SetVisible(menuVisible);
	window_->SetVisible(menuVisible);
}

void CharacterDemo::HandleQuit(StringHash eventType, VariantMap& eventData)
{
	// quitting code here
	engine_->Exit();
}
