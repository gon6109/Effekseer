#include "EffekseerMaterialCompilerGL.h"

#include <iostream>

namespace Effekseer
{
namespace GL
{

static const char g_material_model_vs_src_pre[] =
	R"(
IN vec4 a_Position;
IN vec3 a_Normal;
IN vec3 a_Binormal;
IN vec3 a_Tangent;
IN vec2 a_TexCoord;
IN vec4 a_Color;
)"
#if defined(MODEL_SOFTWARE_INSTANCING)
	R"(
IN float a_InstanceID;
IN vec4 a_UVOffset;
IN vec4 a_ModelColor;
)"
#endif
	R"(

OUT lowp vec4 v_VColor;
OUT mediump vec2 v_UV1;
OUT mediump vec2 v_UV2;
OUT mediump vec3 v_WorldP;
OUT mediump vec3 v_WorldN;
OUT mediump vec3 v_WorldT;
OUT mediump vec3 v_WorldB;
OUT mediump vec2 v_ScreenUV;

)"
#if defined(MODEL_SOFTWARE_INSTANCING)
	R"(
uniform mat4 ModelMatrix[20];
uniform vec4 UVOffset[20];
uniform vec4 ModelColor[20];
)"
#else
	R"(
uniform mat4 ModelMatrix;
uniform vec4 UVOffset;
uniform vec4 ModelColor;
)"
#endif
	R"(
uniform mat4 ProjectionMatrix;
uniform vec4 mUVInversed;
uniform vec4 predefined_uniform;


)";

static const char g_material_model_vs_src_suf1[] =
	R"(

void main()
{
)"
#if defined(MODEL_SOFTWARE_INSTANCING)
	R"(
	mat4 modelMatrix = ModelMatrix[int(a_InstanceID)];
	vec4 uvOffset = a_UVOffset;
	vec4 modelColor = a_ModelColor;
)"
#else
	R"(
	mat4 modelMatrix = ModelMatrix;
	vec4 uvOffset = UVOffset;
	vec4 modelColor = ModelColor * a_Color;
)"
#endif
	R"(
	mat3 modelMatRot = mat3(modelMatrix);
	vec3 worldPos = (modelMatrix * a_Position).xyz;
	vec3 worldNormal = normalize(modelMatRot * a_Normal);
	vec3 worldBinormal = normalize(modelMatRot * a_Binormal);
	vec3 worldTangent = normalize(modelMatRot * a_Tangent);

	// UV
	vec2 uv1 = a_TexCoord.xy * uvOffset.zw + uvOffset.xy;
	vec2 uv2 = uv1;

	vec3 pixelNormalDir = vec3(0.5, 0.5, 1.0);
)";

static const char g_material_model_vs_src_suf2[] =
	R"(
	worldPos = worldPos + worldPositionOffset;

	v_WorldP = worldPos;
	v_WorldN = worldNormal;
	v_WorldB = worldBinormal;
	v_WorldT = worldTangent;
	v_UV1 = uv1;
	v_UV2 = uv2;
	v_VColor = a_Color;
	gl_Position = ProjectionMatrix * vec4(worldPos, 1.0);
	v_ScreenUV.xy = gl_Position.xy / gl_Position.w;
	v_ScreenUV.xy = vec2(v_ScreenUV.x + 1.0, v_ScreenUV.y + 1.0) * 0.5;
}
)";

static const char g_material_sprite_vs_src_pre_simple[] =
	R"(
IN vec4 atPosition;
IN vec4 atColor;
IN vec4 atTexCoord;
)"

	R"(
OUT lowp vec4 v_VColor;
OUT mediump vec2 v_UV1;
OUT mediump vec2 v_UV2;
OUT mediump vec3 v_WorldP;
OUT mediump vec3 v_WorldN;
OUT mediump vec3 v_WorldT;
OUT mediump vec3 v_WorldB;
OUT mediump vec2 v_ScreenUV;
)"

	R"(
uniform mat4 uMatCamera;
uniform mat4 uMatProjection;
uniform vec4 mUVInversed;
uniform vec4 predefined_uniform;

)";

static const char g_material_sprite_vs_src_pre[] =
	R"(
IN vec4 atPosition;
IN vec4 atColor;
IN vec3 atNormal;
IN vec3 atTangent;
IN vec2 atTexCoord;
IN vec2 atTexCoord2;
)"

	R"(
OUT lowp vec4 v_VColor;
OUT mediump vec2 v_UV1;
OUT mediump vec2 v_UV2;
OUT mediump vec3 v_WorldP;
OUT mediump vec3 v_WorldN;
OUT mediump vec3 v_WorldT;
OUT mediump vec3 v_WorldB;
OUT mediump vec2 v_ScreenUV;
)"

	R"(
uniform mat4 uMatCamera;
uniform mat4 uMatProjection;
uniform vec4 mUVInversed;
uniform vec4 predefined_uniform;

)";

static const char g_material_sprite_vs_src_suf1_simple[] =

	R"(

void main() {
	vec3 worldPos = atPosition.xyz;

	// UV
	vec2 uv1 = atTexCoord.xy;
	uv1.y = mUVInversed.x + mUVInversed.y * uv1.y;
	vec2 uv2 = uv1;

	// NBT
	vec3 worldNormal = vec3(0.0, 0.0, 0.0);
	vec3 worldBinormal = vec3(0.0, 0.0, 0.0);
	vec3 worldTangent = vec3(0.0, 0.0, 0.0);
	v_WorldN = worldNormal;
	v_WorldB = worldBinormal;
	v_WorldT = worldTangent;

	vec3 pixelNormalDir = vec3(0.5, 0.5, 1.0);

)";

static const char g_material_sprite_vs_src_suf1[] =

	R"(

void main() {
	vec3 worldPos = atPosition.xyz;

	// UV
	vec2 uv1 = atTexCoord.xy;
	uv1.y = mUVInversed.x + mUVInversed.y * uv1.y;
	vec2 uv2 = atTexCoord2.xy;
	uv2.y = mUVInversed.x + mUVInversed.y * uv2.y;

	// NBT
	vec3 worldNormal = (atNormal - vec3(0.5, 0.5, 0.5)) * 2.0;
	vec3 worldTangent = (atTangent - vec3(0.5, 0.5, 0.5)) * 2.0;
	vec3 worldBinormal = cross(worldNormal, worldTangent);

	v_WorldN = worldNormal;
	v_WorldB = worldBinormal;
	v_WorldT = worldTangent;
	vec3 pixelNormalDir = vec3(0.5, 0.5, 1.0);
)";

static const char g_material_sprite_vs_src_suf2[] =

	R"(
	worldPos = worldPos + worldPositionOffset;

	vec4 cameraPos = uMatCamera * vec4(worldPos, 1.0);
	cameraPos = cameraPos / cameraPos.w;

	gl_Position = uMatProjection * cameraPos;

	v_WorldP = worldPos;
	v_VColor = atColor;

	v_UV1 = uv1;
	v_UV2 = uv2;
	v_ScreenUV.xy = gl_Position.xy / gl_Position.w;
	v_ScreenUV.xy = vec2(v_ScreenUV.x + 1.0, v_ScreenUV.y + 1.0) * 0.5;
}

)";

static const char g_material_fs_src_pre[] =
	R"(

IN lowp vec4 v_VColor;
IN mediump vec2 v_UV1;
IN mediump vec2 v_UV2;
IN mediump vec3 v_WorldP;
IN mediump vec3 v_WorldN;
IN mediump vec3 v_WorldT;
IN mediump vec3 v_WorldB;
IN mediump vec2 v_ScreenUV;

uniform vec4 mUVInversedBack;
uniform vec4 predefined_uniform;

)";

static const char g_material_fs_src_suf1[] =
	R"(

#ifdef __MATERIAL_LIT__

float saturate(float v)
{
	return max(min(v, 1.0), 0.0);
}

float calcD_GGX(float roughness, float dotNH)
{
	float alpha = roughness*roughness;
	float alphaSqr = alpha*alpha;
	float pi = 3.14159;
	float denom = dotNH * dotNH *(alphaSqr-1.0) + 1.0;
	return (alpha / denom) * (alpha / denom) / pi;
}

float calcF(float F0, float dotLH)
{
	float dotLH5 = pow(1.0f-dotLH,5);
	return F0 + (1.0-F0)*(dotLH5);
}

float calcG_Schlick(float roughness, float dotNV, float dotNL)
{
	// UE4
	float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
	// float k = roughness * roughness / 2.0;

	float gV = dotNV*(1.0 - k) + k;
	float gL = dotNL*(1.0 - k) + k;

	return 1.0 / (gV * gL);
}

float calcLightingGGX(vec3 N, vec3 V, vec3 L, float roughness, float F0)
{
	vec3 H = normalize(V+L);

	float dotNL = saturate( dot(N,L) );
	float dotLH = saturate( dot(L,H) );
	float dotNH = saturate( dot(N,H) ) - 0.001;
	float dotNV = saturate( dot(N,V) ) + 0.001;

	float D = calcD_GGX(roughness, dotNH);
	float F = calcF(F0, dotLH);
	float G = calcG_Schlick(roughness, dotNV, dotNL);

	return dotNL * D * F * G / 4.0;
}

vec3 calcDirectionalLightDiffuseColor(vec3 diffuseColor, vec3 normal, vec3 lightDir, float ao)
{
	vec3 color = vec3(0.0,0.0,0.0);

	float NoL = dot(normal,lightDir);
	color.xyz = lightColor.xyz * max(NoL,0.0) * ao / 3.14;
	color.xyz = color.xyz * diffuseColor.xyz;
	return color;
}

#endif

void main()
{
	vec2 uv1 = v_UV1;
	vec2 uv2 = v_UV2;
	vec3 worldPos = v_WorldP;
	vec3 worldNormal = v_WorldN;
	vec3 worldTangent = v_WorldT;
	vec3 worldBinormal = v_WorldB;
	vec3 pixelNormalDir = vec3(0.5, 0.5, 1.0);
)";

static const char g_material_fs_src_suf2_lit[] =
	R"(

	vec3 viewDir = normalize(cameraPosition.xyz - worldPos);
	vec3 diffuse = calcDirectionalLightDiffuseColor(baseColor, pixelNormalDir, lightDirection.xyz, ambientOcclusion);
	vec3 specular = lightColor.xyz * calcLightingGGX(worldNormal, viewDir, lightDirection.xyz, roughness, 0.9);

	vec4 Output =  vec4(metallic * specular + (1.0 - metallic) * diffuse + lightAmbientColor.xyz, opacity);

	if(opacityMask <= 0.0f) discard;

	FRAGCOLOR = Output;


}

)";

static const char g_material_fs_src_suf2_unlit[] =
	R"(

	FRAGCOLOR = vec4(emissive, opacity);
}

)";

static const char g_material_fs_src_suf2_refraction[] =
	R"(
	float airRefraction = 1.0;

	vec3 dir = mat3(cameraMat) * pixelNormalDir;
	vec2 distortUV = dir.xy * (refraction - airRefraction);

	distortUV += v_ScreenUV;
	distortUV.y = mUVInversedBack.x + mUVInversedBack.y * distortUV.y;

	vec4 bg = TEX2D(background, distortUV);
	FRAGCOLOR = bg;
}

)";

std::string Replace(std::string target, std::string from_, std::string to_)
{
	std::string::size_type Pos(target.find(from_));

	while (Pos != std::string::npos)
	{
		target.replace(Pos, from_.length(), to_);
		Pos = target.find(from_, Pos + to_.length());
	}

	return target;
}

struct ShaderData
{
	std::string CodeVS;
	std::string CodePS;
};

ShaderData GenerateShader(Material* material, MaterialShaderType shaderType)
{
	ShaderData shaderData;

	for (int stage = 0; stage < 2; stage++)
	{
		std::ostringstream maincode;

		if (stage == 0)
		{
			if (shaderType == MaterialShaderType::Standard || shaderType == MaterialShaderType::Refraction)
			{
				if (material->GetIsSimpleVertex())
				{
					maincode << g_material_sprite_vs_src_pre_simple;
				}
				else
				{
					maincode << g_material_sprite_vs_src_pre;
				}
			}
			else if (shaderType == MaterialShaderType::Model || shaderType == MaterialShaderType::RefractionModel)
			{
				maincode << g_material_model_vs_src_pre;
			}
		}
		else
		{
			maincode << g_material_fs_src_pre;
		}

		for (int32_t i = 0; i < material->GetUniformCount(); i++)
		{
			auto uniformIndex = material->GetUniformIndex(i);
			auto uniformName = material->GetUniformName(i);

			if (uniformIndex == 0)
				maincode << "uniform float " << uniformName << ";" << std::endl;
			if (uniformIndex == 1)
				maincode << "uniform vec2 " << uniformName << ";" << std::endl;
			if (uniformIndex == 2)
				maincode << "uniform vec3 " << uniformName << ";" << std::endl;
			if (uniformIndex == 3)
				maincode << "uniform vec4 " << uniformName << ";" << std::endl;
		}

		for (size_t i = 0; i < material->GetTextureCount(); i++)
		{
			auto textureIndex = material->GetTextureIndex(i);
			auto textureName = material->GetTextureName(i);

			maincode << "uniform sampler2D " << textureName << ";" << std::endl;
		}

		for (size_t i = material->GetTextureCount(); i < material->GetTextureCount() + 1; i++)
		{
			maincode << "uniform sampler2D "
					 << "background"
					 << ";" << std::endl;
		}

		if (material->GetShadingModel() == ::Effekseer::ShadingModelType::Lit)
		{
			maincode << "uniform vec4 "
					 << "lightDirection"
					 << ";" << std::endl;
			maincode << "uniform vec4 "
					 << "lightColor"
					 << ";" << std::endl;
			maincode << "uniform vec4 "
					 << "lightAmbientColor"
					 << ";" << std::endl;
			maincode << "uniform vec4 "
					 << "cameraPosition"
					 << ";" << std::endl;

			maincode << "#define __MATERIAL_LIT__ 1" << std::endl;
		}
		else if (material->GetShadingModel() == ::Effekseer::ShadingModelType::Unlit)
		{
		}

		if (material->GetHasRefraction() && stage == 1 &&
			(shaderType == MaterialShaderType::Refraction || shaderType == MaterialShaderType::RefractionModel))
		{
			maincode << "uniform mat4 "
					 << "cameraMat"
					 << ";" << std::endl;
		}

		auto baseCode = std::string(material->GetGenericCode());
		baseCode = Replace(baseCode, "$F1$", "float");
		baseCode = Replace(baseCode, "$F2$", "vec2");
		baseCode = Replace(baseCode, "$F3$", "vec3");
		baseCode = Replace(baseCode, "$F4$", "vec4");
		baseCode = Replace(baseCode, "$TIME$", "predefined_uniform.x");
		baseCode = Replace(baseCode, "$UV$", "uv");
		baseCode = Replace(baseCode, "$MOD", "mod");
		baseCode = Replace(baseCode, "$SUFFIX", "");

		// replace textures
		for (size_t i = 0; i < material->GetTextureCount(); i++)
		{
			auto textureIndex = material->GetTextureIndex(i);
			auto textureName = std::string(material->GetTextureName(i));

			std::string keyP = "$TEX_P" + std::to_string(textureIndex) + "$";
			std::string keyS = "$TEX_S" + std::to_string(textureIndex) + "$";

			baseCode = Replace(baseCode, keyP, "TEX2D(" + textureName + ",");
			baseCode = Replace(baseCode, keyS, ")");
		}

		if (stage == 0)
		{
			if (shaderType == MaterialShaderType::Standard || shaderType == MaterialShaderType::Refraction)
			{
				if (material->GetIsSimpleVertex())
				{
					maincode << g_material_sprite_vs_src_suf1_simple;
				}
				else
				{
					maincode << g_material_sprite_vs_src_suf1;
				}
			}
			else if (shaderType == MaterialShaderType::Model || shaderType == MaterialShaderType::RefractionModel)
			{
				maincode << g_material_model_vs_src_suf1;
			}

			maincode << baseCode;

			if (shaderType == MaterialShaderType::Standard || shaderType == MaterialShaderType::Refraction)
			{
				maincode << g_material_sprite_vs_src_suf2;
			}
			else if (shaderType == MaterialShaderType::Model || shaderType == MaterialShaderType::RefractionModel)
			{
				maincode << g_material_model_vs_src_suf2;
			}

			shaderData.CodeVS = maincode.str();
		}
		else
		{
			maincode << g_material_fs_src_suf1;

			maincode << baseCode;

			if (shaderType == MaterialShaderType::Refraction || shaderType == MaterialShaderType::RefractionModel)
			{
				maincode << g_material_fs_src_suf2_refraction;
			}
			else
			{
				if (material->GetShadingModel() == Effekseer::ShadingModelType::Lit)
				{
					maincode << g_material_fs_src_suf2_lit;
				}
				else if (material->GetShadingModel() == Effekseer::ShadingModelType::Unlit)
				{
					maincode << g_material_fs_src_suf2_unlit;
				}
			}

			shaderData.CodePS = maincode.str();
		}
	}
	return shaderData;
}

} // namespace GL

} // namespace Effekseer

namespace Effekseer
{

class CompiledMaterialBinaryGL : public CompiledMaterialBinary, ReferenceObject
{
private:
	std::array<std::vector<uint8_t>, static_cast<int32_t>(MaterialShaderType::Max)> vertexShaders_;

	std::array<std::vector<uint8_t>, static_cast<int32_t>(MaterialShaderType::Max)> pixelShaders_;

public:
	CompiledMaterialBinaryGL() {}

	virtual ~CompiledMaterialBinaryGL() {}

	void SetVertexShaderData(MaterialShaderType type, const std::vector<uint8_t>& data)
	{
		vertexShaders_.at(static_cast<int>(type)) = data;
	}

	void SetPixelShaderData(MaterialShaderType type, const std::vector<uint8_t>& data) { pixelShaders_.at(static_cast<int>(type)) = data; }

	const uint8_t* GetVertexShaderData(MaterialShaderType type) const override { return vertexShaders_.at(static_cast<int>(type)).data(); }

	int32_t GetVertexShaderSize(MaterialShaderType type) const override { return vertexShaders_.at(static_cast<int>(type)).size(); }

	const uint8_t* GetPixelShaderData(MaterialShaderType type) const override { return pixelShaders_.at(static_cast<int>(type)).data(); }

	int32_t GetPixelShaderSize(MaterialShaderType type) const override { return pixelShaders_.at(static_cast<int>(type)).size(); }

	int AddRef() override { return ReferenceObject::AddRef(); }

	int Release() override { return ReferenceObject::Release(); }

	int GetRef() override { return ReferenceObject::GetRef(); }
};

CompiledMaterialBinary* MaterialCompilerGL::Compile(Material* material)
{
	auto binary = new CompiledMaterialBinaryGL();

	auto convertToVector = [](const std::string& str) -> std::vector<uint8_t> {
		std::vector<uint8_t> ret;
		ret.resize(str.size() + 1);
		memcpy(ret.data(), str.data(), str.size());
		ret[str.size()] = 0;
		return ret;
	};

	auto saveBinary = [&material, &binary, &convertToVector](MaterialShaderType type) {
		auto shader = GL::GenerateShader(material, type);
		binary->SetVertexShaderData(type, convertToVector(shader.CodeVS));
		binary->SetPixelShaderData(type, convertToVector(shader.CodePS));
	};

	if (material->GetHasRefraction())
	{
		saveBinary(MaterialShaderType::Refraction);
		saveBinary(MaterialShaderType::RefractionModel);
	}

	saveBinary(MaterialShaderType::Standard);
	saveBinary(MaterialShaderType::Model);

	return binary;
}

} // namespace Effekseer

#ifdef __SHARED_OBJECT__

extern "C"
{
#ifdef _WIN32
#define EFK_EXPORT __declspec(dllexport)
#else
#define EFK_EXPORT
#endif

	EFK_EXPORT Effekseer::MaterialCompiler* EFK_STDCALL CreateCompiler() { return new Effekseer::MaterialCompilerGL(); }
}
#endif