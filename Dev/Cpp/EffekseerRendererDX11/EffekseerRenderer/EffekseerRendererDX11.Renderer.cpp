﻿
//----------------------------------------------------------------------------------
// Include
//----------------------------------------------------------------------------------
#include "EffekseerRendererDX11.Renderer.h"
#include "EffekseerRendererDX11.RendererImplemented.h"
#include "EffekseerRendererDX11.RenderState.h"

#include "EffekseerRendererDX11.Shader.h"
#include "EffekseerRendererDX11.VertexBuffer.h"
#include "EffekseerRendererDX11.IndexBuffer.h"
#include "EffekseerRendererDX11.DeviceObject.h"
//#include "EffekseerRendererDX11.SpriteRenderer.h"
//#include "EffekseerRendererDX11.RibbonRenderer.h"
//#include "EffekseerRendererDX11.RingRenderer.h"
#include "EffekseerRendererDX11.ModelRenderer.h"
//#include "EffekseerRendererDX11.TrackRenderer.h"
#include "EffekseerRendererDX11.TextureLoader.h"
#include "EffekseerRendererDX11.ModelLoader.h"
#include "EffekseerRendererDX11.MaterialLoader.h"

#include "../../EffekseerRendererCommon/EffekseerRenderer.SpriteRendererBase.h"
#include "../../EffekseerRendererCommon/EffekseerRenderer.RibbonRendererBase.h"
#include "../../EffekseerRendererCommon/EffekseerRenderer.RingRendererBase.h"
#include "../../EffekseerRendererCommon/EffekseerRenderer.TrackRendererBase.h"
#include "../../EffekseerRendererCommon/EffekseerRenderer.Renderer_Impl.h"
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
#include "../../EffekseerRendererCommon/EffekseerRenderer.PngTextureLoader.h"
#endif

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
namespace EffekseerRendererDX11
{

namespace Standard_VS
{
	static
#include "Shader/EffekseerRenderer.Standard_VS.h"
}

//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
namespace Standard_PS
{
	static
#include "Shader/EffekseerRenderer.Standard_PS.h"
}

//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
namespace Standard_Distortion_VS
{
	static
#include "Shader/EffekseerRenderer.Standard_Distortion_VS.h"
}

//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
namespace Standard_Distortion_PS
{
	static
#include "Shader/EffekseerRenderer.Standard_Distortion_PS.h"
}

namespace Standard_Lighting_VS
{
static
#include "Shader/EffekseerRenderer.Standard_Lighting_VS.h"
} // namespace Standard_Distortion_VS

namespace Standard_Lighting_PS
{
static
#include "Shader/EffekseerRenderer.Standard_Lighting_PS.h"
} // namespace Standard_Distortion_PS


//-----------------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------------
::Effekseer::TextureLoader* CreateTextureLoader(ID3D11Device* device, ID3D11DeviceContext* context, ::Effekseer::FileInterface* fileInterface)
{
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
	return new TextureLoader(device, context, fileInterface);
#else
	return NULL;
#endif
}

::Effekseer::ModelLoader* CreateModelLoader(ID3D11Device* device, ::Effekseer::FileInterface* fileInterface)
{
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
	return new ModelLoader(device, fileInterface);
#else
	return NULL;
#endif
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
OriginalState::OriginalState()
	: m_blendState		( NULL )
	, m_depthStencilState	( NULL )
	, m_depthStencilStateRef	( 0 )
	, m_vertexConstantBuffer	( NULL )
	, m_pixelConstantBuffer		( NULL )
	, m_layout					( NULL )

	, m_pRasterizerState(nullptr)
	, m_pVS(nullptr)
	, m_pPS(nullptr)
	, m_pVB(nullptr)
	, m_pIB(nullptr)

{
	for( int32_t i = 0; i < 4; i++ )
	{
		m_samplers[i] = NULL;
	}

	for (int32_t i = 0; i < 4; i++)
	{
		m_psSRVs[i] = nullptr;
	}
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
OriginalState::~OriginalState()
{
	ReleaseState();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void OriginalState::SaveState(ID3D11Device* device, ID3D11DeviceContext* context )
{
	context->PSGetSamplers( 0, 4, m_samplers );
	context->OMGetBlendState( &m_blendState, m_blendFactor, &m_blendSampleMask );
	context->OMGetDepthStencilState( &m_depthStencilState, &m_depthStencilStateRef );
	context->RSGetState(&m_pRasterizerState);

	context->VSGetConstantBuffers(0, 1, &m_vertexConstantBuffer);
	context->PSGetConstantBuffers(0, 1, &m_pixelConstantBuffer);

	context->VSGetShader(&m_pVS, nullptr, nullptr);
	context->PSGetShader(&m_pPS, nullptr, nullptr);

	context->IAGetInputLayout( &m_layout );

	context->IAGetPrimitiveTopology(&m_topology);

	context->PSGetShaderResources(0, 4, m_psSRVs);

	context->IAGetVertexBuffers(0, 1, &m_pVB, &m_vbStrides, &m_vbOffset);
	context->IAGetIndexBuffer(&m_pIB, &m_ibFormat, &m_ibOffset);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void OriginalState::LoadState(ID3D11Device* device, ID3D11DeviceContext* context )
{
	context->PSSetSamplers(0, 4, m_samplers );
	context->OMSetBlendState( m_blendState, m_blendFactor, m_blendSampleMask );
	context->OMSetDepthStencilState( m_depthStencilState, m_depthStencilStateRef );
	context->RSSetState(m_pRasterizerState);

	context->VSSetConstantBuffers(0, 1, &m_vertexConstantBuffer);
	context->PSSetConstantBuffers(0, 1, &m_pixelConstantBuffer);

	context->VSSetShader(m_pVS, NULL, 0);
	context->PSSetShader(m_pPS, NULL, 0);

	context->IASetInputLayout( m_layout );

	context->IASetPrimitiveTopology(m_topology);

	context->PSSetShaderResources(0, 4, m_psSRVs);

	context->IASetVertexBuffers(0, 1, &m_pVB, &m_vbStrides, &m_vbOffset);
	context->IASetIndexBuffer(m_pIB, m_ibFormat, m_ibOffset);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void OriginalState::ReleaseState()
{	
	for( int32_t i = 0; i < 4; i++ )
	{
		ES_SAFE_RELEASE( m_samplers[i] );
	}
	ES_SAFE_RELEASE( m_blendState );

	ES_SAFE_RELEASE( m_depthStencilState );

	ES_SAFE_RELEASE(m_pRasterizerState);

	ES_SAFE_RELEASE( m_vertexConstantBuffer );
	ES_SAFE_RELEASE( m_pixelConstantBuffer );

	ES_SAFE_RELEASE(m_pVS);
	ES_SAFE_RELEASE(m_pPS);

	ES_SAFE_RELEASE( m_layout );

	for (int32_t i = 0; i < 4; i++)
	{
		ES_SAFE_RELEASE(m_psSRVs[i]);
	}

	ES_SAFE_RELEASE(m_pVB);
	ES_SAFE_RELEASE(m_pIB);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
Renderer* Renderer::Create(ID3D11Device* device, ID3D11DeviceContext* context, int32_t squareMaxCount, D3D11_COMPARISON_FUNC depthFunc)
{
	RendererImplemented* renderer = new RendererImplemented( squareMaxCount );
	if( renderer->Initialize( device, context, depthFunc) )
	{
		return renderer;
	}
	return NULL;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
RendererImplemented::RendererImplemented( int32_t squareMaxCount )
	: m_device	( NULL )
	, m_context	( NULL )
	, m_vertexBuffer( NULL )
	, m_indexBuffer	( NULL )
	, m_squareMaxCount	( squareMaxCount )
	, m_coordinateSystem	( ::Effekseer::CoordinateSystem::RH )
	, m_renderState		( NULL )
	, m_restorationOfStates( true )

	, m_shader(nullptr)
	, m_shader_distortion(nullptr)
	, m_standardRenderer(nullptr)
	, m_distortingCallback(nullptr)
{
	::Effekseer::Vector3D direction( 1.0f, 1.0f, 1.0f );
	SetLightDirection( direction );
	::Effekseer::Color lightColor( 255, 255, 255, 255 );
	SetLightColor( lightColor );
	::Effekseer::Color lightAmbient( 0, 0, 0, 0 );
	SetLightAmbientColor( lightAmbient );

	m_background.UserPtr = nullptr;

	m_state = new OriginalState();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
RendererImplemented::~RendererImplemented()
{
	GetImpl()->DeleteProxyTextures(this);

	assert(GetRef() == 0);

	ES_SAFE_DELETE(m_distortingCallback);

	auto p = (ID3D11ShaderResourceView*)m_background.UserPtr;
	ES_SAFE_RELEASE(p);

	ES_SAFE_DELETE(m_standardRenderer);
	ES_SAFE_DELETE(m_shader);

	ES_SAFE_DELETE(m_shader_distortion);
	ES_SAFE_DELETE(m_shader_lighting);

	ES_SAFE_DELETE( m_state );

	ES_SAFE_DELETE( m_renderState );
	ES_SAFE_DELETE( m_vertexBuffer );
	ES_SAFE_DELETE( m_indexBuffer );
	ES_SAFE_DELETE(m_indexBufferForWireframe);

	assert(GetRef() == -6);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::OnLostDevice()
{
	for (auto& device : m_deviceObjects)
	{
		device->OnLostDevice();
	}
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::OnResetDevice()
{
	for (auto& device : m_deviceObjects)
	{
		device->OnResetDevice();
	}
}


//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
bool RendererImplemented::Initialize(ID3D11Device* device, ID3D11DeviceContext* context, D3D11_COMPARISON_FUNC depthFunc)
{
	m_device = device;
	m_context = context;
	m_depthFunc = depthFunc;

	// 頂点の生成
	{
		// 最大でfloat * 10 と仮定
		m_vertexBuffer = VertexBuffer::Create( this, sizeof(float) * 10 * m_squareMaxCount * 4, true );
		if( m_vertexBuffer == NULL ) return false;
	}

	// 参照カウントの調整
	Release();

	// インデックスの生成
	{
		m_indexBuffer = IndexBuffer::Create( this, m_squareMaxCount * 6, false );
		if( m_indexBuffer == NULL ) return false;

		m_indexBuffer->Lock();

		// ( 標準設定で　DirectX 時計周りが表, OpenGLは反時計回りが表 )
		for( int i = 0; i < m_squareMaxCount; i++ )
		{
			uint16_t* buf = (uint16_t*)m_indexBuffer->GetBufferDirect( 6 );
			buf[0] = 3 + 4 * i;
			buf[1] = 1 + 4 * i;
			buf[2] = 0 + 4 * i;
			buf[3] = 3 + 4 * i;
			buf[4] = 0 + 4 * i;
			buf[5] = 2 + 4 * i;
		}

		m_indexBuffer->Unlock();
	}

	// 参照カウントの調整
	Release();

	// Generate index buffer for rendering wireframes
	{
		m_indexBufferForWireframe = IndexBuffer::Create(this, m_squareMaxCount * 8, false);
		if (m_indexBufferForWireframe == NULL) return false;

		m_indexBufferForWireframe->Lock();

		for (int i = 0; i < m_squareMaxCount; i++)
		{
			uint16_t* buf = (uint16_t*)m_indexBufferForWireframe->GetBufferDirect(8);
			buf[0] = 0 + 4 * i;
			buf[1] = 1 + 4 * i;
			buf[2] = 2 + 4 * i;
			buf[3] = 3 + 4 * i;
			buf[4] = 0 + 4 * i;
			buf[5] = 2 + 4 * i;
			buf[6] = 1 + 4 * i;
			buf[7] = 3 + 4 * i;
		}

		m_indexBufferForWireframe->Unlock();
	}

	// 参照カウントの調整
	Release();

	m_renderState = new RenderState(this, m_depthFunc);


	// シェーダー
	// 座標(3) 色(1) UV(2)
	D3D11_INPUT_ELEMENT_DESC decl [] = {
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, sizeof(float) * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 4, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	D3D11_INPUT_ELEMENT_DESC decl_distortion [] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, sizeof(float) * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 4, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 6, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 2, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(float) * 9, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	D3D11_INPUT_ELEMENT_DESC decl_lighting[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, sizeof(float) * 3, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 1, DXGI_FORMAT_R8G8B8A8_UNORM, 0, sizeof(float) * 4, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 2, DXGI_FORMAT_R8G8B8A8_UNORM, 0, sizeof(float) * 5, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 6, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 8, D3D11_INPUT_PER_VERTEX_DATA, 0},

	};

	m_shader = Shader::Create(
		this,
		Standard_VS::g_VS,
		sizeof(Standard_VS::g_VS),
		Standard_PS::g_PS,
		sizeof(Standard_PS::g_PS),
		"StandardRenderer", decl, ARRAYSIZE(decl));
	if (m_shader == NULL) return false;

	// 参照カウントの調整
	Release();

	m_shader_distortion = Shader::Create(
		this,
		Standard_Distortion_VS::g_VS,
		sizeof(Standard_Distortion_VS::g_VS),
		Standard_Distortion_PS::g_PS,
		sizeof(Standard_Distortion_PS::g_PS),
		"StandardRenderer Distortion", decl_distortion, ARRAYSIZE(decl_distortion));
	if (m_shader_distortion == NULL) return false;

	// 参照カウントの調整
	Release();

	m_shader->SetVertexConstantBufferSize(sizeof(Effekseer::Matrix44) * 2 + sizeof(float) * 4);
	m_shader->SetVertexRegisterCount(8 + 1);

	m_shader_distortion->SetVertexConstantBufferSize(sizeof(Effekseer::Matrix44) * 2 + sizeof(float) * 4);
	m_shader_distortion->SetVertexRegisterCount(8 + 1);

	m_shader_distortion->SetPixelConstantBufferSize(sizeof(float) * 4 + sizeof(float) * 4);
	m_shader_distortion->SetPixelRegisterCount(1 + 1);

	m_shader_lighting = Shader::Create(this,
									   Standard_Lighting_VS::g_VS,
									   sizeof(Standard_Lighting_VS::g_VS),
									   Standard_Lighting_PS::g_PS,
									   sizeof(Standard_Lighting_PS::g_PS),
										 "StandardRenderer Lighting",
									   decl_lighting,
									   ARRAYSIZE(decl_lighting));
	if (m_shader_lighting == NULL)
		return false;

	m_shader_lighting->SetVertexConstantBufferSize(sizeof(Effekseer::Matrix44) * 2 + sizeof(float) * 4);
	m_shader_lighting->SetVertexRegisterCount(8 + 1);

	m_shader_lighting->SetPixelConstantBufferSize(sizeof(float) * 4 * 3);
	m_shader_lighting->SetPixelRegisterCount(12);
	
	Release();

	m_standardRenderer = new EffekseerRenderer::StandardRenderer<RendererImplemented, Shader, Vertex, VertexDistortion>(
		this, m_shader, m_shader_distortion);

	GetImpl()->CreateProxyTextures(this);

	return true;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::Destroy()
{
	Release();
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetRestorationOfStatesFlag(bool flag)
{
	m_restorationOfStates = flag;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
bool RendererImplemented::BeginRendering()
{
	assert( m_device != NULL );

	::Effekseer::Matrix44::Mul( m_cameraProj, m_camera, m_proj );
	
	// ステートを保存する
	if( m_restorationOfStates )
	{
		m_state->SaveState( m_device, m_context );
	}

	// ステート初期設定
	m_renderState->GetActiveState().Reset();
	m_renderState->Update( true );

	// レンダラーリセット
	m_standardRenderer->ResetAndRenderingIfRequired();

	return true;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
bool RendererImplemented::EndRendering()
{
	assert( m_device != NULL );
	
	// レンダラーリセット
	m_standardRenderer->ResetAndRenderingIfRequired();

	// ステートを復元する
	if( m_restorationOfStates )
	{
		m_state->LoadState( m_device, m_context );
		m_state->ReleaseState();
	}

	return true;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
ID3D11Device* RendererImplemented::GetDevice()
{
	return m_device;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
ID3D11DeviceContext* RendererImplemented::GetContext()
{
	return m_context;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
VertexBuffer* RendererImplemented::GetVertexBuffer()
{
	return m_vertexBuffer;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
IndexBuffer* RendererImplemented::GetIndexBuffer()
{
	if (GetRenderMode() == ::Effekseer::RenderMode::Wireframe)
	{
		return m_indexBufferForWireframe;
	}
	else
	{
		return m_indexBuffer;
	}
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
int32_t RendererImplemented::GetSquareMaxCount() const
{
	return m_squareMaxCount;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::EffekseerRenderer::RenderStateBase* RendererImplemented::GetRenderState()
{
	return m_renderState;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
const ::Effekseer::Vector3D& RendererImplemented::GetLightDirection() const
{
	return m_lightDirection;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetLightDirection( const ::Effekseer::Vector3D& direction )
{
	m_lightDirection = direction;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
const ::Effekseer::Color& RendererImplemented::GetLightColor() const
{
	return m_lightColor;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetLightColor( const ::Effekseer::Color& color )
{
	m_lightColor = color;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
const ::Effekseer::Color& RendererImplemented::GetLightAmbientColor() const
{
	return m_lightAmbient;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetLightAmbientColor( const ::Effekseer::Color& color )
{
	m_lightAmbient = color;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
const ::Effekseer::Matrix44& RendererImplemented::GetProjectionMatrix() const
{
	return m_proj;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetProjectionMatrix( const ::Effekseer::Matrix44& mat )
{
	m_proj = mat;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
const ::Effekseer::Matrix44& RendererImplemented::GetCameraMatrix() const
{
	return m_camera;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetCameraMatrix(const ::Effekseer::Matrix44& mat)
{
	m_cameraFrontDirection = ::Effekseer::Vector3D(mat.Values[0][2], mat.Values[1][2], mat.Values[2][2]);

	auto localPos = ::Effekseer::Vector3D(-mat.Values[3][0], -mat.Values[3][1], -mat.Values[3][2]);
	auto f = m_cameraFrontDirection;
	auto r = ::Effekseer::Vector3D(mat.Values[0][0], mat.Values[1][0], mat.Values[2][0]);
	auto u = ::Effekseer::Vector3D(mat.Values[0][1], mat.Values[1][1], mat.Values[2][1]);

	m_cameraPosition = r * localPos.X + u * localPos.Y + f * localPos.Z;

	m_camera = mat;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::Matrix44& RendererImplemented::GetCameraProjectionMatrix()
{
	return m_cameraProj;
}

::Effekseer::Vector3D RendererImplemented::GetCameraFrontDirection() const
{
	return m_cameraFrontDirection;
}

::Effekseer::Vector3D RendererImplemented::GetCameraPosition() const
{
	return m_cameraPosition;
}

void RendererImplemented::SetCameraParameter(const ::Effekseer::Vector3D& front, const ::Effekseer::Vector3D& position)
{
	m_cameraFrontDirection = front;
	m_cameraPosition = position;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::SpriteRenderer* RendererImplemented::CreateSpriteRenderer()
{
	return new ::EffekseerRenderer::SpriteRendererBase<RendererImplemented, Vertex, VertexDistortion>(this);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::RibbonRenderer* RendererImplemented::CreateRibbonRenderer()
{
	return new ::EffekseerRenderer::RibbonRendererBase<RendererImplemented, Vertex, VertexDistortion>( this );
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::RingRenderer* RendererImplemented::CreateRingRenderer()
{
	return new ::EffekseerRenderer::RingRendererBase<RendererImplemented, Vertex, VertexDistortion>(this);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::ModelRenderer* RendererImplemented::CreateModelRenderer()
{
	return ModelRenderer::Create( this );
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::TrackRenderer* RendererImplemented::CreateTrackRenderer()
{
	return new ::EffekseerRenderer::TrackRendererBase<RendererImplemented, Vertex, VertexDistortion>(this);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::TextureLoader* RendererImplemented::CreateTextureLoader( ::Effekseer::FileInterface* fileInterface )
{
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
	return new TextureLoader(this->GetDevice(), this->GetContext(), fileInterface );
#else
	return NULL;
#endif
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
::Effekseer::ModelLoader* RendererImplemented::CreateModelLoader( ::Effekseer::FileInterface* fileInterface )
{
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
	return new ModelLoader(this->GetDevice(), fileInterface );
#else
	return NULL;
#endif
}

::Effekseer::MaterialLoader* RendererImplemented::CreateMaterialLoader(::Effekseer::FileInterface* fileInterface) { 
#ifdef __EFFEKSEER_RENDERER_INTERNAL_LOADER__
	return new MaterialLoader(this, fileInterface);
#else
	return nullptr;
#endif
}


Effekseer::TextureData* RendererImplemented::GetBackground()
{
	if (m_background.UserPtr == nullptr) return nullptr;
	return &m_background;
}

void RendererImplemented::SetBackground(ID3D11ShaderResourceView* background)
{
	ES_SAFE_ADDREF(background);

	auto p = (ID3D11ShaderResourceView*)m_background.UserPtr;
	ES_SAFE_RELEASE(p);
	m_background.UserPtr = background;
}

EffekseerRenderer::DistortingCallback* RendererImplemented::GetDistortingCallback()
{
	return m_distortingCallback;
}

void RendererImplemented::SetDistortingCallback(EffekseerRenderer::DistortingCallback* callback)
{
	ES_SAFE_DELETE(m_distortingCallback);
	m_distortingCallback = callback;
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetVertexBuffer(ID3D11Buffer* vertexBuffer, int32_t size)
{
	ID3D11Buffer* vBuf = vertexBuffer;
	uint32_t vertexSize = size;
	uint32_t offset = 0;
	GetContext()->IASetVertexBuffers( 0, 1, &vBuf, &vertexSize, &offset ); 
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetVertexBuffer( VertexBuffer* vertexBuffer, int32_t size )
{
	ID3D11Buffer* vBuf = vertexBuffer->GetInterface();
	uint32_t vertexSize = size;
	uint32_t offset = 0;
	GetContext()->IASetVertexBuffers( 0, 1, &vBuf, &vertexSize, &offset ); 
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetIndexBuffer( IndexBuffer* indexBuffer )
{
	GetContext()->IASetIndexBuffer( indexBuffer->GetInterface(), DXGI_FORMAT_R16_UINT, 0);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetIndexBuffer(ID3D11Buffer* indexBuffer)
{
	GetContext()->IASetIndexBuffer( indexBuffer, DXGI_FORMAT_R32_UINT, 0);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetLayout( Shader* shader )
{
	if (GetRenderMode() == Effekseer::RenderMode::Normal)
	{
		GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	else
	{
		GetContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	}

	GetContext()->IASetInputLayout( shader->GetLayoutInterface() );
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::DrawSprites( int32_t spriteCount, int32_t vertexOffset )
{
	impl->drawcallCount++;
	impl->drawvertexCount += spriteCount * 4;

	if (GetRenderMode() == Effekseer::RenderMode::Normal)
	{
		GetContext()->DrawIndexed(
			spriteCount * 2 * 3,
			0,
			vertexOffset);
	}
	else
	{
		GetContext()->DrawIndexed(
			spriteCount * 2 * 4,
			0,
			vertexOffset);
	}
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::DrawPolygon( int32_t vertexCount, int32_t indexCount)
{
	impl->drawcallCount++;
	impl->drawvertexCount += vertexCount;

	GetContext()->DrawIndexed(
		indexCount,
		0,
		0 );
}

Shader* RendererImplemented::GetShader(bool useTexture, ::Effekseer::RendererMaterialType materialType) const
{
	if (materialType == ::Effekseer::RendererMaterialType::BackDistortion)
	{
		if (useTexture && GetRenderMode() == Effekseer::RenderMode::Normal)
		{
			return m_shader_distortion;
		}
		else
		{
			return m_shader_distortion;
		}
	}
	else if (materialType == ::Effekseer::RendererMaterialType::Lighting)
	{
		if (useTexture && GetRenderMode() == Effekseer::RenderMode::Normal)
		{
			return m_shader_lighting;
		}
		else
		{
			return m_shader_lighting;
		}
	}
	else
	{
		if (useTexture && GetRenderMode() == Effekseer::RenderMode::Normal)
		{
			return m_shader;
		}
		else
		{
			return m_shader;
		}
	}
}

void RendererImplemented::BeginShader(Shader* shader)
{
	currentShader = shader;
	GetContext()->VSSetShader( shader->GetVertexShader(), NULL, 0);
	GetContext()->PSSetShader( shader->GetPixelShader(), NULL, 0);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::EndShader(Shader* shader)
{
	currentShader = nullptr;
}

void RendererImplemented::SetVertexBufferToShader(const void* data, int32_t size, int32_t dstOffset)
{
	assert(currentShader != nullptr);
	auto p = static_cast<uint8_t*>(currentShader->GetVertexConstantBuffer()) + dstOffset;
	memcpy(p, data, size);
}

void RendererImplemented::SetPixelBufferToShader(const void* data, int32_t size, int32_t dstOffset)
{
	assert(currentShader != nullptr);
	auto p = static_cast<uint8_t*>(currentShader->GetPixelConstantBuffer()) + dstOffset;
	memcpy(p, data, size);
}

//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
void RendererImplemented::SetTextures(Shader* shader, Effekseer::TextureData** textures, int32_t count)
{
	ID3D11ShaderResourceView* srv[4];
	for (int32_t i = 0; i < count; i++)
	{
		if (textures[i] == nullptr)
		{
			srv[i] = nullptr;
		}
		else
		{
			srv[i] = (ID3D11ShaderResourceView*)textures[i]->UserPtr;
		}
	}
	GetContext()->PSSetShaderResources(0, count, srv);
}

void RendererImplemented::ResetRenderState()
{
	m_renderState->GetActiveState().Reset();
	m_renderState->Update( true );
}

Effekseer::TextureData* RendererImplemented::CreateProxyTexture(EffekseerRenderer::ProxyTextureType type)
{
	std::array<uint8_t, 4> buf;

	if (type == EffekseerRenderer::ProxyTextureType::White)
	{
		buf.fill(255);
	}
	else if (type == EffekseerRenderer::ProxyTextureType::Normal)
	{
		buf.fill(127);
		buf[2] = 255;
		buf[3] = 255;
	}
	else
	{
		assert(0);
	}

	ID3D11Texture2D* tex = nullptr;

	D3D11_TEXTURE2D_DESC TexDesc{};
	TexDesc.Width = 1;
	TexDesc.Height = 1;
	TexDesc.ArraySize = 1;
	TexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	TexDesc.SampleDesc.Count = 1;
	TexDesc.SampleDesc.Quality = 0;
	TexDesc.Usage = D3D11_USAGE_DEFAULT;
	TexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	TexDesc.CPUAccessFlags = 0;
	TexDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = buf.data();
	data.SysMemPitch = TexDesc.Width * 4;
	data.SysMemSlicePitch = TexDesc.Width * TexDesc.Height * 4;

	HRESULT hr = GetDevice()->CreateTexture2D(&TexDesc, nullptr, &tex);

	if (FAILED(hr))
	{
		return nullptr;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
	desc.Format = TexDesc.Format;
	desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = -1;

	ID3D11ShaderResourceView* texture = nullptr;
	hr = GetDevice()->CreateShaderResourceView(tex, &desc, &texture);
	if (FAILED(hr))
	{
		ES_SAFE_RELEASE(texture);
		return nullptr;
	}

	GetContext()->UpdateSubresource(tex, 0, 0, buf.data(), data.SysMemPitch, 0);

	ES_SAFE_RELEASE(tex);

	// Generate mipmap
	GetContext()->GenerateMips(texture);

	auto textureData = new Effekseer::TextureData();
	textureData->UserPtr = texture;
	textureData->UserID = 0;
	textureData->TextureFormat = Effekseer::TextureFormatType::ABGR8;
	textureData->Width = TexDesc.Width;
	textureData->Height = TexDesc.Height;

	return textureData;
}

void RendererImplemented::DeleteProxyTexture(Effekseer::TextureData* data)
{
	if (data != nullptr && data->UserPtr != nullptr)
	{
		auto texture = (ID3D11ShaderResourceView*)data->UserPtr;
		texture->Release();
	}

	if (data != nullptr)
	{
		delete data;
	}
}


//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
}
//----------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------
