#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <expat.h>

#define FILEBUFF_SIZE  4096
#define CHARBUFF_SIZE  4096
#define NAME_LIST_INCREMENT 50

struct xml_info {
  unsigned int pages;
  unsigned int redirects;
  XML_Char **names;
  int name_ind;  /* Also == total of name list */

  //XML_Char *charbuf;
  //int charbuf_size;
  //int charbuf_offset;
};


static void
clear_data(struct xml_info *data)
{
  int i;
  for (i=0; i < data->name_ind; i++)
    free(data->names[i]);
  free(data->names);
  //  free(data);
}


static int
already_found_name(const XML_Char *name, struct xml_info *data)
{
  int i = 0;
  while (i < data->name_ind) {
    if (strcmp(data->names[i], name) == 0)
      return 1;
    i++;
  }
  return 0;
}


static void
add_to_names(const XML_Char *name, struct xml_info *data)
{
  if (!(data->name_ind % NAME_LIST_INCREMENT)) {
    //printf("incrementing namelist size\n");
    XML_Char **incremented_list;
    if (!(incremented_list = malloc(sizeof(XML_Char *) * 
				    (data->name_ind + NAME_LIST_INCREMENT)))) {
      printf("error incrementing namelist size\n");
      clear_data(data);
      exit(EXIT_FAILURE);
    }
    int i;
    for (i=0; i<data->name_ind; i++)
      incremented_list[i] = data->names[i];
    free(data->names);
    data->names = incremented_list;
  }

  int len = strlen(name) + 1;
  if (!(data->names[data->name_ind] = malloc(sizeof(XML_Char) * len))) {
    fprintf(stderr, "Error in malloc'ing space for tag name '%s'\n", name);
    clear_data(data);
    exit(EXIT_FAILURE);
  }
  strcpy(data->names[data->name_ind], name);
  data->name_ind++;
}


static void XMLCALL
startElement(void *userData, const XML_Char *name, const XML_Char **attrs)
{
  struct xml_info *start_data = (struct xml_info *) userData;

  /* Necessary to check depth to make sure redirect inside page??? */
  if (strcmp(name, "page") == 0)
    start_data->pages++;
  else if (strcmp(name, "redirect") == 0)
    start_data->redirects++;

  if (! already_found_name(name, start_data))
    add_to_names(name, start_data);
}  // startElement


int main(int argc, char **argv)
{
  if (argc < 3 || strcmp(argv[1], "--help") == 0) {
    printf("usage: %s <file to parse> <tag list output file>\n", argv[0]);
    exit(0);
  }

  int xml_fd;
  if ((xml_fd = open(argv[1], O_RDONLY)) == -1) {
    printf("Unable to open file %s for parsing\n", argv[1]);
    exit(EXIT_FAILURE);
  }
  /* Initialize user data and parser */
  struct xml_info userdata;
  userdata.pages = userdata.redirects = userdata.name_ind = 0;
  if ((userdata.names =
       malloc(sizeof(XML_Char *) * NAME_LIST_INCREMENT)) == 0) {
    printf("could not initialize namelist\n");
    clear_data(&userdata);
    exit(EXIT_FAILURE);
  }
  XML_Parser parser = XML_ParserCreate("UTF-8");
  XML_SetUserData(parser, &userdata);
  XML_SetElementHandler(parser, startElement, NULL);

  /* Loop through data by blocks */
  printf("Progress:\n");
  const char *monitor_string = "\rBlocks:%12u  Pages:%12u  \
Redirects:%10u  Names:%4d";
  unsigned int mon;
  for (mon=0;;mon++) {
    void *buf = XML_GetBuffer(parser, FILEBUFF_SIZE);
    if (buf == NULL) {
      fprintf(stderr, "Error in XML_GetBuffer\n");
      exit(EXIT_FAILURE);
    }

    int bytes_read;
    bytes_read = read(xml_fd, buf, FILEBUFF_SIZE);
    if (bytes_read < 0){
      printf("Error reading from xml file\n");
      exit(EXIT_FAILURE);
    }

    if (! XML_ParseBuffer(parser, bytes_read, bytes_read == 0)) {
      printf("Error parsing buffer\n");
      exit(EXIT_FAILURE);
    }

    /* Show progress */
    if (mon % 2000 == 0) {
      printf(monitor_string, mon, userdata.pages,
	     userdata.redirects, userdata.name_ind);
      fflush(stdout);
    } 

    if (bytes_read == 0)
      break;
  }  /* parsing (for) loop */
  close(xml_fd);

  /* Output tag list to file */
  FILE *f;
  printf("\nwriting namelist to %s\n", argv[2]);
  if ((f = fopen(argv[2], "w")) == NULL) {
    printf("Unable to open file %s for writing tag names\n", argv[2]);
    clear_data(&userdata);
    exit(EXIT_FAILURE);
  }
  int i;
  for (i=0; i<userdata.name_ind; i++)
    fprintf(f, "%s\n", userdata.names[i]);

  printf("tidying up...\n");
  fclose(f);
  clear_data(&userdata);
  XML_ParserFree(parser);
  return 0;
}  /* main */
