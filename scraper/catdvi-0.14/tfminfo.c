#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "findtfm.h"
#include "fontinfo.h"
#include "bytesex.h"
#include "fixword.h"

#define FONT_INDEX 0
    /* we use only this one font slot */

char * generic_parms[] = {
    "slant",
    "space",
    "space_stretch",
    "space_shrink",
    "x_height",
    "quad",
    "extra_space"
};

char * OMS_parms[] = {
    "num1",
    "num2",
    "num3",
    "denom1",
    "denom2",
    "sup1",
    "sup2",
    "sup3",
    "sub1",
    "sub2",
    "supdrop",
    "subdrop",
    "delim1",
    "delim2",
    "axis_height"
};

char * OMX_parms[] ={
    "default_rule_thickness",
    "big_op_spacing1",
    "big_op_spacing2",
    "big_op_spacing3",
    "big_op_spacing4",
    "big_op_spacing5"
};

static size_t desc_copy(
    char ** to,
    char ** from,
    size_t base,
    size_t count,
    size_t np
)
{
    if(base > np) return (np + 1);
    count = min(count, np + 1 - base);
    memcpy(to + base, from, sizeof(char *) * count);
    return (base + count);
}

int main(int argc, char * argv[])
{
    int np, enp, i;
    size_t b;
    char ** description;

    if(argc != 2) {
    	fputs("Usage error!\n", stderr);
	exit(1);
    }
    
    setup_findtfm(argv[0]);

    font_def(
    	FONT_INDEX,
	0,  	    	    /* checksum - we don't know that */
	double2fw(1.0),     /* scaling */
	double2fw(10.0),    /* design size */
	0,  	    	    /* length of the fontnames directory part */
	strlen(argv[1]),    /* length of the fontnames non-directory part */
	argv[1]     	    /* fontname (e.g. cmr10) */
    );
    np = font_nparams(FONT_INDEX);

    description = malloc((np + 1) * sizeof(char *));
    for(i = 0; i <= np; ++i) description[i] = "unknown";
    
    b = 1;
    enp = lengthof(generic_parms);
    b = desc_copy(description, generic_parms, b, lengthof(generic_parms), np);
    
    if(strcmp(font_enc(FONT_INDEX), "TeX math symbols") == 0) {
    	enp += lengthof(OMS_parms);
    	b = desc_copy(
	    description,
	    OMS_parms,
	    b,
	    lengthof(OMS_parms),
	    np
	);
    }
    else if(strcmp(font_enc(FONT_INDEX), "TeX math extension") == 0) {
    	enp += lengthof(OMX_parms);
    	b = desc_copy(
	    description,
	    OMX_parms,
	    b,
	    lengthof(OMX_parms),
	    np
	);
    }
    
    printf("Encoding: %s\n", font_enc(FONT_INDEX));
    printf("Family: %s\n", font_family(FONT_INDEX));

    printf("Nparams: %d [expected: %d]\n", np, enp);
    for(i = 1; i <= np; ++i) {
    	printf(
	    "Param%02d: %4.7g [%s]\n",
	    i,
	    fw2double(font_param(FONT_INDEX, i)),
	    description[i]
	);
    }
    exit(0);
}

    
