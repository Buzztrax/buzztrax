
#include "glib.h"

int main(int argc, char **argv) {
	gpointer mem;

	g_mem_set_vtable(glib_mem_profiler_table);

	mem=g_malloc(1024);
	g_mem_profile();
	
	g_free(mem);
	g_mem_profile();

	return(0);
}
