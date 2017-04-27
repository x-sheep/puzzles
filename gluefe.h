#ifndef PUZZLES_GLUEFE_H
#define PUZZLES_GLUEFE_H

struct frontend {
	struct preset_menu *preset_menu;
	int printw, printh;
	drawing *printdr;

	float scale;
	config_item *configs;
	void *canvas;
	void *statusbar;
	void *timer;
};

struct frontend_adapter {
	void(*get_random_seed)(void **randseed, int *randseedsize);
	char*(*getenv)(const char *key);
	void(*frontend_default_colour)(frontend *fe, float *output);
	void(*fatal)(const char *msg);
#ifdef DEBUGGING
	void(*debugout)(const char *msg);
#endif
	void(*deactivate_timer)(frontend *fe);
	void(*activate_timer)(frontend *fe);
	void(*check_abort)(void);
};

struct blitter {
	int handle;

	/* The size of the blitter bitmap */
	int w, h;

	/* The actual size of the last saved bitmap */
	int rw, rh;

	/* The position of the last saved bitmap in the window */
	int x, y;
};

void get_random_seed(void **randseed, int *randseedsize);
char *getenv(const char *key);
void frontend_default_colour(frontend *fe, float *output);
void fatal(char *fmt, ...);
void deactivate_timer(frontend *fe);
void activate_timer(frontend *fe);
void check_abort(void);

void set_frontend_adapter(struct frontend_adapter *fea);

#endif /* PUZZLES_GLUEFE_H */
