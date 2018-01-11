#pragma once

#include "CommonInternal.h"
#include "ShortVector.h"
#include "ModificationTracer.h"
#include "Buffer.h"
#include <cstdint>
#include <vector>

namespace Mimi
{
	typedef std::uint8_t CharacterRenderPosition;
	typedef std::uint8_t CharacterRenderStyle;

	struct PerCharacterData
	{
		CharacterRenderPosition Position;
		CharacterRenderStyle RenderStyle; //one bit for word separation?
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
			Topology = 7, //mask only

			Left = 8,
			Right = 16,
			Center = 32,
			Alignment = Left | Right | Center, //mask only
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
	//at the vacant site if possible. This gives best performance on update and
	//position calculation, and also minimum memory footprint, but creating new
	//Labels might be a little bit slow (need to find a free index).
	struct Label
	{
		//Tells the segment how to update the position.
		LabelType::LabelTypeValues Type;
		//Data defined by Handler to store additional data.
		std::uint8_t Data;
		//Index of Label handers (registered in the system).
		std::uint16_t Handler;
		//Position of the Label (updated by the segment).
		std::uint16_t Position;
	};

	class TextSegmentList;

	class ActiveTextSegmentData
	{
	public:
		bool IsModifiedSinceOpen;
		bool IsModifiedSinceSave;
		bool IsModifiedSinceSnapshot;
		bool IsModifiedSinceRendered;

		DynamicBuffer ContentBuffer;
		DynamicBuffer CharacterDataBuffer;
		ShortVector<StaticBuffer> SnapshotBuffers;
		ModificationTracer Modifications;
	};

	class TextSegment
	{
	public:
		TextSegmentList* Parent;
		std::uint8_t Index;
		bool IsContinuous;

	public:
		StaticBuffer ContentBuffer;
		StaticBuffer CharacterDataBuffer;
		ActiveTextSegmentData* ActiveData;

	public:
		ShortVector<Label> Labels;

		//render cache?
		//render height?
	};
}
