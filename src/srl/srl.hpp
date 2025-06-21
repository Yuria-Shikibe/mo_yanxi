#pragma once

#define EMPTY_BUFFER_INIT /*[[indeterminate]]*/

#include <cassert>
#include <srl/byte_buffer.pb.h>
#include <srl/math.pb.h>

import mo_yanxi.math.vector2;
import mo_yanxi.math.rect_ortho;
import mo_yanxi.math.trans2;
import mo_yanxi.math.quad;


namespace mo_yanxi::io{
	struct bad_byte_cast : std::exception{

	};

	namespace detail{
		template <typename B>
		concept buffer_data = requires(B& b){
			{ b.mutable_data() } -> std::same_as<std::string*>;
			{ b.data() } -> std::same_as<const std::string&>;
		};


		template <typename T>
		const T* start_lifetime_as(const void* p) noexcept{
			const auto mp = const_cast<void*>(p);
			const auto bytes = new(mp) std::byte[sizeof(T)];
			const auto ptr = reinterpret_cast<const T*>(bytes);
			(void)*ptr;
			return ptr;
		}
	}

	template <detail::buffer_data B = pb::byte_buffer, typename T>
		requires (std::is_trivially_copyable_v<T>)
	void store_bytes(B& buffer, const T& vec){
		buffer.mutable_data()->resize(sizeof(vec));
		std::memcpy(buffer.mutable_data()->data(), &vec, sizeof(vec));
	}

	template <detail::buffer_data B = pb::byte_buffer, typename T>
		requires (std::is_trivially_copyable_v<T> && !std::is_const_v<T>)
	void load_bytes(const B& buffer, T& vec){
		if(buffer.data().size() < sizeof(T)){
			throw bad_byte_cast{};
		}

		std::memcpy(&vec, buffer.data().data(), sizeof(T));
	}

	template <typename B, typename V>
	struct loader_base{
		using buffer_type = B;
		using value_type = V;
		static void store(buffer_type& buf, const value_type& data) = delete;
		static void load(const buffer_type& buf, value_type& data) = delete;
	};

	template <typename V>
	struct loader_impl : loader_base<void, V>{};

	template <typename V>
	struct loader : loader_impl<V>{
	private:
		using base = loader_impl<V>;
	public:
		static typename base::buffer_type pack(const typename base::value_type& data){
			typename base::buffer_type buf;
			base::store(buf, data);
			return buf;
		}

		static typename base::value_type extract(const typename base::buffer_type& buf){
			typename base::value_type val;
			base::load(buf, val);
			return val;
		}

		static bool serialize_to(std::ostream& stream, const typename base::value_type& data){
			const auto msg = loader::pack(data);
			return static_cast<const google::protobuf::MessageLite&>(msg).SerializeToOstream(&stream);
		}

		static bool parse_from(std::istream& stream, typename base::value_type& data){
			typename base::buffer_type msg{};
			if(static_cast<google::protobuf::MessageLite&>(msg).ParseFromIstream(&stream)){
				base::load(msg, data);
				return true;
			}
			return false;
		}

	};

	template <typename V>
	struct deducing_loader;

	template <typename B, typename V>
		requires requires (B& buf, const V& data){
			requires !std::is_pointer_v<B>;
			loader_impl<V>::store(buf, data);
		}
	void store(B& buf, const V& data){
		loader_impl<V>::store(buf, data);
	}

	template <typename B, typename V>
		requires requires (B& buf, const V& data){
			io::store(buf, data);
		}
	void store(B* buf, const V& data){
		io::store(*buf, data);
	}

	template <typename B, typename V>
		requires requires (const B& buf, V& data){
			loader_impl<V>::load(buf, data);
		}
	void load(const B& buf, V& data){
		loader_impl<V>::load(buf, data);
	}

	template <typename V>
	[[nodiscard]] V extract(const typename loader_impl<V>::buffer_type& buf){
		V v EMPTY_BUFFER_INIT;
		loader_impl<V>::load(buf, v);
		return v;
	}

	template <typename V>
		requires requires(const V& data){
			loader<V>::pack(data);
		}
	auto pack(const V& data){
		return loader<V>::pack(data);
	}


	template <>
	struct loader_impl<math::vector2<float>> : loader_base<pb::math::vector2f, math::vector2<float>>{
		static void store(buffer_type& buf, const value_type& data){
			buf.set_x(data.x);
			buf.set_y(data.y);
		}

		static void load(const buffer_type& buf, value_type& data){
			data.x = buf.x();
			data.y = buf.y();
		}
	};

	template <>
	struct loader_impl<math::vector2<std::uint32_t>> : loader_base<pb::math::vector2u, math::vector2<std::uint32_t>>{
		static void store(buffer_type& buf, const value_type& data){
			buf.set_x(data.x);
			buf.set_y(data.y);
		}

		static void load(const buffer_type& buf, value_type& data){
			data.x = buf.x();
			data.y = buf.y();
		}
	};

	template <>
	struct loader_impl<math::vector2<std::int32_t>> : loader_base<pb::math::vector2i, math::vector2<std::int32_t>>{
		static void store(buffer_type& buf, const value_type& data){
			buf.set_x(data.x);
			buf.set_y(data.y);
		}

		static void load(const buffer_type& buf, value_type& data){
			data.x = buf.x();
			data.y = buf.y();
		}
	};

	template <>
	struct loader_impl<math::rect_ortho<float>> : loader_base<pb::math::rect_orthof, math::rect_ortho<float>>{
		static void store(buffer_type& buf, const value_type& data){
			io::store(buf.mutable_src(), data.src);
			io::store(buf.mutable_size(), data.size());
		}

		static void load(const buffer_type& buf, value_type& data){
			io::load(buf.src(), data.src);
			data.set_size(io::extract<value_type::vec_t>(buf.size()));
		}
	};

	template <>
	struct loader_impl<math::rect_ortho<std::int32_t>> : loader_base<pb::math::rect_orthoi, math::rect_ortho<std::int32_t>>{
		static void store(buffer_type& buf, const value_type& data){
			io::store(buf.mutable_src(), data.src);
			io::store(buf.mutable_size(), data.size());
		}

		static void load(const buffer_type& buf, value_type& data){
			io::load(buf.src(), data.src);
			data.set_size(io::extract<value_type::vec_t>(buf.size()));
		}
	};

	template <>
	struct loader_impl<math::rect_ortho<std::uint32_t>> : loader_base<pb::math::rect_orthou, math::rect_ortho<std::uint32_t>>{
		static void store(buffer_type& buf, const value_type& data){
			io::store(buf.mutable_src(), data.src);
			io::store(buf.mutable_size(), data.size());
		}

		static void load(const buffer_type& buf, value_type& data){
			io::load(buf.src(), data.src);
			data.set_size(io::extract<value_type::vec_t>(buf.size()));
		}
	};

	template <typename Ang>
	struct loader_impl<math::transform2<Ang>> : loader_base<pb::math::transform2f, math::transform2<Ang>>{
		static void store(pb::math::transform2f& buf, const math::transform2<Ang>& data){
			io::store(buf.mutable_vec(), data.vec);
			buf.set_rot(static_cast<float>(data.rot));
		}

		static void load(const pb::math::transform2f& buf, math::transform2<Ang>& data){
			io::load(buf.vec(), data.vec);
			data.rot = Ang{buf.rot()};
		}
	};

	template <>
	struct loader_impl<math::quad<float>> : loader_base<pb::math::quadf, math::quad<float>>{
		static void store(buffer_type& buf, const value_type& data){
			io::store(buf.mutable_v0(), data.v0);
			io::store(buf.mutable_v1(), data.v1);
			io::store(buf.mutable_v2(), data.v2);
			io::store(buf.mutable_v3(), data.v3);
		}

		static void load(const buffer_type& buf, value_type& data){
			io::load(buf.v0(), data.v0);
			io::load(buf.v1(), data.v1);
			io::load(buf.v2(), data.v2);
			io::load(buf.v3(), data.v3);
		}
	};

	template <>
	struct loader_impl<math::quad_bounded<float>> : loader_base<pb::math::quadf, math::quad_bounded<float>>{
		static void store(buffer_type& buf, const value_type& data){
			io::store(buf, data.view_as_quad());
		}

		static void load(const buffer_type& buf, value_type& data){
			const auto temp = loader<math::fquad>::extract(buf);
			data = value_type{temp.v0, temp.v1, temp.v2, temp.v3};
		}
	};

	template <>
	struct loader_impl<math::rect_box<float>> : loader_base<pb::math::quadf, math::rect_box<float>>{
		static void store(buffer_type& buf, const value_type& data){
			io::store(buf, data.view_as_quad());
		}

		static void load(const buffer_type& buf, value_type& data){
			const auto temp = io::extract<math::fquad>(buf);
			data = value_type{temp.v0, temp.v1, temp.v2, temp.v3};
		}
	};

	template <>
	struct loader_impl<math::rect_box_identity<float>> : loader_base<pb::math::rectf_identity, math::rect_box_identity<float>>{
		static void store(buffer_type& buf, const value_type& data){
			io::store(buf.mutable_offset(), data.offset);
			io::store(buf.mutable_size(), data.size);
		}

		static void load(const buffer_type& buf, value_type& data){
			io::load(buf.offset(), data.offset);
			io::load(buf.size(), data.size);
		}
	};

	template <>
	struct loader_impl<math::rect_box_posed> : loader_base<pb::math::rectf_posed, math::rect_box_posed>{
		static void store(buffer_type& buf, const value_type& data){
			io::store(buf.mutable_identity(), data.get_identity());
			io::store(buf.mutable_trans(), data.deduce_transform());
		}

		static void load(const buffer_type& buf, value_type& data){
			const auto idt = io::extract<value_type::rect_idt_t>(buf.identity());
			const auto trs = io::extract<value_type::trans_t>(buf.trans());
			data = {idt, trs};
		}
	};


}