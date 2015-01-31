/*
 * gluefe.c: A wrapper for the frontend functions of the Windows Modern port.
 */

#include <assert.h>
#include <stdarg.h>
#include "puzzles.h"
#include "gluefe.h"

struct frontend_adapter *adapter = NULL;

void get_random_seed(void **randseed, int *randseedsize)
{
	assert(adapter && adapter->get_random_seed);
	adapter->get_random_seed(randseed, randseedsize);
}

char *getenv(char *key)
{
	if (adapter && adapter->getenv)
		return adapter->getenv(key);
	
	return 0;
}

void frontend_default_colour(frontend *fe, float *output)
{

	if (adapter && adapter->frontend_default_colour)
		adapter->frontend_default_colour(fe, output);
	else
		output[0] = output[1] = output[2] = 0.9f;
}

void fatal(char *fmt, ...)
{
	char fullmsg[1024];
	va_list va;
	va_start(va, fmt);
	vsprintf(fullmsg, fmt, va);
	va_end(va);

	if (adapter && adapter->fatal)
		adapter->fatal(fullmsg);
	else
		exit(1);
}

void deactivate_timer(frontend *fe)
{
	if (adapter && adapter->deactivate_timer)
		adapter->deactivate_timer(fe);
}

void activate_timer(frontend *fe)
{
	if (adapter && adapter->activate_timer)
		adapter->activate_timer(fe);
}

void set_frontend_adapter(struct frontend_adapter *fea)
{
	adapter = fea;
}

void check_abort(void)
{
	if (adapter && adapter->check_abort)
		adapter->check_abort();
}

#ifdef DEBUGGING
void debug_printf(char *fmt, ...)
{
	char fullmsg[1024];
	va_list va;
	va_start(va, fmt);
	vsprintf(fullmsg, fmt, va);
	va_end(va);

	if (adapter && adapter->debugout)
		adapter->debugout(fullmsg);
}
#endif