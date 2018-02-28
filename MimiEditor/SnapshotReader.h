#pragma once
#include "File.h"

namespace Mimi
{
	class Snapshot;

	class SnapshotReader final
	{
		friend class Snapshot;

	public:
		SnapshotReader(Snapshot* snapshot)
			: SnapshotPtr(snapshot)
		{
			AbsPosition = BufferIndex = BufferOffset = 0;
		}
		SnapshotReader(const SnapshotReader&) = delete;
		SnapshotReader(SnapshotReader&&) = delete;
		SnapshotReader& operator= (const SnapshotReader&) = delete;
		~SnapshotReader() {}

	private:
		Snapshot* SnapshotPtr;
		std::size_t AbsPosition;
		std::size_t BufferIndex;
		std::size_t BufferOffset;

	public:
		std::size_t GetSize();

		bool Read(std::uint8_t* buffer, std::size_t bufferLen, std::size_t* numRead);
		bool Skip(std::size_t num);

		bool Reset()
		{
			AbsPosition = BufferIndex = BufferOffset = 0;
			return true;
		}

		std::size_t GetPosition()
		{
			return AbsPosition;
		}
	};

	class SnapshotFileReader final : IFileReader
	{
	public:
		SnapshotFileReader(Snapshot* snapshot)
			: Reader(snapshot)
		{
		}
		SnapshotFileReader(const SnapshotFileReader&) = delete;
		SnapshotFileReader(SnapshotFileReader&&) = delete;
		SnapshotFileReader& operator= (const SnapshotFileReader&) = delete;
		virtual ~SnapshotFileReader() {}

	public:
		SnapshotReader Reader;

	public:
		virtual std::size_t GetSize() override
		{
			return Reader.GetSize();
		}

		virtual bool Read(std::uint8_t* buffer, std::size_t bufferLen, std::size_t* numRead) override
		{
			return Reader.Read(buffer, bufferLen, numRead);
		}

		virtual bool Skip(std::size_t num) override
		{
			return Reader.Skip(num);
		}

		virtual bool Reset() override
		{
			return Reader.Reset();
		}

		virtual std::size_t GetPosition() override
		{
			return Reader.GetPosition();
		}
	};
}
