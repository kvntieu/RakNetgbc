//Student Name: Kevin Tieu
//Student #: 101005823
//A 3 player free for all turn based rpg.

#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "BitStream.h"
#include "player.h"
#include "Beserker.h"
#include "Assassin.h"
#include "Paladin.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <mutex>
#include <vector>

static unsigned int SERVER_PORT = 65001;
static unsigned int CLIENT_PORT = 65002;
static unsigned int MAX_CONNECTIONS = 4;

enum NetworkState
{
	NS_Init = 0,
	NS_PendingStart,
	NS_Started,
	NS_Lobby,
	NS_WaitingForOthers,
	NS_Game,
	NS_GameOver
};

bool isServer = false;
bool isRunning = true;
bool doOnce = true;

RakNet::RakPeerInterface *g_rakPeerInterface = nullptr;
RakNet::SystemAddress g_serverAddress;

std::mutex g_networkState_mutex;
NetworkState g_networkState = NS_Init;

enum {
	ID_PLAYER_READY = ID_USER_PACKET_ENUM,
	ID_GAME_START,
	ID_PLAYER_ACTION,
	ID_PLAYER_RETURN_MSG,
	ID_PLAYER_START_TURN,
	ID_GAME_OVER
};

typedef std::map<RakNet::RakNetGUID, Player*> PlayerMap;
PlayerMap m_players;
typedef std::vector<RakNet::RakNetGUID> PlayerList;
PlayerList m_playerIDs;

int activePlayer = -1;

bool isPlayerTurn = false;
unsigned char GetPacketIdentifier(RakNet::Packet*);
bool HandleLowLevelPackets(RakNet::Packet*);
void InputHandler();
void PacketHandler();
void OnConnectionAccepted(RakNet::Packet*);
void OnLostConnection(RakNet::Packet*);
void RegisterPlayer(RakNet::Packet*);
void DisplayMessage(RakNet::Packet*);
std::string GetPlayerStats();
void StartGame(RakNet::Packet*);
void ResolveAction(RakNet::Packet*);
void NextPlayerTurn();
void StartingTurn(RakNet::Packet*);
void EndGame(RakNet::Packet*);

int main()
{
	g_rakPeerInterface = RakNet::RakPeerInterface::GetInstance();

	std::thread InputThread(InputHandler);
	std::thread PacketThread(PacketHandler);

	while (isRunning)
	{
		if (g_networkState == NS_PendingStart)
		{
			if (isServer)
			{
				RakNet::SocketDescriptor socketDescriptors[1];
				socketDescriptors[0].port = SERVER_PORT;
				socketDescriptors[0].socketFamily = AF_INET; // Test out IPV4

				bool isSuccess = g_rakPeerInterface->Startup(MAX_CONNECTIONS, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
				assert(isSuccess);
				g_rakPeerInterface->SetMaximumIncomingConnections(MAX_CONNECTIONS);

				std::cout << "server started" << std::endl;
				g_networkState_mutex.lock();
				g_networkState = NS_Started;
				g_networkState_mutex.unlock();
			}
			else
			{
				RakNet::SocketDescriptor socketDescriptor(CLIENT_PORT, 0);
				socketDescriptor.socketFamily = AF_INET;

				while (RakNet::IRNS2_Berkley::IsPortInUse(socketDescriptor.port, socketDescriptor.hostAddress, socketDescriptor.socketFamily, SOCK_DGRAM) == true)
					socketDescriptor.port++;

				RakNet::StartupResult result = g_rakPeerInterface->Startup(8, &socketDescriptor, 1);
				assert(result == RakNet::RAKNET_STARTED);

				g_networkState_mutex.lock();
				g_networkState = NS_Started;
				g_networkState_mutex.unlock();

				g_rakPeerInterface->SetOccasionalPing(true);
				RakNet::ConnectionAttemptResult car = g_rakPeerInterface->Connect("127.0.0.1", SERVER_PORT, nullptr, 0);
				RakAssert(car == RakNet::CONNECTION_ATTEMPT_STARTED);
				std::cout << "client attempted connection..." << std::endl;
			}
		}
		else if (g_networkState == NS_Game)
		{
			if (isServer)
			{
				if (activePlayer == -1)
				{
					std::cout << "Starting game." << std::endl;

					activePlayer = rand() % 3;

					RakNet::BitStream bs;
					bs.Write((RakNet::MessageID)ID_GAME_START);
					RakNet::RakString msg;
					msg = "Game starting. Displaying all player stats.\n";
					std::string stats = GetPlayerStats();
					msg.AppendBytes(stats.c_str(), stats.length());
					bs.Write(msg);

					g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);

					NextPlayerTurn();
				}
			}
		}
		else if (g_networkState == NS_GameOver)
		{
			if (isServer && doOnce)
			{
				doOnce = false;
				for (int i = 0; i < m_playerIDs.size(); i++)
				{
					RakNet::BitStream bs;
					bs.Write((RakNet::MessageID)ID_GAME_OVER);
					RakNet::RakString msg;
					msg = "Gameover. ";
					std::string str;
					if (i == activePlayer)
					{
						str = "WINNER. You are the last one standing in the pit.";
					}
					else
					{
						str = "Player " + std::to_string(activePlayer + 1) + ": " + m_players.find(m_playerIDs[activePlayer])->second->GetName() + " is the last one standing.";
					}
					msg.AppendBytes(str.c_str(), str.length());
					bs.Write(msg);

					g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, m_playerIDs[i], false);
				}
			}
		}
	}

	InputThread.join();
	PacketThread.join();

	return 0;
}

void InputHandler()
{
	while (isRunning)
	{
		char userInput[255];
		if (g_networkState == NS_Init)
		{
			std::cout << "press (s) for server (c) for client" << std::endl;
			std::cin >> userInput;
			isServer = (userInput[0] == 's');
			g_networkState_mutex.lock();
			g_networkState = NS_PendingStart;
			g_networkState_mutex.unlock();
		}
		else if (g_networkState == NS_Lobby)
		{
			std::cout << "Enter your name" << std::endl;
			std::cin >> userInput;

			RakNet::BitStream bs;
			bs.Write((RakNet::MessageID)ID_PLAYER_READY);
			RakNet::RakString name(userInput);
			bs.Write(name);

			std::cout << "Choose your class: 1 for Beserker, 2 for Assassin, 3 for Paladin" << std::endl;
			std::cin >> userInput;
			while (userInput[0] != '1' && userInput[0] != '2' && userInput[0] != '3')
			{
				std::cout << "Invalid input. Choose your class: 1 for Beserker, 2 for Assassin, 3 for Paladin" << std::endl;
				std::cin >> userInput;
			}
			RakNet::uint24_t classNum;
			if (userInput[0] == '1')
			{
				classNum = 0;
			}
			else if (userInput[0] == '2')
			{
				classNum = 1;
			}
			else
			{
				classNum = 2;
			}
			bs.Write(classNum);
			assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
			g_networkState_mutex.lock();
			g_networkState = NS_WaitingForOthers;
			g_networkState_mutex.unlock();
		}
		else if (g_networkState == NS_WaitingForOthers)
		{
			static bool doOnce = false;
			if (!doOnce)
				std::cout << "Waiting for other players to join..." << std::endl;

			doOnce = true;
		}
		else if (g_networkState == NS_Game)
		{
			if (isPlayerTurn)
			{
				RakNet::uint24_t action;
				RakNet::uint24_t target;
				std::cout << "Enter (a) to attack, (s) to block, (d) to view stats" << std::endl;
				std::cin >> userInput;
				while (userInput[0] != 'a' && userInput[0] != 's' && userInput[0] != 'd')
				{
					std::cout << "Invalid input. Enter (a) to attack, (s) to block, (d) to view stats" << std::endl;
					std::cin >> userInput;
				}
				if (userInput[0] == 'a')
				{
					action = 0;
					std::cout << "Choose player to attack, enter 1, 2 or 3" << std::endl;
					std::cin >> userInput;
					while (userInput[0] != '1' && userInput[0] != '2' && userInput[0] != '3')
					{
						std::cout << "Invalid input. Choose player to attack, enter 1, 2 or 3" << std::endl;
						std::cin >> userInput;
					}
					if (userInput[0] == '1')
					{
						target = 0;
					}
					else if (userInput[0] == '2')
					{
						target = 1;
					}
					else
					{
						target = 2;
					}
				}
				else if (userInput[0] == 'b')
				{
					action = 1;
					target = 0;
				}
				else
				{
					action = 2;
					target = 0;
				}

				RakNet::BitStream bs;
				bs.Write((RakNet::MessageID)ID_PLAYER_ACTION);
				bs.Write(action);
				bs.Write(target);
				assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
				isPlayerTurn = false;
			}
		}
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}
}

void PacketHandler()
{
	while (isRunning)
	{
		for (RakNet::Packet* packet = g_rakPeerInterface->Receive(); packet != nullptr; g_rakPeerInterface->DeallocatePacket(packet), packet = g_rakPeerInterface->Receive())
		{
			if (!HandleLowLevelPackets(packet))
			{
				//our game specific packets
				unsigned char packetIdentifier = GetPacketIdentifier(packet);
				switch (packetIdentifier)
				{
				case ID_PLAYER_READY:
					RegisterPlayer(packet);
					break;
				case ID_GAME_START:
					StartGame(packet);
					break;
				case ID_PLAYER_RETURN_MSG:
					DisplayMessage(packet);
					break;
				case ID_PLAYER_ACTION:
					ResolveAction(packet);
					break;
				case ID_PLAYER_START_TURN:
					StartingTurn(packet);
					break;
				case ID_GAME_OVER:
					EndGame(packet);
					break;
				default:
					break;
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::microseconds(100));
	}
}

unsigned char GetPacketIdentifier(RakNet::Packet *p)
{
	if (p == nullptr)
		return 255;

	if ((unsigned char)p->data[0] == ID_TIMESTAMP)
	{
		RakAssert(p->length > sizeof(RakNet::MessageID) + sizeof(RakNet::Time));
		return (unsigned char)p->data[sizeof(RakNet::MessageID) + sizeof(RakNet::Time)];
	}
	else
		return (unsigned char)p->data[0];
}

bool HandleLowLevelPackets(RakNet::Packet* packet)
{
	bool isHandled = true;
	// We got a packet, get the identifier with our handy function
	unsigned char packetIdentifier = GetPacketIdentifier(packet);

	// Check if this is a network message packet
	switch (packetIdentifier)
	{
	case ID_DISCONNECTION_NOTIFICATION:
		// Connection lost normally
		printf("ID_DISCONNECTION_NOTIFICATION\n");
		break;
	case ID_ALREADY_CONNECTED:
		// Connection lost normally
		printf("ID_ALREADY_CONNECTED with guid %" PRINTF_64_BIT_MODIFIER "u\n", packet->guid);
		break;
	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
		printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
		break;
	case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server telling the clients of another client disconnecting gracefully.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_DISCONNECTION_NOTIFICATION\n");
		break;
	case ID_REMOTE_CONNECTION_LOST: // Server telling the clients of another client disconnecting forcefully.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_CONNECTION_LOST\n");
		break;
	case ID_NEW_INCOMING_CONNECTION:
		//client connecting to server
		printf("ID_NEW_INCOMING_CONNECTION\n");
		break;
	case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_NEW_INCOMING_CONNECTION\n");
		break;
	case ID_CONNECTION_BANNED: // Banned from this server
		printf("We are banned from this server.\n");
		break;
	case ID_CONNECTION_ATTEMPT_FAILED:
		printf("Connection attempt failed\n");
		break;
	case ID_NO_FREE_INCOMING_CONNECTIONS:
		// Sorry, the server is full.  I don't do anything here but
		// A real app should tell the user
		printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
		break;

	case ID_INVALID_PASSWORD:
		printf("ID_INVALID_PASSWORD\n");
		break;

	case ID_CONNECTION_LOST:
		// Couldn't deliver a reliable packet - i.e. the other system was abnormally
		// terminated
		printf("ID_CONNECTION_LOST\n");
		OnLostConnection(packet);
		break;

	case ID_CONNECTION_REQUEST_ACCEPTED:
		// This tells the client they have connected
		printf("ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s\n", packet->systemAddress.ToString(true), packet->guid.ToString());
		printf("My external address is %s\n", g_rakPeerInterface->GetExternalID(packet->systemAddress).ToString(true));
		OnConnectionAccepted(packet);
		break;
	case ID_CONNECTED_PING:
	case ID_UNCONNECTED_PING:
		printf("Ping from %s\n", packet->systemAddress.ToString(true));
		break;
	default:
		isHandled = false;
		break;
	}
	return isHandled;
}

//client
void OnConnectionAccepted(RakNet::Packet* packet)
{
	//server should not ne connecting to anybody, 
	//clients connect to server
	assert(!isServer);
	g_networkState_mutex.lock();
	g_networkState = NS_Lobby;
	g_networkState_mutex.unlock();
	g_serverAddress = packet->systemAddress;
}

void OnLostConnection(RakNet::Packet* packet)
{
	int playerNum = -1;
	for (int i = 0; i < m_playerIDs.size(); i++)
	{
		if (packet->guid == m_playerIDs[i])
		{
			playerNum = i;
			m_players.find(m_playerIDs[i])->second->Death();
		}
	}
	if (playerNum > -1)
	{
		RakNet::BitStream bs;
		bs.Write((RakNet::MessageID)ID_PLAYER_RETURN_MSG);
		RakNet::RakString msg;
		std::string str;
		str = "Player " + std::to_string(playerNum + 1) + ": " + m_players.find(m_playerIDs[playerNum])->second->GetName() + " is dead.";
		msg.AppendBytes(str.c_str(), str.length());
		bs.Write(msg);

		g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);

		if (playerNum == activePlayer)
			NextPlayerTurn();
	}
}

//server
void RegisterPlayer(RakNet::Packet* packet)
{
	assert(isServer);

	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString userName;
	bs.Read(userName);
	RakNet::uint24_t classNum;
	bs.Read(classNum);

	Player* player;
	if (classNum == (RakNet::uint24_t)0)
	{
		player = new Beserker();

	}
	else if (classNum == (RakNet::uint24_t)1)
	{
		player = new Assassin();
	}
	else
	{
		player = new Paladin();
	}
	player->SetName((std::string)userName);
	m_playerIDs.push_back(packet->guid);
	m_players.insert(std::make_pair(packet->guid, player));

	std::cout << userName.C_String() << " has joined. Total players: " << m_players.size() << std::endl;

	std::cout << "Connecting address " << packet->systemAddress.ToString() << std::endl;
	std::cout << "Connecting guid " << packet->guid.ToString() << std::endl;

	int playerCounter = 0;

	for (RakNet::RakNetGUID guid : m_playerIDs)
	{
		RakNet::BitStream output;
		output.Write((RakNet::MessageID)ID_PLAYER_RETURN_MSG);
		RakNet::RakString msg;
		std::string str;

		if (guid == packet->guid)
		{
			str = "Welcome to the pit, " + (std::string)userName + ".\n" + std::to_string(playerCounter + 1) + " player(s) waiting in the lobby.";
		}
		else
		{
			str = (std::string)userName + " has joined the pit.";
		}
		msg.AppendBytes(str.c_str(), str.length());
		output.Write(msg);

		g_rakPeerInterface->Send(&output, HIGH_PRIORITY, RELIABLE_ORDERED, 0, guid, false);

		playerCounter++;
	}

	if (m_players.size() == 3)
	{
		g_networkState_mutex.lock();
		g_networkState = NS_Game;
		g_networkState_mutex.unlock();
	}
}

void DisplayMessage(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString msg;
	bs.Read(msg);

	std::cout << msg.C_String() << std::endl;
}

std::string GetPlayerStats()
{
	std::string statsMsg = "";

	if (!isServer)
		return statsMsg;

	for (int i = 0; i < m_playerIDs.size(); i++)
	{
		std::string playerStats;
		std::string playerClass;
		Player* currentPlayer = m_players.find(m_playerIDs[i])->second;
		playerClass = currentPlayer->GetClass();

		playerStats = "Player " + std::to_string(i + 1) + ": " + currentPlayer->GetName() + " the " + playerClass + "\n";
		playerStats += "HP: " + std::to_string(currentPlayer->GetHealth()) + "  ";
		playerStats += "ATK: " + std::to_string(currentPlayer->GetAttack()) + "  ";
		playerStats += "DEF: " + std::to_string(currentPlayer->GetDefense()) + "  " + "\n";
		statsMsg += playerStats;
	}

	return statsMsg;
}

void StartGame(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString msg;
	bs.Read(msg);

	std::cout << msg.C_String() << std::endl;

	g_networkState_mutex.lock();
	g_networkState = NS_Game;
	g_networkState_mutex.unlock();
}
//This function takes in the player's turn action and responds accordingly
void ResolveAction(RakNet::Packet* packet)
{
	assert(isServer);

	//Get the player's action for the turn
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::uint24_t action;
	bs.Read(action);
	RakNet::uint24_t target;
	bs.Read(target);

	RakNet::BitStream output;
	RakNet::RakString msg;
	bool endTurn = true;

	Player* currentPlayer = m_players.find(m_playerIDs[activePlayer])->second;
	currentPlayer->NewTurn();

	//Player is attacking
	if (action == (RakNet::uint24_t)0)
	{
		output.Write((RakNet::MessageID)ID_PLAYER_RETURN_MSG);
		std::string atkMsg;

		Player* targetPlayer = m_players.find(m_playerIDs[(int)target])->second;
		if (targetPlayer != nullptr)
		{
			if (targetPlayer->IsAlive())
			{
				atkMsg = "Player " + std::to_string(activePlayer + 1) + ": " + currentPlayer->GetName() + " attacked Player " + std::to_string((int)target + 1) + ": " + targetPlayer->GetName() + ". ";
				atkMsg += "Caused " + std::to_string(currentPlayer->Attack(targetPlayer)) + " damage.\n";

				if (!targetPlayer->IsAlive())
				{
					targetPlayer->Death();
					atkMsg += "Player " + std::to_string((int)target + 1) + ": " + targetPlayer->GetName() + " is dead.\n";
				}
			}
			else
			{
				atkMsg = "Player " + std::to_string(activePlayer + 1) + ": " + currentPlayer->GetName() + " attacked nothing.\n";
			}
		}

		msg.AppendBytes(atkMsg.c_str(), atkMsg.length());
	}
	//Player is blocking 
	else if (action == (RakNet::uint24_t)1)
	{
		output.Write((RakNet::MessageID)ID_PLAYER_RETURN_MSG);
		std::string blkMsg;

		currentPlayer->Block();

		blkMsg = "Player " + std::to_string(activePlayer + 1) + ": " + currentPlayer->GetName() + " is blocking.\n";
		msg.AppendBytes(blkMsg.c_str(), blkMsg.length());
	}
	//Player is looking at stats
	else
	{
		output.Write((RakNet::MessageID)ID_PLAYER_START_TURN);
		RakNet::uint24_t isActive = 1;
		output.Write(isActive);
		endTurn = false;
		std::string stats = GetPlayerStats();
		msg.AppendBytes(stats.c_str(), stats.length());
	}
	output.Write(msg);

	if (endTurn)
	{
		g_rakPeerInterface->Send(&output, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
		NextPlayerTurn();
	}
	else
	{
		g_rakPeerInterface->Send(&output, HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->systemAddress, false);
	}
}

void NextPlayerTurn()
{
	bool searchActivePlayer = true;
	int nextPlayer = activePlayer;
	while (searchActivePlayer)
	{
		nextPlayer = nextPlayer + 1;
		if (nextPlayer >= m_playerIDs.size())
		{
			nextPlayer = 0;
		}
		if (nextPlayer == activePlayer)
		{
			searchActivePlayer = false;
			g_networkState_mutex.lock();
			g_networkState = NS_GameOver;
			g_networkState_mutex.unlock();
			std::cout << "Gameover." << std::endl;
		}
		if (m_players.find(m_playerIDs[nextPlayer])->second->IsAlive())
		{
			searchActivePlayer = false;
			activePlayer = nextPlayer;
			std::cout << "Player " + std::to_string(activePlayer + 1) + "'s turn." << std::endl;
		}
	}

	if (g_networkState == NS_Game)
	{
		for (int i = 0; i < m_playerIDs.size(); i++)
		{
			RakNet::BitStream output;
			output.Write((RakNet::MessageID)ID_PLAYER_START_TURN);
			RakNet::uint24_t isActive;
			RakNet::RakString msg;
			std::string str;

			if (i == activePlayer)
			{
				isActive = 1;
				str = "Your turn.";
			}
			else
			{
				isActive = 0;
				str = "Waiting for Player " + std::to_string(activePlayer + 1) + "'s turn.";
			}
			msg.AppendBytes(str.c_str(), str.length());
			output.Write(isActive);
			output.Write(msg);

			g_rakPeerInterface->Send(&output, HIGH_PRIORITY, RELIABLE_ORDERED, 0, m_playerIDs[i], false);
		}
	}
}

void StartingTurn(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::uint24_t isActive;
	bs.Read(isActive);
	RakNet::RakString msg;
	bs.Read(msg);

	if (isActive == (RakNet::uint24_t)1)
	{
		isPlayerTurn = true;
	}
	else
	{
		isPlayerTurn = false;
	}

	std::cout << msg.C_String() << std::endl;
}

void EndGame(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString msg;
	bs.Read(msg);

	std::cout << msg.C_String() << std::endl;

	g_networkState_mutex.lock();
	g_networkState = NS_GameOver;
	g_networkState_mutex.unlock();
}
