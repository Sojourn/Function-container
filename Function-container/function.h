#ifndef FUNCTION_H
#define FUNCTION_H

#include <cstddef>
#include <cstdint>
#include <cassert>

template<class T, size_t capacity = 4 * sizeof(void*)>
struct Function {};

template<class T, class... Args, size_t capacity>
struct Function<T(Args...), capacity>
{
	struct SerializedFunction
	{
		uint8_t raw[capacity];
	};

	struct IFunctionImpl
	{
		virtual ~IFunctionImpl() {}
		virtual T call(Args... args) = 0;
		virtual void copyTo(void *where) const = 0;
		virtual size_t size() const = 0;
	};

	template<class Func>
	struct FunctionImpl : public IFunctionImpl
	{
		FunctionImpl(Func func) :
			func_(func)
		{}

		virtual ~FunctionImpl() {}

		virtual T call(Args... args) override
		{
			return func_(args...);
		}

		virtual void copyTo(void *where) const override
		{
			new(where)FunctionImpl<Func>(func_);
		}

		virtual size_t size() const override
		{
			return sizeof(FunctionImpl<Func>);
		}

		Func func_;
	};

	Function()
	{
		setEmpty();
	}

	template<class Func>
	Function(Func func)
	{
		init(func);
	}

	Function(const SerializedFunction &func)
	{
		setEmpty();
		deserialize(func);
	}

	Function(void *buffer, size_t length)
	{
		setEmpty();
		deserialize(buffer, length);
	}

	Function(const Function &other)
	{
		if (!other.isEmpty())
			other.getFunction()->copyTo(function_);
	}

	~Function()
	{
		fini();
	}

	SerializedFunction serialize()
	{
		SerializedFunction result;
		serialize(result.raw, capacity);
		return result;
	}

	void deserialize(const SerializedFunction &func)
	{
		deserialize(func.raw, capacity);
	}

	void serialize(void *buffer, size_t length)
	{
		assert(!isEmpty());
		assert(buffer != nullptr);
		assert(size() <= length);

		std::memcpy(buffer, function_, size());
		setEmpty();
	}

	void deserialize(const void *buffer, size_t length)
	{
		assert(buffer != nullptr);
		assert(length <= capacity);

		*this = nullptr;
		std::memcpy(function_, buffer, length);
	}

	size_t size() const
	{
		return isEmpty() ? 0 : getFunction()->size();
	}

	const Function &operator=(const Function &rhs)
	{
		if (this != &rhs) {
			fini();

			if (!rhs.isEmpty())
				rhs.getFunction()->copyTo(function_);
		}

		return *this;
	}

	template<class Func>
	const Function &operator=(Func func)
	{
		fini();
		init(func);
		return *this;
	}

	const Function &operator=(nullptr_t)
	{
		fini();
		setEmpty();
		return *this;
	}

	T operator()(Args... args)
	{
		assert(!isEmpty());
		return getFunction()->call(args...);
	}

	explicit operator bool() const
	{
		return !isEmpty();
	}

private:
	union
	{
		uint8_t function_[capacity];
		void *align_vtable_ptr_;
	};

	IFunctionImpl *getFunction()
	{
		return reinterpret_cast<IFunctionImpl*>(function_);
	}

	const IFunctionImpl *getFunction() const
	{
		return reinterpret_cast<const IFunctionImpl*>(function_);
	}

	template<class Func>
	void init(Func func)
	{
		static_assert(sizeof(FunctionImpl<Func>) <= capacity,
			"Function capacity too small to hold the closure");

		new (function_)FunctionImpl<Func>(func);
	}

	void fini()
	{
		if (!isEmpty())
			getFunction()->~IFunctionImpl();
	}

	inline void setEmpty()
	{
		return setEmpty(function_);
	}

	inline void setEmpty(uint8_t *raw)
	{
		(*reinterpret_cast<uint32_t*>(raw)) = 0;
	}

	inline bool isEmpty() const
	{
		return isEmpty(function_);
	}

	inline bool isEmpty(const uint8_t *raw) const
	{
		return (*reinterpret_cast<const uint32_t*>(raw)) == 0;
	}
};

#endif // FUNCTION_H