#include "ParticleLiquid.h"
#include "ParticleSimulation.h"

/// <summary>
/// Handles the movement logic for the particle
/// </summary>
void ParticleLiquid::HandleMovement()
{
	// First, attempt to move downwards
	int itargetX = x;
	int itargetY = y + pProperties.iVelocityY;

	ParticleSimulation::QInstance().LineTest(QID(), x, y, itargetX, itargetY, itargetX , itargetY);
	if (!ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
	{
		// If that failed, attempt to move horizontally one way
		itargetY = y;
		itargetX += pProperties.iVelocityX;
		ParticleSimulation::QInstance().LineTest(QID(), x, y, itargetX, itargetY, itargetX, itargetY);
		if (!ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
		{
			// If that fails, then try the other way
			itargetX = x - pProperties.iVelocityX;
			ParticleSimulation::QInstance().LineTest(QID(), x, y, itargetX, itargetY, itargetX, itargetY);
			if (!ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
			{
				// If all that fails, just stop
				++pProperties.iFailedMoveAttempts;
				if (pProperties.iFailedMoveAttempts >= pProperties.iAttemptsBeforeRest)
				{
					bResting = true;
				}
				return;
			}
		}
	}
	// Assuming we didn't return out after trying each option, assign our new internal position values to our targets
	x = itargetX;
	y = itargetY;
	pProperties.iFailedMoveAttempts = 0;
}
#include <iostream>
/// <summary>
/// Handles fire propogation logic for this particle
/// </summary>
void ParticleLiquid::HandleFireProperties()
{
	// Extinguishes neighbors
	if (pProperties.bShouldExinguish && ParticleSimulation::QInstance().ExtinguishNeighboringParticles(x, y))
	{
		pProperties.uiDeathParticleType = static_cast<uint8_t>(PARTICLE_TYPE::STEAM);
		bExpired = true;
	}

	// Cooling/freezing behavior
	if (pProperties.iCoolingRate > 0)
	{
		if (iTicksSinceCool > pProperties.iCoolingRate)
		{
			iTicksSinceCool = 0;
			temperature--;
			if (temperature <= pProperties.iFreezingTemperature)
			{
				pProperties.uiDeathParticleType = pProperties.uiFrozenParticleType;
				bExpired = true;
			}
		}
		++iTicksSinceCool;
	}
}

/// <summary>
/// Checks if this particle is expired, and should be removed up at the end of this tick
/// </summary>
bool ParticleLiquid::QHasLifetimeExpired()
{
	return bExpired;
}

/// <summary>
/// Returns the type of particle to spawn when this particle is removed
/// </summary>
uint8_t ParticleLiquid::QDeathParticleType()
{
	return pProperties.uiDeathParticleType;
}
