//
// Created by Matrix on 2025/11/7.
//

export module mo_yanxi.data_flow;

export import :node_interface;
export import :manager;
export import :nodes;

//TODO add ring check
//TODO support multi async consumer and better scheduler?

namespace mo_yanxi::data_flow{

export
template <std::ranges::input_range Rng = std::initializer_list<node*>>
void connect_chain(const Rng& chain){
	for (auto && [l, r] : chain | std::views::adjacent<2>){
		if constexpr (std::same_as<decltype(l), node&>){
			l.connect_successors(r);
		}else if(std::same_as<decltype(*l), node&>){
			(*l).connect_successors(*r);
		}

	}
}


void example(){
	manager manager{};

	struct modifier_str_to_num : modifier_transient<int, std::string>{
		using modifier_transient::modifier_transient;
	protected:
		std::optional<int> operator()(const std::stop_token& stop_token, const std::string& arg) const override{
			int val{};

			for(int i = 0; i < 50; ++i){
				if(stop_token.stop_requested()){
					return std::nullopt;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}

			auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), val);
			if(ec == std::errc{}){
				return val * 10;
			}
			return std::nullopt;
		}
	};

	struct modifier_num_to_num : modifier_argument_cached<int, int>{
		using modifier_argument_cached::modifier_argument_cached;
	protected:
		std::optional<int> operator()(const std::stop_token& stop_token, const int& arg) const override{
			return -arg;
		}
	};


	struct printer : terminal_typed<int>{
		std::string prefix;

		[[nodiscard]] explicit printer(const std::string& prefix)
		: prefix(prefix){
		}

		void on_update(const int& data) override{
			terminal_typed::on_update(data);

			std::println(std::cout, "{}: {}", prefix, data);
			std::cout.flush();
		}
	};

	auto& p  = manager.add_provider<provider_cached<std::string>>();
	auto& m0 = manager.add_modifier<modifier_str_to_num>(async_type::async_latest);
	auto& m1 = manager.add_modifier<modifier_num_to_num>(async_type::none, true);
	auto& t0 = manager.add_terminal<printer>("Str To Num(delay 5s)");
	auto& t1 = manager.add_terminal<printer>("Negate of Num");

	data_flow::connect_chain({&p, &m0, &t0});
	data_flow::connect_chain({&m0, &m1, &t1});

	auto thd = std::jthread([&](std::stop_token t){
		while(!t.stop_requested()){
			std::string str;
			std::cin >> str;

			if(str == "/exit"){
				break;
			}

			manager.push_posted_act([&, s = std::move(str)] mutable {
				p.update_value(std::move(s));
			});
		}
	});

	while(true){
		manager.update();

		if(t1.is_expired()){
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			t1.on_update(t1.request(true).value());
		}
	}
}

}
