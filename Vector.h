#pragma once
#include <cstring>
#include <cassert>

// POD dynamic array
template <typename T>
struct Vector
{
	T* data;
	unsigned int size; // Size of allocated space in bytes
	unsigned int length; // Number of elements

	void initialize(const unsigned int& nElements = 0)
	{
		size = nElements;
		data = new T[size];
		length = nElements;
	}

	void free()
	{
		delete[] data;
		data = nullptr;
	}

	void push(const T& element) 
	{
		length++;
		if (length > size)
		{
			size = length;
			T* temp = new T[size];
			memmove(temp, data, sizeof(T) * (length - 1));
			delete[] data;
			data = temp;
		}
		data[length - 1] = element;
	}

	void pop()
	{
		assert(("Cannot pop if length is 0", length > 0));
		length--;
	}

	void remove(const unsigned int& index)
	{
		assert(("Vector index out of range", index < length));
		length--;
		memmove(data + index, data + index + 1, sizeof(T) * (length - index));
	}

	T& operator [](const unsigned int& index)
	{
		assert(("Vector index out of range", index < length));
		return data[index];
	}

	T operator [](const unsigned int& index) const
	{
		assert(("Vector index out of range", index < length));
		return data[index];
	}

	void reserve(const unsigned int& nElements)
	{
		assert(("Cannot reserve less than or equal to length elements", nElements > length));
		size = nElements;
		T* temp = new T[size];
		memmove(temp, data, sizeof(T) * length);
		delete[] data;
		data = temp;
	}

	void resize(const unsigned int& nElements)
	{
		reserve(nElements);
		length = nElements;
	}

	int find(const T& element)
	{
		for (unsigned int i = 0; i < length; i++)
		{
			if (data[i] == element)
				return i;
		}
		return -1;
	}
};