#include "StringConversion.h"

#include <string>
#include <wtypes.h>

static std::string Wstring2String(const std::wstring& in)
{
    LPCWSTR pwsz_src = in.c_str();
    int len = WideCharToMultiByte(CP_ACP, 0, pwsz_src, -1, NULL, 0, NULL, NULL);
    if (len <= 0)
        return "";
    char* psz_dst = new char[len];
    if (NULL == psz_dst)
        return "";
    WideCharToMultiByte(CP_ACP, 0, pwsz_src, -1, psz_dst, len, NULL, NULL);
    psz_dst[len - 1] = 0;
    std::string str(psz_dst);
    delete[] psz_dst;
    return str;
}

static std::wstring String2Wstring(const std::string& in)
{
    LPCSTR psz_src = in.c_str();
    int len = in.size();

    int size = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)psz_src, len, 0, 0);
    if (size <= 0)
        return L"";
    WCHAR* pwsz_dst = new WCHAR[size + 1];
    if (NULL == pwsz_dst)
        return L"";
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)psz_src, len, pwsz_dst, size);
    pwsz_dst[size] = 0;
    if (pwsz_dst[0] == 0xFEFF)
    {
        for (int i = 0; i < size; i++)
            pwsz_dst[i] = pwsz_dst[i + 1];
    }
    std::wstring wstr(pwsz_dst);
    delete pwsz_dst;
    return wstr;
}

const char* StringConversion::ToASCII(const wchar_t* str, char* buf, size_t bufsize)
{
	//wcstombs(buf, str, bufsize);
    std::string str_a = Wstring2String(str);
	strncpy_s(buf, bufsize, str_a.c_str(), bufsize);
	buf[bufsize - 1] = '\0';
	return buf;
}

const wchar_t* StringConversion::ToUTF16(const char* str, wchar_t* buf, size_t bufsize)
{
	//mbstowcs_s(buf, str, bufsize);
    std::wstring str_w = String2Wstring(str);
	wcsncpy_s(buf, bufsize, str_w.c_str(), bufsize);
	buf[bufsize - 1] = L'\0';
	return buf;
}
