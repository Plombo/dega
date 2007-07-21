#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_OBJECT_H
typedef struct _object PyObject;
#endif

extern unsigned int MPyEmbed_Exiting;

void MPyEmbed_SetArgv(int _argc, char **_argv);
void MPyEmbed_Init(void);
void MPyEmbed_Fini(void);
int MPyEmbed_Available(void);
int MPyEmbed_Run(char *file);
int MPyEmbed_Repl(void);
int MPyEmbed_RunThread(char *file);
PyObject *MPyEmbed_GetPostFrame(void);
int MPyEmbed_SetPostFrame(PyObject *postframe);
void MPyEmbed_CBPostFrame(void);

#ifdef __cplusplus
}
#endif
