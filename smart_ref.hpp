#ifndef __SMART_REF___
#define __SMART_REF___
#include <memory>

namespace nicehero
{
	template <class T>
	class const_smart_ref {
	public:
		const_smart_ref(const T* t) {
			pValue = t;
		}
		const_smart_ref(const T& t) {
			T* nt = new T(t);
			value = std::unique_ptr<T>(nt);
		}
		const_smart_ref() = delete;
		const T* pValue = nullptr;
		mutable std::unique_ptr<T> value;
		operator const T& () const noexcept {
			if (pValue) {
				return *pValue;
			}
			return *value.get();
		}
		const T* operator ->() {
			if (pValue) {
				return pValue;
			}
			return value.get();
		}
		const T& operator *() {
			if (pValue) {
				return *pValue;
			}
			return *value.get();
		}
	};

	template <class T>
	class smart_ref {
	public:
		smart_ref(T* t) {
			pValue = t;
		}
		smart_ref(const T& t) {
			T* nt = new T(t);
			value = std::unique_ptr<T>(nt);
		}
		smart_ref() = delete;
		T* pValue = nullptr;
		mutable std::unique_ptr<T> value;
		operator T& () const noexcept {
			if (pValue) {
				return *pValue;
			}
			return *value.get();
		}
		T* operator ->() {
			if (pValue) {
				return pValue;
			}
			return value.get();
		}
		T& operator *() {
			if (pValue) {
				return *pValue;
			}
			return *value.get();
		}
	};

	template <>
	class smart_ref<int> {
	public:
		smart_ref(int* t) {
			pValue = t;
		}
		smart_ref(const int& t) {
			value = t;
		}
		smart_ref() = delete;
		int* pValue = nullptr;
		mutable int value;
		operator int& () const noexcept {
			if (pValue) {
				return *pValue;
			}
			return value;
		}
		int& operator *() {
			if (pValue) {
				return *pValue;
			}
			return value;
		}
	};
}
#endif
