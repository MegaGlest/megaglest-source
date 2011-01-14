/**
 * File:    xml2g.c
 * Written: Jonathan Merritt <jmerritt@warpax.com>
 *
 * Description:
 *   Converts G3Dv4 XML files into the binary representation used in the game.
 * Requirements:
 *   libxml2
 *
 * Copyright (C) Jonathan Merritt 2005.
 * This file may be distributed under the terms of the GNU General Public
 * License.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

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
int xml2g3d(xmlDocPtr doc, FILE *outfile);
void usage(char *execname);
unsigned int countChildren(xmlNode *n, xmlChar *name);
int processMesh(xmlNode *n, FILE *outfile);
int readColorChild(xmlNode *n, char *childName, float32 *color);
int processVertices(xmlNode *n, FILE *outfile, uint32 vertexCount);
int processNormals(xmlNode *n, FILE *outfile, uint32 vertexCount);
int processTexcoords(xmlNode *n, FILE *outfile, uint32 vertexCount);
int processIndices(xmlNode *n, FILE *outfile, uint32 indexCount);


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
	FILE *outfile;
	int successFlag;
	xmlParserCtxtPtr ctxt;
	xmlDocPtr doc;

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
	
	/* attempt to parse the XML file */
	LIBXML_TEST_VERSION
	ctxt = xmlNewParserCtxt();
	if (ctxt == NULL)
	{
		printf("Failed to allocate XML parser context!\n");
		return (EXIT_FAILURE);
	}
	doc = xmlCtxtReadFile(ctxt, infilename, NULL, XML_PARSE_DTDVALID);
	if (doc == NULL)
	{
		printf("Could not parse XML file \"%s\"!\n", infilename);
		xmlFreeParserCtxt(ctxt);
		return (EXIT_FAILURE);
	}

	/* attempt to open the output binary file */
	outfile = fopen(outfilename, "wb");
	if (outfile == NULL)
	{
		printf("Could not open file \"%s\" for writing!\n",
			outfilename);
		xmlFreeDoc(doc);
		xmlFreeParserCtxt(ctxt);
		xmlCleanupParser();
		return (EXIT_FAILURE);
	}

	/* perform the conversion: XML -> binary */
	successFlag = xml2g3d(doc, outfile);

	/* close the files */
	xmlFreeDoc(doc);
	xmlFreeParserCtxt(ctxt);
	xmlCleanupParser();
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
	printf("	%s infile.xml outfile.g3d\n", execname);

	return;
}



/**
 * Performs the conversion from the XML file format to the G3D binary format.
 *
 * @param doc:     XML DOM document for input.
 * @param outfile: Binary file, opened as "wb", for output.
 */
int xml2g3d(xmlDocPtr doc, FILE *outfile)
{	
	struct FileHeader fh;
	struct ModelHeader mh;
	xmlNode *root_element;
	xmlNode *curNode;
	xmlChar version[] = "version";

	/* fetch the root element and check it */
	root_element = xmlDocGetRootElement(doc);
	assert(root_element->type == XML_ELEMENT_NODE);
	if (strcmp((char*)root_element->name, "G3D") != 0)
	{
		printf("G3D document not found!\n");
		return FALSE;
	}
	if (strcmp((char*)xmlGetProp(root_element, version), "4") != 0)
	{
		printf("Only version 4 G3D documents can be handled!\n");
		return FALSE;
	}

	/* write out the file header */
	memset(&fh, 0, sizeof(struct FileHeader));
	fh.id[0] = 'G'; fh.id[1] = '3'; fh.id[2] = 'D'; fh.version=4;
	fwrite(&fh, sizeof(struct FileHeader), 1, outfile);

	/* write out the model header */
	memset(&mh, 0, sizeof(struct ModelHeader));
	mh.meshCount = (uint16)countChildren(root_element, (xmlChar*)"Mesh");
	mh.type = 0;
	fwrite(&mh, sizeof(struct ModelHeader), 1, outfile);

	/* process each mesh in the file */
	curNode = root_element->children;
	for (; curNode; curNode = curNode->next)
	{
		if (curNode->type == XML_ELEMENT_NODE)
		{
			if (strcmp((char*)curNode->name, "Mesh") == 0)
			{
				if (processMesh(curNode, outfile) == FALSE)
					return FALSE;
			}
		}
	}
	
	return TRUE;
}



/**
 * Counts the number of child nodes with the specified name.
 *
 * @param n:    Node to count the children from.
 * @param name: Name of the child nodes to count.
 *
 * @return: The number of child nodes.
 */
unsigned int countChildren(xmlNode *n, xmlChar *name)
{
	unsigned int count = 0;
	xmlNode *curNode = NULL;

	for (curNode = n->children; curNode; curNode = curNode->next)
	{
		if (curNode->type == XML_ELEMENT_NODE)
		{
			if (strcmp((char*)curNode->name, (char*)name) == 0)
				count++;
		}
	}

	return count;
}



/**
 * Processes all <Mesh> elements from the XML file.
 *
 * @param n:       <Mesh> element
 * @param outfile: Binary file, opened as "wb", for output.
 *
 * @return: TRUE if successful, FALSE otherwise.
 */
int processMesh(xmlNode *n, FILE *outfile)
{
	xmlChar name[]           = "name";
	xmlChar frameCount[]     = "frameCount";
	xmlChar vertexCount[]    = "vertexCount";
	xmlChar indexCount[]     = "indexCount";
	xmlChar specularPower[]  = "specularPower";
	xmlChar opacity[]        = "opacity";
	xmlChar twoSided[]       = "twoSided";
	xmlChar customColor[]    = "customColor";
	xmlChar diffuseTexture[] = "diffuseTexture";

	float32 color[3];
	
	struct MeshHeader mh;
	uint8 texname[NAMESIZE];

	int foundFlag = FALSE;
	xmlNode *texn = NULL;
	xmlNode *curNode = NULL;
	
	/* make sure we're dealing with a <Mesh> element */
	assert(strcmp((char*)n->name, "Mesh") == 0);

	/* populate the MeshHeader structure appropriately */
	memset(&mh, 0, sizeof(struct MeshHeader));
	strncpy((char*)mh.name, (char*)xmlGetProp(n, name), NAMESIZE);
	mh.frameCount    = (uint32)atoi((char*)xmlGetProp(n, frameCount));
	mh.vertexCount   = (uint32)atoi((char*)xmlGetProp(n, vertexCount));
	mh.indexCount    = (uint32)atoi((char*)xmlGetProp(n, indexCount));
	if (readColorChild(n, "Diffuse", color) == FALSE)
		return FALSE;
	mh.diffuseColor[0] = color[0];
	mh.diffuseColor[1] = color[1];
	mh.diffuseColor[2] = color[2];
	if (readColorChild(n, "Specular", color) == FALSE)
		return FALSE;
	mh.specularColor[0] = color[0];
	mh.specularColor[1] = color[1];
	mh.specularColor[2] = color[2];
	mh.specularPower = (float32)atof((char*)xmlGetProp(n, specularPower));
	mh.opacity       = (float32)atof((char*)xmlGetProp(n, opacity));
	mh.properties    = 0;
	if (strcmp((char*)xmlGetProp(n, twoSided), "true") == 0)
		mh.properties += (0x1 << mpfTwoSided);
	if (strcmp((char*)xmlGetProp(n, customColor), "true") == 0)
		mh.properties += (0x1 << mpfCustomColor);
	mh.textures      = 0;
	if (strcmp((char*)xmlGetProp(n, diffuseTexture), "true") == 0)
		mh.textures = 1;

	/* write the MeshHeader */
	fwrite(&mh, sizeof(struct MeshHeader), 1, outfile);

	/* if we have a texture, then also write its name */
	foundFlag = FALSE;
	if (mh.textures)
	{
		for (texn=n->children; texn; texn = texn->next)
		{
			if (texn->type == XML_ELEMENT_NODE)
			{
				if (strcmp((char*)texn->name, "Texture") == 0)
				{
					foundFlag = TRUE;
					break;
				}
			}
		}
		if (foundFlag == FALSE)
		{
			printf("Could not find <Texture> element!\n");
			return FALSE;
		}
		memset(texname, 0, NAMESIZE);
		strncpy((char*)texname, 
			(char*)xmlGetProp(texn, (xmlChar*)"name"), NAMESIZE);
		fwrite(texname, NAMESIZE, 1, outfile);
	}

	/* write out vertices */
	foundFlag = FALSE;
	for (curNode=n->children; curNode; curNode = curNode->next)
	{
		if (curNode->type == XML_ELEMENT_NODE)
		{
			if (strcmp((char*)curNode->name, "Vertices") == 0)
			{
				foundFlag = TRUE;
				if (processVertices(curNode, outfile,
					mh.vertexCount) == FALSE)
				{
					return FALSE;
				}
			}
		}
	}
	if (foundFlag == FALSE)
	{
		printf("No <Vertices> found!\n");
		return FALSE;
	}
	
	/* write out normals */
	foundFlag = FALSE;
	for (curNode=n->children; curNode; curNode = curNode->next)
	{
		if (curNode->type == XML_ELEMENT_NODE)
		{
			if (strcmp((char*)curNode->name, "Normals") == 0)
			{
				foundFlag = TRUE;
				if (processNormals(curNode, outfile,
					mh.vertexCount) == FALSE)
				{
					return FALSE;
				}
			}
		}
	}
	if (foundFlag == FALSE)
	{
		printf("No <Normals> found!\n");
		return FALSE;
	}
	
	/* write out texture coordinates */
	if (mh.textures)
	{
		foundFlag = FALSE;
		for (curNode=n->children; curNode; curNode = curNode->next)
		{
			if (curNode->type == XML_ELEMENT_NODE)
			{
				if (strcmp((char*)curNode->name, "TexCoords")
					== 0)
				{
					foundFlag = TRUE;
					if (processTexcoords(curNode,
						outfile, mh.vertexCount) == 
						FALSE)
					{
						return FALSE;
					}
				}
			}
		}
		if (foundFlag == FALSE)
		{
			printf("No <TexCoords> found!\n");
			return FALSE;
		}
	}
	
	/* write out indices */
	foundFlag = FALSE;
	for (curNode=n->children; curNode; curNode = curNode->next)
	{
		if (curNode->type == XML_ELEMENT_NODE)
		{
			if (strcmp((char*)curNode->name, "Indices") == 0)
			{
				foundFlag = TRUE;
				if (processIndices(curNode, outfile,
					mh.indexCount) == FALSE)
				{
					return FALSE;
				}
			}
		}
	}
	if (foundFlag == FALSE)
	{
		printf("No <Indices> found!\n");
	}

	return TRUE;
}



/**
 * Reads the <Color> child of a child of a node.
 *
 * For example:
 * <Mesh>
 * 	<Diffuse>
 * 		<Color r="0.1" g="0.2" b="0.3"/>
 * 	</Diffuse>
 * </Mesh>
 * If <Mesh> is passed in as n and childNode is the string "Diffuse", then 
 * the method will read the <Color> element from within the <Diffuse> element.
 * 
 * @param n:         Node from which the children should be read.
 * @param childNode: Name of the child element that owns a <Color> element.
 * @param color:     float32[3] array to hold RGB color.
 *
 * @return TRUE if the method succeeds, FALSE otherwise.
 */
int readColorChild(xmlNode *n, char *childNode, float32 *color)
{
	int foundFlag = FALSE;
	double r,g,b;
	xmlNode *curNode = NULL;

	/* make sure that n is an element */
	assert(n->type == XML_ELEMENT_NODE);

	/* search for the first child node matching the given name */
	for (curNode=n->children; curNode; curNode = curNode->next)
	{
		if (curNode->type == XML_ELEMENT_NODE)
		{
			if (strcmp((char*)curNode->name, (char*)childNode) == 0)
			{
				foundFlag = TRUE;
				break;
			}
		}
	}
	if (foundFlag == FALSE)
	{
		printf("Could not find child node \"%s\"!\n", childNode);
		return FALSE;
	}

	/* search for a <Color> node child */
	for (curNode=curNode->children; curNode; curNode = curNode->next)
	{
		if (curNode->type == XML_ELEMENT_NODE)
		{
			if (strcmp((char*)curNode->name, "Color") == 0)
			{
				r = atof((char*)xmlGetProp(curNode,
					(xmlChar*)"r"));
				g = atof((char*)xmlGetProp(curNode,
					(xmlChar*)"g"));
				b = atof((char*)xmlGetProp(curNode,
					(xmlChar*)"b"));
				color[0] = (float32)r;
				color[1] = (float32)g;
				color[2] = (float32)b;
				return TRUE;
			}
		}
	}
	printf("Could not find <Color> child of \"%s\"!\n", childNode);
	return FALSE;
}



/**
 * Writes vertices into the binary file.
 *
 * @param n:           <Vertices> element.
 * @param outfile:     Binary file, opened as "wb", for output. 
 * @param vertexCount: Expected number of vertices to process.
 *
 * @return: TRUE if the method succeeds, FALSE otherwise.
 */
int processVertices(xmlNode *n, FILE *outfile, uint32 vertexCount)
{
	xmlNode *v;
	uint32 counted_vertices = 0;
	float32 p[3];
	
	/* check we've been passed a <Vertices> element */
	assert(n->type == XML_ELEMENT_NODE);
	assert(strcmp((char*)n->name, "Vertices") == 0);

	/* iterate over all <Vertex> children */
	for (v=n->children; v; v=v->next)
	{
		if ((v->type == XML_ELEMENT_NODE) &&
			(strcmp((char*)v->name, "Vertex") == 0))
		{
			counted_vertices++;
			p[0] = (float32)atof((char*)xmlGetProp(v,
				(xmlChar*)"x"));
			p[1] = (float32)atof((char*)xmlGetProp(v,
				(xmlChar*)"y"));
			p[2] = (float32)atof((char*)xmlGetProp(v,
				(xmlChar*)"z"));
			fwrite(p, 3*sizeof(float32), 1, outfile);
		}
	}

	/* check that the correct number of vertices were processed */
	if (counted_vertices != vertexCount)
	{
		printf("Found %d vertices, expected %d!\n",
			counted_vertices, vertexCount);
		return FALSE;
	}
	
	return TRUE;
}



/**
 * Writes normals into the binary file.
 *
 * @param n:           <Normals> element.
 * @param outfile:     Binary file, opened as "wb", for output.
 * @param vertexCount: Expected number of normals to process.
 *
 * @return: TRUE if the method succeeds, FALSE otherwise.
 */
int processNormals(xmlNode *n, FILE *outfile, uint32 vertexCount)
{
	xmlNode *v;
	uint32 counted_normals = 0;
	float32 p[3];

	/* check we've been passed a <Normals> element */
	assert(n->type == XML_ELEMENT_NODE);
	assert(strcmp((char*)n->name, "Normals") == 0);

	/* iterate over all <Normal> elements */
	for (v=n->children; v; v=v->next)
	{
		if ((v->type == XML_ELEMENT_NODE) &&
			(strcmp((char*)v->name, "Normal") == 0))
		{
			counted_normals++;
			p[0] = (float32)atof((char*)xmlGetProp(v,
				(xmlChar*)"x"));
			p[1] = (float32)atof((char*)xmlGetProp(v,
				(xmlChar*)"y"));
			p[2] = (float32)atof((char*)xmlGetProp(v,
				(xmlChar*)"z"));
			fwrite(p, 3*sizeof(float32), 1, outfile);
		}
	}

	/* check that the correct number of normals were processed */
	if (counted_normals != vertexCount)
	{
		printf("Found %d normals, expected %d!\n",
			counted_normals, vertexCount);
		return FALSE;
	}
	
	return TRUE;
}



/**
 * Writes texture coordinates into the binary file.
 *
 * @param n:           <TexCoords> element.
 * @param outfile:     Binary file, opened as "wb", for output.
 * @param vertexCount: Expected number of vertices to process.
 *
 * @return: TRUE if the method succeeds, FALSE otherwise.
 */
int processTexcoords(xmlNode *n, FILE *outfile, uint32 vertexCount)
{
	xmlNode *v;
	uint32 counted_texco = 0;
	float32 p[2];

	/* check we've been passed a <TexCoords> element */
	assert(n->type == XML_ELEMENT_NODE);
	assert(strcmp((char*)n->name, "TexCoords") == 0);

	/* iterate over all <ST> children */
	for (v=n->children; v; v=v->next)
	{
		if ((v->type == XML_ELEMENT_NODE) &&
			(strcmp((char*)v->name, "ST") == 0))
		{
			++counted_texco;
			p[0] = (float32)atof((char*)xmlGetProp(v,
				(xmlChar*)"s"));
			p[1] = (float32)atof((char*)xmlGetProp(v,
				(xmlChar*)"t"));
			fwrite(p, 2*sizeof(float32), 1, outfile);
		}
	}

	/* check that the correct number of texco were processed */
	if (counted_texco != vertexCount)
	{
		printf("Found %d texture coordinates, expected %d!\n",
			counted_texco, vertexCount);
		return FALSE;
	}
	
	return TRUE;
}



/**
 * Writes indices into the binary file.
 *
 * @param n:          <Indices> element.
 * @param outfile:    Binary file, opened as "wb", for output. 
 * @param indexCount: Expected number of indices to process.
 *
 * @return: TRUE if the method succeeds, FALSE otherwise.
 */
int processIndices(xmlNode *n, FILE *outfile, uint32 indexCount)
{
	xmlNode *v;
	uint32 counted_indices = 0;
	uint32 index;

	/* check we've been passed an <Indices> element */
	assert(n->type == XML_ELEMENT_NODE);
	assert(strcmp((char*)n->name, "Indices") == 0);

	/* iterate over all <Ix> children */
	for (v=n->children; v; v=v->next)
	{
		if ((v->type == XML_ELEMENT_NODE) &&
			(strcmp((char*)v->name, "Ix") == 0))
		{
			++counted_indices;
			index = (uint32)atoi((char*)xmlGetProp(v,
				(xmlChar*)"i"));
			fwrite(&index, 1*sizeof(uint32), 1, outfile);
		}
	}

	/* check that the correct number of indices were processed */
	if (counted_indices != indexCount)
	{
		printf("Found %d indices, expected %d!\n",
			counted_indices, indexCount);
		return FALSE;
	}
	
	return TRUE;
}

