
#pragma once

#include "Sample.h"
#include "Boids.h"

namespace Urho3D
{

	class Node;
	class Scene;
	class Window;

}

class Character;
class Touch;

/// Moving character example.
/// This sample demonstrates:
///     - Controlling a humanoid character through physics
///     - Driving animations using the AnimationController component
///     - Manual control of a bone scene node
///     - Implementing 1st and 3rd person cameras, using raycasts to avoid the 3rd person camera clipping into scenery
///     - Defining attributes of a custom component so that it can be saved and loaded
///     - Using touch inputs/gyroscope for iOS/Android (implemented through an external file)
class CharacterDemo : public Sample
{
	URHO3D_OBJECT(CharacterDemo, Sample);

public:
	/// Construct.
	CharacterDemo(Context* context);
	/// Destruct.
	~CharacterDemo();

	/// Which port this is running on
	static const unsigned short SERVER_PORT = 2345;
	///initialise Window
	SharedPtr<Window> window_;

	/// Setup after engine initialization and before running the main loop.
	virtual void Start();

	const int CTRL_FORWARD = 1;
	const int CTRL_BACK = 2;
	const int CTRL_LEFT = 4;
	const int CTRL_RIGHT = 8;
	const int CTRL_TOGGLE = 84;
	const int CTRL_UP = 32;
	const int CTRL_DOWN = 17;


	///create buttons and linedit
	Button* CreateButton(const String& text, int pHeight, Urho3D::Window*whichWindow, Button* button);
	LineEdit* CreateLineEdit(const String& text, int pHeight, Urho3D::Window*whichWindow, LineEdit* lineedit);


protected:
	/// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
	virtual String GetScreenJoystickPatchString() const {
		return
			"<patch>"
			"    <add sel=\"/element\">"
			"        <element type=\"Button\">"
			"            <attribute name=\"Name\" value=\"Button3\" />"
			"            <attribute name=\"Position\" value=\"-120 -120\" />"
			"            <attribute name=\"Size\" value=\"96 96\" />"
			"            <attribute name=\"Horiz Alignment\" value=\"Right\" />"
			"            <attribute name=\"Vert Alignment\" value=\"Bottom\" />"
			"            <attribute name=\"Texture\" value=\"Texture2D;Textures/TouchInput.png\" />"
			"            <attribute name=\"Image Rect\" value=\"96 0 192 96\" />"
			"            <attribute name=\"Hover Image Offset\" value=\"0 0\" />"
			"            <attribute name=\"Pressed Image Offset\" value=\"0 0\" />"
			"            <element type=\"Text\">"
			"                <attribute name=\"Name\" value=\"Label\" />"
			"                <attribute name=\"Horiz Alignment\" value=\"Center\" />"
			"                <attribute name=\"Vert Alignment\" value=\"Center\" />"
			"                <attribute name=\"Color\" value=\"0 0 0 1\" />"
			"                <attribute name=\"Text\" value=\"Gyroscope\" />"
			"            </element>"
			"            <element type=\"Text\">"
			"                <attribute name=\"Name\" value=\"KeyBinding\" />"
			"                <attribute name=\"Text\" value=\"G\" />"
			"            </element>"
			"        </element>"
			"    </add>"
			"    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/attribute[@name='Is Visible']\" />"
			"    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">1st/3rd</replace>"
			"    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]\">"
			"        <element type=\"Text\">"
			"            <attribute name=\"Name\" value=\"KeyBinding\" />"
			"            <attribute name=\"Text\" value=\"F\" />"
			"        </element>"
			"    </add>"
			"    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]/attribute[@name='Is Visible']\" />"
			"    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">Jump</replace>"
			"    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]\">"
			"        <element type=\"Text\">"
			"            <attribute name=\"Name\" value=\"KeyBinding\" />"
			"            <attribute name=\"Text\" value=\"SPACE\" />"
			"        </element>"
			"    </add>"
			"</patch>";
	}

private:

	LineEdit* serverAddressLineEdit_;

	Controls FromClientToServerControls();

	void ProcessClientControls();
	void HandlePhysicsPreStep(StringHash eventType, VariantMap & eventData);
	void HandleClientFinishedLoading(StringHash eventType, VariantMap& eventData);

	/// Create static scene content.
	void CreateScene();

	void CreateClientScene();

	///Create Main Menu
	void CreateMainMenu();

	/// Construct an instruction text to the UI.
	void CreateInstructions();
	/// Subscribe to necessary events.
	void SubscribeToEvents();
	/// Handle application update. Set controls to character.
	void HandleUpdate(StringHash eventType, VariantMap& eventData);
	/// Handle application post-update. Update camera position after character has 
	void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
	///Handle the serverStartup
	void HandleStartServer(StringHash eventType, VariantMap&eventData);
	///Handles connecting and disconnecting
	void HandleConnect(StringHash eventType, VariantMap& eventData);
	void HandleDisconnect(StringHash eventType, VariantMap& eventData);
	///Quit from the application
	void HandleQuit(StringHash eventType, VariantMap& eventData);
	///connecting to a server for client
	void HandleClientConnected(StringHash eventType, VariantMap& eventData);
	///Disconnecting from a server for client
	void HandleClientDisconnected(StringHash eventType, VariantMap& eventData);
	///When the client wants to start the game
	void HandleClientStartGame(StringHash eventType, VariantMap & eventData);
	///update camera to follow player, remains a set value apart from the model
	void MoveCamera();
	///toggles menu with a button
	void HandleResume(StringHash eventType, VariantMap& eventData);

	Node* CreateControllableObject(); // Server: Create a controllable ball
	unsigned clientObjectID_ = 0; // Client: ID of own object
	HashMap<Connection*, WeakPtr<Node> > serverObjects_; // Server Client/Object HashMap
														 // Handle remote event from server to Client to share controlled object node ID.
	void HandleServerToClientObjectID(StringHash eventType, VariantMap& eventData);
	/// Handle remote event, client tells server that client is ready to start game
	void HandleClientToServerReadyToStart(StringHash eventType, VariantMap& eventData);
	/// Touch utility object.
	SharedPtr<Touch> touch_;
	/// The controllable character component.
	WeakPtr<Character> character_;
	/// First person camera flag.
	bool firstPerson_;
	///bool determines if the menu is visible or not
	bool menuVisible = false;
	///boidset class obj
	BoidSet boidSet;
	///shared pointed for all instances of clients object node
	SharedPtr<Node> ballNode;
	/// Reflection camera scene node.
	SharedPtr<Node> reflectionCameraNode_;
	/// Water body scene node.
	SharedPtr<Node> waterNode_;
	/// Reflection plane representing the water surface.
	Plane waterPlane_;
	/// Clipping plane for reflection rendering. Slightly biased downward from the reflection plane to avoid artifacts.
	Plane waterClipPlane_;

};
