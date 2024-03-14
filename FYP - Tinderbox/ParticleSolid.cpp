#include "ParticleSolid.h"
#include <iostream>

/// <summary>
/// Handles the movement logic for this particle
/// </summary>
void ParticleSolid::HandleMovement()
{
	if (!QIsOnFire())
	{
		bResting = true;
	}
}

/// <summary>
/// Handles the fire propogation logic for this particle
/// </summary>
void ParticleSolid::HandleFireProperties()
{
	// Melting
	if (!QIsOnFire() && pProperties.iMeltingPoint > 0 && temperature >= pProperties.iMeltingPoint && temperature < pProperties.iIgnitionTemperature)
	{
		bExpired = true;
	}

	// Ignition
	if (temperature >= pProperties.iIgnitionTemperature)
	{
		Ignite();
	}

	// Burning
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
void ParticleSolid::Ignite()
{
	if (!QIsOnFire())
	{
		temperature = FIRE_TEMP;
		eFireState = PARTICLE_FIRE_STATE::BURNING;
		bResting = false;
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
	return pProperties.iIgnitionTemperature;
}

/// <summary>
/// Returns the current fuel level of this particle
/// </summary>
int ParticleSolid::QFuel()
{
	return pProperties.iFuel;
}

uint8_t ParticleSolid::QDeathParticleType()
{
	return bMelted || QFuel() <= 0 ? pProperties.uiMeltedParticleType : 0;
}
