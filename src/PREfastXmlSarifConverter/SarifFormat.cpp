#include "SarifFormat.h"
#include <iterator>
#include <array>

void
GetXmlToSarifMapping(const std::wstring &prefastTag, std::wstring &sarifTag)
{
	// Full list for additional info is not available, so far it is only RULECATEGORY
	static std::map<std::wstring, std::wstring> mapping =
	{
		{ L"RULECATEGORY", L"ruleCategory" }
	};

	_ASSERTE(mapping.find(prefastTag) != mapping.end());

	std::map<std::wstring, std::wstring>::iterator it = mapping.find(prefastTag);
	if (it != mapping.end())
		sarifTag = it->second;
}

bool 
hasEnding(const _bstr_t &strValue, const std::wstring &ending)
{  
    std::wstring fullString = strValue;
    if (fullString.length() <= ending.length())
        return false;

    size_t extPos = fullString.length() - ending.length();
    for (size_t i = 0, j = extPos; i < ending.length(); i++, j++)
    {
        if (towlower(fullString[j]) != towlower(ending[i]))
            return false; 
    }
    return true; 
}

std::wstring
AddEscapeCharacters(const std::wstring &data)
{
	static const std::array<const wchar_t, 10> src_chars = {
		'\a', L'\b', L'\f', L'\n', L'\r', L'\t', '\v',
		L'\\', L'"', L'/'
	};

	static const std::array<const wchar_t, 10> mapped_chars = {
		L'a', L'b', L'f', L'n', L'r', L't', L'v',
		L'\\', L'"', L'/'
	};

	std::wstring result;
	result.reserve(data.length() * 2);

	// Stripping off legacy PREFAST_NEWLINE markers from SARIF descriptions
	// Replacing control characters with literal characters by prepending '\\'
	for (std::wstring::const_iterator it = data.begin(); it != data.end(); ++it)
	{	
		if (*it == L'\n')
		{
			auto beg = std::distance(data.begin(), it);
			auto end = data.find(L"PREFAST_NEWLINE\n", beg);
			if (end != std::wstring::npos)
			{
				// If end != std::wstring::npos, then it is safe to add iterator by countof(L"PREFAST_NEWLINE\n") - 1. 
				it += _countof(L"PREFAST_NEWLINE\n") - 1;
			}
		}

		auto mapit = std::find(src_chars.begin(), src_chars.end(), *it);
		if (mapit != src_chars.end())
		{
			result += '\\';
			auto pos = std::distance(src_chars.begin(), mapit);
			result += mapped_chars[pos];
		}
		else
		{
			result += *it;
		}

	}
	result.shrink_to_fit();

	return result;
}


std::wstring
MakeItUri(const std::wstring &path)
{
    std::wstring uri = L"file:///";
    uri += path;
    std::replace(uri.begin(), uri.end(), L'\\', L'/');
    return uri;
}

std::wstring 
GetDefectUri(const XmlSfa &sfa)
{	
    std::wstring path = sfa.GetFilePath();
    path += sfa.GetFileName();
    return MakeItUri(path);
}

bool 
SarifRegion::IsValid()
{
    //This is handling a special case.PREfast indicates an internal error by emitting a "defect" whose line and column are 0, 0. 
    //That is, PREfast is saying that this defect is not associated with any particular region in the file.
    //The IsValid method returns false precisely when line, column = 0, 0.
    //SARIF indicates that a defect is not associated with a region by omitting the region property from the physicalLocationComponent object.
    //So here, we only set the region property when the defect is associated with a region.

    if (m_startLine > 0 && m_startColumn >= 0)
        return true;
    else if (m_startLine == 0 && m_startColumn == 0)
        return false;
    std::string error = "Invalid region specified.";
    error += "Start Line : " + std::to_string(m_startLine);
    error += "Start Column : " + std::to_string(m_startColumn);
    throw std::exception(error.c_str());
}

void 
SarifRegion::SetStartLine(int value)
{
    m_startLine = value;
    m_values[L"startLine"] = json::Value(value);
}

void 
SarifRegion::SetStartColumn(int value)
{
    m_startColumn = value;
    m_values[L"startColumn"] = json::Value(value);
}

void 
SarifRegion::SetStartLine(const std::wstring &value)
{
	wchar_t * pEnd;
	int intVal = wcstol(value.c_str(), &pEnd, BASE10);
    if (*pEnd)
        throw std::exception("Invalid Start Line specified.");
	SetStartLine(intVal);
}

void 
SarifRegion::SetStartColumn(const std::wstring &value)
{
	wchar_t * pEnd;
	int intVal = wcstol(value.c_str(), &pEnd, BASE10);
    if (*pEnd)
        throw std::exception("Invalid Start Column specified.");
	SetStartColumn(intVal);
}

void 
SarifRegion::SetEndLine(const std::wstring &value)
{
	wchar_t * pEnd;
	int intVal = wcstol(value.c_str(), &pEnd, BASE10);
    if (*pEnd)
        throw std::exception("Invalid End Line specified.");
	SetEndLine(intVal);
}

void 
SarifRegion::SetEndColumn(const std::wstring &value)
{
	wchar_t * pEnd;
	int intVal = wcstol(value.c_str(), &pEnd, BASE10);
    if (*pEnd)
        throw std::exception("Invalid End Column specified.");
	SetEndColumn(intVal);
}

void 
SarifRegion::SetOffset
(const std::wstring &value)
{
	wchar_t * pEnd;
	int intVal = wcstol(value.c_str(), &pEnd, BASE10);
    if (*pEnd)
        throw std::exception("Invalid Char Offset specified.");
	SetCharOffset(intVal);
}

void 
SarifRegion::SetLength(const std::wstring &value)
{
	wchar_t * pEnd;
	int intVal = wcstol(value.c_str(), &pEnd, BASE10);
    if (*pEnd)
        throw std::exception("Invalid Length specified.");
	SetLength(intVal);
}

void 
SarifFileChange::AddReplacement(const SarifReplacement &replacement)
{
    m_values.GetArrayElement(L"replacements").push_back(replacement.m_values);
}

void 
SarifLocation::AddLogicalLocationComponent(const std::wstring &name, const wchar_t *locationKind)
{
    SarifLogicalLocationComponent location;
    location.SetLocationKind(locationKind);
    location.SetName(name);
    AddLogicalLocationComponent(location);
}

void 
SarifLocation::AddLogicalLocationComponent(const SarifLogicalLocationComponent &component)
{
    m_values.GetArrayElement(L"logicalLocation").push_back(component.m_values);
}

void
SarifLocation::AddProperty(const std::wstring &key, const std::wstring &value)
{
	if (m_values.find(L"properties") == m_values.end())
		m_values[L"properties"] = json::Object();
	m_values[L"properties"][key] = value;
}

void
SarifCodeFlow::AddAnnotatedCodeLocation(const SarifAnnotatedCodeLocation &location)
{
    m_values.push_back(location.m_values);
}

void
SarifAnnotatedCodeLocation::AddProperty(const std::wstring &key, const std::wstring &value)
{
	if (m_values.find(L"properties") == m_values.end())
		m_values[L"properties"] = json::Object();
	m_values[L"properties"][key] = value;
}

void 
SarifFix::AddFileChange(const SarifFileChange &change)
{
    m_values.GetArrayElement(L"fileChanges").push_back(change.m_values);
}

void 
SarifResult::AddProperty(const std::wstring &key, const std::wstring &value)
{
    if (m_values.find(L"properties") == m_values.end())
        m_values[L"properties"] = json::Object();
    m_values[L"properties"][key] = value;
}

void
SarifResult::AddLocation(const SarifLocation &location)
{
    m_values.GetArrayElement(L"locations").push_back(location.m_values);
}

void
SarifResult::AddCodeFlow(const SarifCodeFlow &codeFlow)
{
    m_values.GetArrayElement(L"codeFlows").push_back(codeFlow.m_values);
}

void
SarifResult::AddFix(const SarifFix &fix)
{
    m_values.GetArrayElement(L"fixes").push_back(fix.m_values);
}

void 
SarifFile::AddHash(const std::wstring &algoritm, const std::wstring &value)
{
    SarifHash hash;
    hash.SetAlgorithm(algoritm);
    hash.SetValue(value);
    AddHash(hash);
}

void
SarifFile::AddHash(const SarifHash &hash)
{
    m_values.GetArrayElement(L"hashes").push_back(hash.m_values);
}

void
SarifRun::AddFile(const std::wstring &key, const SarifFile &file)
{
	if (m_values.find(L"files") == m_values.end())
		m_values[L"file"] = json::Object();

	m_values[L"files"][key] = file.m_values;
}

void 
SarifRun::AddResult(const SarifResult &result)
{
    m_values.GetArrayElement(L"results").push_back(result.m_values);
}

void
SarifLog::AddRun(const SarifRun &run)
{
    m_values.GetArrayElement(L"runs").push_back(run.m_values);
}

