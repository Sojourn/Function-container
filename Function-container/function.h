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
	struct IFunctionImpl
	{
		virtual ~IFunctionImpl() {}
		virtual T call(Args... args) = 0;
		virtual void copyTo(void *where) const = 0;
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

	Function(const Function &other)
	{
		if (!other.isEmpty())
			other.getFunction()->copyTo(function_);
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

	~Function()
	{
		fini();
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
	static inline bool isAligned(const void *ptr, size_t alignment)
	{
		return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0;
	}

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

		assert(isAligned(function_, sizeof(void*)));
		new (function_)FunctionImpl<Func>(func);
	}

	void fini()
	{
		if (!isEmpty())
			getFunction()->~IFunctionImpl();
	}

	void setEmpty()
	{
		(*reinterpret_cast<uint32_t*>(function_)) = 0;
	}

	bool isEmpty() const
	{
		return (*reinterpret_cast<const uint32_t*>(function_)) == 0;
	}

private:
	uint8_t function_[capacity];
};

#endif // FUNCTION_H