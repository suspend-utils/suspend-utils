/*
 * splash.c
 *
 * Bootsplash integration into the suspend and resume tools
 *
 * Copyright (C) 2006 Holger Macht <holger@homac.de>
 *
 * This file is released under the GPLv2.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "splash.h"
#include "bootsplash.h"
#include "splashy_funcs.h"
#include "encrypt.h"
#include <unistd.h>
#include <termios.h>

/**
 *	dummy functions in case if no splash system was found or
 *	bootsplashing is disabled
 */
static int splash_dummy_int_void(void) { return 0; }
static int splash_dummy_int_int(int p) { return 0; }
static void splash_dummy_void_void(void) { return; }
#ifndef CONFIG_ENCRYPT
static void splash_dummy_readpass(char *a, int b) { }
#endif
static int splash_dialog(const char *prompt) 
{
	printf(prompt);
	return getchar();
}
static int prepare_abort(struct termios *oldtrm, struct termios *newtrm) 
{
	int ret;

	ret = tcgetattr(0, oldtrm);
	if (!ret) {
		*newtrm = *oldtrm;
		newtrm->c_cc[VMIN] = 0;
		newtrm->c_cc[VTIME] = 1;
		newtrm->c_iflag = IGNBRK | IGNPAR | ICRNL | IMAXBEL;
		newtrm->c_lflag = 0;
		ret = tcsetattr(0, TCSANOW, newtrm);
	}

	return ret;
}

static int key_pressed(const char key)
{
	char c;
	if (read(0, &c, 1) > 0 && c == key) 
		return 1;

	return 0;
}

static void restore_abort(struct termios *oldtrm) 
{
	tcsetattr(0, TCSANOW, oldtrm);
}

/* Tries to find a splash system and initializes interface functions */
void splash_prepare(struct splash *splash, int enabled)
{
	int error = 0;

	splash->finish      = splash_dummy_int_void;
	splash->progress    = splash_dummy_int_int;
	splash->switch_to   = splash_dummy_void_void;
	splash->dialog	    = splash_dialog;
#ifdef CONFIG_ENCRYPT
	splash->read_password   = read_password;
#else
	splash->read_password   = splash_dummy_readpass;
#endif
	splash->prepare_abort	= prepare_abort;
	splash->restore_abort	= restore_abort;
	splash->key_pressed	= key_pressed;
	if (!enabled)
		return;

	printf("Looking for splash system... ");

	if (!(error = bootsplash_open())) {
		splash->finish      = bootsplash_finish;
		splash->progress    = bootsplash_progress;
		splash->switch_to   = bootsplash_switch_to;
		splash->dialog	    = bootsplash_dialog;
		splash->read_password = bootsplash_read_password;
#ifdef CONFIG_SPLASHY
	} else if (!splashy_open()) {
		splash->finish      = splashy_finish;
		splash->progress    = splashy_progress;
		splash->dialog	    = splashy_dialog;
		splash->read_password   = splashy_read_password;
#endif
	} else if (0) {
		/* add another splash system here */
	} else {
		printf("none\n");
		return;
	}
	printf("found\n");

	splash->progress(0);
}
