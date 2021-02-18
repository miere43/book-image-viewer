#pragma once
#include "common.hpp"
#include "miniz.h"
#include "array.hpp"

struct EPubItem {
    String id;
    String href;
    String mediaType;
};

struct EPub {
    String fileName;
    String contentRootFolder;
    Array<EPubItem*> items;
    Array<EPubItem*> linearItemOrder;
    Array<String> images;
    mz_zip_archive zip;

    EPubItem* getItemById(const String& id);
    void parse(const String& fileName);
    String readFile(const String& fileName);
    void destroy();
};
