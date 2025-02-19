﻿

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
#include "Effekseer.EffectNode.h"
#include "Effekseer.Effect.h"
#include "Effekseer.EffectImplemented.h"
#include "Effekseer.Manager.h"

#include "Effekseer.Vector3D.h"

#include "Effekseer.Instance.h"
#include "Effekseer.InstanceContainer.h"
#include "Effekseer.InstanceGlobal.h"

#include "Effekseer.EffectNodeRibbon.h"
#include "Effekseer.EffectNodeRing.h"
#include "Effekseer.EffectNodeRoot.h"
#include "Effekseer.EffectNodeSprite.h"
#include "Sound/Effekseer.SoundPlayer.h"

#include "Effekseer.Setting.h"

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
namespace Effekseer
{

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
EffectNodeImplemented::EffectNodeImplemented(Effect* effect, unsigned char*& pos)
	: m_effect(effect)
	, m_userData(NULL)
	, IsRendered(true)
	, TranslationFCurve(NULL)
	, RotationFCurve(NULL)
	, ScalingFCurve(NULL)
	, SoundType(ParameterSoundType_None)
	, RenderingOrder(RenderingOrder_FirstCreatedInstanceIsFirst)
{
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void EffectNodeImplemented::LoadParameter(unsigned char*& pos, EffectNode* parent, Setting* setting)
{
	int size = 0;
	int node_type = 0;
	auto ef = (EffectImplemented*)m_effect;

	memcpy(&node_type, pos, sizeof(int));
	pos += sizeof(int);

	if (node_type == -1)
	{
		TranslationType = ParameterTranslationType_None;
		LocationAbs.type = LocationAbsType::None;
		RotationType = ParameterRotationType_None;
		ScalingType = ParameterScalingType_None;
		CommonValues.MaxGeneration = 1;

		GenerationLocation.EffectsRotation = 0;
		GenerationLocation.type = ParameterGenerationLocation::TYPE_POINT;
		GenerationLocation.point.location.reset();

		RenderingPriority = -1;
	}
	else
	{
		if (m_effect->GetVersion() >= 10)
		{
			int32_t rendered = 0;
			memcpy(&rendered, pos, sizeof(int32_t));
			pos += sizeof(int32_t);

			IsRendered = rendered != 0;
		}

		// To render with priority, nodes are assigned a list.
		if (m_effect->GetVersion() >= 13)
		{
			memcpy(&RenderingPriority, pos, sizeof(int32_t));
			pos += sizeof(int32_t);
		}
		else
		{
			RenderingPriority = -1;
		}

		memcpy(&size, pos, sizeof(int));
		pos += sizeof(int);

		if (ef->GetVersion() >= 14)
		{
			assert(size == sizeof(ParameterCommonValues));
			memcpy(&CommonValues, pos, size);
			pos += size;
		}
		else if (m_effect->GetVersion() >= 9)
		{
			memcpy(&CommonValues.MaxGeneration, pos, size);
			pos += size;
		}
		else
		{
			assert(size == sizeof(ParameterCommonValues_8));
			ParameterCommonValues_8 param_8;
			memcpy(&param_8, pos, size);
			pos += size;

			CommonValues.MaxGeneration = param_8.MaxGeneration;
			CommonValues.TranslationBindType = param_8.TranslationBindType;
			CommonValues.RotationBindType = param_8.RotationBindType;
			CommonValues.ScalingBindType = param_8.ScalingBindType;
			CommonValues.RemoveWhenLifeIsExtinct = param_8.RemoveWhenLifeIsExtinct;
			CommonValues.RemoveWhenParentIsRemoved = param_8.RemoveWhenParentIsRemoved;
			CommonValues.RemoveWhenChildrenIsExtinct = param_8.RemoveWhenChildrenIsExtinct;
			CommonValues.life = param_8.life;
			CommonValues.GenerationTime.max = param_8.GenerationTime;
			CommonValues.GenerationTime.min = param_8.GenerationTime;
			CommonValues.GenerationTimeOffset.max = param_8.GenerationTimeOffset;
			CommonValues.GenerationTimeOffset.min = param_8.GenerationTimeOffset;
		}

		memcpy(&TranslationType, pos, sizeof(int));
		pos += sizeof(int);

		if (TranslationType == ParameterTranslationType_Fixed)
		{
			int32_t translationSize = 0;
			memcpy(&translationSize, pos, sizeof(int));
			pos += sizeof(int);

			if (ef->GetVersion() >= 14)
			{
				memcpy(&TranslationFixed, pos, sizeof(ParameterTranslationFixed));
			}
			else
			{
				memcpy(&(TranslationFixed.Position), pos, sizeof(float) * 3);

				// make invalid
				if (TranslationFixed.Position.X == 0.0f && TranslationFixed.Position.Y == 0.0f && TranslationFixed.Position.Z == 0.0f)
				{
					TranslationType = ParameterTranslationType_None;
					EffekseerPrintDebug("LocationType Change None\n");
				}
			}

			pos += translationSize;
		}
		else if (TranslationType == ParameterTranslationType_PVA)
		{
			if (ef->GetVersion() >= 14)
			{
				memcpy(&size, pos, sizeof(int));
				pos += sizeof(int);
				assert(size == sizeof(ParameterTranslationPVA));
				memcpy(&TranslationPVA, pos, size);
				pos += size;
			}
			else
			{
				memcpy(&size, pos, sizeof(int));
				pos += sizeof(int);
				memcpy(&TranslationPVA.location, pos, size);
				pos += size;
			}
		}
		else if (TranslationType == ParameterTranslationType_Easing)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);

			if (ef->GetVersion() >= 14)
			{
				assert(size == sizeof(ParameterTranslationEasing));
				memcpy(&TranslationEasing, pos, size);
				pos += size;
			}
			else
			{
				assert(size == sizeof(easing_vector3d));
				memcpy(&TranslationEasing.location, pos, size);
				pos += size;
			}
		}
		else if (TranslationType == ParameterTranslationType_FCurve)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);

			TranslationFCurve = new FCurveVector3D();
			pos += TranslationFCurve->Load(pos, m_effect->GetVersion());
		}

		/* 位置拡大処理 */
		if (ef->IsDyanamicMagnificationValid())
		{
			DynamicFactor.Tra[0] *= m_effect->GetMaginification();
			DynamicFactor.Tra[1] *= m_effect->GetMaginification();
			DynamicFactor.Tra[2] *= m_effect->GetMaginification();

			if (TranslationType == ParameterTranslationType_Fixed)
			{
				TranslationFixed.Position *= m_effect->GetMaginification();
			}
			else if (TranslationType == ParameterTranslationType_PVA)
			{
				TranslationPVA.location.min *= m_effect->GetMaginification();
				TranslationPVA.location.max *= m_effect->GetMaginification();
				TranslationPVA.velocity.min *= m_effect->GetMaginification();
				TranslationPVA.velocity.max *= m_effect->GetMaginification();
				TranslationPVA.acceleration.min *= m_effect->GetMaginification();
				TranslationPVA.acceleration.max *= m_effect->GetMaginification();
			}
			else if (TranslationType == ParameterTranslationType_Easing)
			{
				TranslationEasing.location.start.min *= m_effect->GetMaginification();
				TranslationEasing.location.start.max *= m_effect->GetMaginification();
				TranslationEasing.location.end.min *= m_effect->GetMaginification();
				TranslationEasing.location.end.max *= m_effect->GetMaginification();
			}
			else if (TranslationType == ParameterTranslationType_FCurve)
			{
				TranslationFCurve->X.Maginify(m_effect->GetMaginification());
				TranslationFCurve->Y.Maginify(m_effect->GetMaginification());
				TranslationFCurve->Z.Maginify(m_effect->GetMaginification());
			}
		}

		memcpy(&LocationAbs.type, pos, sizeof(int));
		pos += sizeof(int);

		// Calc attraction forces
		if (LocationAbs.type == LocationAbsType::None)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);
			assert(size == 0);
			memcpy(&LocationAbs.none, pos, size);
			pos += size;
		}
		else if (LocationAbs.type == LocationAbsType::Gravity)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);
			assert(size == sizeof(vector3d));
			memcpy(&LocationAbs.gravity, pos, size);
			pos += size;
		}
		else if (LocationAbs.type == LocationAbsType::AttractiveForce)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);
			assert(size == sizeof(LocationAbs.attractiveForce));
			memcpy(&LocationAbs.attractiveForce, pos, size);
			pos += size;
		}

		// Magnify attraction forces
		if (ef->IsDyanamicMagnificationValid())
		{
			if (LocationAbs.type == LocationAbsType::None)
			{
			}
			else if (LocationAbs.type == LocationAbsType::Gravity)
			{
				LocationAbs.gravity *= m_effect->GetMaginification();
			}
			else if (LocationAbs.type == LocationAbsType::AttractiveForce)
			{
				LocationAbs.attractiveForce.control *= m_effect->GetMaginification();
				LocationAbs.attractiveForce.force *= m_effect->GetMaginification();
				LocationAbs.attractiveForce.minRange *= m_effect->GetMaginification();
				LocationAbs.attractiveForce.maxRange *= m_effect->GetMaginification();
			}
		}

		memcpy(&RotationType, pos, sizeof(int));
		pos += sizeof(int);
		EffekseerPrintDebug("RotationType %d\n", RotationType);
		if (RotationType == ParameterRotationType_Fixed)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);

			if (ef->GetVersion() >= 14)
			{
				assert(size == sizeof(ParameterRotationFixed));
				memcpy(&RotationFixed, pos, size);
			}
			else
			{
				memcpy(&RotationFixed.Position, pos, size);
			}
			pos += size;

			// make invalid
			if (RotationFixed.RefEq < 0 && RotationFixed.Position.X == 0.0f && RotationFixed.Position.Y == 0.0f &&
				RotationFixed.Position.Z == 0.0f)
			{
				RotationType = ParameterRotationType_None;
				EffekseerPrintDebug("RotationType Change None\n");
			}
		}
		else if (RotationType == ParameterRotationType_PVA)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);
			if (ef->GetVersion() >= 14)
			{
				assert(size == sizeof(ParameterRotationPVA));
				memcpy(&RotationPVA, pos, size);
			}
			else
			{
				memcpy(&RotationPVA.rotation, pos, size);
			}
			pos += size;
		}
		else if (RotationType == ParameterRotationType_Easing)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);
			if (ef->GetVersion() >= 14)
			{
				assert(size == sizeof(RotationEasing));
				memcpy(&RotationEasing, pos, size);
			}
			else
			{
				assert(size == sizeof(easing_vector3d));
				memcpy(&RotationEasing.rotation, pos, size);
			}

			pos += size;
		}
		else if (RotationType == ParameterRotationType_AxisPVA)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);
			assert(size == sizeof(ParameterRotationAxisPVA));
			memcpy(&RotationAxisPVA, pos, size);
			pos += size;
		}
		else if (RotationType == ParameterRotationType_AxisEasing)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);
			assert(size == sizeof(ParameterRotationAxisEasing));
			memcpy(&RotationAxisEasing, pos, size);
			pos += size;
		}
		else if (RotationType == ParameterRotationType_FCurve)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);

			RotationFCurve = new FCurveVector3D();
			pos += RotationFCurve->Load(pos, m_effect->GetVersion());
		}

		memcpy(&ScalingType, pos, sizeof(int));
		pos += sizeof(int);
		EffekseerPrintDebug("ScalingType %d\n", ScalingType);
		if (ScalingType == ParameterScalingType_Fixed)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);

			if (ef->GetVersion() >= 14)
			{
				assert(size == sizeof(ParameterScalingFixed));
				memcpy(&ScalingFixed, pos, size);
				pos += size;
			}
			else
			{
				memcpy(&ScalingFixed.Position, pos, size);
				pos += size;
			}

			// make invalid
			if (ScalingFixed.RefEq < 0 && ScalingFixed.Position.X == 1.0f && ScalingFixed.Position.Y == 1.0f &&
				ScalingFixed.Position.Z == 1.0f)
			{
				ScalingType = ParameterScalingType_None;
				EffekseerPrintDebug("ScalingType Change None\n");
			}
		}
		else if (ScalingType == ParameterScalingType_PVA)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);
			if (ef->GetVersion() >= 14)
			{
				assert(size == sizeof(ParameterScalingPVA));
				memcpy(&ScalingPVA, pos, size);
			}
			else
			{
				memcpy(&ScalingPVA.Position, pos, size);
			}
			pos += size;
		}
		else if (ScalingType == ParameterScalingType_Easing)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);
			if (ef->GetVersion() >= 14)
			{
				assert(size == sizeof(ParameterScalingEasing));
				memcpy(&ScalingEasing, pos, size);
			}
			else
			{
				memcpy(&ScalingEasing.Position, pos, size);
			}
			pos += size;
		}
		else if (ScalingType == ParameterScalingType_SinglePVA)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);
			assert(size == sizeof(ParameterScalingSinglePVA));
			memcpy(&ScalingSinglePVA, pos, size);
			pos += size;
		}
		else if (ScalingType == ParameterScalingType_SingleEasing)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);
			assert(size == sizeof(easing_float));
			memcpy(&ScalingSingleEasing, pos, size);
			pos += size;
		}
		else if (ScalingType == ParameterScalingType_FCurve)
		{
			memcpy(&size, pos, sizeof(int));
			pos += sizeof(int);

			ScalingFCurve = new FCurveVector3D();
			pos += ScalingFCurve->Load(pos, m_effect->GetVersion());
			ScalingFCurve->X.SetDefaultValue(1.0f);
			ScalingFCurve->Y.SetDefaultValue(1.0f);
			ScalingFCurve->Z.SetDefaultValue(1.0f);
		}

		/* Spawning Method */
		GenerationLocation.load(pos, m_effect->GetVersion());

		/* Spawning Method 拡大処理*/
		if (ef->IsDyanamicMagnificationValid()
			/* && (this->CommonValues.ScalingBindType == BindType::NotBind || parent->GetType() == EFFECT_NODE_TYPE_ROOT)*/)
		{
			if (GenerationLocation.type == ParameterGenerationLocation::TYPE_POINT)
			{
				GenerationLocation.point.location.min *= m_effect->GetMaginification();
				GenerationLocation.point.location.max *= m_effect->GetMaginification();
			}
			else if (GenerationLocation.type == ParameterGenerationLocation::TYPE_LINE)
			{
				GenerationLocation.line.position_end.min *= m_effect->GetMaginification();
				GenerationLocation.line.position_end.max *= m_effect->GetMaginification();
				GenerationLocation.line.position_start.min *= m_effect->GetMaginification();
				GenerationLocation.line.position_start.max *= m_effect->GetMaginification();
				GenerationLocation.line.position_noize.min *= m_effect->GetMaginification();
				GenerationLocation.line.position_noize.max *= m_effect->GetMaginification();
			}
			else if (GenerationLocation.type == ParameterGenerationLocation::TYPE_SPHERE)
			{
				GenerationLocation.sphere.radius.min *= m_effect->GetMaginification();
				GenerationLocation.sphere.radius.max *= m_effect->GetMaginification();
			}
			else if (GenerationLocation.type == ParameterGenerationLocation::TYPE_CIRCLE)
			{
				GenerationLocation.circle.radius.min *= m_effect->GetMaginification();
				GenerationLocation.circle.radius.max *= m_effect->GetMaginification();
			}
		}

		// Load depth values
		if (m_effect->GetVersion() >= 12)
		{
			memcpy(&DepthValues.DepthOffset, pos, sizeof(float));
			pos += sizeof(float);

			auto IsDepthOffsetScaledWithCamera = 0;
			memcpy(&IsDepthOffsetScaledWithCamera, pos, sizeof(int32_t));
			pos += sizeof(int32_t);

			DepthValues.IsDepthOffsetScaledWithCamera = IsDepthOffsetScaledWithCamera > 0;

			auto IsDepthOffsetScaledWithParticleScale = 0;
			memcpy(&IsDepthOffsetScaledWithParticleScale, pos, sizeof(int32_t));
			pos += sizeof(int32_t);

			if (m_effect->GetVersion() >= 15)
			{
				memcpy(&DepthValues.DepthParameter.SuppressionOfScalingByDepth, pos, sizeof(float));
				pos += sizeof(float);

				memcpy(&DepthValues.DepthParameter.DepthClipping, pos, sizeof(float));
				pos += sizeof(float);
			}
	
			if (m_effect->GetVersion() >= 13)
			{
				memcpy(&DepthValues.ZSort, pos, sizeof(int32_t));
				pos += sizeof(int32_t);

				memcpy(&DepthValues.DrawingPriority, pos, sizeof(int32_t));
				pos += sizeof(int32_t);
			}

			memcpy(&DepthValues.SoftParticle, pos, sizeof(float));
			pos += sizeof(float);

			DepthValues.DepthOffset *= m_effect->GetMaginification();
			DepthValues.SoftParticle *= m_effect->GetMaginification();

			if (DepthValues.DepthParameter.DepthClipping < FLT_MAX / 10)
			{
				DepthValues.DepthParameter.DepthClipping *= m_effect->GetMaginification();
			}

			DepthValues.DepthParameter.DepthOffset = DepthValues.DepthOffset;
			DepthValues.DepthParameter.IsDepthOffsetScaledWithCamera = DepthValues.IsDepthOffsetScaledWithCamera;
			DepthValues.DepthParameter.IsDepthOffsetScaledWithParticleScale = DepthValues.IsDepthOffsetScaledWithParticleScale;
		}

		// Convert right handle coordinate system into left handle coordinate system
		if (setting->GetCoordinateSystem() == CoordinateSystem::LH)
		{
			// Translation
			DynamicFactor.Tra[2] *= -1.0f;

			if (TranslationType == ParameterTranslationType_Fixed)
			{
				TranslationFixed.Position.Z *= -1.0f;
			}
			else if (TranslationType == ParameterTranslationType_PVA)
			{
				TranslationPVA.location.max.z *= -1.0f;
				TranslationPVA.location.min.z *= -1.0f;
				TranslationPVA.velocity.max.z *= -1.0f;
				TranslationPVA.velocity.min.z *= -1.0f;
				TranslationPVA.acceleration.max.z *= -1.0f;
				TranslationPVA.acceleration.min.z *= -1.0f;
			}
			else if (TranslationType == ParameterTranslationType_Easing)
			{
				TranslationEasing.location.start.max.z *= -1.0f;
				TranslationEasing.location.start.min.z *= -1.0f;
				TranslationEasing.location.end.max.z *= -1.0f;
				TranslationEasing.location.end.min.z *= -1.0f;
			}

			// Rotation
			DynamicFactor.Rot[0] *= -1.0f;
			DynamicFactor.Rot[1] *= -1.0f;

			if (RotationType == ParameterRotationType_Fixed)
			{
				RotationFixed.Position.X *= -1.0f;
				RotationFixed.Position.Y *= -1.0f;
			}
			else if (RotationType == ParameterRotationType_PVA)
			{
				RotationPVA.rotation.max.x *= -1.0f;
				RotationPVA.rotation.min.x *= -1.0f;
				RotationPVA.rotation.max.y *= -1.0f;
				RotationPVA.rotation.min.y *= -1.0f;
				RotationPVA.velocity.max.x *= -1.0f;
				RotationPVA.velocity.min.x *= -1.0f;
				RotationPVA.velocity.max.y *= -1.0f;
				RotationPVA.velocity.min.y *= -1.0f;
				RotationPVA.acceleration.max.x *= -1.0f;
				RotationPVA.acceleration.min.x *= -1.0f;
				RotationPVA.acceleration.max.y *= -1.0f;
				RotationPVA.acceleration.min.y *= -1.0f;
			}
			else if (RotationType == ParameterRotationType_Easing)
			{
				RotationEasing.rotation.start.max.x *= -1.0f;
				RotationEasing.rotation.start.min.x *= -1.0f;
				RotationEasing.rotation.start.max.y *= -1.0f;
				RotationEasing.rotation.start.min.y *= -1.0f;
				RotationEasing.rotation.end.max.x *= -1.0f;
				RotationEasing.rotation.end.min.x *= -1.0f;
				RotationEasing.rotation.end.max.y *= -1.0f;
				RotationEasing.rotation.end.min.y *= -1.0f;
			}
			else if (RotationType == ParameterRotationType_AxisPVA)
			{
				RotationAxisPVA.axis.max.z *= -1.0f;
				RotationAxisPVA.axis.min.z *= -1.0f;
			}
			else if (RotationType == ParameterRotationType_AxisEasing)
			{
				RotationAxisEasing.axis.max.z *= -1.0f;
				RotationAxisEasing.axis.min.z *= -1.0f;
			}
			else if (RotationType == ParameterRotationType_FCurve)
			{
				RotationFCurve->X.ChangeCoordinate();
				RotationFCurve->Y.ChangeCoordinate();
			}

			// GenerationLocation
			if (GenerationLocation.type == ParameterGenerationLocation::TYPE_POINT)
			{
			}
			else if (GenerationLocation.type == ParameterGenerationLocation::TYPE_SPHERE)
			{
				GenerationLocation.sphere.rotation_x.max *= -1.0f;
				GenerationLocation.sphere.rotation_x.min *= -1.0f;
				GenerationLocation.sphere.rotation_y.max *= -1.0f;
				GenerationLocation.sphere.rotation_y.min *= -1.0f;
			}
		}

		// generate inversed parameter
		for (size_t i = 0; i < DynamicFactor.Tra.size(); i++)
		{
			DynamicFactor.TraInv[i] = 1.0f / DynamicFactor.Tra[i];
		}

		for (size_t i = 0; i < DynamicFactor.Rot.size(); i++)
		{
			DynamicFactor.RotInv[i] = 1.0f / DynamicFactor.Rot[i];
		}

		for (size_t i = 0; i < DynamicFactor.Scale.size(); i++)
		{
			DynamicFactor.ScaleInv[i] = 1.0f / DynamicFactor.Scale[i];
		}

		if (m_effect->GetVersion() >= 3)
		{
			RendererCommon.load(pos, m_effect->GetVersion());
		}
		else
		{
			RendererCommon.reset();
		}

		LoadRendererParameter(pos, m_effect->GetSetting());

		if (m_effect->GetVersion() >= 1)
		{
			// Sound
			memcpy(&SoundType, pos, sizeof(int));
			pos += sizeof(int);
			if (SoundType == ParameterSoundType_Use)
			{
				memcpy(&Sound.WaveId, pos, sizeof(int32_t));
				pos += sizeof(int32_t);
				memcpy(&Sound.Volume, pos, sizeof(random_float));
				pos += sizeof(random_float);
				memcpy(&Sound.Pitch, pos, sizeof(random_float));
				pos += sizeof(random_float);
				memcpy(&Sound.PanType, pos, sizeof(ParameterSoundPanType));
				pos += sizeof(ParameterSoundPanType);
				memcpy(&Sound.Pan, pos, sizeof(random_float));
				pos += sizeof(random_float);
				memcpy(&Sound.Distance, pos, sizeof(float));
				pos += sizeof(float);
				memcpy(&Sound.Delay, pos, sizeof(random_int));
				pos += sizeof(random_int);
			}
		}
	}

	// ノード
	int nodeCount = 0;
	memcpy(&nodeCount, pos, sizeof(int));
	pos += sizeof(int);
	EffekseerPrintDebug("ChildrenCount : %d\n", nodeCount);
	m_Nodes.resize(nodeCount);
	for (size_t i = 0; i < m_Nodes.size(); i++)
	{
		m_Nodes[i] = EffectNodeImplemented::Create(m_effect, this, pos);
	}
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
EffectNodeImplemented::~EffectNodeImplemented()
{
	for (size_t i = 0; i < m_Nodes.size(); i++)
	{
		ES_SAFE_DELETE(m_Nodes[i]);
	}

	ES_SAFE_DELETE(TranslationFCurve);
	ES_SAFE_DELETE(RotationFCurve);
	ES_SAFE_DELETE(ScalingFCurve);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
Effect* EffectNodeImplemented::GetEffect() const { return m_effect; }

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
int EffectNodeImplemented::GetChildrenCount() const { return (int)m_Nodes.size(); }

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
EffectNode* EffectNodeImplemented::GetChild(int index) const
{
	if (index >= GetChildrenCount())
		return NULL;
	return m_Nodes[index];
}

EffectBasicRenderParameter EffectNodeImplemented::GetBasicRenderParameter()
{
	EffectBasicRenderParameter param;
	param.ColorTextureIndex = RendererCommon.ColorTextureIndex;
	param.AlphaBlend = RendererCommon.AlphaBlend;
	param.Distortion = RendererCommon.Distortion;
	param.DistortionIntensity = RendererCommon.DistortionIntensity;
	param.FilterType = RendererCommon.FilterType;
	param.WrapType = RendererCommon.WrapType;
	param.ZTest = RendererCommon.ZTest;
	param.ZWrite = RendererCommon.ZWrite;
	return param;
}

void EffectNodeImplemented::SetBasicRenderParameter(EffectBasicRenderParameter param)
{
	RendererCommon.ColorTextureIndex = param.ColorTextureIndex;
	RendererCommon.AlphaBlend = param.AlphaBlend;
	RendererCommon.Distortion = param.Distortion;
	RendererCommon.DistortionIntensity = param.DistortionIntensity;
	RendererCommon.FilterType = param.FilterType;
	RendererCommon.WrapType = param.WrapType;
	RendererCommon.ZTest = param.ZTest;
	RendererCommon.ZWrite = param.ZWrite;
}

EffectModelParameter EffectNodeImplemented::GetEffectModelParameter()
{
	EffectModelParameter param;
	param.Lighting = false;

	if (GetType() == EFFECT_NODE_TYPE_MODEL)
	{
		auto t = (EffectNodeModel*)this;
		param.Lighting = t->Lighting;
	}

	return param;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void EffectNodeImplemented::LoadRendererParameter(unsigned char*& pos, Setting* setting)
{
	int32_t type = 0;
	memcpy(&type, pos, sizeof(int));
	pos += sizeof(int);
	assert(type == GetType());
	EffekseerPrintDebug("Renderer : None\n");
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void EffectNodeImplemented::BeginRendering(int32_t count, Manager* manager) {}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void EffectNodeImplemented::BeginRenderingGroup(InstanceGroup* group, Manager* manager) {}

void EffectNodeImplemented::EndRenderingGroup(InstanceGroup* group, Manager* manager) {}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void EffectNodeImplemented::Rendering(const Instance& instance, const Instance* next_instance, Manager* manager) {}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void EffectNodeImplemented::EndRendering(Manager* manager) {}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void EffectNodeImplemented::InitializeRenderedInstanceGroup(InstanceGroup& instanceGroup, Manager* manager) {}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void EffectNodeImplemented::InitializeRenderedInstance(Instance& instance, Manager* manager) {}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void EffectNodeImplemented::UpdateRenderedInstance(Instance& instance, Manager* manager) {}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
float EffectNodeImplemented::GetFadeAlpha(const Instance& instance)
{
	float alpha = 1.0f;

	if (RendererCommon.FadeInType == ParameterRendererCommon::FADEIN_ON && instance.m_LivingTime < RendererCommon.FadeIn.Frame)
	{
		float v = 1.0f;
		RendererCommon.FadeIn.Value.setValueToArg(v, 0.0f, 1.0f, (float)instance.m_LivingTime / (float)RendererCommon.FadeIn.Frame);

		alpha *= v;
	}

	if (RendererCommon.FadeOutType == ParameterRendererCommon::FADEOUT_ON &&
		instance.m_LivingTime + RendererCommon.FadeOut.Frame > instance.m_LivedTime)
	{
		float v = 1.0f;
		RendererCommon.FadeOut.Value.setValueToArg(v,
												   1.0f,
												   0.0f,
												   (float)(instance.m_LivingTime + RendererCommon.FadeOut.Frame - instance.m_LivedTime) /
													   (float)RendererCommon.FadeOut.Frame);

		alpha *= v;
	}

	return alpha;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void EffectNodeImplemented::PlaySound_(Instance& instance, SoundTag tag, Manager* manager)
{
	auto instanceGlobal = instance.m_pContainer->GetRootInstance();

	SoundPlayer* player = manager->GetSoundPlayer();
	if (player == NULL)
	{
		return;
	}

	if (Sound.WaveId >= 0)
	{
		SoundPlayer::InstanceParameter parameter;
		parameter.Data = m_effect->GetWave(Sound.WaveId);
		parameter.Volume = Sound.Volume.getValue(*instanceGlobal);
		parameter.Pitch = Sound.Pitch.getValue(*instanceGlobal);
		parameter.Pan = Sound.Pan.getValue(*instanceGlobal);

		parameter.Mode3D = (Sound.PanType == ParameterSoundPanType_3D);
		Vector3D::Transform(parameter.Position, Vector3D(0.0f, 0.0f, 0.0f), instance.GetGlobalMatrix43());
		parameter.Distance = Sound.Distance;

		player->Play(tag, parameter);
	}
}

EffectInstanceTerm EffectNodeImplemented::CalculateInstanceTerm(EffectInstanceTerm& parentTerm) const
{
	EffectInstanceTerm ret;

	auto addWithClip = [](int v1, int v2) -> int {
		v1 = Max(v1, 0);
		v2 = Max(v2, 0);

		if (v1 >= INT_MAX / 2)
			return INT_MAX;

		if (v2 >= INT_MAX / 2)
			return INT_MAX;

		return v1 + v2;
	};

	int lifeMin = CommonValues.life.min;
	int lifeMax = CommonValues.life.max;

	if (CommonValues.RemoveWhenLifeIsExtinct <= 0)
	{
		lifeMin = INT_MAX;
		lifeMax = INT_MAX;
	}

	auto firstBeginMin = CommonValues.GenerationTimeOffset.min;
	auto firstBeginMax = CommonValues.GenerationTimeOffset.max;
	auto firstEndMin = addWithClip(firstBeginMin, lifeMin);
	auto firstEndMax = addWithClip(firstBeginMax, lifeMax);

	auto lastBeginMin = 0;
	auto lastBeginMax = 0;
	if (CommonValues.MaxGeneration > INT_MAX / 2)
	{
		lastBeginMin = INT_MAX / 2;
	}
	else
	{
		lastBeginMin = CommonValues.GenerationTimeOffset.min + (CommonValues.MaxGeneration - 1) * (CommonValues.GenerationTime.min);
	}

	if (CommonValues.MaxGeneration > INT_MAX / 2)
	{
		lastBeginMax = INT_MAX / 2;
	}
	else
	{
		lastBeginMax = CommonValues.GenerationTimeOffset.max + (CommonValues.MaxGeneration - 1) * (CommonValues.GenerationTime.max);
	}

	auto lastEndMin = addWithClip(lastBeginMin, lifeMin);
	auto lastEndMax = addWithClip(lastBeginMax, lifeMax);

	auto parentFirstTermMin = parentTerm.FirstInstanceEndMin - parentTerm.FirstInstanceStartMin;
	auto parentFirstTermMax = parentTerm.FirstInstanceEndMax - parentTerm.FirstInstanceStartMax;
	auto parentLastTermMin = parentTerm.LastInstanceEndMin - parentTerm.LastInstanceStartMin;
	auto parentLastTermMax = parentTerm.LastInstanceEndMax - parentTerm.LastInstanceStartMax;

	if (CommonValues.RemoveWhenParentIsRemoved > 0)
	{
		if (firstEndMin - firstBeginMin > parentFirstTermMin)
			firstEndMin = firstBeginMin + parentFirstTermMin;

		if (firstEndMax - firstBeginMax > parentFirstTermMax)
			firstEndMax = firstBeginMax + parentFirstTermMax;

		if (lastEndMin > INT_MAX / 2)
		{
			lastBeginMin = parentLastTermMin;
			lastEndMin = parentLastTermMin;
		}
		else if (lastEndMin - lastBeginMin > parentLastTermMin)
		{
			lastEndMin = lastBeginMin + parentLastTermMin;
		}

		if (lastEndMax > INT_MAX / 2)
		{
			lastBeginMax = parentLastTermMax;
			lastEndMax = parentLastTermMax;
		}
		else if (lastEndMax - lastBeginMax > parentLastTermMax)
		{
			lastEndMax = lastBeginMax + parentLastTermMax;
		}
	}

	ret.FirstInstanceStartMin = addWithClip(parentTerm.FirstInstanceStartMin, firstBeginMin);
	ret.FirstInstanceStartMax = addWithClip(parentTerm.FirstInstanceStartMax, firstBeginMax);
	ret.FirstInstanceEndMin = addWithClip(parentTerm.FirstInstanceStartMin, firstEndMin);
	ret.FirstInstanceEndMax = addWithClip(parentTerm.FirstInstanceStartMax, firstEndMax);

	ret.LastInstanceStartMin = addWithClip(parentTerm.LastInstanceStartMin, lastBeginMin);
	ret.LastInstanceStartMax = addWithClip(parentTerm.LastInstanceStartMax, lastBeginMax);
	ret.LastInstanceEndMin = addWithClip(parentTerm.LastInstanceStartMin, lastEndMin);
	ret.LastInstanceEndMax = addWithClip(parentTerm.LastInstanceStartMax, lastEndMax);

	// check children
	if (CommonValues.RemoveWhenChildrenIsExtinct > 0)
	{
		int childFirstEndMin = 0;
		int childFirstEndMax = 0;
		int childLastEndMin = 0;
		int childLastEndMax = 0;

		for (int32_t i = 0; i < GetChildrenCount(); i++)
		{
			auto child = static_cast<EffectNodeImplemented*>(GetChild(i));
			auto childTerm = child->CalculateInstanceTerm(ret);
			childFirstEndMin = Max(childTerm.FirstInstanceEndMin, childFirstEndMin);
			childFirstEndMax = Max(childTerm.FirstInstanceEndMax, childFirstEndMax);
			childLastEndMin = Max(childTerm.LastInstanceEndMin, childLastEndMin);
			childLastEndMax = Max(childTerm.LastInstanceEndMax, childLastEndMax);
		}

		ret.FirstInstanceEndMin = Min(ret.FirstInstanceEndMin, childFirstEndMin);
		ret.FirstInstanceEndMax = Min(ret.FirstInstanceEndMax, childFirstEndMax);
		ret.LastInstanceEndMin = Min(ret.LastInstanceEndMin, childLastEndMin);
		ret.LastInstanceEndMax = Min(ret.LastInstanceEndMax, childLastEndMax);
	}

	return ret;
}

EffectNodeImplemented* EffectNodeImplemented::Create(Effect* effect, EffectNode* parent, unsigned char*& pos)
{
	EffectNodeImplemented* effectnode = NULL;

	int node_type = 0;
	memcpy(&node_type, pos, sizeof(int));

	if (node_type == EFFECT_NODE_TYPE_ROOT)
	{
		EffekseerPrintDebug("* Create : EffectNodeRoot\n");
		effectnode = new EffectNodeRoot(effect, pos);
	}
	else if (node_type == EFFECT_NODE_TYPE_NONE)
	{
		EffekseerPrintDebug("* Create : EffectNodeNone\n");
		effectnode = new EffectNodeImplemented(effect, pos);
	}
	else if (node_type == EFFECT_NODE_TYPE_SPRITE)
	{
		EffekseerPrintDebug("* Create : EffectNodeSprite\n");
		effectnode = new EffectNodeSprite(effect, pos);
	}
	else if (node_type == EFFECT_NODE_TYPE_RIBBON)
	{
		EffekseerPrintDebug("* Create : EffectNodeRibbon\n");
		effectnode = new EffectNodeRibbon(effect, pos);
	}
	else if (node_type == EFFECT_NODE_TYPE_RING)
	{
		EffekseerPrintDebug("* Create : EffectNodeRing\n");
		effectnode = new EffectNodeRing(effect, pos);
	}
	else if (node_type == EFFECT_NODE_TYPE_MODEL)
	{
		EffekseerPrintDebug("* Create : EffectNodeModel\n");
		effectnode = new EffectNodeModel(effect, pos);
	}
	else if (node_type == EFFECT_NODE_TYPE_TRACK)
	{
		EffekseerPrintDebug("* Create : EffectNodeTrack\n");
		effectnode = new EffectNodeTrack(effect, pos);
	}
	else
	{
		assert(0);
	}

	effectnode->LoadParameter(pos, parent, effect->GetSetting());

	return effectnode;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------

} // namespace Effekseer

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------