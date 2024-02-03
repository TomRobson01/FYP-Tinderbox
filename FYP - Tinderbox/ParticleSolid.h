#pragma once
#include "Particle.h"

class ParticleSolid : public Particle
{
public:
	ParticleSolid(int aiID, unsigned int aiX, unsigned int aiY, sf::Color acColor)
	{
		iParticleID = aiID;
		x = aiX;
		y = aiY;
		cColor = acColor;

		bResting = true;
	}

	void HandleMovement() override;
	void HandleFireProperties() override;
	bool QHasLifetimeExpired() override;

private:

};
