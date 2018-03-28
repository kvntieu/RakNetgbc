#pragma once
#include<string>
class Player {
public:
	Player();
	int Attack(Player* target);
	void Block();
	void SetName(std::string input);
	void Death();
	void NewTurn();
	int GetHealth() const;
	int GetDefense()const;
	int GetAttack()const;
	std::string GetName()const;
	std::string GetClass()const;
	bool IsAlive();
	
protected:
	std::string name;
	std::string className;
	int health;
	int attack;
	int defense;
	bool isBlocking;
};