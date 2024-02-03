#pragma once
#include "Particle.h"

class ParticlePowder : public Particle
{
public:
	ParticlePowder(int aiID, unsigned int aiX, unsigned int aiY, sf::Color acColor)
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
	int iAttemptsBeforeRest = 1000;
	int iFailedMoveAttempts = 0;
};

