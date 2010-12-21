/* $Id: testigddescparse.c,v 1.1 2008/04/23 11:53:45 nanard Exp $ */
/* Project : miniupnp
 * http://miniupnp.free.fr/
 * Author : Thomas Bernard
 * Copyright (c) 2008 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution.
 * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "igd_desc_parse.h"
#include "minixml.h"

int test_igd_desc_parse(char * buffer, int len)
{
	struct IGDdatas igd;
	struct xmlparser parser;
	memset(&igd, 0, sizeof(struct IGDdatas));
	memset(&parser, 0, sizeof(struct xmlparser));
	parser.xmlstart = buffer;
	parser.xmlsize = len;
	parser.data = &igd;
	parser.starteltfunc = IGDstartelt;
	parser.endeltfunc = IGDendelt;
	parser.datafunc = IGDdata;
	parsexml(&parser);
	printIGD(&igd);
	return 0;
}

int main(int argc, char * * argv)
{
	FILE * f;
	char * buffer;
	int len;
	int r = 0;
	if(argc<2) {
		fprintf(stderr, "Usage: %s file.xml\n", argv[0]);
		return 1;
	}
	f = fopen(argv[1], "r");
	if(!f) {
		fprintf(stderr, "Cannot open %s for reading.\n", argv[1]);
		return 1;
	}
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	buffer = malloc(len);
	size_t ret = fread(buffer, 1, len, f);
	fclose(f);
	r = test_igd_desc_parse(buffer, len);
	free(buffer);
	return r;
}

