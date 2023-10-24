#include "GraphicsDevice.h"

namespace EffekseerGodot
{
namespace Backend
{

VertexBuffer::VertexBuffer(GraphicsDevice* graphicsDevice)
{
}

VertexBuffer::~VertexBuffer()
{
	Deallocate();
}

bool VertexBuffer::Allocate(int32_t size, bool isDynamic)
{
	resources_.resize(static_cast<size_t>(size));
	return true;
}

void VertexBuffer::Deallocate()
{
}

bool VertexBuffer::Init(int32_t size, bool isDynamic)
{
	size_ = size;
	isDynamic_ = isDynamic;

	return Allocate(size_, isDynamic_);
}

void VertexBuffer::UpdateData(const void* src, int32_t size, int32_t offset)
{
	if (size == 0 || src == nullptr)
	{
		return;
	}

	memcpy(resources_.data() + offset, src, size);
}

IndexBuffer::IndexBuffer(GraphicsDevice* graphicsDevice)
{
}

IndexBuffer::~IndexBuffer()
{
}

bool IndexBuffer::Allocate(int32_t elementCount, int32_t stride)
{
	resources_.resize(elementCount_ * stride_);
	strideType_ = stride == 4 ? Effekseer::Backend::IndexBufferStrideType::Stride4 : Effekseer::Backend::IndexBufferStrideType::Stride2;
	return true;
}

void IndexBuffer::Deallocate()
{
}

bool IndexBuffer::Init(int32_t elementCount, int32_t stride)
{
	elementCount_ = elementCount;
	stride_ = stride;

	return Allocate(elementCount_, stride_);
}

void IndexBuffer::UpdateData(const void* src, int32_t size, int32_t offset)
{
	if (size == 0 || src == nullptr)
	{
		return;
	}

	memcpy(resources_.data() + offset, src, size);
}

bool UniformBuffer::Init(int32_t size, const void* initialData)
{
	buffer_.resize(size);

	if (auto data = static_cast<const uint8_t*>(initialData))
	{
		buffer_.assign(data, data + size);
	}

	return true;
}

void UniformBuffer::UpdateData(const void* src, int32_t size, int32_t offset)
{
	assert(buffer_.size() >= size + offset && offset >= 0);

	if (auto data = static_cast<const uint8_t*>(src))
	{
		memcpy(buffer_.data() + offset, src, size);
	}
}

Texture::Texture(GraphicsDevice* graphicsDevice)
{
}

Texture::~Texture()
{
}

bool Texture::Init(const Effekseer::Backend::TextureParameter& param, const Effekseer::CustomVector<uint8_t>& initialData)
{
	param_ = param;
	return true;
}

bool Texture::Init(const Effekseer::Backend::RenderTextureParameter& param)
{
	Effekseer::Backend::TextureParameter paramInternal;
	paramInternal.Size = {param.Size[0], param.Size[1], 1};
	paramInternal.Format = param.Format;
	paramInternal.MipLevelCount = 1;
	paramInternal.SampleCount = param.SamplingCount;
	paramInternal.Dimension = 2;
	paramInternal.Usage = Effekseer::Backend::TextureUsageType::RenderTarget;
	return Init(paramInternal, {});
}

bool Texture::Init(const Effekseer::Backend::DepthTextureParameter& param)
{
	return true;
}

GraphicsDevice::GraphicsDevice()
{
}

GraphicsDevice::~GraphicsDevice()
{
}

Effekseer::Backend::VertexBufferRef GraphicsDevice::CreateVertexBuffer(int32_t size, const void* initialData, bool isDynamic)
{
	auto ret = Effekseer::MakeRefPtr<VertexBuffer>(this);

	if (!ret->Init(size, isDynamic))
	{
		return nullptr;
	}

	ret->UpdateData(initialData, size, 0);

	return ret;
}

Effekseer::Backend::IndexBufferRef GraphicsDevice::CreateIndexBuffer(int32_t elementCount, const void* initialData, Effekseer::Backend::IndexBufferStrideType stride)
{
	auto ret = Effekseer::MakeRefPtr<IndexBuffer>(this);

	if (!ret->Init(elementCount, stride == Effekseer::Backend::IndexBufferStrideType::Stride4 ? 4 : 2))
	{
		return nullptr;
	}

	ret->UpdateData(initialData, elementCount * (stride == Effekseer::Backend::IndexBufferStrideType::Stride4 ? 4 : 2), 0);

	return ret;
}

Effekseer::Backend::TextureRef GraphicsDevice::CreateTexture(const Effekseer::Backend::TextureParameter& param, const Effekseer::CustomVector<uint8_t>& initialData)
{
	auto ret = Effekseer::MakeRefPtr<Texture>(this);

	if (!ret->Init(param, initialData))
	{
		return nullptr;
	}

	return ret;
}

Effekseer::Backend::TextureRef GraphicsDevice::CreateRenderTexture(const Effekseer::Backend::RenderTextureParameter& param)
{
	auto ret = Effekseer::MakeRefPtr<Texture>(this);

	if (!ret->Init(param))
	{
		return nullptr;
	}

	return ret;
}

Effekseer::Backend::TextureRef GraphicsDevice::CreateDepthTexture(const Effekseer::Backend::DepthTextureParameter& param)
{
	auto ret = Effekseer::MakeRefPtr<Texture>(this);

	if (!ret->Init(param))
	{
		return nullptr;
	}

	return ret;
}

bool GraphicsDevice::CopyTexture(Effekseer::Backend::TextureRef& dst, Effekseer::Backend::TextureRef& src, const std::array<int, 3>& dstPos, const std::array<int, 3>& srcPos, const std::array<int, 3>& size, int32_t dstLayer, int32_t srcLayer)
{
	return true;
}

Effekseer::Backend::UniformBufferRef GraphicsDevice::CreateUniformBuffer(int32_t size, const void* initialData)
{
	auto ret = Effekseer::MakeRefPtr<UniformBuffer>();

	if (!ret->Init(size, initialData))
	{
		return nullptr;
	}

	return ret;
}

void GraphicsDevice::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height)
{
}

void GraphicsDevice::Draw(const Effekseer::Backend::DrawParameter& drawParam)
{
}

void GraphicsDevice::BeginRenderPass(Effekseer::Backend::RenderPassRef& renderPass, bool isColorCleared, bool isDepthCleared, Effekseer::Color clearColor)
{
}

void GraphicsDevice::EndRenderPass()
{
}

bool GraphicsDevice::UpdateUniformBuffer(Effekseer::Backend::UniformBufferRef& buffer, int32_t size, int32_t offset, const void* data)
{
	if (buffer == nullptr)
	{
		return false;
	}

	auto b = static_cast<UniformBuffer*>(buffer.Get());

	b->UpdateData(data, size, offset);

	return true;
}

} // namespace Backend
} // namespace EffekseerRendererGL