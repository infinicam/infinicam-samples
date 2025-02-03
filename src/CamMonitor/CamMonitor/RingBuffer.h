#pragma once

#include <string>
#include <memory>
#include <stdexcept>


class RingBuffer
{
public:
	RingBuffer(std::size_t bufferSize, std::size_t imageSize);
	~RingBuffer();

	void AddImage(const unsigned char* imageData);
	const unsigned  char* GetImage();

	double GetFillPercentage() const;
	std::size_t GetImageCount() const;
	std::size_t GetBufferSize() const;
	std::size_t GetImageSize() const;
	bool IsEmpty() const;
	void Clear();

private:
	std::size_t bufferSize;
	std::size_t imageSize;
	std::size_t head;
	std::size_t tail;
	bool isFull;
	std::unique_ptr<unsigned char[]> buffer;
};
