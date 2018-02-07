#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"

#include <iostream>

static unsigned int SERVER_PORT = 65000;
static unsigned int CLIENT_PORT = 65050;
static unsigned int MAX_CONNECTIONS =  4;
unsigned char GetPacketIdentifier(RakNet::Packet *p);
int main() {
	RakNet::RakPeerInterface *rakPeerInterface = RakNet::RakPeerInterface::GetInstance();
	RakNet::RakPeerInterface *rakClient = RakNet::RakPeerInterface::GetInstance();
	
	char userInput[255];
	std::cout << "press (s) for server (c) for client" << std::endl;
	std::cin >> userInput;
	if (userInput[0] == 's') {
		RakNet::SocketDescriptor socketDescriptors[1];
		socketDescriptors[0].port = SERVER_PORT;
		socketDescriptors[0].socketFamily = AF_INET; // Test out IPV4

		bool isSuccess = rakPeerInterface->Startup(MAX_CONNECTIONS, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
		assert(isSuccess);
		//ensure wer are server
		rakPeerInterface->SetMaximumIncomingConnections(MAX_CONNECTIONS);
		std::cout << "server started" << std::endl;
	}
	else if (userInput[0] == 'c') {
		RakNet::SocketDescriptor socketDescriptor(CLIENT_PORT, 0);
		socketDescriptor.socketFamily = AF_INET;
		rakClient->Startup(8, &socketDescriptor, 1);
		rakClient->SetOccasionalPing(true);
		bool isSuccess = rakClient->Connect("127.0.0.1", SERVER_PORT,0 ,0 , 0) == RakNet::CONNECTION_ATTEMPT_STARTED;
		assert(isSuccess);
		std::cout << "client started" << std::endl;

	}


	std::cout << "press q and then return to exit" << std::endl;
	std::cin >> userInput;
	return 0;
}