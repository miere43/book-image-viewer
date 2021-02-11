#include "common.hpp"
#include "epub.hpp"
#include <stdio.h>
#include "miniz.h"
#include "xml.hpp"
#include "string.hpp"

static String removeLastPathComponent(const String& path) {
	int slashIndex = lastIndexOf(path, '/');
	return slashIndex == -1 ? path : substring(path, 0, slashIndex);
}

static void splitPathIntoComponents(const String& path, std::vector<String>& components) {
	int lastComponentEndIndex = 0;
	for (int i = 0; i <= path.count; ++i) {
		char c = path[i];
		if (c == '/' || i == path.count) {
			int componentCharacterCount = i - lastComponentEndIndex;
			if (componentCharacterCount > 0) {
				components.push_back(substring(path, lastComponentEndIndex, componentCharacterCount));
			}
			lastComponentEndIndex = i + 1;
		}
	}
}

static String combinePath(const std::vector<String>& components) {
	if (components.size() == 0) {
		return copyString("");
	}
	int count = components.size() - 1;
	for (const auto& component : components) {
		count += component.count;
	}
	auto memory = new char[count];
	auto now = memory;
	for (const auto& component : components) {
		memcpy(now, component.chars, component.count);
		now += component.count;
		*now++ = '/';
	}
	return { memory, count };
}

static String resolveRelativePath(const String& currentDirectory, const String& targetFilePath) {
	std::vector<String> components;
	splitPathIntoComponents(currentDirectory, components);
	splitPathIntoComponents(targetFilePath, components);

	for (int i = 0; i < components.size(); ++i) {
		const auto& component = components[i];
		if (component == "..") {
			verify(i >= 1);
			components.erase(components.begin() + i);
			components.erase(components.begin() + (i - 1));
			i -= 2;
		}
	}

	return combinePath(components);
}

static void parseContent(EPub& epub, const String& content, const String& currentDirectory) {
	auto root = parseXml(content);

	auto manifest = root->element("manifest");
	verify(manifest);

	auto items = manifest->findElements("item");
	for (auto& item : items) {
		auto parsedItem = new EPubItem();
		parsedItem->id = item->attr("id");
		parsedItem->href = resolveRelativePath(currentDirectory, item->attr("href"));
		parsedItem->mediaType = item->attr("media-type");
		epub.items.push_back(parsedItem);
	}

	auto spine = root->element("spine");
	verify(spine);

	auto itemrefs = spine->findElements("itemref");
	for (auto& itemref : itemrefs) {
		auto idref = itemref->attr("idref");
		auto item = epub.getItemById(idref);
		verify(item);
		epub.linearItemOrder.push_back(item);
	}
}

static void collectImage(EPub& epub, const String& src) {
	for (const auto& image : epub.images) {
		if (stringEqualsCaseInsensitive(image, src)) {
			return;
		}
	}
	epub.images.push_back(src);
}

static void collectPageImages(EPub& epub, const String& content, const String& currentDirectory) {
	auto root = parseXml(content);
	auto images = root->getElementsByTagName("img");

	for (const auto& image : images) {
		auto src = image->attr("src");
		if (src.count == 0) continue;
		collectImage(epub, resolveRelativePath(currentDirectory, src));
	}
	int z = 1;
}

EPubItem* EPub::getItemById(const String& id) {
	for (auto& item : items) {
		if (item->id == id) {
			return item;
		}
	}
	return nullptr;
}

static String discoverContentRoot(EPub& epub) {
	/*
		<?xml version="1.0" encoding="UTF-8"?>
		<container version="1.0" xmlns="urn:oasis:names:tc:opendocument:xmlns:container">
			<rootfiles>
				<rootfile full-path="OEBPS/content.opf" media-type="application/oebps-package+xml"/>
			</rootfiles>
		</container>
	*/
	auto file = epub.readFile("META-INF/container.xml");
	auto content = parseXml(file);
	auto rootFiles = content->getElementsByTagName("rootfile");
	for (const auto& rootFile : rootFiles) {
		if (rootFile->attr("media-type") == "application/oebps-package+xml") {
			auto fullPath = rootFile->attr("full-path");
			int slashIndex = indexOf(fullPath, '/');
			epub.contentRootFolder = copyString(slashIndex == -1 ? fullPath : substring(fullPath, 0, slashIndex));
			return fullPath;
		}
	}
	verify(!"Could not find content root file.");
	return {};
}

void EPub::parse(const String& fileName) {
	mz_zip_zero_struct(&zip);

	this->fileName = copyString(fileName);
	
	wchar_t* wideFileName = toUtf16(fileName, nullptr);
	FILE* file = nullptr;
	verify(0 == _wfopen_s(&file, wideFileName, L"rb"));

	verify(mz_zip_reader_init_cfile(&zip, file, 0, 0));

	auto contentRootFile = discoverContentRoot(*this);
	auto content = readFile(contentRootFile);
	parseContent(*this, content, removeLastPathComponent(contentRootFile));

	for (const auto& item : linearItemOrder) {
		auto page = readFile(item->href);
		collectPageImages(*this, page, removeLastPathComponent(item->href));
	}
}

String EPub::readFile(const String& fileName) {
	auto normalizedFileName = toCString(fileName);
	size_t size;
	void* data = mz_zip_reader_extract_file_to_heap(&zip, normalizedFileName, &size, 0);
	verify(data);
	delete normalizedFileName;
	return { (char*)data, (int)size };
}

void EPub::destroy() {
	mz_zip_end(&zip);
	items.clear();
	linearItemOrder.clear();
	images.clear();
}
