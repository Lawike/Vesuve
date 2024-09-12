# Scalability

## Deletion Queue

std::function stores a lambda, and we can use it to store a callback with some data, which is perfect for this case.

Doing callbacks like this is inneficient at scale, because we are storing whole std::functions for every object we are deleting, which is not going to be optimal. For the amount of objects we will use in this tutorial, its going to be fine. but if you need to delete thousands of objects and want them deleted faster, a better implementation would be to store arrays of vulkan handles of various types such as VkImage, VkBuffer, and so on. And then delete those from a loop.

## Image

- transitionImageLayout
- copyImageToImage
  Those two functions are useful when setting up the engine, but later its best to ignore them and write your own version that can do extra logic on a fullscreen fragment shader.

## Shaders

Have a script that compiles shaders automatically when the app is launched. Allows to adds shaders dynamically.

Abstract the shader file path with a config file.

## Command pools / buffers

GPU vendors advise to reset the command pools and not the command buffers individually.
So resetting the pool of each frame can be handy to avoid using the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag which can complicate the driver memory management .

Also in the UploadMesh function, we use a staging buffer in which we copy vertex & indices data from the CPU. Currently this pattern is not very efficient, as we are waiting for the GPU command to fully execute before continuing with our CPU side logic. This is something people generally put on a background thread, whose sole job is to execute uploads like this one, and deleting/reusing the staging buffers.

# Tips

## Descriptor sets

Each descriptor set has a cost, the less we have the better.

To sample a texture we can either have 2 descriptors with one for the sampler and one for the sampled image or a single one that combines both but is less efficient. It is harder to deal with 2 descriptors so for now we will have a single one that combine both. May be improved later.

## Push constants

Allows to bind a very small amount of data to the shaders, fast, but its also very limited in space and must be written as you encode the commands. its main use is that you can give it some per-object data, if your objects dont need much of it, but according to gpu vendors, their best use case is to send some indexes to the shader to be used to access some bigger data buffers.

I did had a ComputePushConstants struct in the engine header, this is what will be required for any puhs constants. This one is a placeholder and should be moved/removed later on.

## UI
The compute effect struct is also a placehjolder that should be remove and is useful to perform tests.

## Controls
Try to improve camera control with multiple input pressed at the same time.
Can also use a deltaTime instead of just being based of the engine speed (fps). But i'd have to update the swapchain options to allow it.

## Image formats
I may use the KTX or DDS format since they can be uploaded almost directly to the GPU and are a compressed format tha the GPU reads directly so it saves VRAM.

## Fix inverted depth testing
We currently sort the rendering of opaque objects but we could also sort the transparent one. To sort the transparent object we need to perform a depth sort.

## Sorting objects
We will first index the draw array, and check if the material is the same, and if it is, sort by indexBuffer. But if its not, then we directly compare the material pointer. Another way of doing this is that we would calculate a sort key , and then our opaque_draws would be something like 20 bits draw index, and 44 bits for sort key/hash. That way would be faster than this as it can be sorted through faster methods.
But sorting by depth is incompatible with sorting by pipeline, so you will need to decide what works better for your case.

## Memory managment
To know which kinf of memory for which usage :
https://www.fevrierdorian.com/carnet/pages/ecrire-un-moteur-de-rendu-vulkan-performant.html#selection-de-la-memoire-de-tas

Always allocate image ressources first on bufferImageGranularity since they usually require a bigger alignment.