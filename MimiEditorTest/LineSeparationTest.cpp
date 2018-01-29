#include "../MimiEditor/FileTypeDetector.h"
#include <string>
#include <iostream>

using namespace Mimi;
using namespace std;

int TestLineSeparation()
{
	string path;
	getline(cin, path);
	String pathStr = String::FromUtf8Ptr(path.c_str());
	unique_ptr<IFile> file = unique_ptr<IFile>(IFile::CreateFromPath(std::move(pathStr)));
	unique_ptr<IFileReader> r = unique_ptr<IFileReader>(file->Read());
	
	FileTypeDetectionOptions option;
	option.InvalidCharAction = InvalidCharAction::ReplaceQuestionMark;
	option.MaxRead = 1000;
	option.MinRead = 100;
	
	FileTypeDetector detector(std::move(r), option);

	while (detector.ReadNextLine())
	{
		String str(
			reinterpret_cast<const mchar8_t*>(detector.CurrentLineData.GetRawData()),
			detector.CurrentLineData.GetLength(),
			detector.GetCodePage());
	}
	return 0;
}
