#include "FileTypeDetector.h"

Mimi::FileTypeDetector::FileTypeDetector(std::unique_ptr<IFileReader> reader, FileTypeDetectionOptions options)
	: Reader(std::move(reader)), CurrentLineData(0)
{
	Options = options;
	Detect();
}
