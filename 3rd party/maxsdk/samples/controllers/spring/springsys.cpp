/**********************************************************************
 *<
	FILE: SpringSys.cpp

	DESCRIPTION:	A procedural Mass/Spring Position controller 
					Spring System Implementation

	CREATED BY:		Adam Felt

	HISTORY: 

 *>	Copyright (c) 1999-2000, All Rights Reserved.
 **********************************************************************/

#include <SpringSys.h>
#include <units.h>
#include <maxapi.h>
#include "springs.h"

bool SpringSysGlobalSettings::s_alwaysAccurate = true;


/***************************************************************************

  Public Member Functions

***************************************************************************/

SpringSys& SpringSys::operator=(const SpringSys& from)
{
	int i;
	
	referenceTime = from.referenceTime;
	lastTime = from.lastTime;
	isValid = from.isValid;
	
	frameCache = from.frameCache;
	
	parts.removeAll();
	initPosTab.ZeroCount();
	initPosTab.Shrink();
	pos_cache.ZeroCount();
	pos_cache.Shrink();
	vel_cache.ZeroCount();
	vel_cache.Shrink();
	
	SSParticle part;
	Point3 pTemp;

	for (i=0;i<from.parts.length();i++){
		part = from.parts[i]; parts.append(part); }
	for (i=0;i<from.initPosTab.Count();i++){
		pTemp = from.initPosTab[i]; initPosTab.Append(1, &pTemp);}
	for (i=0;i<from.pos_cache.Count();i++){
		pTemp = from.pos_cache[i]; pos_cache.Append(1, &pTemp);}
	for (i=0;i<from.vel_cache.Count();i++){
		pTemp = from.vel_cache[i]; vel_cache.Append(1, &pTemp);}

	return *this;
}

SpringSys SpringSys::Copy(const SpringSys* from)
{
	int i;
	
	referenceTime = from->referenceTime;
	lastTime = from->lastTime;
	isValid = from->isValid;
	
	frameCache = from->frameCache;
	
	parts.removeAll();
	initPosTab.ZeroCount(); initPosTab.Shrink();
	pos_cache.ZeroCount(); pos_cache.Shrink();
	vel_cache.ZeroCount(); vel_cache.Shrink();
	
	SSParticle* part = new SSParticle();
	Point3 pTemp;

	for (i=0;i<from->parts.length();i++){
		SSParticle foo = from->parts[i];
		part->Copy(foo);
		parts.append(*part); 
	}
	for (i=0;i<from->initPosTab.Count();i++){
		pTemp = from->initPosTab[i]; 
		initPosTab.Append(1, &pTemp);
	}
	for (i=0;i<from->pos_cache.Count();i++){
		pTemp = from->pos_cache[i]; 
		pos_cache.Append(1, &pTemp);
	}
	for (i=0;i<from->vel_cache.Count();i++){
		pTemp = from->vel_cache[i];
		vel_cache.Append(1, &pTemp);
	}

	return *this;
}

void SpringSys::SetParticleCount(int count)
{
	initPosTab.SetCount(count);

	//if it is getting smaller or staying the same size
	int ct = count - (int)parts.length();
	if (ct < 0)
	{
		parts.setLengthUsed(count);
		return;
	}
	//otherwise add as many as you need
	for (int i=0;i<ct;i++)
	{
		SSParticle part = SSParticle();
		parts.append(part);
	}
}

void SpringSys::SetInitialPosition (Point3 p, int index)
{
	if (index < 0 || index > parts.length() ) return;

	initPosTab[index] = p;
	GetParticle(index)->SetPos(p);
	return;
}
	

void SpringSys::SetInitialVelocity (Point3 p, int index)
{
	if (index < 0 || index > parts.length() ) return;

	GetParticle(index)->SetVel(p);
	return;
}

void SpringSys::SetInitialBoneStates(Tab<Matrix3> boneTMs)
{
	for (int x = 0; x< GetParticles()->length(); x++)
	{
		SSParticle *p = GetParticle(x);
		for (int tmId = 0; tmId< boneTMs.Count(); tmId++)
		{
			for (int i=0;i<p->GetSprings()->length();i++)
			{
				if (tmId == p->GetSpring(i)->GetPointConstraint()->GetIndex())
				{
					if (tmId == 0) 
						p->GetSpring(i)->SetLength(Point3::Origin);
					else p->GetSpring(i)->SetLength( initPosTab[x] * Inverse(boneTMs[tmId]) );
					
					p->GetSpring(i)->GetPointConstraint()->SetPos(initPosTab[x]);
					p->GetSpring(i)->GetPointConstraint()->SetVel(Point3::Origin);
				}
			}
			if (frameCache.bone.length() <= tmId) 
			{
				SSConstraintPoint contBone = SSConstraintPoint(tmId);
				frameCache.bone.append(contBone);
			}
		}
		//frameCache.bone.SetCount(tmId+1);
	}
}

void SpringSys::Invalidate ()
{
	isValid = false;
}


void SpringSys::Solve (int t, float DeltaT)
{
	if (!client) return;

	if (t >= referenceTime)
	{
		int tpf = GetTicksPerFrame();
		int fract =   (t-int(referenceTime)) % tpf;
		int frames =  (t-int(referenceTime)) / tpf;
		TimeValue startTime;
		int i = 0;
		bool rollingStart = false;
		
		if (t <= lastTime || !isValid)  // if something changes start from scratch
		{
			startTime = referenceTime;
			if (!SpringSysGlobalSettings::IsAlwaysAccurate())
			{
				// Use the rolling start, but don't go before referenceTime
				startTime = max(referenceTime, 
					t - (int)GetCOREInterface8()->GetSpringRollingStart()*tpf);

				// Make sure startTime is on a frame even if "t" isn't
				startTime = (startTime/tpf)*tpf;
				frames = (t - startTime)/tpf;
				rollingStart = true;
			}
		}
		else // if forward calculate from the last time calculated and use the caches
		{
			GetParticle(0)->SetPos(frameCache.pos);
			GetParticle(0)->SetVel(frameCache.vel);
			for (int b=0; b < GetParticle(0)->GetSprings()->length(); b++)
				GetParticle(0)->GetSpring(b)->SetPointConstraint(frameCache.bone[b]);
			startTime = lastTime;
			if (isValid)
				startTime += DeltaT;
			else
			{
				pos_cache.ZeroCount();
				vel_cache.ZeroCount();
				GetParticle(0)->SetVel(Point3::Origin);
				GetParticle(0)->SetPos(initPosTab[0]);
			}
		}

		// If the parents position hasn't changed and neither have your refs, 
		// use the cache and reset the start frame
		if (pos_cache.Count() > frames && isValid)
		{
			GetParticle(0)->SetPos(pos_cache[frames]); 
			GetParticle(0)->SetVel(vel_cache[frames]);
			
			startTime = referenceTime; 
			lastTime = max (lastTime, startTime + (frames*tpf)); 
			
			for (i = lastTime + DeltaT; i <= t; i += DeltaT)
			{
				UpdateParticleState(i, client->GetForceMatrices(i), 0, DeltaT);
			}
			// This is how much is left over
			fract = t - (i - DeltaT);   
		}

		// If cache is no good recompute from the last good frame
		else {
			if (startTime > referenceTime)
			{
				int nbCachedFrames = rollingStart? 0: (startTime - referenceTime)/tpf + 1;
				pos_cache.SetCount(nbCachedFrames);
				vel_cache.SetCount(nbCachedFrames);
				if (rollingStart)
				{
					GetParticle(0)->SetVel(Point3::Origin);
					GetParticle(0)->SetPos(initPosTab[0]);
					
					for (int b = 0; b<GetParticle(0)->GetSprings()->length(); b++)
						GetParticle(0)->GetSpring(b)->GetPointConstraint()->SetVel(Point3::Origin);
				}
			}

			for (i = startTime; i <= t; i += DeltaT)
			{
				// If you're starting from scratch make sure you are really starting from scratch
				if (i == referenceTime) 
				{
					pos_cache.ZeroCount();
					vel_cache.ZeroCount();
					GetParticle(0)->SetVel(Point3::Origin);
					GetParticle(0)->SetPos(initPosTab[0]);
					
					for (int b = 0; b<GetParticle(0)->GetSprings()->length(); b++)
						GetParticle(0)->GetSpring(b)->GetPointConstraint()->SetVel(Point3::Origin);
				}
				// Update the Particle state for this time step
				UpdateParticleState(i, client->GetForceMatrices(i), 0, DeltaT);

				if (i%tpf == 0)  // if you're sampling a frame cache it.
				{
					SSParticle* particleZero = GetParticle(0);
					if (particleZero)
					{
						Point3 pos = particleZero->GetPos();
						pos_cache.Append(1, &pos, pos_cache.Count()*0.5);
						Point3 velocity = particleZero->GetVel();
						vel_cache.Append(1, &velocity, vel_cache.Count()*0.5);

						// This is done repetitively here,
						// the other whole frame caching is deferred until the end
						frameCache.bone.setLengthUsed(particleZero->GetSprings()->length());
						for (int b=0; b < particleZero->GetSprings()->length(); b++) 
							frameCache.bone[b] = *(particleZero->GetSpring(b)->GetPointConstraint());
					}

				}
			}
			lastTime = (t/tpf)*tpf;
			fract = t - (i - DeltaT);   // This is how much time is left over
		}
		// Copy that last whole frame to cache
		frameCache.pos = pos_cache[frames];
		frameCache.vel = vel_cache[frames];

		// If you're in quickEdit mode you'll be invalidated later
		isValid = TRUE;
		
		// Compute remainder in case of motion blur, fields, or realtime playback
		if (fract > 0) 
			UpdateParticleState(t, client->GetForceMatrices(t), 0, fract);
	}
}

void SpringSys::GetPosition (Point3& p, int index)
{
	if (index < 0 || index > parts.length() ) return;
	p = GetParticle(index)->GetPos();
}

/***************************************************************************

  Private Member Functions

***************************************************************************/


//The do it all function 
/*  Parameters
	TimeValue t -- The time to sample the particle 
	Tab<Matrix3> tmArray -- The force matrices from bones or self influence
	int pIndex	-- The particle number
	TimeValue Delta	-- The time step amount in which to scale the derivative by
********************************************************************************/

void SpringSys::UpdateParticleState(TimeValue t, Tab<Matrix3> tmArray, int index, TimeValue Delta)
{
	Point3 t_pos, t_vel, orig_pos, orig_vel;
	Matrix3 tm;
	for (int b=0;b<parts[index].GetSprings()->length();b++)
	{
		if (parts[index].GetSpring(b)->GetPointConstraint()->GetIndex() < tmArray.Count())
		{
			tm = tmArray[parts[index].GetSpring(b)->GetPointConstraint()->index];
			ComputeControlledParticleForce(tm, index, b);  /* compute bone deriv used by ApplySpring */
		}
	}

	Clear_Forces(index);							/* zero the force accumulators */
	Compute_Forces(t, index);						/* magic force function */
	ComputeDerivative(index, t_pos, t_vel);		/* get deriv */
	//ComputeDerivative(pIndex, t_pos, t_vel, Delta);		/* get deriv */
	ScaleVectors(t_pos, t_vel, Delta);				/* scale it */
	GetParticleState(index, orig_pos, orig_vel);	/* get state */
	AddVectors(orig_pos, orig_vel, t_pos, t_vel);	/* add -> temp2 */
	SetParticleState(index, t_pos, t_vel);			/* update state */
}

/* gather state from the particles and return them */
void SpringSys::GetParticleState(int index, Point3 &pos, Point3 &vel)
{
	pos = parts[index].GetPos();
	vel = parts[index].GetVel();
}

/* scatter state into the particles */
void SpringSys::SetParticleState(int index, Point3 pos, Point3 vel)
{
	parts[index].SetPos(pos);
	parts[index].SetVel(vel);
}

void SpringSys::ScaleVectors(Point3 &pos, Point3 &vel, float delta)
{
	pos = delta*pos;
	vel = delta*vel;
}

void SpringSys::AddVectors(Point3 pos1, Point3 vel1, Point3 &pos, Point3 &vel)
{
	pos += pos1;	
	vel += vel1;
}
void SpringSys::ComputeControlledParticleForce(Matrix3 tm, int vertIndex, int springIndex)
{
	float mass = 99999.9f;  //Bone has nearly infinate mass
	Point3 tempPos;

	//Calculate the new point using the position and rotation of the TM
	//tempPos = tm * parts[vertIndex].GetSpring(springIndex)->GetLength(); 
	tempPos = tm * parts[vertIndex].GetSpring(springIndex)->GetLength(); 

	//force vector determining how far the bone has moved since it was last calculated
	Point3 force = (parts[vertIndex].GetSpring(springIndex)->GetPointConstraint()->pos - tempPos);

	//Update the position and velocity of the bone
	//Note: This could be done globally not per vertex
	parts[vertIndex].GetSpring(springIndex)->GetPointConstraint()->pos = tempPos;		
	parts[vertIndex].GetSpring(springIndex)->GetPointConstraint()->vel = force/mass; /* velocity derivative = force/mass */
}

void SpringSys::ComputeDerivative(int index, Point3 &pos, Point3 &vel)
{
	float mass = parts[index].GetMass();
	if (mass == 0) mass = 0.001f;  //don't divide by zero
	pos = (parts[index].vel + (parts[index].force/mass))/2;		/* midpoint method (More accurate than Euler)*/
	vel = parts[index].force/mass;										/* vdot = force/mass */
}

void SpringSys::Clear_Forces(int index)
{
	parts[index].SetForce(Point3::Origin);
}

void SpringSys::Compute_Forces(TimeValue t, int index)
{
	ApplyDrag(index);
	ApplyUnaryForces(t, index);
	ApplySpring(t, index);
}

void SpringSys::ApplyDrag(int index)
{
	parts[index].force -= ( parts[index].GetDrag() * parts[index].GetVel() );
}

void SpringSys::ApplyUnaryForces(TimeValue t, int index)
{

	parts[index].force += (16000.0f * client->GetDynamicsForces(t, parts[index].GetPos(), parts[index].GetVel()));
}


void SpringSys::ApplySpring(TimeValue , int index)
{
	float spring, dampening, mass; 
	Point3 p_last, SpringForce, RestLength;
	
	SSParticle p = parts[index];
	mass = p.mass;

	for (int i=0;i< p.springs.length();i++)
	{
		spring = p.GetSpring(i)->GetTension();
		dampening = p.GetSpring(i)->GetDampening() * 60 * spring;

		spring = spring/100.0f;
		dampening = dampening/100.0f;
	
		//p_last = p.pos;
		SpringForce = Point3(0,0,0);
		RestLength = p.GetSpring(i)->GetLength();
		
		//determine the stretch vector
		Point3 stretch = (p.pos - (p.GetSpring(i)->GetPointConstraint()->pos));				//stretch delta
		Point3 accel =   p.GetSpring(i)->GetPointConstraint()->vel - p.vel;	 				 //velocity delta
		
		//compute the springback
		if (stretch.x != 0) 
			SpringForce.x = ( (spring*(fabs(stretch.x) )) + (-dampening*((accel.x*stretch.x)/fabs(stretch.x))) )*(stretch.x/fabs(stretch.x));
		if (stretch.y != 0) 
			SpringForce.y = ( (spring*(fabs(stretch.y) )) + (-dampening*((accel.y*stretch.y)/fabs(stretch.y))) )*(stretch.y/fabs(stretch.y));
		if (stretch.z != 0) 
			SpringForce.z = ( (spring*(fabs(stretch.z) )) + (-dampening*((accel.z*stretch.z)/fabs(stretch.z))) )*(stretch.z/fabs(stretch.z));
			
		parts[index].force -= SpringForce;		
	}
}


#define PARTSYS_CHUNK		0x1000

/*
//PreRelease files are being saved incorrectly.  The way they are saved makes them dependant on member alignment
IOResult SpringSys::Save(ISave *isave)
{
	IOResult	res;
	ULONG		nb;
	SSParticle	part;
	SSSpring	spring;
	
	//res		= isave->Write(&modData->partsys.lastTime, sizeof(float), &nb);

	int ct	= parts.length();
	isave->BeginChunk(PARTSYS_CHUNK);
	res	= isave->Write(&ct, sizeof(int), &nb);
	for(int i=0;i<ct;i++)
	{
		part = parts[i];
		int partsiz = sizeof(SSParticle)+sizeof(Tab<SSSpring>);
		int springsiz = (sizeof(SSSpring));
		int estsize = sizeof(float);
		int estsize1 = sizeof(int);
		isave->Write(&part, sizeof(SSParticle), &nb); //sizeof(SSParticle)+sizeof(Tab<SSSpring>)
		
		int springCt = parts[i].springs.Count();
		isave->Write(&springCt, sizeof(int), &nb);
		
		for (int x=0;x<springCt;x++)
		{
			//spring = parts[i].springs[x];
			isave->Write(&(parts[i].springs[x]), sizeof(SSSpring), &nb);
		}
	}
	isave->EndChunk();
	return IO_OK;
}
*/

#define SPRINGSYS_PART_COUNT_CHUNK			0x2200
#define SPRINGSYS_PART_MASS_CHUNK			0x2210
#define SPRINGSYS_PART_DRAG_CHUNK			0x2215
#define SPRINGSYS_PART_POS_CHUNK			0x2220
#define SPRINGSYS_PART_VEL_CHUNK			0x2225
#define SPRINGSYS_SPRING_COUNT_CHUNK		0x2230
#define SPRINGSYS_SPRING_TENSION_CHUNK		0x2240
#define SPRINGSYS_SPRING_DAMPENING_CHUNK	0x2245
#define SPRINGSYS_SPRING_LENGTH_CHUNK		0x2250
#define SPRINGSYS_CONSTRAINT_POS_CHUNK		0x2255
#define SPRINGSYS_CONSTRAINT_VEL_CHUNK		0x2260
#define SPRINGSYS_CONSTRAINT_INDEX_CHUNK	0x2265

IOResult SpringSys::Save(ISave *isave)
{
	IOResult	res;
	ULONG		nb;
	SSParticle	part;
	SSSpring	spring;
	int tempInt = 0;
	float tempFloat = 0.0f;
	Point3 tempP3 = Point3(0,0,0);
	
	//save the number of particles
	int ct	= (int)parts.length();
	isave->BeginChunk(SPRINGSYS_PART_COUNT_CHUNK);
	res	= isave->Write(&ct, sizeof(int), &nb);
	isave->EndChunk();

	//save each particle
	for(int i=0;i<ct;i++)
	{
		//save the particle data
		part = parts[i];
		isave->BeginChunk(SPRINGSYS_PART_MASS_CHUNK);
		isave->Write(&(part.mass), sizeof(float), &nb);
		isave->EndChunk();
		isave->BeginChunk(SPRINGSYS_PART_DRAG_CHUNK);
		isave->Write(&(part.drag), sizeof(float), &nb);
		isave->EndChunk();
		isave->BeginChunk(SPRINGSYS_PART_POS_CHUNK);
		tempP3 = part.GetPos();
		isave->Write(&tempP3, sizeof(Point3), &nb);
		isave->EndChunk();
		isave->BeginChunk(SPRINGSYS_PART_VEL_CHUNK);
		tempP3 = part.GetVel();
		isave->Write(&tempP3, sizeof(Point3), &nb);
		isave->EndChunk();
		
		//save the spring count
		int springCt = (int)parts[i].springs.length();
		isave->BeginChunk(SPRINGSYS_SPRING_COUNT_CHUNK);
		isave->Write(&springCt, sizeof(int), &nb);
		isave->EndChunk();
		
		//save the springs
		for (int x=0;x<springCt;x++)
		{
			//save the spring data
			spring = parts[i].springs[x];
			tempFloat = spring.GetTension();
			isave->BeginChunk(SPRINGSYS_SPRING_TENSION_CHUNK);
			isave->Write(&tempFloat, sizeof(float), &nb);
			isave->EndChunk();
			tempFloat = spring.GetDampening();
			isave->BeginChunk(SPRINGSYS_SPRING_DAMPENING_CHUNK);
			isave->Write(&tempFloat, sizeof(float), &nb);
			isave->EndChunk();
			tempP3 = spring.GetLength();
			isave->BeginChunk(SPRINGSYS_SPRING_LENGTH_CHUNK);
			isave->Write(&tempP3, sizeof(Point3), &nb);
			isave->EndChunk();

			//save the constraint point data
			tempP3 = spring.GetPointConstraint()->GetPos();
			isave->BeginChunk(SPRINGSYS_CONSTRAINT_POS_CHUNK);
			isave->Write(&tempP3, sizeof(Point3), &nb);
			isave->EndChunk();
			tempP3 = spring.GetPointConstraint()->GetVel();
			isave->BeginChunk(SPRINGSYS_CONSTRAINT_VEL_CHUNK);
			isave->Write(&tempP3, sizeof(Point3), &nb);
			isave->EndChunk();
			tempInt = spring.GetPointConstraint()->GetIndex();
			isave->BeginChunk(SPRINGSYS_CONSTRAINT_INDEX_CHUNK);
			isave->Write(&tempInt, sizeof(int), &nb);
			isave->EndChunk();
		}
	}
	return IO_OK;
}

class TempSpring
{
public:
	float tension;     
	float dampening;
	Point3 length;
	SSConstraintPoint bone;       
};


IOResult SpringSys::Load(ILoad *iload)
{
	IOResult	res;
	ULONG		nb;
	lastTime = 0.0f;
	int partCt;
	int springCt;
	SSParticle part;
	SSSpring spring;
	TempSpring temp_spring;

	int partNum = -1;
	int springNum = -1;
	int tempInt = 0;
	float tempFloat = 0.0f;
	Point3 tempP3 = Point3(0,0,0);
	MaxSDK::Array<SSSpring> newSprings;
	
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(iload->CurChunkID())  {
			case SPRINGSYS_PART_COUNT_CHUNK:
				parts.removeAll();
				iload->Read(&partCt, sizeof(int), &nb);
				for (partNum=0;partNum<partCt;partNum++)
				{
					SSParticle newPart = SSParticle();
					parts.append(newPart);
				}
				partNum = -1;
				break;
			case SPRINGSYS_PART_MASS_CHUNK:
				iload->Read(&tempFloat, sizeof(float), &nb);
				GetParticle(++partNum)->SetMass(tempFloat);
				break;
			case SPRINGSYS_PART_DRAG_CHUNK:
				iload->Read(&tempFloat, sizeof(float), &nb);
				GetParticle(partNum)->SetDrag(tempFloat);
				break;
			case SPRINGSYS_PART_POS_CHUNK:
				iload->Read(&tempP3, sizeof(Point3), &nb);
				GetParticle(partNum)->SetPos(tempP3);
				break;
			case SPRINGSYS_PART_VEL_CHUNK:
				iload->Read(&tempP3, sizeof(Point3), &nb);
				GetParticle(partNum)->SetVel(tempP3);
				break;
			case SPRINGSYS_SPRING_COUNT_CHUNK:
				iload->Read(&springCt, sizeof(int), &nb);
				newSprings.removeAll();
				for(springNum=0;springNum<springCt;springNum++)
				{
					SSSpring newSpring = SSSpring();
					newSprings.append(newSpring);
				}
				GetParticle(partNum)->SetSprings(newSprings);
				springNum = -1;
				break;
			case SPRINGSYS_SPRING_TENSION_CHUNK:
				iload->Read(&tempFloat, sizeof(float), &nb);
				GetParticle(partNum)->GetSpring(++springNum)->SetTension(tempFloat);
				break;
			case SPRINGSYS_SPRING_DAMPENING_CHUNK:
				iload->Read(&tempFloat, sizeof(float), &nb);
				GetParticle(partNum)->GetSpring(springNum)->SetDampening(tempFloat);
				break;
			case SPRINGSYS_SPRING_LENGTH_CHUNK:
				iload->Read(&tempP3, sizeof(Point3), &nb);
				GetParticle(partNum)->GetSpring(springNum)->SetLength(tempP3);
				break;
			case SPRINGSYS_CONSTRAINT_POS_CHUNK:
				iload->Read(&tempP3, sizeof(Point3), &nb);
				GetParticle(partNum)->GetSpring(springNum)->GetPointConstraint()->SetPos(tempP3);
				break;
			case SPRINGSYS_CONSTRAINT_VEL_CHUNK:
				iload->Read(&tempP3, sizeof(Point3), &nb);
				GetParticle(partNum)->GetSpring(springNum)->GetPointConstraint()->SetVel(tempP3);
				break;
			case SPRINGSYS_CONSTRAINT_INDEX_CHUNK:
				iload->Read(&tempInt, sizeof(int), &nb);
				GetParticle(partNum)->GetSpring(springNum)->GetPointConstraint()->SetIndex(tempInt);
				break;

			//the PARTSYS_CHUNK comes from files saved prior R4 FRC
			//There is a problem loading them due to member alignment
			//This chunk is legacy
			case PARTSYS_CHUNK:
				//iload->Read(&lastTime, sizeof(float), &nb);
				//lastTime = lastTime;
				iload->Read(&partCt, sizeof(int), &nb);
				parts.removeAll();
				for (int i=0;i<partCt;i++)
				{
					part = SSParticle();
					iload->ReadVoid(&part, sizeof(SSParticle), &nb); // no TCHAR data
					part.springs.removeAll();
					parts.append(part);

					iload->Read(&springCt, sizeof(int), &nb);
					parts[i].pos = Point3(0,0,0);
					
					MaxSDK::Array<SSSpring> springTab;
					//parts[i].springs = NULL;
					for (int x=0;x<springCt;x++)
					{
						DWORD ver = iload->GetFileSaveVersion();
						if ( (HIWORD(ver)) <= 4000 && (LOWORD(ver)) < 46) 
						{
							iload->ReadVoid(&temp_spring, sizeof(TempSpring), &nb); // no TCHAR data
							spring.SetTension(temp_spring.tension);
							spring.SetDampening(temp_spring.dampening);
							spring.SetLength(temp_spring.length);
							spring.SetPointConstraint(temp_spring.bone);
						}
						else 
							iload->ReadVoid(&spring, sizeof(SSSpring), &nb); // no TCHAR data

						springTab.append(spring);
					}
					parts[i].SetSprings(springTab);
				}
				break;

		}
		iload->CloseChunk();
		if (res!=IO_OK) { return res; }
	}
	return IO_OK;
}

// de-inlined from springsys.h:
BOOL SSParticle::AddSpring(SSConstraintPoint *bone, Point3 length, float tension, float dampening)
{
	for (int i=0;i<springs.length();i++)
	{
		//if it already exists in the list make it stronger
		if ( springs[i].GetPointConstraint()->GetIndex() == bone->GetIndex() )
		{ springs[i].SetTension( springs[i].GetTension()*2 ); return false; }
	}
	//if it doesn't exist, add a new one
	SSSpring spring(bone, length, tension, dampening);
	springs.append(spring);
	return true;
}

