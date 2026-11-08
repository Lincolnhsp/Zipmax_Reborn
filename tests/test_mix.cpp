// Portable replica of ZipMax's WriteLFH/WriteCFH/WriteEOZ/BuildNewZip (same logic, FILE* instead of CStdioFile)
#include "Zip7Engine.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <map>
#pragma pack(push,1)
struct LFH{uint16_t vn,fl,m,t,d;uint32_t crc,cs,us;uint16_t fnl,efl;};
struct CFH{uint16_t vmb,vn,fl,m,t,d;uint32_t crc,cs,us;uint16_t fnl,efl,cl,dn,ia;uint32_t ea,off;};
struct EOCD{uint32_t sig;uint16_t d,dc,n1,n2;uint32_t sz,off;uint16_t cl;};
#pragma pack(pop)
struct Entry{const Zip7::ZipHeader*h; std::string arch;};
static bool WriteLFH(FILE*o,const Entry&e,uint32_t*off){
  auto*h=e.h; LFH l; l.vn=h->versionNeeded;l.fl=h->flags&0xfff7;l.m=h->method;l.t=h->modTime;l.d=h->modDate;l.crc=h->crc32;l.cs=h->comprSize;l.us=h->uncomprSize;l.fnl=(uint16_t)h->rawName.size();l.efl=0;
  *off=(uint32_t)ftell(o); uint32_t id=0x04034b50; fwrite(&id,4,1,o); fwrite(&l,sizeof l,1,o);
  FILE*f=fopen(e.arch.c_str(),"rb"); if(!f) return false; fseek(f,h->localHeaderOffset,SEEK_SET);
  uint32_t oid; LFH ol; if(fread(&oid,4,1,f)!=1||oid!=0x04034b50||fread(&ol,sizeof ol,1,f)!=1){fclose(f);return false;}
  fseek(f,ol.fnl+ol.efl,SEEK_CUR); fwrite(h->rawName.data(),h->rawName.size(),1,o);
  std::vector<char> buf(h->comprSize?h->comprSize:1); if(h->comprSize && fread(&buf[0],1,h->comprSize,f)!=h->comprSize){fclose(f);return false;}
  fwrite(&buf[0],1,h->comprSize,o); fclose(f); return true;}
int main(int c,char**v){
  std::string err; std::vector<Zip7::ZipHeader> ho,hi; 
  // v[1]=original zip, v[2]=7-zip recompressed zip, v[3]=output
  if(!Zip7::ReadHeaders(v[1],&ho,&err)||!Zip7::ReadHeaders(v[2],&hi,&err)){printf("hdr fail %s\n",err.c_str());return 1;}
  std::map<std::wstring,size_t> mi; for(size_t k=0;k<hi.size();k++) mi.insert({hi[k].key,k});
  std::vector<Entry> es;
  for(auto&h:ho){ if(h.IsDirectory()) continue; auto it=mi.find(h.key); if(it==mi.end()){printf("name not found\n");return 1;}
    Entry e{&h,v[1]}; if(hi[it->second].comprSize<h.comprSize){e.h=&hi[it->second];e.arch=v[2];} es.push_back(e);}
  FILE*o=fopen(v[3],"wb"); std::vector<uint32_t> offs;
  for(auto&e:es){uint32_t off; if(!WriteLFH(o,e,&off)){printf("LFH fail\n");return 1;} offs.push_back(off);}
  uint32_t cd=(uint32_t)ftell(o);
  for(size_t i=0;i<es.size();i++){auto*h=es[i].h; CFH f; f.vmb=h->versionMadeBy;f.vn=h->versionNeeded;f.fl=h->flags&0xfff7;f.m=h->method;f.t=h->modTime;f.d=h->modDate;f.crc=h->crc32;f.cs=h->comprSize;f.us=h->uncomprSize;f.fnl=(uint16_t)h->rawName.size();f.efl=f.cl=f.dn=0;f.ia=h->internalAttr;f.ea=h->externalAttr;f.off=offs[i];
    uint32_t id=0x02014b50; fwrite(&id,4,1,o); fwrite(&f,sizeof f,1,o); fwrite(h->rawName.data(),h->rawName.size(),1,o);}
  EOCD z; z.sig=0x06054b50;z.d=z.dc=0;z.n1=z.n2=(uint16_t)es.size();z.sz=(uint32_t)ftell(o)-cd;z.off=cd;z.cl=0; fwrite(&z,sizeof z,1,o); fclose(o);
  bool ok=Zip7::TestArchive(v[3],&err); printf("mixed zip test: %s %s\n",ok?"OK":"FAIL",err.c_str()); return ok?0:1;}
