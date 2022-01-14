#ifndef _____TYPE_____
#define _____TYPE_____

#include <stdint.h>
#include <string>
#include <memory>
#include <memory.h>
#include <utility>

const int NETWORK_BUF_SIZE = 1024 * 16;

using ui8 = uint8_t;
using i16 = int16_t;
using ui16 = uint16_t;
using i32 = int32_t;
using ui32 = uint32_t;
using i64 = int64_t;
using ui64 = uint64_t;

const ui16 SYSPING_REQ = 0xFFFA;// 65530
const ui16 SYSPING_ACK = 0xFFFB;// 65531
const ui16 PROTOCOL_VERSION_NTF = 0xFFFC;// 65532
const ui16 PROTOCOL_VERSION_AVAILABLE_NTF = 0xFFFD;// 65533
const ui16 KCP_READY_NTF = 0xFFF0;// 65529
const ui16 KCP_CLOSE = 0xFFEF;// 65528


namespace nicehero 
{
	using oper_uint_base = ui32;
	using store_uint_base = ui64;
	const store_uint_base oper_uint_base_max = store_uint_base(UINT32_MAX);

	class OperUInt;
	class StoreUInt
	{
		friend class OperUInt;
	public:
		StoreUInt() {
			impl = store_uint_base(0);
		}
		StoreUInt(store_uint_base impl_) {
			impl = store_uint_base(impl_);
		}
		bool operator>=(const store_uint_base& other) const {
			return impl >= other;
		}
		bool operator<=(const store_uint_base& other) const {
			return impl <= other;
		}
		bool operator>(const store_uint_base& other) const {
			return impl > other;
		}
		bool operator<(const store_uint_base& other) const {
			return impl < other;
		}
		bool operator==(const store_uint_base& other) const {
			return impl == other;
		}
		bool operator!=(const store_uint_base& other) const {
			return impl != other;
		}
		operator store_uint_base() const {
			return impl;
		}
		bool add(const store_uint_base& other) {
			store_uint_base ret = impl + other;
			if (ret < impl || ret < other) {
				return false;
			}
			impl = ret;
			return true;
		}
		bool add(oper_uint_base other) {
			store_uint_base o(other);
			return add(o);
		}
		bool add(OperUInt other);
		bool minus(const store_uint_base& other) {
			if (other > impl) {
				return false;
			}
			impl -= other;
			return true;
		}
		bool minus(oper_uint_base other) {
			store_uint_base o(other);
			return minus(o);
		}
		bool minus(OperUInt other);
		std::pair<bool, OperUInt> toOper();
		store_uint_base impl;
	};

	class OperUInt
	{
		friend class StoreUInt;
	public:
		OperUInt() {
			impl = oper_uint_base(0);
		}
		OperUInt(oper_uint_base impl_) {
			impl = impl_;
		}
		StoreUInt operator+(const OperUInt& right)const {
			return StoreUInt(store_uint_base(impl) + store_uint_base(right.impl));
		}
		StoreUInt operator*(const OperUInt& right)const {
			return StoreUInt(store_uint_base(impl) * (store_uint_base(right.impl)));
		}
		StoreUInt operator/(const OperUInt& right)const {
			return StoreUInt(store_uint_base(impl) / store_uint_base(right.impl));
		}
		StoreUInt operator%(const OperUInt& right)const {
			return StoreUInt(store_uint_base(impl) % (store_uint_base(right.impl)));
		}
		oper_uint_base impl;
	};
	inline bool StoreUInt::add(OperUInt other) {
		return add(other.impl);
	}
	inline bool StoreUInt::minus(OperUInt other) {
		return minus(other.impl);
	}
	inline std::pair<bool, OperUInt> StoreUInt::toOper()
	{
		if (impl > oper_uint_base_max)
		{
			return std::make_pair(false, OperUInt(oper_uint_base(0)));
		}
		return std::make_pair(true, OperUInt(oper_uint_base(impl)));
	}

	struct Binary
	{
		Binary() {}
		Binary(ui32 s, const void* data) 
			:m_Size(s)
		{
			if (s > 0)
			{
				m_Data = std::unique_ptr<char[]>(new char[m_Size]);
				memcpy(m_Data.get(), data, m_Size);
			}
		}
		ui32 m_Size = 0;
		std::unique_ptr<char[]> m_Data;
	};

	class Code
	{
	public:
		Code(ui32 value, const char *file, ui32 line);
		template <class T>
		operator T() const;
	private:
		ui32	m_value;
		const char * m_file;
		ui32	m_line;
	};

	template <class T>
	Code::operator T() const
	{
		T ret;
		ret.value = m_value;
		ret.file = m_file;
		ret.line = m_line;
		return ret;
	}

	inline Code::Code(ui32 value, const char *file, ui32 line)
	{
		m_value = value;
		m_file = file;
		m_line = line;
	}
}

#define MAKE_CODE(VALUE) nicehero::Code(VALUE,__FILE__,__LINE__)

template <typename T>
class Initializable
{
public:
	Initializable()
	{
		impl = 0;
	}
	Initializable(const T& t)
	{
		impl = t;
	}
	Initializable<T> operator+(const Initializable<T>& right) const
	{
		return Initializable<T>(impl + right.impl);
	}
	Initializable<T> operator-(const Initializable<T>& right) const
	{
		return Initializable<T>(impl - right.impl);
	}
	Initializable<T> operator*(const Initializable<T>& right) const
	{
		return Initializable<T>(impl * right.impl);
	}
	Initializable<T> operator/(const Initializable<T>& right) const
	{
		return Initializable<T>(impl / right.impl);
	}
	Initializable<T> operator%(const Initializable<T>& right) const
	{
		return Initializable<T>(impl % right.impl);
	}
	Initializable<T> operator>>(const Initializable<T>& right) const
	{
		return Initializable<T>(impl >> right.impl);
	}
	Initializable<T> operator<<(const Initializable<T>& right) const
	{
		return Initializable<T>(impl << right.impl);
	}
	Initializable<T> operator&(const Initializable<T>& right) const
	{
		return Initializable<T>(impl & right.impl);
	}
	Initializable<T> operator|(const Initializable<T>& right) const
	{
		return Initializable<T>(impl | right.impl);
	}
	Initializable<T> operator^(const Initializable<T>& right) const
	{
		return Initializable<T>(impl ^ right.impl);
	}
	Initializable<T>& operator=(const Initializable<T>& other)
	{
		impl = other.impl;
		return *this;
	}
	Initializable<T>& operator+=(const Initializable<T>& other)
	{
		impl += other.impl;
		return *this;
	}
	Initializable<T>& operator-=(const Initializable<T>& other)
	{
		impl -= other.impl;
		return *this;
	}
	Initializable<T>& operator*=(const Initializable<T>& other)
	{
		impl *= other.impl;
		return *this;
	}
	Initializable<T>& operator/=(const Initializable<T>& other)
	{
		impl /= other.impl;
		return *this;
	}
	Initializable<T>& operator%=(const Initializable<T>& other)
	{
		impl %= other.impl;
		return *this;
	}
	Initializable<T>& operator>>=(const Initializable<T>& other)
	{
		impl >>= other.impl;
		return *this;
	}
	Initializable<T>& operator<<=(const Initializable<T>& other)
	{
		impl <<= other.impl;
		return *this;
	}
	Initializable<T>& operator&=(const Initializable<T>& other)
	{
		impl &= other.impl;
		return *this;
	}
	Initializable<T>& operator|=(const Initializable<T>& other)
	{
		impl |= other.impl;
		return *this;
	}
	Initializable<T>& operator^=(const Initializable<T>& other)
	{
		impl ^= other.impl;
		return *this;
	}
	Initializable<T>& operator ++()
	{
		++impl;
		return *this;
	}
	const Initializable<T> operator ++(int)
	{
		Initializable<T> old = *this;
		++impl;
		return old;
	}
	Initializable<T>& operator --()
	{
		--impl;
		return *this;
	}
	const Initializable<T> operator --(int)
	{
		Initializable<T> old = *this;
		--impl;
		return old;
	}
	Initializable<T> operator~()
	{
		return ~impl;
	}
	bool operator!()
	{
		return !impl;
	}
	bool operator==(const Initializable<T>& other)
	{
		return impl == other.impl;
	}
	bool operator!=(const Initializable<T>& other)
	{
		return impl != other.impl;
	}
	bool operator<(const Initializable<T>& other)
	{
		return impl < other.impl;
	}
	bool operator>(const Initializable<T>& other)
	{
		return impl > other.impl;
	}
	bool operator>=(const Initializable<T>& other)
	{
		return impl >= other.impl;
	}
	bool operator<=(const Initializable<T>& other)
	{
		return impl <= other.impl;
	}
	operator T()
	{
		return impl;
	}
	T impl;
};

template<int ...>
struct seq { };
template<int N, int ...S>
struct gens : gens<N-1, N-1, S...> { };
template<int ...S>
struct gens<0, S...> {
  typedef seq<S...> type;
};

#endif

