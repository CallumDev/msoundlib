#include "mystdlib.h"

//Deal with Win32's complete non-support for UTF-8
#ifdef _WIN32
static wchar_t *conv_str(const char *src)
{
	size_t size = mbstowcs(NULL, src, 30000); //arbitrarily large value for count
	wchar_t *buffer = (wchar_t*)malloc(size);
	mbstowcs(buffer, src, 30000);
	return buffer;
}

FILE *fopen_wrapper(const char *filename, const char *mode)
{
	printf("Converting strings\n");
	wchar_t *fname = conv_str(filename);
	wchar_t *m = conv_str(mode);
	printf("wfopen\n");
	FILE *file = _wfopen(fname,m);
	printf("free\n");
	free(fname); 
	free(m);
	return file;
}
#endif