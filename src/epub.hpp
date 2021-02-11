#pragma once
#include "common.hpp"
#include <vector>
#include "miniz.h"

struct EPubItem {
	String id;
	String href;
	String mediaType;
};

struct EPub {
	String fileName;
	String contentRootFolder;
	std::vector<EPubItem*> items;
	std::vector<EPubItem*> linearItemOrder;
	std::vector<String> images;
	mz_zip_archive zip;

	EPubItem* getItemById(const String& id);
	void parse(const String& fileName);
	String readFile(const String& fileName);
	void destroy();
};
