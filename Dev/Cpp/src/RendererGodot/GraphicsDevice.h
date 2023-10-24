
#ifndef __EFFEKSEER_GODOT_GRAPHICS_DEVICE_H__
#define __EFFEKSEER_GODOT_GRAPHICS_DEVICE_H__

#include <Effekseer.h>
#include <assert.h>
#include <functional>
#include <set>

namespace EffekseerGodot
{
namespace Backend
{

class GraphicsDevice;
class VertexBuffer;
class IndexBuffer;
class UniformBuffer;

using GraphicsDeviceRef = Effekseer::RefPtr<GraphicsDevice>;
using VertexBufferRef = Effekseer::RefPtr<VertexBuffer>;
using IndexBufferRef = Effekseer::RefPtr<IndexBuffer>;
using UniformBufferRef = Effekseer::RefPtr<UniformBuffer>;

class VertexBuffer
	: public Effekseer::Backend::VertexBuffer
{
private:
	std::vector<uint8_t> resources_;
	GraphicsDevice* graphicsDevice_ = nullptr;
	int32_t size_ = 0;
	bool isDynamic_ = false;

public:
	VertexBuffer(GraphicsDevice* graphicsDevice);

	~VertexBuffer() override;

	bool Allocate(int32_t size, bool isDynamic);

	void Deallocate();

	bool Init(int32_t size, bool isDynamic);

	void UpdateData(const void* src, int32_t size, int32_t offset);

	const uint8_t* Refer() const { return resources_.data(); }
};

class IndexBuffer
	: public Effekseer::Backend::IndexBuffer
{
private:
	std::vector<uint8_t> resources_;
	GraphicsDevice* graphicsDevice_ = nullptr;
	int32_t stride_ = 0;

public:
	IndexBuffer(GraphicsDevice* graphicsDevice);

	~IndexBuffer() override;

	bool Allocate(int32_t elementCount, int32_t stride);

	void Deallocate();

	bool Init(int32_t elementCount, int32_t stride);

	void UpdateData(const void* src, int32_t size, int32_t offset);

	const uint8_t* Refer() const { return resources_.data(); }
};

class UniformBuffer
	: public Effekseer::Backend::UniformBuffer
{
private:
	Effekseer::CustomVector<uint8_t> buffer_;

public:
	UniformBuffer() = default;
	~UniformBuffer() override = default;

	bool Init(int32_t size, const void* initialData);

	const Effekseer::CustomVector<uint8_t>& GetBuffer() const
	{
		return buffer_;
	}

	Effekseer::CustomVector<uint8_t>& GetBuffer()
	{
		return buffer_;
	}

	void UpdateData(const void* src, int32_t size, int32_t offset);
};

class Texture
	: public Effekseer::Backend ::Texture
{
private:
	int32_t target_ = -1;

	GraphicsDevice* graphicsDevice_ = nullptr;
	std::function<void()> onDisposed_;

public:
	Texture(GraphicsDevice* graphicsDevice);
	~Texture() override;

	bool Init(const Effekseer::Backend::TextureParameter& param, const Effekseer::CustomVector<uint8_t>& initialData);

	bool Init(const Effekseer::Backend::RenderTextureParameter& param);

	bool Init(const Effekseer::Backend::DepthTextureParameter& param);

	int32_t GetTarget() const
	{
		return target_;
	}
};

enum class DevicePropertyType
{
	MaxVaryingVectors,
	MaxVertexUniformVectors,
	MaxFragmentUniformVectors,
	MaxVertexTextureImageUnits,
	MaxTextureImageUnits,
};

class GraphicsDevice
	: public Effekseer::Backend::GraphicsDevice
{
public:
	GraphicsDevice();

	~GraphicsDevice() override;

	Effekseer::Backend::VertexBufferRef CreateVertexBuffer(int32_t size, const void* initialData, bool isDynamic) override;

	Effekseer::Backend::IndexBufferRef CreateIndexBuffer(int32_t elementCount, const void* initialData, Effekseer::Backend::IndexBufferStrideType stride) override;

	Effekseer::Backend::TextureRef CreateTexture(const Effekseer::Backend::TextureParameter& param, const Effekseer::CustomVector<uint8_t>& initialData) override;

	Effekseer::Backend::TextureRef CreateRenderTexture(const Effekseer::Backend::RenderTextureParameter& param) override;

	Effekseer::Backend::TextureRef CreateDepthTexture(const Effekseer::Backend::DepthTextureParameter& param) override;

	bool CopyTexture(Effekseer::Backend::TextureRef& dst, Effekseer::Backend::TextureRef& src, const std::array<int, 3>& dstPos, const std::array<int, 3>& srcPos, const std::array<int, 3>& size, int32_t dstLayer, int32_t srcLayer) override;

	Effekseer::Backend::UniformBufferRef CreateUniformBuffer(int32_t size, const void* initialData) override;

	void Draw(const Effekseer::Backend::DrawParameter& drawParam) override;

	void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) override;

	void BeginRenderPass(Effekseer::Backend::RenderPassRef& renderPass, bool isColorCleared, bool isDepthCleared, Effekseer::Color clearColor) override;

	void EndRenderPass() override;

	bool UpdateUniformBuffer(Effekseer::Backend::UniformBufferRef& buffer, int32_t size, int32_t offset, const void* data) override;

	std::string GetDeviceName() const override
	{
		return "Godot";
	}
};

} // namespace Backend
} // namespace EffekseerGodot

#endif