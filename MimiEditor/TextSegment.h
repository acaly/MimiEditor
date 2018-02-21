#pragma once
#include "ShortVector.h"
#include "ModificationTracer.h"
#include "Buffer.h"
#include "TextDocumentPosition.h"
#include "TextDocumentLabel.h"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <limits>

namespace Mimi
{
	class TextDocument;
	class TextSegment;
	class TextSegmentList;
	class LabelOwnerChangedEvent;
	struct TextDocumentLabelAccess;

	struct ModifiedFlag
	{
	public:
		ModifiedFlag() = default;
		ModifiedFlag(std::uint8_t val) : Value(val) {}

	public:
		static const std::uint8_t Open = 1, Save = 2, Snapshot = 4, Render = 8;
		static const std::uint8_t All = Open | Save | Snapshot | Render;
		static const std::uint8_t NotModified = 0;

	public:
		std::uint8_t Value;

	public:
		void Modify()
		{
			Value |= All;
		}

		bool SinceOpen() { return Value & Open; }
		bool SinceSave() { return Value & Save; }
		bool SinceSnapshot() { return Value & Snapshot; }
		bool SinceRender() { return Value & Render; }

		void ClearSave() { Value &= ~Save; }
		void ClearSnapshot() { Value &= ~Snapshot; }
		void ClearRender() { Value &= ~Render; }
	};

	struct ContinuousFlag
	{
	public:
		ContinuousFlag() = default;
		ContinuousFlag(bool continuous, bool unfinished)
		{
			Value = 0;
			SetContinuous(continuous);
			SetUnfinished(unfinished);
		}

	public:
		static const std::uint8_t Continuous = 1, Unfinished = 2;

	public:
		std::uint8_t Value;

	public:
		bool IsContinuous() { return Value & Continuous; }
		bool IsUnfinished() { return Value & Unfinished; }
		void SetContinuous(bool val)
		{
			Value = (Value & ~Continuous) | (val << 0);
		}
		void SetUnfinished(bool val)
		{
			Value = (Value & ~Unfinished) | (val << 1);
		}
	};

	class ActiveTextSegmentData final
	{
	public:
		ActiveTextSegmentData(StaticBuffer content)
			: ContentBuffer(content)
		{
			LastModifiedTime = 0;
			SnapshotCache = content.MoveRef();
		}

		ActiveTextSegmentData()
			: ContentBuffer(0)
		{
			LastModifiedTime = 0;
			SnapshotCache.Clear();
		}

		~ActiveTextSegmentData()
		{
			SnapshotCache.TryClearRef();
		}

	public:
		std::uint32_t LastModifiedTime;

		DynamicBuffer ContentBuffer;
		StaticBuffer SnapshotCache;
		ModificationTracer Modifications;
	};

	class TextSegment final
	{
		friend class TextSegmentList; //Index, Parent
		friend struct TextDocumentLabelAccess; //ReadLabelData
		friend class TextDocumentLabelIterator; //Multiple Label functions
		friend class LabelOwnerChangedEvent; //ReadLabelData
		friend class TextDocument; //Label manipulation

		static const std::size_t MaxLength = 0xFFFF;

	public:
		TextSegment(DynamicBuffer& buffer, bool continuous, bool unfinished, ModifiedFlag modified);
		TextSegment(bool continuous, bool unfinished, ModifiedFlag modified);

		TextSegment(const TextSegment&) = delete;
		TextSegment(TextSegment&&) = delete;
		TextSegment& operator= (const TextSegment&) = delete;

		~TextSegment();

	private:
		TextSegmentList* Parent;
		std::uint16_t Index;

		ContinuousFlag Continuous;
		ModifiedFlag Modified;

		StaticBuffer ContentBuffer;
		ShortVector<LabelData> Labels;

		ActiveTextSegmentData* ActiveData;
		//render height?

	private:
		void AddToList(TextSegmentList* list, std::size_t index);

	public:
		TextSegmentList* GetParent()
		{
			return Parent;
		}

		std::size_t GetIndexInList()
		{
			return Index;
		}

		TextSegment* GetPreviousSegment();
		TextSegment* GetNextSegment();

		//0: same or error, 1: s1 is after s2, -1: s1 is before s2.
		static int ComparePosition(TextSegment* s1, TextSegment* s2);

		bool IsContinuous()
		{
			return Continuous.IsContinuous();
		}

		bool IsUnfinished()
		{
			return Continuous.IsUnfinished();
		}

		TextDocument* GetDocument();
		std::size_t GetLineIndex();

		bool IsActive()
		{
			return ActiveData != nullptr;
		}

		std::size_t GetCurrentLength()
		{
			if (IsActive())
			{
				return ActiveData->ContentBuffer.GetLength();
			}
			return ContentBuffer.GetSize();
		}

	private:
		void MakeActive();
		void MakeInactive();
		TextSegment* Split(std::size_t pos, bool newLine);
		void Merge();

	public:
		//Change content and update labels.
		void ReplaceText(std::size_t pos, std::size_t sel, DynamicBuffer* content, std::size_t globalPosition);

		void MarkModified(std::uint32_t time)
		{
			if (IsActive())
			{
				ActiveData->LastModifiedTime = time;
			}
		}

		void CheckAndMakeInactive(std::uint32_t time);

		DocumentPositionS InsertLineBreak(std::size_t pos)
		{
			return { Split(pos, true), 0 };
		}

		void EnsureInsertionSize(std::size_t pos, std::size_t size,
			DocumentPositionS* before = nullptr, DocumentPositionS* after = nullptr);

		bool HasLineBreak();
		void CheckLineBreak();

	public:
		StaticBuffer MakeSnapshot(bool resize);
		void DisposeSnapshot(std::size_t num, bool resize);
		std::size_t ConvertSnapshotPosition(std::size_t snapshot, std::size_t pos, int dir);
		std::size_t GetHistoryLength(std::size_t snapshot);

	private:
		static std::size_t GetLabelLength(LabelData* label)
		{
			if (label->Type == 0) return 0;
			std::size_t ret = 1;
			if ((label->Type & LabelType::Topology) == LabelType::Range)
			{
				ret += 1;
			}
			if (label->Type & LabelType::Long)
			{
				ret += 1;
			}
			return ret;
		}

		std::size_t AllocateLabelSpace(std::size_t size)
		{
			assert(size > 0);
			LabelData* first = Labels.GetPointer();
			std::size_t count = 0;
			for (std::size_t i = 0; i < Labels.GetCount(); ++i)
			{
				auto len = GetLabelLength(&first[i]);
				if (len == 0)
				{
					if (++count == size)
					{
						return i - (size - 1);
					}
				}
				else
				{
					count = 0;
				}
			}
			//Remove empty from tail.
			if (count > 0)
			{
				Labels.RemoveRange(Labels.GetCount() - count, count);
			}
			return Labels.Emplace(size) - Labels.GetPointer(); //Possible reallocation
		}

		void EraseLabelSpace(std::size_t index, std::size_t size)
		{
			std::memset(ReadLabelData(index), 0, size * sizeof(LabelData));
		}

		LabelData* ReadLabelData(std::size_t index)
		{
			return &Labels.GetPointer()[index];
		}

		std::size_t GetLabelIndex(LabelData* label)
		{
			return label - Labels.GetPointer();
		}

		std::size_t FirstLabel()
		{
			return std::numeric_limits<std::size_t>::max();
		}

		bool NextLabel(std::size_t* index)
		{
			std::size_t len;
			if (*index == FirstLabel())
			{
				if (Labels.GetCount() == 0)
				{
					return false;
				}
				*index = 0;
			}
			else
			{
				len = GetLabelLength(ReadLabelData(*index));
				*index += len;
			}
			len = GetLabelLength(ReadLabelData(*index));
			while (len == 0 && *index < Labels.GetCount())
			{
				*index += 1;
				len = GetLabelLength(ReadLabelData(*index));
			}
			return len != 0;
		}

	private:
		void UpdateRangeLabel(std::size_t i, std::size_t pos, std::size_t sel,
			std::size_t insertLen, std::size_t globalPosition);

		void UpdateLabels(std::size_t pos, std::size_t sel, std::size_t insertLen,
			std::size_t globalPosition);
		void UpdateLabelsDeleteAll(TextSegment* moveBack, TextSegment* moveForward,
			std::size_t globalPosition);

		void MoveLabels(TextSegment* dest, std::size_t begin);

		void LabelSplit(TextSegment* other, std::size_t pos)
		{
			MoveLabels(other, pos);
		}

		void LabelMerge(TextSegment* other)
		{
			assert(other == GetPreviousSegment());
			MoveLabels(other, 0);
		}

		void NotifyLabelOwnerChanged(TextSegment* newOwner, std::size_t begin, std::size_t end, std::ptrdiff_t change);
		void NotifyLabelOwnerChanged(TextSegment* newOwner, std::size_t id, std::size_t newId);
		void NotifyLabelRemoved(std::size_t index, std::size_t globalPosition);
	};
}
