#include "SnapshotReader.h"
#include "Buffer.h"
#include "Snapshot.h"

std::size_t Mimi::SnapshotReader::GetSize()
{
	return SnapshotPtr->DataLength;
}

bool Mimi::SnapshotReader::Read(std::uint8_t* buffer, std::size_t bufferLen, std::size_t* numRead)
{
	assert(BufferIndex < SnapshotPtr->BufferList.size());
	StaticBuffer src = SnapshotPtr->BufferList[BufferIndex];
	assert(src.GetSize() >= BufferOffset);
	std::size_t numReadVal = 0;
	std::uint8_t* bufferPtr = buffer;
	while (numReadVal < bufferLen)
	{
		if (src.GetSize() == BufferOffset)
		{
			if (BufferIndex == SnapshotPtr->BufferList.size() - 1)
			{
				break;
			}
			BufferIndex += 1;
			BufferOffset = 0;
			src = SnapshotPtr->BufferList[BufferIndex];
		}
		std::size_t canRead = src.GetSize() - BufferOffset;
		if (canRead > bufferLen - numReadVal)
		{
			canRead = bufferLen - numReadVal;
		}
		std::memcpy(bufferPtr, &src.GetRawData()[BufferOffset], canRead);
		bufferPtr += canRead;
		numReadVal += canRead;
		BufferOffset += canRead;
		AbsPosition += canRead;
	}
	if (numRead)
	{
		*numRead = numReadVal;
	}
	return true;
}

bool Mimi::SnapshotReader::Skip(std::size_t num)
{
	assert(BufferIndex < SnapshotPtr->BufferList.size());
	StaticBuffer src = SnapshotPtr->BufferList[BufferIndex];
	assert(src.GetSize() >= BufferOffset);
	std::size_t numSkip = 0;
	while (numSkip < num)
	{
		if (src.GetSize() == BufferOffset)
		{
			if (BufferIndex == SnapshotPtr->BufferList.size() - 1)
			{
				return false;
			}
			BufferIndex += 1;
			BufferOffset = 0;
			src = SnapshotPtr->BufferList[BufferIndex];
		}
		std::size_t canSkip = src.GetSize() - BufferOffset;
		if (canSkip > num - numSkip)
		{
			canSkip = num - numSkip;
		}
		numSkip += canSkip;
		BufferOffset += canSkip;
		AbsPosition += canSkip;
	}
	return true;
}
