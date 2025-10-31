//
// Created by Matrix on 2025/11/1.
//

export module mo_yanxi.gui.infrastructure:events;

import mo_yanxi.math.vector2;
import mo_yanxi.input_handle;
import std;

namespace mo_yanxi::gui::events{
using key_set = input_handle::key_set;
struct click{
	math::vec2 pos;
	key_set key;
};

struct scroll{
	math::vec2 pos;
	input_handle::mode mode;
};

struct cursor_move{
	math::vec2 src;
	math::vec2 dst;
};

struct drag{
	math::vec2 src;
	math::vec2 dst;
	key_set key;

	[[nodiscard]] math::vec2 delta() const noexcept{
		return dst - src;
	}
};

enum struct op_afterwards{
	intercepted,
	fall_through,
};

}