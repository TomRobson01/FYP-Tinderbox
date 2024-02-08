#include "ParticleSolid.h"

void ParticleSolid::HandleMovement()
{
	bResting = true;
}

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

void ParticleSolid::Ignite()
{
	if (!QIsOnFire())
	{
		temperature = iIgnitionTemperature;
		eFireState = PARTICLE_FIRE_STATE::BURNING;
	}
}

bool ParticleSolid::QHasLifetimeExpired()
{
	return bExpired;
}

int ParticleSolid::QIgnitionTemperature()
{
	return iIgnitionTemperature;
}

int ParticleSolid::QFuel()
{
	return iFuel;
}
