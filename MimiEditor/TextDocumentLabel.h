#pragma once
#include <cstdint>

namespace Mimi
{

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
		union
		{
			//Line, Point, Range (head)
			struct
			{
				//Tells the segment how to update the position.
				std::uint8_t Type;
				//Data defined by Handler to store additional data.
				std::uint8_t Data;
				//Index of Label handers (registered in the system).
				std::uint16_t Handler;
				//Position of the Label (updated by the segment).
				std::uint16_t Position;
			};
			//Range (tail)
			struct
			{
				std::uint16_t Previous;
				std::uint16_t Next;
				std::uint16_t Position;
			};
			//Long (additional)
			struct
			{
				std::uint16_t Unused;
				std::uint32_t Additional;
			};
		};
	};
}
