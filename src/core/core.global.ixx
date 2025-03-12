//
// Created by Matrix on 2025/3/1.
//

export module mo_yanxi.core.global;

export import mo_yanxi.core.ctrl.key_binding;
export import mo_yanxi.core.ctrl.main_input;
export import mo_yanxi.core.application_timer;

export import mo_yanxi.graphic.camera;

namespace mo_yanxi::core::global{
	export inline application_timer<float> timer{application_timer<float>::get_default()};
	export inline ctrl::main_input input{};
	export inline graphic::camera2 camera{};
}