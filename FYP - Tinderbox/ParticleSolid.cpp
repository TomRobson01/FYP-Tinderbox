#include "ParticleSolid.h"

/// <summary>
/// Handles the movement logic for this particle
/// </summary>
void ParticleSolid::HandleMovement()
{
	bResting = true;
}

/// <summary>
/// Handles the fire propogation logic for this particle
/// </summary>
void ParticleSolid::HandleFireProperties()
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
void ParticleSolid::Ignite()
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
bool ParticleSolid::QHasLifetimeExpired()
{
	return bExpired;
}

/// <summary>
/// Returns the temperature at which this particle ignites
/// </summary>
int ParticleSolid::QIgnitionTemperature()
{
	return iIgnitionTemperature;
}

/// <summary>
/// Returns the current fuel level of this particle
/// </summary>
int ParticleSolid::QFuel()
{
	return iFuel;
}
