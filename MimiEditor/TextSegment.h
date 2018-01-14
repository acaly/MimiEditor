#pragma once

#include "CommonInternal.h"
#include "ShortVector.h"
#include "ModificationTracer.h"
#include "Buffer.h"
#include <cstdint>
#include <vector>

namespace Mimi
{
	class Document;
	class TextSegmentList;
	class TextSegmentRenderCache;

	namespace LabelType
	{
		enum LabelTypeValues : std::uint8_t
		{
			Empty = 0, //not a label

			Line = 1,
			Point = 2,
			Range = 3,
			Topology = 3, //mask only

			Long = 4,

			Left = 8,
			Right = 16,
			Center = 24,
			Alignment = Left | Right | Center, //mask only

			Continuous = 32,
			Unfinished = 64,

			Referred = 128,
		};
	}

	//About Label
	//Label is a auto-modified anchor in the TextSegment. Whenever the data is
	//modified, the position is also modified.
	//Labels are all stored in an array (ShortVector) in each segment. When the
	//api user needs to obtain a reference to a label, the index in the array is
	//returned. Given the index, the segment is able to provide the position at
	//any time. When Labels are created and deleted, the array resizes but the
	//existing Labels are not moved to different indexes. New Labels will be created
	//at the vacant slot if possible. This gives best performance on update and
	//position calculation, and also minimum memory footprint, but creating new
	//Labels might be a little bit slow (need to find a free slot).
	//As an optimization, slot 0 and 1 are always reserved for cursor.
	struct LabelData
	{
		//Tells the segment how to update the position.
		std::uint8_t Type;
		//Data defined by Handler to store additional data.
		std::uint8_t Data;
		union
		{
			struct
			{
				//Index of Label handers (registered in the system).
				std::uint16_t Handler;
				//Position of the Label (updated by the segment).
				std::uint16_t Position;
			};
			struct
			{
				//Index of Label handers (registered in the system).
				std::uint16_t Previous;
				//Position of the Label (updated by the segment).
				std::uint16_t Next;
			};
			std::uint32_t Additional;
		};
	};

	class LabelOwnerChangeEvent
	{
		friend class TextSegment;
	private:
		TextSegment* OldOwner;
		TextSegment* NewOwner;
		std::uint32_t BeginPosition;
		std::uint32_t EndPositon;
	};

	struct ModifiedFlag
	{
	public:
		const std::uint8_t Open = 1, Save = 2, Snapshot = 4, Render = 8;
		const std::uint8_t ModifyMask = Open | Save | Snapshot | Render;

	public:
		std::uint8_t Value;

	public:
		void Modify()
		{
			Value |= ModifyMask;
		}

		bool SinceOpen() { return Value | Open; }
		bool SinceSave() { return Value | Save; }
		bool SinceSnapshot() { return Value | Snapshot; }
		bool SinceRender() { return Value | Render; }
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
		const std::uint8_t Continuous = 1, Unfinished = 2;

	public:
		std::uint8_t Value;

	public:
		bool IsContinuous() { return Value | Continuous; }
		bool IsUnfinished() { return Value | Unfinished; }
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
			SnapshotCache = content.NewRef();
		}

		ActiveTextSegmentData()
			: ContentBuffer(0)
		{
			LastModifiedTime = 0;
			SnapshotCache.Clear();
		}

		~ActiveTextSegmentData()
		{
			SnapshotCache.ClearRef();
		}

	public:
		std::uint32_t LastModifiedTime;

		DynamicBuffer ContentBuffer;
		StaticBuffer SnapshotCache;
		ModificationTracer Modifications;
	};

	class TextSegment final
	{
		friend class TextSegmentList;

	public:
		TextSegment(DynamicBuffer& buffer, bool continuous, bool unfinished);
		TextSegment(bool continuous, bool unfinished);

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
		TextSegmentRenderCache* RenderCache;
		//render height?

	private:
		void AddToList(TextSegmentList* list, std::uint32_t index)
		{
			assert(Parent == nullptr);
			assert(list != nullptr);
			Parent = list;
			Index = index;
		}

	public:
		TextSegmentList* GetParent()
		{
			return Parent;
		}

		std::uint16_t GetIndexInList()
		{
			return Index;
		}

		TextSegment* GetPreviousSegment();
		TextSegment* GetNextSegment();

		bool IsContinuous()
		{
			return Continuous.IsContinuous();
		}

		bool IsUnfinished()
		{
			return Continuous.IsUnfinished();
		}

		Document* GetDocument();
		std::uint32_t GetLineNumber();

		bool IsActive()
		{
			return ActiveData != nullptr;
		}

	private:
		void MakeActive();
		void MakeInactive();
		void Split(std::uint32_t pos, bool newLine);
		void Merge();

	public:
		//Replace text within the range given by a label. New content is given in a buffer.
		//The label parameter can be either a point or range label. If the label is point,
		//it is moved to where the replace ends. If it is range, it is updated to cover the
		//whole content or changed to a point label depending on the option parameter.
		//Use null buffer pointer to delete text.
		//Return the position after the inserted content.
		std::uint32_t ReplaceText(std::uint32_t labelSelection, DynamicBuffer* content,
			bool collapseLabel);
		void MarkModified(std::uint32_t time);
		void CheckAndMakeStatic(std::uint32_t time);

	public:
		TextSegmentRenderCache* GetRenderCache();
		void DisposeRenderCache();

	public:
		StaticBuffer MakeSnapshot();
		void DisposeSnapshot(std::uint32_t id);
		std::uint32_t ConvertSnapshotPosition(std::uint32_t pos);

	private:
		void LabelSplit(TextSegment* other, std::uint32_t pos);
		void LabelMerge(TextSegment* other);

	public: //TODO consider separating to new class
		std::uint32_t AddLineLabel(std::uint32_t handler, std::uint8_t data);
		std::uint32_t AddPointLabel(std::uint32_t pos, std::uint32_t handler, std::uint8_t data);
		std::uint32_t AddRangeLabel(std::uint32_t pos1, std::uint32_t pos2, std::uint32_t handler,
			std::uint8_t data, bool continuous);
		void RemoveLabel(std::uint32_t id);
		void GetLabel(std::uint32_t id /*, Label* result*/); //TODO support continuous
		void NotifyLabelOwnerChange(); //TODO
		//enumerate label

	public:
		//enumerate char
	};
}
