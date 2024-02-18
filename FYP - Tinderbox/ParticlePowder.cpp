#include "ParticlePowder.h"
#include "ParticleSimulation.h"

/// <summary>
/// Handles the movement logic for this particle
/// </summary>
void ParticlePowder::HandleMovement()
{
	// First, attempt to move downwards
	int itargetX = x;
	int itargetY = y + pProperties.iVelocityY;

	ParticleSimulation::QInstance().LineTest(QID(), x, y, itargetX, itargetY, itargetX, itargetY);
	if (!ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
	{
		// If that failed, attempt to move diagonally one way
		itargetY = y + 1;
		itargetX += 1;
		bool bCornerCheck = ParticleSimulation::QInstance().IsSpaceOccupied(itargetX, y);
		if (bCornerCheck || !ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
		{
			// If that fails, then try the other way
			itargetX = x - 1; bCornerCheck = ParticleSimulation::QInstance().IsSpaceOccupied(itargetX, y);
			if (bCornerCheck || !ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
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

/// <summary>
/// Handles the fire propogation logic for this particle+-
/// </summary>
void ParticlePowder::HandleFireProperties()
{
	if (temperature >= pProperties.iIgnitionTemperature)
	{
		Ignite();
	}
	if (QIsOnFire())
	{
		temperature = FIRE_TEMP;

		pProperties.iFuel -= pProperties.iBurningFuelConsumption;
		if (pProperties.iFuel <= 0)
		{
			bExpired = true;
		}
	}
}

/// <summary>
/// Enters this particle into the burning state
/// </summary>
void ParticlePowder::Ignite()
{
	if (!QIsOnFire())
	{
		temperature = QIgnitionTemperature();
		eFireState = PARTICLE_FIRE_STATE::BURNING;
	}
}

/// <summary>
/// Checks if this particle is expired, and should be removed up at the end of this tick
/// </summary>
bool ParticlePowder::QHasLifetimeExpired()
{
	return bExpired;
}

/// <summary>
/// Returns the temperature at which this particle ignites
/// </summary>
int ParticlePowder::QIgnitionTemperature()
{
	return pProperties.iIgnitionTemperature;
}

/// <summary>
/// Returns the current fuel level of this particle
/// </summary>
int ParticlePowder::QFuel()
{
	return pProperties.iFuel;
}
