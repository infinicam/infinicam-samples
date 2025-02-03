#include "stdafx.h"
#include "RingBuffer.h"

RingBuffer::RingBuffer(std::size_t bufferSize, std::size_t imageSize)
	: bufferSize(bufferSize), imageSize(imageSize), head(0), tail(0), isFull(false)
{
	buffer = std::make_unique<unsigned char[]>(bufferSize * imageSize);
}

RingBuffer::~RingBuffer()
{
}

void RingBuffer::AddImage(const unsigned char* imageData) 
{
	std::memcpy(buffer.get() + (tail * imageSize), imageData, imageSize);
	tail = (tail + 1) % bufferSize;

	if (isFull)
	{
		head = (head + 1) % bufferSize;
	}
	else if (tail == head)
	{
		isFull = true;
	}
}

const unsigned char* RingBuffer::GetImage()
{
	if (!IsEmpty())
	{
		const unsigned char* image = buffer.get() + (head * imageSize);
		head = (head + 1) % bufferSize;
		return image;
	}
	else
	{
		throw std::runtime_error("Buffer is empty");
	}
}

double RingBuffer::GetFillPercentage() const
{
	if (isFull)
	{
		return 100.0;
	}

	return (static_cast<double>(tail >= head ? tail - head : bufferSize + tail - head) / bufferSize) * 100.0;
}

std::size_t RingBuffer::GetImageCount() const
{
	if (isFull)
	{
		return bufferSize;
	}

	return (tail >= head ? tail - head : bufferSize + tail - head);
}

bool RingBuffer::IsEmpty() const
{
	return !isFull && head == tail;
}

void RingBuffer::Clear()
{
	head = 0;
	tail = 0;
	isFull = false;
}

std::size_t RingBuffer::GetBufferSize() const
{
	return bufferSize;
}

std::size_t RingBuffer::GetImageSize() const
{
	return imageSize;
}



