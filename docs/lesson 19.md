# Framebuffers
In the last lesson we've created the swapchain that contains the images we need to render to, but we're still missing the framebuffers that will establish the connection between our graphics pipeline and those images. This is what we're going to take care of today.

## Framebuffers
Creating framebuffers once more follows the familiar pattern:
```
class Device
{
    ...
    UniqueFramebuffer createFramebufferUnique( const FramebufferCreateInfo& createInfo, ... );
    ...
};
```
This time the necessary create info structure is relatively small:
```
struct FramebufferCreateInfo
{
    FramebufferCreateInfo& setFlags( FramebufferCreateFlags flags_ );
    FramebufferCreateInfo& setRenderPass( RenderPass renderPass_ );
    FramebufferCreateInfo& setAttachments( const container_t< const ImageView >& attachments_ );
    FramebufferCreateInfo& setWidth( uint32_t width_ );
    FramebufferCreateInfo& setHeight( uint32_t height_ );
    FramebufferCreateInfo& setLayers( uint32_t layers_ );    
};
```
- once again we can ignore the `flags_` because they are only relevant for more advanced use cases
- `renderPass_` should be self-explanatory (1)
- we already know what `attachments_` are in this context. The only new information here is that they have to be of type `ImageView`.
- `width_` and `height_` are also straightforward. It is actually possible to pass dimensions that are different from the actual dimensions of the attached images, but usually you'll simply want to pass their size here. 
- we already came across the concept of 'image array layers' in the previous lesson. The `layers_` parameter here is equivalent to the one in `SwapchainCreateInfoKHR`

Alright, looks like the only thing we're missing here are the `ImageView`s representing the swapchain images (we will need one framebuffer for each image in the swapchain) so that we can pass them in as attachments. Obtaining the images from the swapchain is easy. The only thing that is mildly surprising is that we need to ask the logical device for the images and not the swapchain directly:
```
class Device
{
    ...
    std::vector< Image > getSwapchainImagesKHR( SwapchainKHR swapchain, ... )
    ...
};
```
So, now we have the images, but to create the framebuffers we need `ImageViews`. Apparently we're still missing one step here.

## Image Views
Putting it simple, image views are references to the actual pixel data stored in images. That means an `ImageView` doesn't necessarily reference the whole `Image`, but may also only represent e.g. a certain mip level or one of the layers. The Vulkan API uses `ImageViews` almost exclusively in its function signatures and creating them uses the familiar pattern once more:
```
class Device
{
    ...
    UniqueImageView createImageViewUnique( const ImageViewCreateInfo & createInfo, ... );
    ...
};
```
`ImageViewCreateInfo` looks like this:
```
struct ImageViewCreateInfo
{
    ...
    ImageViewCreateInfo& setFlags( ImageViewCreateFlags flags_ );
    ImageViewCreateInfo& setImage( Image image_ );
    ImageViewCreateInfo& setViewType( ImageViewType viewType_ );
    ImageViewCreateInfo& setFormat( Format format_ );
    ImageViewCreateInfo& setComponents( const ComponentMapping& components_ );
    ImageViewCreateInfo& setSubresourceRange( const ImageSubresourceRange& subresourceRange_ );
    ...
};
```
Again a relatively small struct:
- there are only two `ImageViewCreateFlagBits` defined, but they refer to more advanced features, so we once again ignore the `flags_` parameter
- `image_` should be self-explanatory
- `viewType_` determines whether it's a 1D, 2D, 3D or cube image, or an array image
- the `format_` parameter describes the color format of the image, just as in the info structures we looked at previously. It is however possible to create a view that uses a different color format than the source image, as long as the two formats are compatible (i.e. they have the same number of bits per channel). That is where the ...
- ... `ComponentMapping` comes into play: by using this structure it is possible to define the reordering of color channels from the source image to the image view. If source and view color format are the same we don't need this parameter.
- the `subresourceRange_` defines which subset of the source image is referenced by the view

So it looks like the only thing missing is the `ImageSubresourceRange`. That is the structure that allows for referencing only a subset of the source image:
```
struct ImageSubresouceRange
{
    ...
    ImageSubresourceRange& setAspectMask( ImageAspectFlags aspectMask_ );
    ImageSubresourceRange& setBaseMipLevel( uint32_t baseMipLevel_ );
    ImageSubresourceRange& setLevelCount( uint32_t levelCount_ );
    ImageSubresourceRange& setBaseArrayLayer( uint32_t baseArrayLayer_ );
    ImageSubresourceRange& setLayerCount( uint32_t layerCount_ );
    ...
};
```
- 'aspects' in this context are the logical components of an image. E.g. for a depth-stencil image, although the data is stored interleaved, one can isolate the depth data in a view of its own by setting the `aspectMask_` accordingly
- `baseMipLevel_` denotes the first mip level to be referenced by this view
- `levelCount_` sets the number of mip levels (starting with the `baseMipLevel_`) this view references
- for a layered image, `baseArrayLayer_` specifies the first layer to be referenced by this view
- `layerCount_` gives the number of layers that are going to be referenced

## Putting it into action
With all that information we're now equipped to actually create the framebuffers. Let's start by creating a generic function to create an `ImageView` from an `Image`:
```
vk::UniqueImageView create_image_view(
    const vk::Device& logicalDevice,
    const vk::Image& image,
    const vk::Format& format
)
{
    const auto subresourceRange = vk::ImageSubresourceRange{}
        .setAspectMask( vk::ImageAspectFlagBits::eColor )
        .setBaseMipLevel( 0 )
        .setLevelCount( 1 )
        .setBaseArrayLayer( 0 )
        .setLayerCount( 1 );

    const auto createInfo = vk::ImageViewCreateInfo{}
        .setImage( image )
        .setViewType( vk::ImageViewType::e2D )
        .setFormat( format )
        .setSubresourceRange( subresourceRange );

    return logicalDevice.createImageViewUnique( createInfo );
}
```
Now we can use this function to create image views for all the swapchain images:
```
std::vector< vk::UniqueImageView > create_swapchain_image_views(
    const vk::Device& logicalDevice,
    const vk::SwapchainKHR& swapChain,
    const vk::Format& imageFormat
)
{
    auto swapChainImages = logicalDevice.getSwapchainImagesKHR( swapChain );

    std::vector< vk::UniqueImageView > swapChainImageViews;
    for( const auto img : swapChainImages )
    {
        swapChainImageViews.push_back( 
            create_image_view( logicalDevice, img, imageFormat )
        );
    }

    return swapChainImageViews;
}
```
And with those image views we can finally create our framebuffers:
```
std::vector< vk::UniqueFramebuffer > create_framebuffers(
    const vk::Device& logicalDevice,
    const std::vector< vk::UniqueImageView >& imageViews,
    const vk::Extent2D& imageExtent,
    const vk::RenderPass& renderPass
)
{
    std::vector< vk::UniqueFramebuffer > result;
    for( const auto& v : imageViews )
    {
        std::array< vk::ImageView, 1 > attachments = { *v };
        const auto frameBufferCreateInfo = vk::FramebufferCreateInfo{}
            .setRenderPass( renderPass )
            .setAttachments( attachments )
            .setWidth( imageExtent.width )
            .setHeight( imageExtent.height )
            .setLayers( 1 );

        result.push_back( logicalDevice.createFramebufferUnique( frameBufferCreateInfo ) );
    }

    return result;
}
```
The image views need to be persistent, so we cannot create them directly from within `create_framebuffers`. That's why we need to pass them in and create our framebuffers in `main` like so:
```
const auto imageViews = vcpp::create_swapchain_image_views(
    logicalDevice,
    *swapchain,
    surfaceFormats[0].format );

const auto framebuffers = create_framebuffers(
    logicalDevice,
    imageViews,
    swapchainExtent,
    *renderPass
);
```
If you compile and run this version, you'll hopefully still get no errors, but you also still don't see anything on screen. That is because while we've made all the necessary static connections we're still not yet actually executing our pipeline. This is the last step missing and this is what we'll do next time.

1. Note again the equivalence to the descriptor sets where we were using the `DescriptorSetLayout` as a template to create the actual descriptor sets. Here it's the `RenderPass` that serves the same purpose.
