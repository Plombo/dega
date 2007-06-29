// Emu module
#include "app.h"
#include "zlib.h"

char StateName[256]="";
static FILE *sf=NULL; // State file
static gzFile gf=NULL; // State file (compressed)
int AutoLoadSave=0;
int VideoReadOnly=0;

static char *MakeAutoName(int Battery)
{
  char *Name=NULL; int Len=0,Extra=0;
  if (EmuTitle==NULL) return NULL;
  Len=strlen(EmuTitle);

  if (Battery) Extra=11; // saves,\ and .sav 0
  else         Extra=16; // saves,\ and _Auto.dgz 0

  Name=(char *)malloc(Len+Extra);
  if (Name==NULL) return NULL;
  memcpy(Name,"saves\\",6);
  memcpy(Name+6,EmuTitle,Len);
  if (Battery) memcpy(Name+6+Len,".sav",5);
  else         memcpy(Name+6+Len,"_auto.dgz",10);
  return Name;
}

static int StateLoadAcb(struct MastArea *pma)
{
       if (gf!=NULL) gzread(gf,pma->Data,pma->Len);
  else if (sf!=NULL) fread(pma->Data,1,pma->Len,sf);
  return 0;
}

int StateLoad(int Meka)
{
  if (StateName[0]==0) return 1;
  
  if (Meka==0) gf=gzopen(StateName,"rb");
  else         sf=fopen (StateName,"rb");
  if (gf==NULL && sf==NULL) return 1;

  // Scan state
  MastAcb=StateLoadAcb;
  if (Meka) MastAreaMeka(); else MastAreaDega();
  MastAcb=MastAcbNull;

  if (sf!=NULL) fclose(sf); sf=NULL;
  if (gf!=NULL) gzclose(gf); gf=NULL;

  MvidPostLoadState(VideoReadOnly);

  return 0;
}

// ------------------------------------------------------------

static int StateSaveAcb(struct MastArea *pma)
{
       if (gf!=NULL) gzwrite(gf,pma->Data,pma->Len);
  else if (sf!=NULL) fwrite(pma->Data,1,pma->Len,sf);
  return 0;
}

int StateSave(int Meka)
{
  if (StateName[0]==0) return 1;

  if (Meka==0) gf=gzopen(StateName,"wb");
  else         sf=fopen (StateName,"wb");
  if (gf==NULL && sf==NULL) return 1;

  // Scan state
  MastAcb=StateSaveAcb;
  if (Meka) MastAreaMeka(); else MastAreaDega();
  MastAcb=MastAcbNull;

  if (sf!=NULL) fclose(sf); sf=NULL;
  if (gf!=NULL) gzclose(gf); gf=NULL;
  return 0;
}

// ------------------------------------------------------------
static FILE *bf=NULL; // Battery file
static int bs=0; // 0=Battery load 1=Battery save 2=Check for something
static int BattCont=0; // 1 if the battery contains something

static int BatteryAcb(struct MastArea *pma)
{
  unsigned char *pd,*pe;
  if (bs==0) { fread (pma->Data,1,pma->Len,bf); return 0; }
  if (bs==1) { fwrite(pma->Data,1,pma->Len,bf); return 0; }

  // Search for non-zero memory in the battery
  if (BattCont) return 0; // Already found something

  pd=(unsigned char *)pma->Data; pe=pd+pma->Len;
  if (pd>=pe) return 0;
  do { if (pd[0]) { BattCont=1; return 0; }  pd++; } while (pd<pe);
  return 0;
}

// Save battery ram
int BatterySave()
{
  char *Name=NULL;
  // Get name of save ram
  Name=MakeAutoName(1); if (Name==NULL) return 1;

  bf=fopen(Name,"rb"); // See if there is anything already saved
  if (bf!=NULL) { fclose(bf); } // Yes
  else
  {
    // We haven't saved before. See if we have a blank battery
    BattCont=0;
    MastAcb=BatteryAcb; bs=2; MastAreaBattery(); bs=0; MastAcb=MastAcbNull; 
    if (BattCont==0) { free(Name); return 0; } // Blank battery: don't make a file
  }

  CreateDirectory("saves",NULL); // Make sure there is a directory
  bf=fopen(Name,"wb");
  free(Name);
  if (bf==NULL) return 1;
  // Save battery
  MastAcb=BatteryAcb; bs=1; MastAreaBattery(); bs=0; MastAcb=MastAcbNull;
  fclose(bf); bf=NULL;
  return 0;
}

// Load battery ram
int BatteryLoad()
{
  char *Name=NULL;
  // Get name of save ram
  Name=MakeAutoName(1); if (Name==NULL) return 1;
  bf=fopen(Name,"rb");
  free(Name);
  if (bf==NULL) return 1;
  MastAcb=BatteryAcb; bs=0; MastAreaBattery(); MastAcb=MastAcbNull;
  fclose(bf); bf=NULL;
  return 0;
}

static int StateAutoState(int Save)
{
  char *Name=NULL;
  // Load/Save state

  // Make the name of the auto save file
  Name=MakeAutoName(0); if (Name==NULL) return 1;
  strncpy(StateName,Name,255);
  free(Name);

  if (Save)
  {
    CreateDirectory("saves",NULL); // Make sure there is a directory
    StateSave(0);
  }
  else
  {
    StateLoad(0);
  }

  memset(StateName,0,sizeof(StateName));
  return 0;
}

// Load/Save battery ram and state
int StateAuto(int Save)
{
  if (AutoLoadSave) StateAutoState(Save);
  if (Save) BatterySave(); else BatteryLoad();
  return 0;
}
// ------------------------------------------------------------
