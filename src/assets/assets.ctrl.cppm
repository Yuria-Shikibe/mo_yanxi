//
// Created by Matrix on 2024/5/10.
//

export module mo_yanxi.assets.ctrl;

export import mo_yanxi.core.global;
export import mo_yanxi.core.global.graphic;
export import mo_yanxi.core.global.ui;
export import mo_yanxi.ui.primitives;
export import mo_yanxi.ui.root;
export import mo_yanxi.graphic.camera;

import std;

import mo_yanxi.heterogeneous;

namespace mo_yanxi::assets::ctrl{
	float baseCameraMoveSpeed = 40;
	float fastCameraMoveSpeed = 250;
	bool disableMove = false;
	float cameraMoveSpeed = baseCameraMoveSpeed;

	mo_yanxi::graphic::camera2& get_camera(){
		if(core::global::ui::root->get_current_focus().has_camera_focus()){
			return *core::global::ui::root->get_current_focus().focused_camera;
		}else{
			return core::global::graphic::world.camera;
		}
	}

	export
	void load(){
		using namespace mo_yanxi;
		namespace cc = mo_yanxi::core::ctrl;

		core::global::input.main_binds.register_bind(cc::key::P, cc::act::press, [](auto){
			core::global::timer.set_pause(!core::global::timer.is_paused());
		});

		core::global::input.main_binds.register_bind(cc::key::A, cc::act::continuous, [](auto){

			 if(!disableMove) get_camera().move({
					(-cameraMoveSpeed * core::global::timer.global_delta_tick()), 0
			});
		});

		core::global::input.main_binds.register_bind(cc::key::D, cc::act::continuous, [](auto){
			if(!disableMove) get_camera().move({
					(+cameraMoveSpeed * core::global::timer.global_delta_tick()), 0
			});
		});

		core::global::input.main_binds.register_bind(cc::key::S, cc::act::continuous, [](auto){
			if(!disableMove) get_camera().move({
					0, -cameraMoveSpeed * core::global::timer.global_delta_tick()
			});
		});

		core::global::input.main_binds.register_bind(cc::key::W, cc::act::continuous, [](auto){
			if(!disableMove) get_camera().move({
					0, +cameraMoveSpeed * core::global::timer.global_delta_tick()
			});
		});

		core::global::input.main_binds.register_bind(cc::key::Left_Shift, cc::mode::ignore, [](auto){
			cameraMoveSpeed = fastCameraMoveSpeed;
		}, [](auto){
			cameraMoveSpeed = baseCameraMoveSpeed;
		});

		core::global::input.scroll_listeners.emplace_back([](const float x, const float y) -> void {
			if(!core::global::ui::root->scrollIdle()){
				core::global::ui::root->input_scroll(x, y);
			}else{
				core::global::graphic::world.camera.set_target_scale(core::global::graphic::world.camera.get_target_scale() + y * 0.05f);
			}
		 });
		//
		// input->scrollListeners.emplace_back([](const float x, const float y) -> void {
		// 	  if(!UI::root->focusIdle()){
		// 		  UI::root->inputScroll(x, y);
		// 	  }else if(Focus::camera){
		// 		  Focus::camera->setTargetScale(Focus::camera->getTargetScale() + y * 0.05f);
		// 	  }
		//   });
	}

 //    namespace CC = Core::Ctrl;
 //
	// CC::Operation cameraMoveLeft{"cmove-left", CC::KeyBind(CC::Key::A, CC::Act::Continuous, +[]{
	// 	if(!disableMove)Core::Global::Focus::camera.move({static_cast<float>(-cameraMoveSpeed * Core::Global::timer.globalDeltaTick()), 0});
	// })};
 //
	// CC::Operation cameraMoveRight{"cmove-right", CC::KeyBind(CC::Key::D, CC::Act::Continuous, +[]{
	// 	if(!disableMove)Core::Global::Focus::camera.move({static_cast<float>(cameraMoveSpeed * Core::Global::timer.globalDeltaTick()), 0});
	// })};
 //
	// CC::Operation cameraMoveUp{"cmove-up", CC::KeyBind(CC::Key::W, CC::Act::Continuous, +[]{
	// 	if(!disableMove)Core::Global::Focus::camera.move({0, static_cast<float>(cameraMoveSpeed * Core::Global::timer.globalDeltaTick())
	// 		});
	// })};
 //
	// CC::Operation cameraMoveDown{"cmove-down", CC::KeyBind(CC::Key::S, CC::Act::Continuous, +[] {
	// 	if(!disableMove)Core::Global::Focus::camera.move({0, static_cast<float>(-cameraMoveSpeed * Core::Global::timer.globalDeltaTick())
	// 		});
	// })};

	// CC::Operation cameraTeleport{"cmove-telp", CC::KeyBind(CC::Mouse::LMB, CC::Act::DoubleClick, +[] {
	// 	if(Core::ui->cursorCaptured()){
	//
	// 	}else{
	// 		core::global::camera->setPosition(Core::Util::getMouseToWorld());
	// 	}
	// })};
	//
	// CC::Operation cameraLock{"cmove-lock", CC::KeyBind(CC::Key::M, CC::Act::Press, +[] {
	// 	disableMove = !disableMove;
	// })};
	//
	// CC::Operation boostCamera_Release{"cmrboost-rls", CC::KeyBind(CC::Key::Left_Shift, CC::Act::Release, +[] {
	// 	cameraMoveSpeed = baseCameraMoveSpeed;
	// })};
	//
	// CC::Operation boostCamera_Press{"cmrboost-prs", CC::KeyBind(CC::Key::Left_Shift, CC::Act::Press, +[] {
	// 	cameraMoveSpeed = fastCameraMoveSpeed;
	// }), {boostCamera_Release.name}};
	// //
	// CC::Operation pause{"pause", CC::KeyBind(CC::Key::P, CC::Act::Press, +[] {
	// 	// std::cout << "pressed" << std::endl;
	// 	Core::Global::timer.setPause(!Core::Global::timer.isPaused());
	// })};

	// CC::Operation hideUI{"ui-hide", CC::KeyBind(CC::Key::H, CC::Act::Press, +[] {
	// 	if(Core::ui->isHidden){
	// 		Core::ui->show();
	// 	} else{
	// 		if(!Core::ui->hasTextFocus())Core::ui->hide();
	// 	}
	// })};

	// CC::Operation shoot{"shoot", CC::KeyBind(CC::Key::F, CC::Act::Continuous, +[] {

	// })};

	// export{
	// 	CC::KeyMapping* mainControlGroup{};
 //
	// 	CC::OperationGroup basicGroup{
	// 			"basic-group", {
	// 				CC::Operation{cameraMoveLeft},
	// 				CC::Operation{cameraMoveRight},
	// 				CC::Operation{cameraMoveUp},
	// 				CC::Operation{cameraMoveDown},
	// 				CC::Operation{boostCamera_Press},
	// 				CC::Operation{boostCamera_Release},
 //
	// 				CC::Operation{cameraLock},
	// 				// CC::Operation{cameraTeleport},
 //
	// 				CC::Operation{pause},
	// 				// CC::Operation{hideUI},
	// 			}
	// 		};
 //
	// 	CC::OperationGroup gameGroup{"game-group"};
 //
	// 	ext::string_hash_map<CC::OperationGroup*> allGroups{
	// 		{basicGroup.getName(), &basicGroup},
	// 		{gameGroup.getName(), &gameGroup}
	// 	};
 //
	// 	ext::string_hash_map<CC::KeyMapping*> relatives{};
 //
	// 	void apply(){
	// 		mainControlGroup->clear();
 //
	// 		for (auto group : allGroups | std::ranges::views::values){
	// 			for (const auto & bind : group->getBinds() | std::ranges::views::values){
	// 				mainControlGroup->registerBind(CC::InputBind{bind.customeBind});
	// 			}
	// 		}
	// 	}
 //
 //        void load() {
 //            mainControlGroup = &Core::Global::input->registerSubInput("game");
 //            basicGroup.targetGroup = mainControlGroup;
 //            apply();
 //
	// 		using namespace Core::Global;
 //
 //            input->scrollListeners.emplace_back([](const float x, const float y) -> void {
 //            	if(!UI::root->focusIdle()){
 //            		UI::root->inputScroll(x, y);
 //            	}else if(Focus::camera){
 //            		Focus::camera->setTargetScale(Focus::camera->getTargetScale() + y * 0.05f);
 //            	}
 //            });
        // }
	// }
}
