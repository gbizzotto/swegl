
#ifndef SWE_MAIN
#define SWE_MAIN

#define SCR_WIDTH 800
#define SCR_HEIGHT 600

#if defined(_DEBUG) || defined(DEBUG)
	void AssertFailed(char * cond, char * filename, int line);
	#define ASSERT(cond) do { if (!(cond)) {AssertFailed(#cond, __FILE__, __LINE__);} } while(0)
	extern unsigned int g_trianglesdrawn;
	extern unsigned int g_pixelsdrawn;
#else
	#define ASSERT(cond)
#endif

#endif // SWE_MAIN