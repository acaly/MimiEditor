#pragma once

#include "CommonInternal.h"
#include "ShortVector.h"
#include "ModificationTracer.h"
#include "Buffer.h"
#include <cstdint>
#include <vector>

namespace Mimi
{
	struct PerCharacterData
	{
		std::uint8_t RenderWidth;
	};

	namespace LabelType
	{
		enum LabelTypeValues : std::uint8_t
		{
			Empty = 0, //not a label

			Line = 1,
			Point = 2,
			RangeAny = 4, //mask only
			RangeOpen = 5,
			RangeClose = 6,
			Reserve = 7,
			Topology = 7, //mask only

			Left = 8,
			Right = 16,
			Center = 32,
			Alignment = Left | Right | Center, //mask only

			Continuous = 64,
			Unfinished = 128,
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
		//Index of Label handers (registered in the system).
		//For continuous flag, this is the index of previous label.
		std::uint16_t Handler;
		//Position of the Label (updated by the segment).
		std::uint16_t Position;
	};

	struct Label
	{
		std::uint8_t Topology;
		std::uint8_t Data;
		std::uint16_t Handler;
		std::uint16_t Position1;
		std::uint16_t Position2;
		bool IsContinuous;
		bool IsUnfinished;
	};
	
	class Document;
	class TextSegmentList;

	class ActiveTextSegmentData
	{
	public:
		bool IsModifiedSinceOpen;
		bool IsModifiedSinceSave;
		bool IsModifiedSinceSnapshot;
		bool IsModifiedSinceRendered;

		std::uint32_t LastModifiedTime;

		DynamicBuffer ContentBuffer;
		DynamicBuffer CharacterDataBuffer;
		ShortVector<StaticBuffer> SnapshotBuffers;
		ModificationTracer Modifications;
	};

	class TextSegment
	{
	public:
		TextSegment(StaticBuffer buffer);

		TextSegment(const TextSegment&) = delete;
		TextSegment(TextSegment&&) = delete;
		TextSegment& operator= (const TextSegment&) = delete;

		~TextSegment();

	private:
		TextSegmentList* Parent;
		std::uint16_t Index;
		bool IsContinuous;
		bool IsUnfinished;

	private:
		StaticBuffer ContentBuffer;
		StaticBuffer CharacterDataBuffer;
		ActiveTextSegmentData* ActiveData;

	private:
		ShortVector<LabelData> Labels;

		//render cache?
		//render height?

	public:
		TextSegmentList* GetParent();
		std::uint16_t GetIndexInList();
		TextSegment* GetPreviousSegment();
		TextSegment* GetNextSegment();
		bool IsContinuous();
		bool IsUnfinished();
		Document* GetDocument();
		std::uint32_t GetLineNumber();

	private:
		void MakeDynamic();
		void MakeStatic();
		void SplitLeft(std::uint32_t pos);
		void SplitRight(std::uint32_t pos);
		void MergeWithLeft();
		void MergeWithRight();

	public:
		//Replace text within the range given by a label. New content is given in a buffer.
		//The label parameter can be either a point or range label. If the label is point,
		//it is moved to where the replace ends. If it is range, it is updated to cover the
		//whole content or changed to a point label depending on the option parameter.
		//Use null buffer pointer to delete text.
		//Return the position after the inserted content.
		std::uint32_t ReplaceText(std::uint32_t labelSelection, DynamicBuffer* content,
			bool collapseLabel);
		void CheckAndMakeStatic(std::uint32_t time);

	public:
		StaticBuffer MakeSnapshot();
		void DisposeSnapshot(std::uint32_t id);
		std::uint32_t ConvertSnapshotPosition(std::uint32_t pos);

	public:
		std::uint32_t AddLineLabel(std::uint32_t handler, std::uint8_t data, bool continuous);
		std::uint32_t AddPointLabel(std::uint32_t pos, std::uint32_t handler, std::uint8_t data,
			bool continuous);
		std::uint32_t AddRangeLabel(std::uint32_t pos1, std::uint32_t pos2, std::uint32_t handler,
			std::uint8_t data, bool continuous);
		void RemoveLabel(std::uint32_t id);
		void GetLabel(std::uint32_t id, Label* result);
		//enumerate label

	public:
		//enumerate char
	};
}
