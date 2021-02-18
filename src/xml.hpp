#pragma once
#include "common.hpp"
#include "array.hpp"

enum class XmlTokenType {
    // <?xml ... ?>
    StartDeclaration = 1,
    EndDeclaration,
    Attribute,
    StartElement,
    EndElement,
    Text,
};

struct XmlAttribute {
    String key;
    String value;
};

struct XmlToken {
    XmlTokenType type = (XmlTokenType)0;
    union {
        String declarationName;
        XmlAttribute attribute;
        String startElementName;
        String endElementName;
        String text;
    };
    inline XmlToken() {}
    void print();
};

enum class XmlNodeType {
    Document,
    Element,
    Text,
};

struct XmlNode {
    XmlNodeType type;
};

struct XmlElement : public XmlNode {
    String name;
    Array<XmlNode*> children;
    Array<XmlAttribute> attributes;

    String attr(const String& key) const;
    int attrInt(const String& key) const;
    bool attrBoolean(const String& key) const;

    Array<XmlElement*> getElementsByTagName(const String& name);
    void getElementsByTagName(const String& name, Array<XmlElement*>& result);

    Array<XmlElement*> getElementsByTagNames(const String* names, size_t count);
    void getElementsByTagNames(const String* names, size_t count, Array<XmlElement*>& result);

    void findAll(const String& key, Array<XmlElement*>& result) const;
    Array<XmlElement*> findElements(const String& name) const;
    XmlElement* element(const String& key) const;
    String text() const;
};

struct XmlDocument : public XmlNode {
    int childCount = 0;
    XmlElement* children[8];
};

struct XmlText : public XmlNode {
    String text;
};

struct XmlParser {
    char* start = 0;
    char* now = 0;
    char* end = 0;

    String lastElementTagName;
    int elementDepth = 0;
    bool insideDeclaration = false;
    bool insideElement = false;

    void init(const String& source);
    void destroy();

    bool next(XmlToken* token);
private:
    void parseText(XmlToken* token);
    void parseAttribute(XmlToken* token);
    String parseAttributeKey();
    String parseAttributeValue();
    void parseDeclarationTag(XmlToken* token);
    void parseBangDeclarationTag(XmlToken* token);
    bool parseElementTag(XmlToken* token);
    void skipWhiteSpace();
    void skipWhiteSpaceAndNewLines();
};

XmlElement* parseXml(const String& source);