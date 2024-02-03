#pragma once
#include "Particle.h"

class ParticleGas : public Particle
{
public:
	ParticleGas(int aiID, unsigned int aiX, unsigned int aiY, sf::Color acColor)
	{
		iParticleID = aiID;
		x = aiX;
		y = aiY;
		cColor = acColor;
	}

	void HandleMovement() override;
	void HandleFireProperties() override;
	bool QHasLifetimeExpired() override;

private:
	int iLifeTime = 100;
};

