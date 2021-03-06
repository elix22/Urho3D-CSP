#ifndef CLIENT_SIDE_REDICTION_H
#define CLIENT_SIDE_REDICTION_H

#include <Urho3D/Container/HashSet.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <functional>
#include <unordered_set>
#include <vector>

namespace Urho3D
{
	class Context;
	class Serializable;
	class Controls;
	class Component;
	class Scene;
	class Node;
	class Connection;
	class MemoryBuffer;

	/* Client Side Prediction  Message IDs */
	/* Client -> server */
	// Custom input message to add update ID and be in sync with the update rate
	constexpr int MSG_CSP_INPUT = 32;
	/* Server -> client */
	// Sends a complete snapshot of the world
	constexpr int MSG_CSP_STATE = 33;
}

using namespace Urho3D;

/*
Client side prediction subsystem.
Works alongside the Network subsystem.
Usage:
Add LOCAL nodes which you want to be predicted.
Note: Uses the PhysicsWorld Fps as a fixed timestep.
*/
struct ClientSidePrediction : Object
{
	URHO3D_OBJECT(ClientSidePrediction, Object)

	using ID = unsigned;

	ClientSidePrediction(Context* context);

	// Register object factory and attributes.
	static void RegisterObject(Context* context);


	// Fixed timestep length
	float timestep = 0;

	// Server: Add a node to the client side prediction
	void add_node(Node* node);

	// Apply a given input locally
	std::function<void(const Controls& input, float timestep)> apply_local_input = nullptr;
	// Apply a given input to a specific client
	std::function<void(const Controls& input, float timestep, Connection* connection)> apply_client_input = nullptr;

	// Tags the input with "id" extraData, adds it to the input buffer, and sends it to the server.
	void add_input(Controls& input);


protected:
	// Networked scenes
	HashSet<Scene*> network_scenes;
	// Client-side predicted nodes
	HashMap<Scene*, std::vector<Node*>> scene_nodes;
	// State snapshot of each scene
	HashMap<Scene*, VectorBuffer> scene_states;
	// Reusable message buffer
	VectorBuffer state_message;
	// Reuseable hash set for tracking unused nodes
	std::unordered_set<Node*> unused_nodes;
	
	// current client-side update ID
	ID id = 0;
	// The current recieved ID from the server
	ID server_id = -1;

	// Input buffer
	std::vector<Controls> input_buffer;
	// Reusable message buffer
	VectorBuffer input_message;
	// Client input ID map
	HashMap<Connection*, ID> client_input_IDs;


	// Handle custom network messages
	void HandleNetworkMessage(StringHash eventType, VariantMap& eventData);
	// Send state snapshots
	void HandleRenderUpdate(StringHash eventType, VariantMap& eventData);

	void HandleInterceptNetworkUpdate(StringHash eventType, VariantMap& eventData);

	// Sends the client's input to the server
	void send_input(Controls& controls);
	// Read input sent from the client and apply it
	void read_input(Connection* connection, MemoryBuffer& message);

	// Prepare state snapshot for each networked scene
	void prepare_state_snapshots();
	// For each connection send the last received input ID and scene state snapshot
	void send_state_updates();
	// Send a state update to a given connection
	void send_state_update(Connection* connection);

	/*
	state serialization structure:
	- Last input ID
	- num of nodes
	- for each node
		- ID (unsigned to include local nodes)
		- attributes
		- User variables
		- num of components
		- for each component
			- ID (unsigned)
			- type
			- attributes
	*/
	// Process scene update received from the server
	void read_scene_state(MemoryBuffer& message);
	void read_node(MemoryBuffer& message);
	void read_component(MemoryBuffer& message, Node* node);

	// Get a complete network state snapshot
	void write_scene_state(VectorBuffer& message, Scene* scene);
	void write_node(VectorBuffer& message, Node& node);
	void write_component(VectorBuffer& message, Component& component);

	// Write all the network attributes
	void write_network_attributes(Serializable& object, Serializer& dest);
	// Read all the network attributes
	void read_network_attributes(Serializable& object, Deserializer& source);

	// Intercept network attributes to avoid replication overriding prediction
	void set_intercept_network_attributes(Serializable& object);

	// do client-side prediction
	void predict();

	// Re-apply all the inputs since after the current server ID to the current ID to correct the current network state.
	void reapply_inputs();

	// Remove all the elements in the buffer which are behind the server_id, including it since it was already applied.
	void remove_obsolete_history();

private:
	// Update time interval
	//TODO same as timestep?
	float updateInterval_ = 1.f / 30.f;	// default to 30 FPS
	// Update time accumulator
	float updateAcc_ = 0;
};


#endif//guard
