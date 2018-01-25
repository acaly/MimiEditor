#pragma once
#include <cstddef>
#include <vector>
#include "CodePage.h"

namespace Mimi
{
	struct FileTypeDetectionOptions
	{
		std::size_t MaxRead;
		std::size_t MinRead;
		std::vector<CodePage> AdditionalCodePages;
	};

	class FileTypeDetector final
	{
	public:
		FileTypeDetector();
		FileTypeDetector(const FileTypeDetector&) = delete;
		FileTypeDetector(FileTypeDetector&&) = delete;
		FileTypeDetector& operator= (const FileTypeDetector&) = delete;
		~FileTypeDetector();

	public:

	};
}
