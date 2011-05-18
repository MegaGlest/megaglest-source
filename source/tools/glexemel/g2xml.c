/**
 * File:    g2xml.c
 * Written: Jonathan Merritt <jmerritt@warpax.com>
 * 
 * Description:
 *   Converts G3D format files into an XML representation.
 *
 * Copyright (C) Jonathan Merritt 2005.
 * This file may be distributed under the terms of the GNU General Public
 * License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "g3dv4.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif



/**
 * Forward function declarations.
 */
int g3d2xml(FILE *infile, FILE *outfile);
void usage(char *execname);



/**
 * Program entry point.
 *
 * @param argc: Number of arguments passed to the program.
 * @param argv: Array of string arguments.
 *
 * @returns: EXIT_SUCCESS or EXIT_FAILURE depending upon success or failure
 *           of the conversion to XML.
 */
int main(int argc, char **argv)
{
	char *infilename, *outfilename;
	FILE *infile, *outfile;
	int successFlag;
	
	/* parse command line arguments */
	if (argc != 3)
	{
		usage(argv[0]);
		return (EXIT_FAILURE);
	}
	else
	{
		infilename  = argv[1];
		outfilename = argv[2];
	}

	/* attempt to open the input and output files */
//#ifdef WIN32
//	infile = _wfopen(utf8_decode(infilename).c_str(), L"rb");
//#else
	infile  = fopen(infilename, "rb");
//#endif
	if (infile == NULL) {
		printf("Could not open file \"%s\" for binary reading.\n",
			infilename);
		return (EXIT_FAILURE);
	}
//#ifdef WIN32
//	outfile = _wfopen(utf8_decode(outfilename).c_str(), L"w");
//#else
	outfile = fopen(outfilename, "w");
//#endif
	if (outfile == NULL)
	{
		printf("Could not open file \"%s\" for writing.\n",
			outfilename);
		fclose(infile);
		return (EXIT_FAILURE);
	}

	/* perform the XML conversion */
	successFlag = g3d2xml(infile, outfile);
	
	/* close the two files */
	fclose(infile);
	fclose(outfile);

	/* return a success or failure flag */
	if (successFlag)
		return (EXIT_SUCCESS);
	else
		return (EXIT_FAILURE);
}




/**
 * Prints a "Usage:" string for the program.
 *
 * @param execname: Executable name of the program.
 */
void usage(char *execname)
{
	printf("Usage:\n");
	printf("	%s infile.g3d outfile.xml\n", execname);

	return;
}



/**
 * Performs the conversion from the G3D file format to XML.
 *
 * @param infile:  G3D binary file, opened as "rb", for input.
 * @param outfile: Text file, opened as "w", for XML output.
 *
 * @returns: TRUE if conversion to XML was successful, and FALSE otherwise.
 */
int g3d2xml(FILE *infile, FILE *outfile)
{
	struct FileHeader fileHeader;
	struct ModelHeader modelHeader;
	struct MeshHeader meshHeader;
	size_t nBytes;
	uint8 textureName[NAMESIZE];
	float32 *fdata;
	uint32 *idata;
	int ii, jj, kk;

	/* read in the FileHeader */
	nBytes = sizeof(struct FileHeader);
	if (fread(&fileHeader, nBytes, 1, infile) != 1)
	{
		printf("Could not read file header!\n");
		return FALSE;
	}
	if (strncmp((char*)fileHeader.id, "G3D", 3) != 0)
	{
		printf("Expected \"G3D\" id was not found!\n");
		return FALSE;
	}
	if (fileHeader.version != 4)
	{
		printf("Version 4 expected, but version %d found!\n",
			fileHeader.version);
		return FALSE;
	}
	fprintf(outfile, "<?xml version=\"1.0\" encoding=\"ASCII\" ?>\n");
	fprintf(outfile, "<!DOCTYPE G3D SYSTEM \"g3d.dtd\">\n");
	fprintf(outfile, "<!--\n");
	fprintf(outfile, "\tThis file is an XML encoding of a G3D binary\n");
	fprintf(outfile, "\tfile.  The XML format is by Jonathan Merritt\n");
	fprintf(outfile, "\t(not yet accepted as part of Glest!).\n");
	fprintf(outfile, "-->\n");
	fprintf(outfile, "<G3D version=\"%d\">\n", fileHeader.version);
	
	/* read in the ModelHeader */
	nBytes = sizeof(struct ModelHeader);
	if (fread(&modelHeader, nBytes, 1, infile) != 1)
	{
		printf("Could not read model header!\n");
		return FALSE;
	}
	if (modelHeader.type != mtMorphMesh)
	{
		printf("Unrecognized mesh type!\n");
		return FALSE;
	}

	/* read in the meshes */
	for (ii = 0; ii < modelHeader.meshCount; ii++)
	{
		/* read in the MeshHeader */
		nBytes = sizeof(struct MeshHeader);
		if (fread(&meshHeader, nBytes, 1, infile) != 1)
		{
			printf("Could not read mesh header!\n");
			return FALSE;
		}
		
		/* write out XML mesh header */
		fprintf(outfile, "\t<Mesh name=\"%s\" ", meshHeader.name);
		fprintf(outfile, "frameCount=\"%d\" ", meshHeader.frameCount);
		fprintf(outfile, "vertexCount=\"%d\" ",
			meshHeader.vertexCount);
		fprintf(outfile, "indexCount=\"%d\" ", meshHeader.indexCount);
		fprintf(outfile, "specularPower=\"%f\" ",
			meshHeader.specularPower);
		fprintf(outfile, "opacity=\"%f\" ", meshHeader.opacity);
		if (meshHeader.properties & (0x1 << mpfTwoSided))
			fprintf(outfile, "twoSided=\"true\" ");
		else
			fprintf(outfile, "twoSided=\"false\" ");
		if (meshHeader.properties & (0x1 << mpfCustomColor))
			fprintf(outfile, "customColor=\"true\" ");
		else
			fprintf(outfile, "customColor=\"false\" ");
		if (meshHeader.textures)
			fprintf(outfile, "diffuseTexture=\"true\"");
		else
			fprintf(outfile, "diffuseTexture=\"false\"");
		fprintf(outfile, ">\n");

		/* write out diffuse and specular colors */
		fprintf(outfile, "\t\t<Diffuse>\n");
		fprintf(outfile, "\t\t\t<Color r=\"%f\" g=\"%f\" b=\"%f\"/>\n",
			meshHeader.diffuseColor[0],
			meshHeader.diffuseColor[1],
			meshHeader.diffuseColor[2]);
		fprintf(outfile, "\t\t</Diffuse>\n");
		fprintf(outfile, "\t\t<Specular>\n");
		fprintf(outfile, "\t\t\t<Color r=\"%f\" g=\"%f\" b=\"%f\"/>\n",
			meshHeader.specularColor[0],
			meshHeader.specularColor[1],
			meshHeader.specularColor[2]);
		fprintf(outfile, "\t\t</Specular>\n");

		/* read / write the texture name if present */
		if (meshHeader.textures)
		{
			nBytes = sizeof(textureName);
			if (fread(&textureName, nBytes, 1, infile) != 1)
			{
				printf("Could not read texture name!\n");
				return FALSE;
			}
			fprintf(outfile, "\t\t<Texture name=\"%s\"/>\n",
				textureName);
		}

		/* read / write each set of vertex data */
		for (jj=0; jj < meshHeader.frameCount; jj++)
		{
			nBytes = sizeof(float32)*meshHeader.vertexCount*3;
			fdata = malloc(nBytes);
			if (fdata == NULL)
			{
				printf("Could not allocate buffer!\n");
				return FALSE;
			}
			if (fread(fdata, nBytes, 1, infile) != 1)
			{
				printf("Could not read vertex data!\n");
				free(fdata);
				return FALSE;
			}
			fprintf(outfile, "\t\t<Vertices frame=\"%d\">\n",
				jj);
			for (kk=0; kk < meshHeader.vertexCount; kk++)
			{
				fprintf(outfile, "\t\t\t<Vertex ");
				/*fprintf(outfile, "i=\"%d\" ",
					kk);*/
				fprintf(outfile, "x=\"%f\" ",
					fdata[3*kk]);
				fprintf(outfile, "y=\"%f\" ",
					fdata[3*kk+1]);
				fprintf(outfile, "z=\"%f\"/>\n",
					fdata[3*kk+2]);
			}
			fprintf(outfile, "\t\t</Vertices>\n");
			free(fdata);
		}

		/* read / write each set of normal data */
		for (jj=0; jj < meshHeader.frameCount; jj++)
		{
			nBytes = sizeof(float32)*meshHeader.vertexCount*3;
			fdata = malloc(nBytes);
			if (fdata == NULL)
			{
				printf("Could not allocate buffer!\n");
				return FALSE;
			}
			if (fread(fdata, nBytes, 1, infile) != 1)
			{
				printf("Could not read normal data!\n");
				free(fdata);
				return FALSE;
			}
			fprintf(outfile, "\t\t<Normals frame=\"%d\">\n",
				jj);
			for (kk=0; kk < meshHeader.vertexCount; kk++)
			{
				fprintf(outfile, "\t\t\t<Normal ");
				/*fprintf(outfile, "i=\"%d\" ",
					kk);*/
				fprintf(outfile, "x=\"%f\" ",
					fdata[3*kk]);
				fprintf(outfile, "y=\"%f\" ",
					fdata[3*kk+1]);
				fprintf(outfile, "z=\"%f\"/>\n",
					fdata[3*kk+2]);
			}
			fprintf(outfile, "\t\t</Normals>\n");
			free(fdata);
		}

		/* read / write texture coordinates */
		if (meshHeader.textures)
		{
			nBytes = sizeof(float32)*meshHeader.vertexCount*2;
			fdata = malloc(nBytes);
			if (fdata == NULL)
			{
				printf("Could not allocate buffer!\n");
				return FALSE;
			}
			if (fread(fdata, nBytes, 1, infile) != 1)
			{
				printf("Could not read texture coords!\n");
				free(fdata);
				return FALSE;
			}
			fprintf(outfile, "\t\t<TexCoords>\n");
			for (kk=0; kk < meshHeader.vertexCount; kk++)
			{
				fprintf(outfile, "\t\t\t<ST ");
				/*fprintf(outfile, "i=\"%d\" ",
					kk);*/
				fprintf(outfile, "s=\"%f\" ",
					fdata[2*kk]);
				fprintf(outfile, "t=\"%f\"/>\n",
					fdata[2*kk+1]);
			}
			fprintf(outfile, "\t\t</TexCoords>\n");
			free(fdata);
		}

		/* read / write face indices */
		nBytes = sizeof(uint32)*meshHeader.indexCount;
		idata = malloc(nBytes);
		if (idata == NULL)
		{
			printf("Could not allocate buffer!\n");
			return FALSE;
		}
		if (fread(idata, nBytes, 1, infile) != 1)
		{
			printf("Could not read indexes!\n");
			free(idata);
			return FALSE;
		}
		fprintf(outfile, "\t\t<Indices>\n");
		for (kk=0; kk < meshHeader.indexCount; kk++)
		{
			fprintf(outfile, "\t\t\t<Ix i=\"%d\"/>\n",
				idata[kk]);
		}
		fprintf(outfile, "\t\t</Indices>\n");
		free(idata);
		
		fprintf(outfile, "\t</Mesh>\n");
	}

	fprintf(outfile, "</G3D>\n");
	
	return TRUE;

}

