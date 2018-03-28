#include "player.h"

Player::Player() {
	isBlocking = false;
}
int Player::Attack(Player* target) {
	int totalDamage;
	if (target->isBlocking) {
		totalDamage = (attack - target->defense)/2;
	}
	else
		totalDamage = attack - target->defense;
	
	target->health -= totalDamage;
	return totalDamage;
}

void Player::Block() {
	isBlocking = true;
}

void Player::SetName(std::string input) {
	name = input;
}
void Player::Death() {
	health = 0;
}
void Player::NewTurn() {
	isBlocking = false;
}
int Player::GetHealth()const {
	return health;
}

int Player::GetAttack()const {
	return attack;
}

int Player::GetDefense()const {
	return defense;
}

std::string Player::GetName() const {
	return name;
}

std::string Player::GetClass()const {
	return className;
}

bool Player::IsAlive() {
	return health > 0;
}