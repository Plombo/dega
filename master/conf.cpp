// Config file module
#include "app.h"

static char *Config="dega.ini";

static char *LabelCheck(char *s,char *Label)
{
#define SKIP_WS(s) for (;;) { if (*s!=' ' && *s!='\t') break;  s++; } // skip whitespace
  int Len;
  if (s==NULL) return NULL;  if (Label==NULL) return NULL;
  Len=strlen(Label);
  SKIP_WS(s) // skip whitespace

  if (strncmp(s,Label,Len)!=0) return NULL; // doesn't match
  return s+Len;
}

// Read in the config file for the whole application
int ConfLoad()
{
  char Line[256];
  FILE *h=NULL;
  h=fopen(Config,"rt");
  if (h==NULL) return 1;

  // Go through each line of the config file
  for (;;)
  {
    int i, Len=0;
    if (fgets(Line,sizeof(Line),h)==NULL) break; // End of config file

    Len=strlen(Line);
    // Get rid of the linefeed at the end
    if (Line[Len-1]==10) { Line[Len-1]=0; Len--; }

#define VAR(x) { char *Value; Value=LabelCheck(Line,#x); \
    if (Value!=NULL) x=strtol(Value,NULL,0); }
#define STR(x) { char *Value; Value=LabelCheck(Line,#x " "); \
    if (Value!=NULL) strcpy(x,Value); }

    STR(RomFolder)

    VAR(SetupPal)
    VAR(MastEx)
    VAR(TryOverlay)
    VAR(StartInFullscreen)

    VAR(DSoundSamRate)
    VAR(DpsgEnhance)

    VAR(UseJoystick)

    STR(StateFolder)
    VAR(AutoLoadSave)

    for (i = 0; i < KMAPCOUNT; i++)
    {
      char buf[20], *value;
      snprintf(buf, sizeof(buf), "KeyMapping%d", i);
      value = LabelCheck(Line, buf);
      if (value!=NULL) KeyMappings[i] = strtol(value,NULL,0);
    }
#undef STR
#undef VAR
  }

  fclose(h);
  return 0;
}

// Write out the config file for the whole application
int ConfSave()
{
  int i;
  FILE *h=NULL;
  h=fopen(Config,"wt");
  if (h==NULL) return 1;

  // Write title
  fprintf (h,"// %s Config File\n",AppName());

#define VAR(x) fprintf (h,#x " %d\n",x);
#define STR(x) fprintf (h,#x " %s\n",x);

    fprintf (h,"\n// File\n");
    STR(RomFolder)

    fprintf (h,"\n// Setup\n");
    VAR(SetupPal)
    VAR(MastEx)
    VAR(TryOverlay)
    VAR(StartInFullscreen)

    fprintf (h,"\n// Sound\n");
    VAR(DSoundSamRate)
    VAR(DpsgEnhance)

    fprintf (h,"\n// Input\n");
    VAR(UseJoystick)

    fprintf (h,"\n// State\n");
    STR(StateFolder)
    VAR(AutoLoadSave)

    fprintf (h, "\n// Keys\n");
    for (i = 0; i < KMAPCOUNT; i++)
    {
      fprintf( h, "KeyMapping%d %d\n", i, KeyMappings[i]);
    }
    
#undef STR
#undef VAR

  fclose(h);
  return 0;
}
