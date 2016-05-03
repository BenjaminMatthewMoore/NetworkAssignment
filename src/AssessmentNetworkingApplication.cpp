#include "AssessmentNetworkingApplication.h"
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>

#include "Gizmos.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using glm::vec3;
using glm::vec4;
int sanity;
bool firstFrame = true;
unsigned int expectedPacketNumber = 0;

AssessmentNetworkingApplication::AssessmentNetworkingApplication() 
: m_camera(nullptr),
m_peerInterface(nullptr) {

}

AssessmentNetworkingApplication::~AssessmentNetworkingApplication() {

}

bool AssessmentNetworkingApplication::startup() {

	// setup the basic window
	createWindow("Client Application", 1280, 720);

	Gizmos::create();

	// set up basic camera
	m_camera = new Camera(glm::pi<float>() * 0.25f, 16 / 9.f, 0.1f, 1000.f);
	m_camera->setLookAtFrom(vec3(10, 10, 10), vec3(0));

	// start client connection
	m_peerInterface = RakNet::RakPeerInterface::GetInstance();
	
	RakNet::SocketDescriptor sd;
	m_peerInterface->Startup(1, &sd, 1);

	// request access to server
	std::string ipAddress = "127.0.0.1";
	std::cout << "Connecting to server at: ";
	//std::cin >> ipAddress;
	RakNet::ConnectionAttemptResult res = m_peerInterface->Connect(ipAddress.c_str(), SERVER_PORT, nullptr, 0);

	if (res != RakNet::CONNECTION_ATTEMPT_STARTED) {
		std::cout << "Unable to start connection, Error number: " << res << std::endl;
		return false;
	}

	return true;
}

void AssessmentNetworkingApplication::shutdown() {
	// delete our camera and cleanup gizmos
	delete m_camera;
	Gizmos::destroy();

	// destroy our window properly
	destroyWindow();
}

bool AssessmentNetworkingApplication::update(float deltaTime) {

	// close the application if the window closes
	if (glfwWindowShouldClose(m_window) ||
		glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		return false;

	// update camera
	m_camera->update(deltaTime);

	// handle network messages
	RakNet::Packet* packet;


	for (packet = m_peerInterface->Receive(); packet;
	m_peerInterface->DeallocatePacket(packet),
		packet = m_peerInterface->Receive()) 
	{
		switch (packet->data[0]) {
		case ID_CONNECTION_REQUEST_ACCEPTED:
			std::cout << "Our connection request has been accepted." << std::endl;
			break;
		case ID_CONNECTION_ATTEMPT_FAILED:
			std::cout << "Our connection request failed!" << std::endl;
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			std::cout << "The server is full." << std::endl;
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			std::cout << "We have been disconnected." << std::endl;
			break;
		case ID_CONNECTION_LOST:
			std::cout << "Connection lost." << std::endl;
			break;
		case ID_ENTITY_LIST: {

			// receive list of entities
			RakNet::BitStream stream(packet->data, packet->length, false);
			stream.IgnoreBytes(sizeof(RakNet::MessageID));
			unsigned int size = 0;
			unsigned int sizeOfEntityData= 0;
			stream.Read(size);
			sizeOfEntityData = size - sizeof(unsigned int);
			unsigned int *packetNumber = new unsigned int;
			stream.Read((char*)packetNumber, 4);
			if (firstFrame)
				expectedPacketNumber = *packetNumber;
			std::vector<AIEntity> newData;
			// first time receiving entities
			if (newData.size() == 0) 
			{
				newData.resize(sizeOfEntityData / sizeof(AIEntity));
				m_aiEntities.resize(sizeOfEntityData / sizeof(AIEntity));
			}

			stream.Read((char*)newData.data(), sizeOfEntityData);

			if (expectedPacketNumber == *packetNumber )
			{
				std::cout << "Packet Valid" << std::endl;
				//do Normal;
				m_aiEntities = newData;
			}

			if (*packetNumber < expectedPacketNumber )
			{
				std::cout << "Packet Delayed" << std::endl;
				//delayed packet
				for (auto& ai : m_aiEntities)
				{
					ai.position.x += ai.velocity.x * 0.016666667;
					ai.position.y += ai.velocity.y * 0.016666667;
				}
			}

			if (*packetNumber > expectedPacketNumber)
			{
				std::cout << "Packets Lost" << std::endl;
				//packets lost, lerp to new pos
				int i = 0;
				for (auto& ai : m_aiEntities)
				{
					glm::vec2 oldPos;
					oldPos.x = ai.position.x;
					oldPos.y = ai.position.y;
					glm::vec2 newPos;
					newPos.x = newData[i].position.x;
					newPos.y = newData[i].position.y;

					glm::vec2 movePos = oldPos + (newPos - oldPos) * 0.2f;
					i++;
					ai.position.x = oldPos.x;
					ai.position.y = oldPos.y;
				}
				expectedPacketNumber = *packetNumber;
			}


			/*float lastPosX = m_aiEntities[1].position.x;
			float lastPosY = m_aiEntities[1].position.y;
			*/
			
			
			
			

			//if ((std::abs(newData[1].position.x - lastPosX) > 0.5 &&std::abs(newData[1].position.x - lastPosX) <40) || (std::abs(newData[1].position.y - lastPosY) > 0.5 &&std::abs(newData[1].position.y - lastPosY) <40) && firstFrame == false)
			//if ((packetNumber - expectedPacketNumber) > 3)
			//{
			//	std::cout << "This is working!" << std::endl;

			//	std::cout << "delayed package handled" << std::endl;
			//	sanity++;
			//	if (sanity > 5)
			//	{
			//		//m_aiEntities = newData;
			//		std::cout << "reset_________________________________________________________" << std::endl;
			//		sanity = 0;

			//else
			//{
			//	; firstFrame = false; sanity = 0;
			//}
			//	
			expectedPacketNumber++;
			firstFrame = false;
			break;
		}

		default:
			std::cout << "Received unhandled message." << std::endl;
			break;
		}
		
	}

	if (packet == nullptr && firstFrame == false);
		for (auto& ai : m_aiEntities)
		{
			ai.position.x += ai.velocity.x * 0.016666667;
			ai.position.y += ai.velocity.y * 0.016666667;
		}

	Gizmos::clear();

	// add a grid
	for (int i = 0; i < 21; ++i) {
		Gizmos::addLine(vec3(-10 + i, 0, 10), vec3(-10 + i, 0, -10),
						i == 10 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));

		Gizmos::addLine(vec3(10, 0, -10 + i), vec3(-10, 0, -10 + i),
						i == 10 ? vec4(1, 1, 1, 1) : vec4(0, 0, 0, 1));
	}

	return true;
}

void AssessmentNetworkingApplication::draw() {

	// clear the screen for this frame
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	int i = 0;
	int x = 0;
	int z = 0;
	// draw entities
	for (auto& ai : m_aiEntities) {
		
		vec3 p1 = vec3(ai.position.x + ai.velocity.x * 0.25f, 0, ai.position.y + ai.velocity.y * 0.25f);
		vec3 p2 = vec3(ai.position.x, 0, ai.position.y) - glm::cross(vec3(ai.velocity.x, 0, ai.velocity.y), vec3(0, 1, 0)) * 0.1f;
		vec3 p3 = vec3(ai.position.x, 0, ai.position.y) + glm::cross(vec3(ai.velocity.x, 0, ai.velocity.y), vec3(0, 1, 0)) * 0.1f;
		glm::vec4 colour;
		
		colour = glm::vec4(i/255.0, x/255.0, z/255.0, 1);

		Gizmos::addTri(p1, p2, p3, colour);
		
		i+=7; x += 13; z += 21;
		if (i > 255)
			i -=255;
		if (x > 255)
			x -=255;
		if (z > 255)
			z -= 255;
		
	}

	// display the 3D gizmos
	Gizmos::draw(m_camera->getProjectionView());
}