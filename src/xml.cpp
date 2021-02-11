#include "xml.hpp"
#include "string.hpp"

inline static bool isWhiteSpace(char c) {
    return c == ' ' || c == '\t';
}

inline static bool isNewLine(char c) {
    return c == '\r' || c == '\n';
}

void XmlParser::init(const String& source) {
    start = source.chars;
    now = start;
    end = source.chars + source.count;
}

void XmlParser::destroy() {
}

bool XmlParser::next(XmlToken* token) {
    bool skippedTextWhiteSpace = false;
    while (true) {
        if (insideDeclaration) {
            parseDeclarationTag(token);
            return true;
        } else if (insideElement) {
            if (parseElementTag(token)) {
                continue;
            }
            return true;
        }

        if (elementDepth <= 0) {
            skipWhiteSpaceAndNewLines();
        }

        if (now >= end) {
            return false;
        }

        char c = *now;
        if (c == '<') {
            ++now;
            verify(now < end);
            c = *now;
            if (c == '?') {
                ++now;
                verify(now < end);

                insideDeclaration = true;
                token->type = XmlTokenType::StartDeclaration;
                token->declarationName = parseAttributeKey();
                return true;
            } else if (c == '!') {
                // Skip !DOCTYPE
                ++now;
                do {
                    verify(now < end);
                    c = *now++;
                    if (c == '>') {
                        break;
                    }
                } while (true);
                continue;
            } else if (c == '/') {
                ++now;
                verify(now < end);
                String name = parseAttributeKey();
                verify(now < end);
                verify(*now == '>');
                ++now;
                token->type = XmlTokenType::EndElement;
                token->endElementName = name;
                --elementDepth;
                verify(elementDepth >= 0);
                return true;
            } else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-' || c == ':') {
                insideElement = true;
                token->type = XmlTokenType::StartElement;
                token->startElementName = parseAttributeKey();
                lastElementTagName = token->startElementName;
                ++elementDepth;
                return true;
            } else {
                verify(false);
            }
        } else {
            verify(!skippedTextWhiteSpace);
            if (elementDepth <= 0) {
                skipWhiteSpaceAndNewLines();
                skippedTextWhiteSpace = true;
                continue; // Scan next token.
            } else {
                parseText(token);
                return true;
            }
        }

        verify(false);
        return false;
    }
}

void XmlParser::parseText(XmlToken* token) {
    char* start = now;
    while (now < end) {
        char c = *now;
        // @TODO: c == special character, not just '<'.
        if (c == '<') {
            break;
        } else {
            ++now;
        }
    }
    int count = (int)(now - start);
    verify(count > 0);
    token->type = XmlTokenType::Text;
    token->text = { start, count };
}

void XmlParser::parseAttribute(XmlToken* token) {
    auto key = parseAttributeKey();
    skipWhiteSpace();
    verify(now < end);
    verify(*now == '=');
    ++now;
    skipWhiteSpace();
    auto value = parseAttributeValue();

    token->type = XmlTokenType::Attribute;
    token->attribute.key = key;
    token->attribute.value = value;
}

String XmlParser::parseAttributeKey() {
    char* start = now;
    while (now < end) {
        char c = *now;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == ':') {
            ++now;
        } else {
            break;
        }
    }
    int count = (int)(now - start);
    verify(count > 0);
    return { start, count };
}

String XmlParser::parseAttributeValue() {
    auto start = now;
    verify(now < end);
    char quoteChar = 0;
    char c = *now;
    if (c == '"' || c == '\'') {
        quoteChar = c;
        ++start;
        ++now;
    }

    while (now < end) {
        char c = *now;
        if (quoteChar ? c == quoteChar : isWhiteSpace(c)) {
            break;
        } else if (isNewLine(c)) {
            verify(false);
        } else {
            ++now;
        }
    }
    int count = (int)(now - start);
    verify(quoteChar || count > 0);
    if (quoteChar) ++now;

    return { start, count };
}

void XmlParser::parseDeclarationTag(XmlToken* token) {
    skipWhiteSpace();
    verify(now < end);
    char c = *now;
    if (c == '?') {
        ++now;
        verify(now < end);
        c = *now;
        if (c == '>') {
            ++now;
            token->type = XmlTokenType::EndDeclaration;
            insideDeclaration = false;
        } else {
            verify(false); // Expected '>' after '?' (end of declaration)
        }
    } else {
        parseAttribute(token);
    }
}

bool XmlParser::parseElementTag(XmlToken* token) {
    skipWhiteSpace();
    verify(now < end);
    char c = *now;
    bool selfClosing = c == '/';
    if (selfClosing) {
        ++now;
        verify(now < end);
        c = *now;
    }

    if (c == '>') {
        ++now;
        insideElement = false;
        if (selfClosing) {
            token->type = XmlTokenType::EndElement;
            token->endElementName = lastElementTagName;
            --elementDepth;
            verify(elementDepth >= 0);
        } else {
            return true;
        }
    } else {
        verify(!selfClosing);
        parseAttribute(token);
    }
    return false;
}

void XmlParser::skipWhiteSpace() {
    while (now < end) {
        if (isWhiteSpace(*now)) {
            ++now;
        } else {
            break;
        }
    }
}

void XmlParser::skipWhiteSpaceAndNewLines() {
    while (now < end) {
        char c = *now;
        if (isWhiteSpace(c) || isNewLine(c)) {
            ++now;
        } else {
            break;
        }
    }
}

XmlText* parseText(XmlParser& parser, const XmlToken& thisTextToken) {
    auto result = new XmlText();
    result->type = XmlNodeType::Text;
    result->text = thisTextToken.text;
    return result;
}

XmlElement* parseElement(XmlParser& parser, const XmlToken& thisElementToken) {
    XmlToken token;
    auto element = new XmlElement();
    element->type = XmlNodeType::Element;
    element->name = thisElementToken.startElementName;

    // Attributes
    while (parser.next(&token)) {
        switch (token.type) {
            case XmlTokenType::Attribute: {
                element->attributes.push_back(token.attribute);
            } break;
            default: {
                goto finishAttributes;
            }
        }
    }

finishAttributes:

    do {
        switch (token.type) {
            case XmlTokenType::EndElement: {
                verify(thisElementToken.startElementName == token.endElementName);
                return element;
            }
            case XmlTokenType::StartElement: {
                element->children.push_back(parseElement(parser, token));
            } break;
            case XmlTokenType::Text: {
                element->children.push_back(parseText(parser, token));
            } break;
            default: {
                verify(false);
            } break;
        }
    } while (parser.next(&token));

    verify(false);
    return nullptr;
}

XmlElement* parseXml(const String& source) {
    XmlToken token;
    XmlParser parser;
    parser.init(source);

    XmlDocument doc;
    doc.type = XmlNodeType::Document;
    
    // Skip declaration first.
    {
        bool insideDeclaration = false;
        while (parser.next(&token)) {
            switch (token.type) {
                case XmlTokenType::StartDeclaration: {
                    insideDeclaration = true;
                } break;
                case XmlTokenType::EndDeclaration: {
                    verify(insideDeclaration);
                    goto declarationFinished;
                } break;
                case XmlTokenType::Attribute: {
                    verify(insideDeclaration);
                } break;
                default: {
                    verify(!insideDeclaration);
                    // Missing declaration.
                    goto continueWithLastToken;
                    break; 
                } break;
            }
        }
    }

    declarationFinished: {}

    while (parser.next(&token)) {
        continueWithLastToken: {}
        switch (token.type) {
            case XmlTokenType::StartElement: {
                verify(doc.childCount < _countof(doc.children));
                doc.children[doc.childCount++] = parseElement(parser, token);
            } break;
            default: {
                verify(false);
            } break;
        }
    }

    parser.destroy();

    // We can parse multiple elements at root level, but for now we don't need it,
    // so we return first element.
    verify(doc.childCount == 1);
    return doc.children[0];
}

String XmlElement::attr(const String& key) const {
    for (int i = 0; i < attributes.size(); ++i) {
        const auto& attr = attributes[i];
        if (attr.key == key) {
            return attr.value;
        }
    }
    return {};
}

int XmlElement::attrInt(const String& key) const {
    return parseInt(attr(key));
}

std::vector<XmlElement*> XmlElement::getElementsByTagName(const String& name) {
    std::vector<XmlElement*> results;
    getElementsByTagName(name, results);
    return results;
}

void XmlElement::getElementsByTagName(const String& name, std::vector<XmlElement*>& result) {
    for (auto child : children) {
        if (child->type != XmlNodeType::Element) {
            continue;
        }
        auto* element = (XmlElement*)child;
        if (stringEqualsCaseInsensitive(element->name, name)) {
            result.push_back(element);
        }
        element->getElementsByTagName(name, result);
    }
}

bool XmlElement::attrBoolean(const String& key) const {
    return parseBoolean(attr(key));
}

void XmlElement::findAll(const String& key, std::vector<XmlElement*>& result) const {
    for (auto& child : children) {
        auto element = (XmlElement*)child;
        if (element->type == XmlNodeType::Element && element->name == key) {
            result.push_back(element);
        }
    }
}

std::vector<XmlElement*> XmlElement::findElements(const String& name) const {
    std::vector<XmlElement*> elements;
    for (auto child : children) {
        if (child->type == XmlNodeType::Element && ((XmlElement*)child)->name == name) {
            elements.push_back((XmlElement*)child);
        }
    }
    return elements;
}

XmlElement* XmlElement::element(const String& key) const {
    for (auto& child : children) {
        auto element = (XmlElement*)child;
        if (element->type == XmlNodeType::Element && element->name == key) {
            return element;
        }
    }
    return nullptr;
}

String XmlElement::text() const {
    verify(children.size() == 1 && children.data()[0]->type == XmlNodeType::Text);
    return ((XmlText*)children.data()[0])->text;
}
