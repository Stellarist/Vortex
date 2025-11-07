#include "GpuTexture.hpp"

GpuTexture::GpuTexture(Context& context, std::shared_ptr<Texture> texture, std::shared_ptr<Sampler> default_sampler) :
    context(&context), source_texture(texture)
{
	if (!texture || !texture->valid())
		throw std::runtime_error("Cannot create gpu texture from invalid texture");

	image = std::make_unique<Image>(
	    context,
	    texture->getData().data(),
	    texture->getWidth(),
	    texture->getHeight());

	if (default_sampler)
		sampler = default_sampler;
	else
		sampler = std::make_shared<Sampler>(context);

	image->setSampler(*sampler);
}

Image* GpuTexture::getImage() const
{
	return image.get();
}

Sampler* GpuTexture::getSampler() const
{
	return sampler.get();
}

std::shared_ptr<Texture> GpuTexture::getSourceTexture() const
{
	return source_texture;
}
