#pragma once
#include "Particle.h"

class ParticleLiquid : public Particle
{
public:
	ParticleLiquid(int aiID, unsigned int aiX, unsigned int aiY, sf::Color acColor)
	{
		iParticleID = aiID;
		x = aiX;
		y = aiY;
		cColor = acColor;
	}

	void HandleMovement() override;
	void HandleFireProperties() override;
	bool QHasLifetimeExpired() override;
	uint8_t QDeathParticleType() override;

private:
	int iAttemptsBeforeRest = 1000;
	int iFailedMoveAttempts = 0;
	uint8_t uiDeathParticleType = 0;
};

