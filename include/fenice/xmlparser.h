#ifndef XML_PARSER_H
#define XML_PARSER_H

/* parser modes */
#define XML_PARSER_CASE_INSENSITIVE  0
#define XML_PARSER_CASE_SENSITIVE    1

/* return codes */
#define XML_PARSER_OK                0
#define XML_PARSER_ERROR             1


/* xml property */
typedef struct xml_property_s {
	char *name;
	char *value;
	struct xml_property_s *next;
} xml_property_t;

/* xml node */
typedef struct xml_node_s {
	char *name;
	char *data;
	struct xml_property_s *props;
	struct xml_node_s *child;
	struct xml_node_s *next;
} xml_node_t;

void xml_parser_init(char * buf, int size, int mode);

int xml_parser_build_tree(xml_node_t **root_node);

void xml_parser_free_tree(xml_node_t *root_node);

char *xml_parser_get_property (xml_node_t *node, const char *name);
int   xml_parser_get_property_int (xml_node_t *node, const char *name, 
				   int def_value);
int xml_parser_get_property_bool (xml_node_t *node, const char *name, 
				  int def_value);

/* for debugging purposes: dump read-in xml tree in a nicely
 * indented fashion
 */

void xml_parser_dump_tree (xml_node_t *node) ;

#endif
