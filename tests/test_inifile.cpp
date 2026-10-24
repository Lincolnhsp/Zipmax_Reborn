#include "IniFile.h"
#include <stdio.h>
#include <assert.h>
static std::string N(const std::wstring&w){std::string s;for(wchar_t c:w)s+=(c<128)?(char)c:'?';return s;}
int main(int c,char**v){
  CIniFile ini; assert(ini.Load(L"/home/claude/out/ZipMax/zipmax.ini"));
  std::wstring val;
  assert(ini.Get(L"WORK-RECURSIVE",&val) && val==L"1");
  assert(ini.Get(L"packer-cmd-1",&val)); printf("cmd1=%s\n",N(val).c_str());
  assert(!ini.Get(L"sevenzip-dll",&val));            // commented example
  assert(!ini.Get(L"packer-exe-2",&val));
  size_t before=ini.Lines().size();
  std::vector<std::wstring> b; b.push_back(L"work-recursive = 0"); b.push_back(L"language = Portugues-Brasil"); b.push_back(L"// sevenzip-dll = C:\\x\\7z.dll"); b.push_back(L"packer-exe-1 = 7za.exe"); b.push_back(L"packer-cmd-1 = a -r %1 %2"); b.push_back(L"packer-exe-2 = ZIP\u30c6\u30b9\u30c8.exe");
  ini.ReplaceManagedBlock(b);
  assert(ini.Get(L"work-recursive",&val) && val==L"0");
  assert(ini.Get(L"packer-exe-2",&val) && val==L"ZIP\u30c6\u30b9\u30c8.exe");
  assert(!ini.Get(L"logfile",&val));                   // old active managed line removed (not in block)
  assert(ini.Save(L"/tmp/zm_test.ini"));
  CIniFile i2; assert(i2.Load(L"/tmp/zm_test.ini"));
  assert(i2.Get(L"packer-exe-2",&val) && val==L"ZIP\u30c6\u30b9\u30c8.exe");
  // second replace must not duplicate anything
  i2.ReplaceManagedBlock(b); assert(i2.Save(L"/tmp/zm_test2.ini"));
  CIniFile i3; assert(i3.Load(L"/tmp/zm_test2.ini"));
  size_t keys=0,begins=0; for(auto&l:i3.Lines()){ if(l.find(L"work-recursive = ")==0) keys++; if(l==CIniFile::BlockBegin()) begins++; }
  printf("lines before=%zu after1=%zu after2=%zu; work-recursive lines=%zu; blocks=%zu\n",before,i2.Lines().size(),i3.Lines().size(),keys,begins);
  assert(keys==1&&begins==1&&i2.Lines().size()==i3.Lines().size());
  // comments preserved
  bool ex=false,hdr=false; for(auto&l:i3.Lines()){ if(l.find(L"examples for even more external packers")!=std::wstring::npos) ex=true; if(l==L"[ZIPMAX]") hdr=true; }
  assert(ex&&hdr);
  // no header case
  CIniFile e; std::vector<std::wstring> only; only.push_back(L"logfile = 1"); e.ReplaceManagedBlock(only); assert(e.Lines()[0]==L"[ZIPMAX]"); assert(e.Get(L"logfile",&val)&&val==L"1");
  assert(CIniFile::IsManagedKey(L"packer-cmd-12")&&CIniFile::IsManagedKey(L"packer-exe")&&!CIniFile::IsManagedKey(L"packer-exe-x")&&!CIniFile::IsManagedKey(L"foo"));
  // SetValue: existing active key is replaced in place; new key goes into the managed block; no header -> created
  { CIniFile s1; assert(s1.Load(L"/tmp/zm_test2.ini"));
    size_t n0=s1.Lines().size();
    s1.SetValue(L"language", L"English");                      // exists inside the managed block
    assert(s1.Get(L"language",&val) && val==L"English" && s1.Lines().size()==n0);
    s1.SetValue(L"nobeep", L"1");                              // new key -> inside the block, before the end marker
    assert(s1.Get(L"nobeep",&val) && val==L"1" && s1.Lines().size()==n0+1);
    size_t pn=0,pe=0; for(size_t i=0;i<s1.Lines().size();i++){ if(s1.Lines()[i].find(L"nobeep = ")==0) pn=i; if(s1.Lines()[i]==CIniFile::BlockEnd()) pe=i; }
    assert(pn+1==pe);
    CIniFile s2; assert(s2.Load(L"/home/claude/out/ZipMax/zipmax.ini")); size_t m0=s2.Lines().size();   // original ini, no block
    s2.SetValue(L"language", L"Portugues-Brasil");              // the shipped ini already has an active "language": replaced in place
    assert(s2.Get(L"language",&val)&&val==L"Portugues-Brasil"&&s2.Lines().size()==m0);
    s2.SetValue(L"sevenzip-dll", L"C:\\x\\7z.dll");                // only a commented example exists: a new line goes under the header
    assert(s2.Get(L"sevenzip-dll",&val)&&val==L"C:\\x\\7z.dll"&&s2.Lines().size()==m0+1);
    s2.SetValue(L"language", L"English"); assert(s2.Get(L"language",&val)&&val==L"English"&&s2.Lines().size()==m0+1);
    CIniFile s3; s3.SetValue(L"language", L"English"); assert(s3.Lines()[0]==L"[ZIPMAX]" && s3.Get(L"language",&val));
    assert(s2.Save(L"/tmp/zm_test3.ini")); CIniFile s4; assert(s4.Load(L"/tmp/zm_test3.ini")&&s4.Get(L"language",&val)&&val==L"English");
  }
  printf("INI TESTS OK\n"); return 0; }
