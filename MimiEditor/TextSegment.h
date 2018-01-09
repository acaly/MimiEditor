#pragma once

#include "CommonInternal.h"
#include "ShortVector.h"
#include "ModificationTracer.h"
#include <cstdint>
#include <vector>

namespace Mimi
{
	enum TextSegmentNewLine : std::uint8_t
	{
		None,
		CRLF,
		LF,
	};

	typedef std::uint8_t CharacterPosition;
	typedef std::uint8_t CharacterRenderStyle;

	struct PerCharacterData
	{
		CharacterPosition Position;
		CharacterRenderStyle RenderStyle; //one bit for word separation?
	};

	enum LabelType : std::uint8_t
	{
		Line,
		Point,
		Range,
	};

	struct LineLabel
	{
	};

	struct PointLabel
	{

	};

	struct RangeLabel
	{

	};

	struct Label
	{
		LabelType Type;
		union
		{
			LineLabel Line;
			PointLabel Point;
			RangeLabel Range;
		};
	};

	class TextSegmentList;

	class TextSegment
	{
	public:
		TextSegmentList* Parent;
		std::uint8_t Index;

	public:
		TextSegmentNewLine NewLine;

		bool IsContinuous;
		bool IsModifiedSinceOpen;
		bool IsModifiedSinceSave;
		bool IsModifiedSinceSnapshot;
		bool IsModifiedSinceRendered;

		bool IsContentGlobalStorage;
		bool IsSnapshotGlobalStorage;

	public:
		std::uint16_t ContentBufferSize;
		std::uint16_t SnapshotBufferSize;
		std::uint16_t CharacterDataBufferSize;
		std::uint8_t* ContentBuffer;
		std::uint8_t* SnapshotBuffer;
		PerCharacterData* CharacterDataBuffer;

	public:
		ShortVector<Label> Labels;

	public:
		ModificationTracer Modifications; //Traces bytes or chars?

		//change event?
		//render cache?
	};
}
