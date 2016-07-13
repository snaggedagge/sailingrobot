/****************************************************************************************
 *
 * File:
 * 		MessageBus.h
 *
 * Purpose:
 *		The message bus manages message distribution to nodes allowing nodes to
 *		communicate with one another.
 *
 * Developer Notes:
 *		Nodes can only be added before the run function is called currently. This is to
 *		reduce the number of thread locks in place and because once the system has 
 *		started its very rare that a node should be registered afterwards on the fly.
 *
 *
 ***************************************************************************************/

#pragma once

#include "Node.h"
#include "Messages/Message.h"
#include <vector>


class MessageBus {
public:
	///----------------------------------------------------------------------------------
 	/// Default constructor
 	///
 	///----------------------------------------------------------------------------------
	MessageBus();

	///----------------------------------------------------------------------------------
	///----------------------------------------------------------------------------------
	~MessageBus();

	///----------------------------------------------------------------------------------
 	/// Registers a node onto the message bus allowing it receive direct messages. The 
 	/// message bus does not own the node.
 	///
 	/// @param node 			Pointer to the node that should be registered.
 	///----------------------------------------------------------------------------------
	void registerNode(Node* node);

	///----------------------------------------------------------------------------------
 	/// Registers a node onto the message bus allowing it receive direct messages and
 	/// also subscribes it for a particular message type.
 	///
 	/// @param node 			Pointer to the node that should be registered.
 	/// @param msgType 			The type of message to register for
 	///----------------------------------------------------------------------------------
	void registerNode(Node* node, MessageType msgType);

	///----------------------------------------------------------------------------------
 	/// Enqueues a message onto the message queue for distribution through the message 
 	/// bus.
 	///
 	/// @param msg 				Pointer to the message that should be enqeued, this
 	///							passes ownership to the MessageBus.
 	///----------------------------------------------------------------------------------
	void sendMessage(Message* msg);

	///----------------------------------------------------------------------------------
 	/// Begins running the message bus and distributing messages to nodes that have been
 	/// registered. This function won't ever return.
 	///----------------------------------------------------------------------------------
	void run();
private:
	///----------------------------------------------------------------------------------
 	/// Stores information about a registered node and the message types it is interested
 	/// in.
 	///----------------------------------------------------------------------------------
	struct RegisteredNode {
		RegisteredNode(Node* node) : nodePtr(node) { }

		const Node* nodePtr;

		///------------------------------------------------------------------------------
 		/// Returns true if a registered node is interested in a message type.
 		///
 		/// @param type 		The message type which is checked for.
 		///------------------------------------------------------------------------------
		bool isInterested(MessageType type)
		{
			for(auto msgType : interestedList) { 
				if(type == msgType) { return true; } 
			}
			return false;
		}

		///------------------------------------------------------------------------------
 		/// Subscribes the registered node for a particular type of message
 		///
 		/// @param type 		The message type to subscribe this registered node to.
 		///------------------------------------------------------------------------------
		void subscribe(MessageType type) 
		{
			// Maintain only one copy of each interested type.
			if(not isInterested(type)) { interestedList.push_back(type); }
		}


	private:
		std::vector<MessageType> interestedList;
	};

	///----------------------------------------------------------------------------------
 	/// Looks for existing registered node for a given node pointer and returns a pointer
 	/// to it. If the node has not yet been registered, it is then registered and a 
 	/// pointer to the new RegisteredNode is returned.
 	///----------------------------------------------------------------------------------
	RegisteredNode* getRegisteredNode(Node* node);

	bool 							m_Running;
	std::vector<RegisteredNode*> 	m_RegisteredNodes;
};