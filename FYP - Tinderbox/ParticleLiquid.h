#pragma once
#include "Particle.h"

struct LiquidProperties
{
	LiquidProperties() = default;
	LiquidProperties(int aiAttemptsBeforeRest, uint8_t auiDeathParticleType, bool abShouldExinguish, bool abHeatSurroundings, int aiVelocityX, int aiVelocityY, sf::Color acColor, int aiFreezingTemperature, uint8_t auiFrozenParticleType, int aiCoolingRate)
	{
		iAttemptsBeforeRest = aiAttemptsBeforeRest;
		iFailedMoveAttempts = 0;
		uiDeathParticleType = auiDeathParticleType;
		bShouldExinguish = abShouldExinguish;
		bHeatSurroundings = abHeatSurroundings;
		iVelocityX = aiVelocityX;
		iVelocityY = aiVelocityY;
		cColor = acColor;
		iFreezingTemperature = aiFreezingTemperature;
		uiFrozenParticleType = auiFrozenParticleType;
		iCoolingRate = aiCoolingRate;
	}
	int iAttemptsBeforeRest = 30;
	int iFailedMoveAttempts = 0;
	uint8_t uiDeathParticleType = 0;
	bool bShouldExinguish;
	bool bHeatSurroundings;
	int iVelocityX = 2;
	int iVelocityY = 4;
	sf::Color cColor;
	int iFreezingTemperature = -5;
	uint8_t uiFrozenParticleType = 0;
	int iCoolingRate = 0;
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
		eFireState = apProperties.bHeatSurroundings ? PARTICLE_FIRE_STATE::BURNING : PARTICLE_FIRE_STATE::NONE;
		temperature = apProperties.bHeatSurroundings ? FIRE_TEMP : 0;
	}

	void HandleMovement() override;
	void HandleFireProperties() override;
	bool QHasLifetimeExpired() override;
	uint8_t QDeathParticleType() override;

private:
	LiquidProperties pProperties;
	int iTicksSinceCool = 0;
};

