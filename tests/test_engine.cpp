#include "Zip7Engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <locale.h>

#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } else { printf("ok   : %s\n", #c); } } while(0)

static std::wstring W(const std::string &s){ std::wstring w; for(size_t i=0;i<s.size();i++) w+=(wchar_t)(unsigned char)s[i]; return w; }
// UTF-8 aware (for names with non-ASCII)
static std::wstring U(const std::string &s){ std::wstring w; for(size_t i=0;i<s.size();){ unsigned char c=s[i]; unsigned cp; int n; if(c<0x80){cp=c;n=1;} else if((c>>5)==6){cp=c&31;n=2;} else if((c>>4)==14){cp=c&15;n=3;} else {cp=c&7;n=4;} for(int k=1;k<n;k++) cp=(cp<<6)|(s[i+k]&63); w+=(wchar_t)cp; i+=n;} return w; }
static std::string N(const std::wstring &w){ std::string s; for(size_t i=0;i<w.size();i++){ unsigned c=w[i]; if(c<0x80) s+=(char)c; else if(c<0x800){ s+=(char)(0xC0|(c>>6)); s+=(char)(0x80|(c&63)); } else if(c<0x10000){ s+=(char)(0xE0|(c>>12)); s+=(char)(0x80|((c>>6)&63)); s+=(char)(0x80|(c&63)); } else { s+=(char)(0xF0|(c>>18)); s+=(char)(0x80|((c>>12)&63)); s+=(char)(0x80|((c>>6)&63)); s+=(char)(0x80|(c&63)); } } return s; }

int main(int argc, char **argv)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	setlocale(LC_ALL, "C.UTF-8");	// (Linux only: Windows paths are always UTF-16)
	int fails = 0;
	std::wstring err;
	std::string work = argv[2];

	CHECK(Zip7::Initialize(U(argv[1]).c_str(), &err));

	std::wstring src = U(work + "/src"), zipA = U(work + "/A.zip"), ex = U(work + "/ex");
	mkdir((work + "/ex").c_str(), 0755);

	std::vector<std::wstring> files;
	files.push_back(L"a.txt"); files.push_back(L"sub/b.bin"); files.push_back(L"sub/deep/c.txt"); files.push_back(L"empty.dat"); files.push_back(L"rand.bin");
	// non-ASCII (Japanese) file name
	files.push_back(U("\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E/\xE3\x83\x86\xE3\x82\xB9\xE3\x83\x88.txt"));

	Zip7::CompressOptions opt;
	CHECK(Zip7::CompressFiles(zipA.c_str(), src.c_str(), files, opt, &err));
	if (!err.empty()) printf("err=%s\n", N(err).c_str());

	std::vector<Zip7::ZipHeader> h;
	CHECK(Zip7::ReadHeaders(zipA.c_str(), &h, &err));
	CHECK(h.size() == 6);
	if (h.size() < 6) { printf("FAIL: only %zu headers; err=%s\n", h.size(), N(err).c_str()); return 1; }
	CHECK(h[5].name == U("\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E/\xE3\x83\x86\xE3\x82\xB9\xE3\x83\x88.txt"));
	CHECK((h[5].flags & 0x800) != 0);
	CHECK(Zip7::TestArchive(zipA.c_str(), &err));
	CHECK(Zip7::ExtractAll(zipA.c_str(), ex.c_str(), &err));
	{
		std::string jp = work + "/ex/\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E/\xE3\x83\x86\xE3\x82\xB9\xE3\x83\x88.txt";
		CHECK(access(jp.c_str(), F_OK) == 0);
	}

	Zip7::ArchiveInfo info;
	CHECK(Zip7::DescribeArchive(zipA.c_str(), &info, &err));
	printf("  info: %s | %s | %s | %s\n", N(info.type).c_str(), N(info.compression).c_str(), N(info.dictionary).c_str(), N(info.method).c_str());

	std::wstring zipB = U(work + "/B.zip"), exB = U(work + "/exB");
	mkdir((work + "/exB").c_str(), 0755);
	std::vector<Zip7::ZipHeader> hb;
	CHECK(Zip7::ReadHeaders(zipB.c_str(), &hb, &err));
	CHECK(Zip7::TestArchive(zipB.c_str(), &err));
	CHECK(Zip7::ExtractAll(zipB.c_str(), exB.c_str(), &err));

	CHECK(!Zip7::TestArchive(U(work + "/C.zip").c_str(), &err));
	printf("  corrupt err: %s\n", N(err).c_str());

	mkdir((work + "/exS").c_str(), 0755);
	CHECK(!Zip7::ExtractAll(U(work + "/S.zip").c_str(), U(work + "/exS").c_str(), &err));
	printf("  slip err: %s\n", N(err).c_str());

	CHECK(!Zip7::ReadHeaders(U(work + "/notzip.zip").c_str(), &h, &err));
	CHECK(!Zip7::TestArchive(U(work + "/notzip.zip").c_str(), &err));

	std::vector<std::wstring> bad; bad.push_back(L"nope.txt");
	CHECK(!Zip7::CompressFiles(U(work + "/D.zip").c_str(), src.c_str(), bad, opt, &err));
	printf("  missing err: %s\n", N(err).c_str());

	Zip7::Shutdown();
	printf("FAILS=%d\n", fails);
	return fails;
}
