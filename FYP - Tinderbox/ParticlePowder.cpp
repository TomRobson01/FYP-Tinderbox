#include "ParticlePowder.h"
#include "ParticleSimulation.h"

/// <summary>
/// Handles the movement logic for this particle
/// </summary>
void ParticlePowder::HandleMovement()
{
	// First, attempt to move downwards
	int itargetX = x;
	int itargetY = y + 1;	// TO-DO: Replace 1 with a "velocity" value

	if (!ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
	{
		// If that failed, attempt to move diagonally one way
		itargetX += 1;
		if (!ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
		{
			// If that fails, then try the other way
			itargetX = x - 1;
			if (!ParticleSimulation::QInstance().RequestParticleMove(iParticleID, itargetX, itargetY))
			{
				// If all that fails, just stop
				++iFailedMoveAttempts;
				if (iFailedMoveAttempts >= iAttemptsBeforeRest)
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
	iFailedMoveAttempts = 0;
}

/// <summary>
/// Handles the fire propogation logic for this particle+-
/// </summary>
void ParticlePowder::HandleFireProperties()
{
	if (temperature >= iIgnitionTemperature)
	{
		Ignite();
	}
	if (QIsOnFire())
	{
		temperature = iIgnitionTemperature;

		iFuel -= iBurningFuelConsumption;
		if (iFuel <= 0)
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
		temperature = iIgnitionTemperature;
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
	return iIgnitionTemperature;
}

/// <summary>
/// Returns the current fuel level of this particle
/// </summary>
int ParticlePowder::QFuel()
{
	return iFuel;
}
