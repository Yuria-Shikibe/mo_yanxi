module mo_yanxi.vk.command_buffer;

import mo_yanxi.vk.command_pool;

mo_yanxi::vk::command_buffer::command_buffer(const command_pool& command_pool, const VkCommandBufferLevel level)
	: command_buffer(command_pool.get_device(), command_pool, level)
{

}
