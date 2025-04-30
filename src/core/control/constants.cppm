module;

#include <GLFW/glfw3.h>

export module mo_yanxi.core.ctrl.constants;

import std;

consteval std::size_t genMaskFromBits(std::size_t bits){
	std::size_t rst{};

	for(std::size_t i = 0; i < bits; ++i){
		rst |= static_cast<std::size_t>(1) << i;
	}

	return rst;
}

namespace mo_yanxi::core::ctrl{
	export using key_code_t = unsigned;

	namespace act{
		export constexpr key_code_t press = GLFW_PRESS;
		export constexpr key_code_t release = GLFW_RELEASE;
		export constexpr key_code_t repeat = GLFW_REPEAT;
		export constexpr key_code_t continuous = 3;
		export constexpr key_code_t DoubleClick = 4;

		export constexpr unsigned Bits = 8;
		export constexpr unsigned Mask = genMaskFromBits(Bits);
		export constexpr key_code_t ignore = std::numeric_limits<key_code_t>::max() & Mask;

		export constexpr float multiPressMaxSpacing = 0.25f * 60;  //ticks!
		export constexpr float doublePressMaxSpacing = 0.25f * 60; //ticks!

		export [[nodiscard]] constexpr bool matched(const key_code_t act, const key_code_t expectedAct) noexcept{
			return act == expectedAct || (expectedAct & Mask) == ignore;
		}

		export [[nodiscard]] constexpr bool is_continuous(const key_code_t mode) noexcept{
			return mode == continuous;
		}
	}


	namespace mode{
		export constexpr key_code_t None = 0;
		export constexpr key_code_t Shift = GLFW_MOD_SHIFT;
		export constexpr key_code_t ctrl = GLFW_MOD_CONTROL;
		export constexpr key_code_t Alt = GLFW_MOD_ALT;
		export constexpr key_code_t Super = GLFW_MOD_SUPER;

		export constexpr key_code_t CapLock = GLFW_MOD_CAPS_LOCK;
		export constexpr key_code_t NumLock = GLFW_MOD_NUM_LOCK;
		//6Bit

		export constexpr key_code_t Ctrl_Shift = ctrl | Shift;

		// constexpr key_code_t NoIgnore = static_cast<key_code_t>(0b11'00'00'00);
		export constexpr key_code_t ignore = 0b10'00'00'00;

		constexpr unsigned Bits = 8;
		export constexpr unsigned Mask = genMaskFromBits(Bits);

		export constexpr key_code_t Frequent_Bound = Shift | ctrl | Alt/* | Super | CapLock | NumLock */ + 1;

		export [[nodiscard]] constexpr bool matched(const key_code_t mode, const key_code_t expectedMode) noexcept{
			return (mode & Mask) == (expectedMode & Mask) || (expectedMode & Mask) & ignore;
		}
	}


	// /**
	//  * @code
	//  *	0b 0000'0000  0000'0000  0000'0000  0000'0000
	//  *	   [  Mode ]  [  Act  ]  [     Key Code     ]
	//  * @endcode
	//  */
	// constexpr int getFullKey(const int keyCode, const int act, const int mode) noexcept{
	// 	return
	// 		keyCode
	// 		| act << 16
	// 		| mode << (16 + 8);
	// }
	//
	// /**
	//  * @brief
	//  * @return [keyCode, act, mode]
	//  */
	// constexpr std::tuple<int, int, int> getSeperatedKey(const int fullKey) noexcept{
	// 	return {fullKey & 0xffff, fullKey >> 16 & 0xff, fullKey >> 24 & 0xff};
	// }


	//Key Total Count -- 350 -- 9bit
	constexpr auto HAT_CENTERED = 0;
	constexpr auto HAT_UP = 1;
	constexpr auto HAT_RIGHT = 2;
	constexpr auto HAT_DOWN = 4;
	constexpr auto HAT_LEFT = 8;
	constexpr auto HAT_RIGHT_UP = GLFW_HAT_UP;
	constexpr auto HAT_RIGHT_DOWN = GLFW_HAT_DOWN;
	constexpr auto HAT_LEFT_UP = GLFW_HAT_UP;
	constexpr auto HAT_LEFT_DOWN = GLFW_HAT_DOWN;


	constexpr auto Unknown = -1;


	namespace key{
		export constexpr key_code_t Ignore = std::numeric_limits<key_code_t>::max();
		export constexpr key_code_t Space = 32;
		export constexpr key_code_t Apostrophe = 39; /* ' */
		export constexpr key_code_t Comma = 44;      /* , */
		export constexpr key_code_t Minus = 45;      /* - */
		export constexpr key_code_t Period = 46;     /* . */
		export constexpr key_code_t Slash = 47;      /* / */
		export constexpr key_code_t _0 = 48;
		export constexpr key_code_t _1 = 49;
		export constexpr key_code_t _2 = 50;
		export constexpr key_code_t _3 = 51;
		export constexpr key_code_t _4 = 52;
		export constexpr key_code_t _5 = 53;
		export constexpr key_code_t _6 = 54;
		export constexpr key_code_t _7 = 55;
		export constexpr key_code_t _8 = 56;
		export constexpr key_code_t _9 = 57;
		export constexpr key_code_t Semicolon = 59; /* ; */
		export constexpr key_code_t Equal = 61;     /* = */
		export constexpr key_code_t A = 65;
		export constexpr key_code_t B = 66;
		export constexpr key_code_t C = 67;
		export constexpr key_code_t D = 68;
		export constexpr key_code_t E = 69;
		export constexpr key_code_t F = 70;
		export constexpr key_code_t G = 71;
		export constexpr key_code_t H = 72;
		export constexpr key_code_t I = 73;
		export constexpr key_code_t J = 74;
		export constexpr key_code_t K = 75;
		export constexpr key_code_t L = 76;
		export constexpr key_code_t M = 77;
		export constexpr key_code_t N = 78;
		export constexpr key_code_t O = 79;
		export constexpr key_code_t P = 80;
		export constexpr key_code_t Q = 81;
		export constexpr key_code_t R = 82;
		export constexpr key_code_t S = 83;
		export constexpr key_code_t T = 84;
		export constexpr key_code_t U = 85;
		export constexpr key_code_t V = 86;
		export constexpr key_code_t W = 87;
		export constexpr key_code_t X = 88;
		export constexpr key_code_t Y = 89;
		export constexpr key_code_t Z = 90;
		export constexpr key_code_t LeftBracket = 91;  /* [ */
		export constexpr key_code_t Backslash = 92;    /* \ */
		export constexpr key_code_t RightBracket = 93; /* ] */
		export constexpr key_code_t GraveAccent = 96;  /* ` */

		export constexpr key_code_t WORLD_1 = 161; /* non-US #1 */
		export constexpr key_code_t WORLD_2 = 162; /* non-US #2 */

		export constexpr key_code_t Esc = 256;
		export constexpr key_code_t Enter = 257;
		export constexpr key_code_t Tab = 258;
		export constexpr key_code_t Backspace = 259;
		export constexpr key_code_t Insert = 260;
		export constexpr key_code_t Delete = 261;
		export constexpr key_code_t Right = 262;
		export constexpr key_code_t Left = 263;
		export constexpr key_code_t Down = 264;
		export constexpr key_code_t Up = 265;
		export constexpr key_code_t PageUp = 266;
		export constexpr key_code_t PageDown = 267;
		export constexpr key_code_t Home = 268;
		export constexpr key_code_t End = 269;
		export constexpr key_code_t CapsLock = 280;
		export constexpr key_code_t ScrollLock = 281;
		export constexpr key_code_t NumLock = 282;
		export constexpr key_code_t PrintScreen = 283;
		export constexpr key_code_t Pause = 284;
		export constexpr key_code_t F1 = 290;
		export constexpr key_code_t F2 = 291;
		export constexpr key_code_t F3 = 292;
		export constexpr key_code_t F4 = 293;
		export constexpr key_code_t F5 = 294;
		export constexpr key_code_t F6 = 295;
		export constexpr key_code_t F7 = 296;
		export constexpr key_code_t F8 = 297;
		export constexpr key_code_t F9 = 298;
		export constexpr key_code_t F10 = 299;
		export constexpr key_code_t F11 = 300;
		export constexpr key_code_t F12 = 301;
		export constexpr key_code_t F13 = 302;
		export constexpr key_code_t F14 = 303;
		export constexpr key_code_t F15 = 304;
		export constexpr key_code_t F16 = 305;
		export constexpr key_code_t F17 = 306;
		export constexpr key_code_t F18 = 307;
		export constexpr key_code_t F19 = 308;
		export constexpr key_code_t F20 = 309;
		export constexpr key_code_t F21 = 310;
		export constexpr key_code_t F22 = 311;
		export constexpr key_code_t F23 = 312;
		export constexpr key_code_t F24 = 313;
		export constexpr key_code_t F25 = 314;

		export constexpr key_code_t KP_0 = 320;
		export constexpr key_code_t KP_1 = 321;
		export constexpr key_code_t KP_2 = 322;
		export constexpr key_code_t KP_3 = 323;
		export constexpr key_code_t KP_4 = 324;
		export constexpr key_code_t KP_5 = 325;
		export constexpr key_code_t KP_6 = 326;
		export constexpr key_code_t KP_7 = 327;
		export constexpr key_code_t KP_8 = 328;
		export constexpr key_code_t KP_9 = 329;

		export constexpr key_code_t KP_DECIMAL = 330;
		export constexpr key_code_t KP_DIVIDE = 331;
		export constexpr key_code_t KP_MULTIPLY = 332;
		export constexpr key_code_t KP_SUBTRACT = 333;
		export constexpr key_code_t KP_ADD = 334;
		export constexpr key_code_t KP_ENTER = 335;
		export constexpr key_code_t KP_EQUAL = 336;

		export constexpr key_code_t Left_Shift = 340;
		export constexpr key_code_t Left_Control = 341;
		export constexpr key_code_t Left_Alt = 342;
		export constexpr key_code_t Left_Super = 343;

		export constexpr key_code_t Right_Shift = 344;
		export constexpr key_code_t Right_Control = 345;
		export constexpr key_code_t Right_Alt = 346;
		export constexpr key_code_t Right_Super = 347;

		export constexpr key_code_t MENU = 348;

		export constexpr key_code_t LAST = GLFW_KEY_MENU;
		export constexpr key_code_t Count = LAST + 1;

		export [[nodiscard]] constexpr bool matched(const key_code_t key, const key_code_t expectedKey) noexcept{
			return key == expectedKey || expectedKey == Ignore;
		}
	}


	namespace mouse{
		export constexpr key_code_t _1 = 0;
		export constexpr key_code_t _2 = 1;
		export constexpr key_code_t _3 = 2;
		export constexpr key_code_t _4 = 3;
		export constexpr key_code_t _5 = 4;
		export constexpr key_code_t _6 = 5;
		export constexpr key_code_t _7 = 6;
		export constexpr key_code_t _8 = 7;

		export constexpr key_code_t Count = 8;

		export constexpr key_code_t LEFT = _1;
		export constexpr key_code_t RIGHT = _2;
		export constexpr key_code_t MIDDLE = _3;

		export constexpr key_code_t LMB = _1;
		export constexpr key_code_t RMB = _2;
		export constexpr key_code_t CMB = _3;
	}


	export constexpr auto AllKeyCount = key::Count;

	export constexpr bool on_mouse(const key_code_t keyCode) noexcept{
		return keyCode < mouse::Count;
	}

	/**
	 * @brief Caution: Cannot confirm if it is really on the keyboard
	 */
	export constexpr bool on_keyboard(const key_code_t keyCode) noexcept{
		return !on_mouse(keyCode) && keyCode < key::Count;
	}

	constexpr std::array KeyNames{
			[]() constexpr{
				std::array<std::string_view, AllKeyCount> kMap{};

				kMap[0] = "mouse_left";
				kMap[1] = "mouse_right";
				kMap[2] = "mouse_scroll";
				kMap[3] = "mouse_4";
				kMap[4] = "mouse_5";
				kMap[5] = "mouse_6";
				kMap[6] = "mouse_7";
				kMap[7] = "mouse_8";

				kMap[32] = "keyboard_space";
				kMap[39] = "keyboard_apostrophe";
				kMap[48] = "keyboard_0";
				kMap[49] = "keyboard_1";
				kMap[50] = "keyboard_2";
				kMap[51] = "keyboard_3";
				kMap[52] = "keyboard_4";
				kMap[53] = "keyboard_5";
				kMap[54] = "keyboard_6";
				kMap[55] = "keyboard_7";
				kMap[56] = "keyboard_8";
				kMap[57] = "keyboard_9";
				kMap[59] = "keyboard_semicolon";
				kMap[61] = "keyboard_equal";
				kMap[65] = "keyboard_a";
				kMap[66] = "keyboard_b";
				kMap[67] = "keyboard_c";
				kMap[68] = "keyboard_d";
				kMap[69] = "keyboard_e";
				kMap[70] = "keyboard_f";
				kMap[71] = "keyboard_g";
				kMap[72] = "keyboard_h";
				kMap[73] = "keyboard_i";
				kMap[74] = "keyboard_j";
				kMap[75] = "keyboard_k";
				kMap[76] = "keyboard_l";
				kMap[77] = "keyboard_m";
				kMap[78] = "keyboard_n";
				kMap[79] = "keyboard_o";
				kMap[80] = "keyboard_p";
				kMap[81] = "keyboard_q";
				kMap[82] = "keyboard_r";
				kMap[83] = "keyboard_s";
				kMap[84] = "keyboard_t";
				kMap[85] = "keyboard_u";
				kMap[86] = "keyboard_v";
				kMap[87] = "keyboard_w";
				kMap[88] = "keyboard_x";
				kMap[89] = "keyboard_y";
				kMap[90] = "keyboard_z";
				kMap[91] = "keyboard_leftbracket";
				kMap[92] = "keyboard_backslash";
				kMap[93] = "keyboard_rightbracket";
				kMap[96] = "keyboard_graveaccent";
				kMap[161] = "keyboard_world_1";
				kMap[162] = "keyboard_world_2";
				kMap[256] = "keyboard_esc";
				kMap[257] = "keyboard_enter";
				kMap[258] = "keyboard_tab";
				kMap[259] = "keyboard_backspace";
				kMap[260] = "keyboard_insert";
				kMap[261] = "keyboard_delete";
				kMap[262] = "keyboard_right";
				kMap[263] = "keyboard_left";
				kMap[264] = "keyboard_down";
				kMap[265] = "keyboard_up";
				kMap[266] = "keyboard_pageup";
				kMap[267] = "keyboard_pagedown";
				kMap[268] = "keyboard_home";
				kMap[269] = "keyboard_end";
				kMap[280] = "keyboard_capslock";
				kMap[281] = "keyboard_scrolllock";
				kMap[282] = "keyboard_numlock";
				kMap[283] = "keyboard_printscreen";
				kMap[284] = "keyboard_pause";
				kMap[290] = "keyboard_f1";
				kMap[291] = "keyboard_f2";
				kMap[292] = "keyboard_f3";
				kMap[293] = "keyboard_f4";
				kMap[294] = "keyboard_f5";
				kMap[295] = "keyboard_f6";
				kMap[296] = "keyboard_f7";
				kMap[297] = "keyboard_f8";
				kMap[298] = "keyboard_f9";
				kMap[299] = "keyboard_f10";
				kMap[300] = "keyboard_f11";
				kMap[301] = "keyboard_f12";
				kMap[302] = "keyboard_f13";
				kMap[303] = "keyboard_f14";
				kMap[304] = "keyboard_f15";
				kMap[305] = "keyboard_f16";
				kMap[306] = "keyboard_f17";
				kMap[307] = "keyboard_f18";
				kMap[308] = "keyboard_f19";
				kMap[309] = "keyboard_f20";
				kMap[310] = "keyboard_f21";
				kMap[311] = "keyboard_f22";
				kMap[312] = "keyboard_f23";
				kMap[313] = "keyboard_f24";
				kMap[314] = "keyboard_f25";
				kMap[320] = "keyboard_kp_0";
				kMap[321] = "keyboard_kp_1";
				kMap[322] = "keyboard_kp_2";
				kMap[323] = "keyboard_kp_3";
				kMap[324] = "keyboard_kp_4";
				kMap[325] = "keyboard_kp_5";
				kMap[326] = "keyboard_kp_6";
				kMap[327] = "keyboard_kp_7";
				kMap[328] = "keyboard_kp_8";
				kMap[329] = "keyboard_kp_9";
				kMap[330] = "keyboard_kp_decimal";
				kMap[331] = "keyboard_kp_divide";
				kMap[332] = "keyboard_kp_multiply";
				kMap[333] = "keyboard_kp_subtract";
				kMap[334] = "keyboard_kp_add";
				kMap[335] = "keyboard_kp_enter";
				kMap[336] = "keyboard_kp_equal";
				kMap[340] = "keyboard_left_shift";
				kMap[341] = "keyboard_left_control";
				kMap[342] = "keyboard_left_alt";
				kMap[343] = "keyboard_left_super";
				kMap[344] = "keyboard_right_shift";
				kMap[345] = "keyboard_right_control";
				kMap[346] = "keyboard_right_alt";
				kMap[347] = "keyboard_right_super";
				kMap[348] = "keyboard_menu";

				return kMap;
			}()
		};

	constexpr std::array ModeNames{
			[]() constexpr{
				std::array<std::string_view, mode::Bits + 1> names{};

				/*
					constexpr int Shift = GLFW_MOD_SHIFT;
					constexpr int Ctrl = GLFW_MOD_CONTROL;
					constexpr int Alt = GLFW_MOD_ALT;
					constexpr int Super = GLFW_MOD_SUPER;

					constexpr int CapLock = GLFW_MOD_CAPS_LOCK;
					constexpr int NumLock = GLFW_MOD_NUM_LOCK;
				 */
				names[/*0b0000'0000*/ 0] = "none";
				names[/*0b0000'0001*/ 1] = "shift";
				names[/*0b0000'0010*/ 2] = "ctrl";
				names[/*0b0000'0100*/ 3] = "alt";
				names[/*0b0000'1000*/ 4] = "super";
				names[/*0b0001'0000*/ 5] = "cap-lock";
				names[/*0b0010'0000*/ 6] = "num-lock";

				return names;
			}()
		};

	constexpr std::array ActNames{
		[]() constexpr{
			std::array<std::string_view, 5> names{};

			/*
			constexpr int Press = GLFW_PRESS;
			constexpr int Release = GLFW_RELEASE;
			constexpr int Repeat = GLFW_REPEAT;
			constexpr int Continuous = 3;
			constexpr int DoubleClick = 4;
			 */
			names[act::press]       = "press";
			names[act::release]     = "release";
			names[act::repeat]      = "repeat";
			names[act::DoubleClick] = "double";
			names[act::continuous]  = "continuous";

			return names;
		}()
	};

	constexpr std::vector<std::string_view> getModesStr(const int mode, const bool emptyPatch = false){
		std::vector<std::string_view> strs{};

		for(int i = 0; i < mode::Bits; ++i){
			int curMode = 1 << i;
			if(mode & curMode){
				strs.push_back(ModeNames[i + 1]);
				strs.push_back(" + ");
			}
		}


		if(strs.empty()){
			if(emptyPatch)strs.push_back(ModeNames[0]);
		}else{
			strs.pop_back();
		}

		return strs;
	}
}
