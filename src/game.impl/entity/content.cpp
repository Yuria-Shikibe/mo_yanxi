module mo_yanxi.game.content;

void mo_yanxi::game::content::load(){
	auto& manager = content_manager;
	manager = meta::content_manager{};

	using namespace meta::chamber;

	{
		auto& r = manager.emplace<armor>("wall");
		r.extent = {1, 1};
	}

	{
		auto& b = manager.emplace<radar>("radar");
		b.extent = {4, 4};
		b.targeting_range_radius = {1200, 6000};
		b.targeting_range_angular = {-math::pi_half / 3.f, math::pi_half / 3.f};
		b.transform.vec = {.5f, .5f};
	}
	{
		auto& b = manager.emplace<turret_base>("turret");
		b.extent = {2, 4};
		// b. = {1200, 6000};
		// b.targeting_range_angular = {-math::pi_half / 3.f, math::pi_half / 3.f};
		b.transform.vec = {.5f, .5f};
	}

	manager.freeze();
}
