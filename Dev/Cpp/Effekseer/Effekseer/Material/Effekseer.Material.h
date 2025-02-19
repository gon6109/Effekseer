﻿
#ifndef __EFFEKSEER_MATERIAL_H__
#define __EFFEKSEER_MATERIAL_H__

#include "../Effekseer.Base.Pre.h"
#include <array>
#include <assert.h>
#include <map>
#include <sstream>
#include <string.h>
#include <vector>

namespace Effekseer
{

class Material
{
private:
	struct Texture
	{
		std::string Name;
		int32_t Index;
	};

	struct Uniform
	{
		std::string Name;
		int32_t Index;
	};

	uint64_t guid_ = 0;

	std::string genericCode_;

	bool hasRefraction_ = false;

	bool isSimpleVertex_ = false;

	ShadingModelType shadingModel_;

	std::vector<Texture> textures_;

	std::vector<Uniform> uniforms_;

public:
	Material() = default;
	virtual ~Material() = default;

	virtual bool Load(const uint8_t* data, int32_t size);

	virtual ShadingModelType GetShadingModel() const;

	virtual void SetShadingModel(ShadingModelType shadingModel);

	virtual bool GetIsSimpleVertex() const;

	virtual void SetIsSimpleVertex(bool isSimpleVertex);

	virtual bool GetHasRefraction() const;

	virtual void SetHasRefraction(bool hasRefraction);

	virtual const char* GetGenericCode() const;

	virtual void SetGenericCode(const char* code);

	virtual uint64_t GetGUID() const;

	virtual void SetGUID(uint64_t guid);

	virtual int32_t GetTextureIndex(int32_t index) const;

	virtual void SetTextureIndex(int32_t index, int32_t value);

	virtual const char* GetTextureName(int32_t index) const;

	virtual void SetTextureName(int32_t index, const char* name);

	virtual int32_t GetTextureCount() const;

	virtual void SetTextureCount(int32_t count);

	virtual int32_t GetUniformIndex(int32_t index) const;

	virtual void SetUniformIndex(int32_t index, int32_t value);

	virtual const char* GetUniformName(int32_t index) const;

	virtual void SetUniformName(int32_t index, const char* name);

	virtual int32_t GetUniformCount() const;

	virtual void SetUniformCount(int32_t count);
};

} // namespace Effekseer

#endif // __EFFEKSEER_MATERIAL_H__