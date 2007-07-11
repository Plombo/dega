// Zip module
#include "app.h"
#include "unzip.h"

static unzFile Zip=NULL;

int ZipOpen(char *ZipName)
{
  int Len=0;
  Len=strlen(ZipName);
  if (Len<4) return 1;
  if (stricmp(ZipName+Len-4,".zip")!=0) return 1; // Not a zip file

  Zip=unzOpen(ZipName);
  if (Zip==NULL) return 1;
  unzGoToFirstFile(Zip);

  return 0;
}

int ZipClose()
{
  if (Zip!=NULL) unzClose(Zip);  Zip=NULL;
  return 0;
}

int ZipRead(unsigned char **pMem,int *pLen)
{
  int Ret=0,i=0;
  unz_file_info Info;
  char Name[256];
  int RomPos=0; // Which entry in the zip file is the rom
  unsigned char *Mem=NULL; int Len=0;

  if (Zip==NULL) return 1;
  memset(&Info,0,sizeof(Info));
  memset(Name,0,sizeof(Name));

  MastEx&=~MX_GG;
  // Find out which entry is the rom, and also auto-detect Game Gear
  unzGoToFirstFile(Zip);
  for (i=0; ;i++)
  {
    int Len=0;
    unzGetCurrentFileInfo(Zip,&Info,Name,sizeof(Name),NULL,0,NULL,0);
    Len=strlen(Name);
    if (Len>=4) if (stricmp(Name+Len-4,".sms")==0) { RomPos=i; break; }
    if (Len>=3) if (stricmp(Name+Len-3,".gg" )==0) { RomPos=i; MastEx|=MX_GG; break; }

    Ret=unzGoToNextFile(Zip); if (Ret!=UNZ_OK) break;
  }
  // If we didn't find the correct extension, just assume it's the first entry

  // Now go to the rom again
  unzGoToFirstFile(Zip);
  for (i=0;i<RomPos;i++) unzGoToNextFile(Zip);
  unzGetCurrentFileInfo(Zip,&Info,NULL,0,NULL,0,NULL,0);

  // Find out how long it is and allocate space
  Len=Info.uncompressed_size;
  Mem=(unsigned char *)malloc(Len);
  if (Mem==NULL) return 1;
  memset(Mem,0,Len);
  // Read in the rom
  unzOpenCurrentFile(Zip);
  unzReadCurrentFile(Zip,Mem,Len);
  unzCloseCurrentFile(Zip);
  // Set name
  MastSetRomName(Name);
  // Return the allocated memory
  *pMem=Mem; *pLen=Len;
  return 0;
}
