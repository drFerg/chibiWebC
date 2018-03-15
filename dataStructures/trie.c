typedef struct trie
{
	Element root;
}Trie;

struct element
{
	char c;
	int isPath;
	struct element sibling;
	struct element child;
};


int findMatch(Trie* trie, char* src) {
	return _findMatch(trie->root, src);
}

int _findMatch(Element node, char* src) {
	if (src[0] == node->c) {
		if (strlen(src) == 1){ // Matched last character
			// Check match is a complete path
			if (node->isPath) {
				return 1; 
			}
			else {
				return 0;
			}
		} // Continue search down to children
		else if (node->child) {
			return _findMatch(node->child, src + 1);
		} 
		else { // src chars left but no child to search
			return 0;
		}
	} 
	else if (node->sibling) { // Not matched against current char, try siblings...
		return _findMatch(node->sibling, src);	
	} 
	else { // No match found
		return 0;
	}
}