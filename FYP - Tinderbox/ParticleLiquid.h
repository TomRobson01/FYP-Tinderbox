#pragma once
#include "Particle.h"

struct LiquidProperties
{
	LiquidProperties() = default;
	LiquidProperties(int aiAttemptsBeforeRest, uint8_t auiDeathParticleType, int aiVelocityX, int aiVelocityY, sf::Color acColor)
	{
		iAttemptsBeforeRest = aiAttemptsBeforeRest;
		iFailedMoveAttempts = 0;
		uiDeathParticleType = auiDeathParticleType;
		iVelocityX = aiVelocityX;
		iVelocityY = aiVelocityY;
		cColor = acColor;
	}
	int iAttemptsBeforeRest = 30;
	int iFailedMoveAttempts = 0;
	uint8_t uiDeathParticleType = 0;
	int iVelocityX = 2;
	int iVelocityY = 4;
	sf::Color cColor;
};

class ParticleLiquid : public Particle
{
public:
	ParticleLiquid(int aiID, unsigned int aiX, unsigned int aiY, uint8_t auiParticleType, LiquidProperties apProperties)
	{
		iParticleID = aiID;
		uiParticleType = auiParticleType;
		x = aiX;
		y = aiY;
		pProperties = apProperties;
		cColor = pProperties.cColor;
	}

	void HandleMovement() override;
	void HandleFireProperties() override;
	bool QHasLifetimeExpired() override;
	uint8_t QDeathParticleType() override;

private:
	LiquidProperties pProperties;
};

