/*Small program to fix my ebook that has all instances of "ll" transformed into "l "*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#define dictwordcount 126643 /*Both of these defines do not need to be exact, however they must be >= the real value*/
#define dictmaxwordlength 40

/*Define which scenario has occurred, we should either do nothing, input an l, or input an l and join the two words*/
enum action {nothing, addl, joinl};

/*Arguments for the analysis function, which are the file name to look at and the dictionary to compare to*/
struct argsforanalysis{
	char Fname[21];
	char** dictionaryptr;
	unsigned int numwords;
};

/*Typical quicksort implementation, based on http://c.happycodings.com/code-snippets/a-quicksort-for-strings.html*/
void quicksort(char** dict, unsigned int left, unsigned int right){
	unsigned int i = left;
	unsigned int j = right;
	char swap[dictmaxwordlength];
	char pivot[dictmaxwordlength];
	strcpy(pivot,dict[(left+right)>>1]);/*Good pivot choice for my somewhat sorted list*/
	do{
		while(i<right && strcmp(dict[i],pivot)<0){/*<0 means already ordered*/
			i++;
		}
		while(j>left && strcmp(dict[j],pivot)>0){
			j--;
		}

		if(i<j){
			/*Found one greater than pivot, and one less than it*/
			strcpy(swap,dict[j]);
			strcpy(dict[j],dict[i]);
			strcpy(dict[i], swap);
			i++;
			j--;
		}
		if(i==j){
			j--;
			i++;
		}
	} while(i<=j);
	if(left < j){/*Recurse on the left side*/
		quicksort(dict,left,j);
	}
	if(i < right){/*Recurse on the right side*/
		quicksort(dict,i,right);
	}
	return;
}

/*Simply returns if the word is in the sorted dictionary list*/
_Bool wordfound(char* word, char** dictlist, unsigned int numwords){

	unsigned int highindex = numwords;
	unsigned int lowindex = 0;
	int temp = 0;
	while(1){
		temp = strcmp(word, dictlist[(highindex+lowindex)>>1]);
		
		if(!temp){/*Found the word*/
			return 1;
		}
		else if (lowindex == highindex || lowindex+1 == highindex){
			break;
		}
		else if (temp < 0){
			highindex = (highindex+lowindex)>>1;
		}
		else{
			lowindex = (highindex+lowindex)>>1;
		}
	}
	return 0;
}

/*Determines if the connection of the two words in some way with an l is in the sorted dictionary*/
/*Return value indicates which action to take: join the words, or add an l, or do nothing*/
enum action wordindict(char* word1, char* word2, char** dictlist, unsigned int numwords){
	/*Sometimes the first word is capitalized and will NOT be in the dictionary because of it, so we force downcast it*/
	const unsigned short word1len = strlen(word1);
	const unsigned short word2len = strlen(word2);
	char combined[word1len+word2len+2];/*Need room to add an l and the '\0'*/
	char firstl[word1len+2];/*Need room to add the l and the '\0'*/
	char lccombined[word1len+word2len+2];/*Holds the result of the forced lowercase*/
	char lcfirstl[word1len+2]; 

	_Bool islower = word1[0] >= 'a' && word1[0] <= 'z';
	strcpy(firstl, word1);
	firstl[word1len] = 'l';
	firstl[word1len+1] = '\0';
	strcpy(lcfirstl, firstl);
	lcfirstl[0] += 'a'-'A';

	strcpy(combined, firstl);
	strcat(combined, word2);
	strcpy(lccombined, combined);
	lccombined[0] += 'a'-'A';
	if(wordfound(combined, dictlist, numwords) || (!islower && wordfound(lccombined, dictlist, numwords))){
		return joinl;
	}
	else if(wordfound(firstl, dictlist, numwords) || (!islower && wordfound(lcfirstl, dictlist, numwords))){
		return addl;
	}
	else{
		return nothing;
	}
}

/*Reads in a file and creates a file with the "l " occurrences replaced with "ll" if the combined word is in the dictionary*/
void* analyzeFile(void* args){
	struct argsforanalysis *input = (struct argsforanalysis*) args;
	char* Fname = input->Fname;
	char** dictionaryptr = input->dictionaryptr;
	unsigned int numwords = input->numwords;

	/*The output file name is the input file name with a .mod extension*/	
	char combined[strlen(Fname)+4];
	strcpy(combined,Fname);
	strcat(combined,".mod");
	
	FILE* currentfile = NULL;
	FILE* outputfile = NULL;
	
	/*Variables for analysis*/
	char line[10000] = "";	
	unsigned int linesize = 0;
	unsigned int wordlen = 0;
	unsigned int jcopy = 0;
	unsigned int k = 0;
	enum action dictresult = nothing;

	char joinword1[100]; /*The right side word of the occurrence of "l "*/
	char joinword2[100]; /*The left side word of the occurrence of "l "*/

	
	outputfile = fopen(combined,"w");
	currentfile = fopen(Fname,"r");

	while(fgets(line, 100000, currentfile)){
		linesize = strlen(line);
		for(int j = 1; j<linesize; j++){
			/*Possible candidate for replacement*/
			if(line[j-1] == 'l' && line[j] == ' '){
				wordlen = 0;
				jcopy = j;
				/*Loop determines if the character to the left of the index is alphabetical*/
				while((line[jcopy-1] >= 'a' && line[jcopy-1] <= 'z') || (line[jcopy-1] >= 'A' && line[jcopy-1] <= 'Z')){
					jcopy--;
					wordlen++;
				}
				/*We are now at the first letter in the word to the left of the space*/
				for(k = 0; k<wordlen; k++){
					joinword1[k] = line[jcopy++];
				}
				joinword1[k] = '\0';
				/*Now we look at the word on the right hand of the space*/	
				jcopy = j;
				k = 0;
				while((line[jcopy+1] >= 'a' && line[jcopy+1] <= 'z') || (line[jcopy+1] >= 'A' && line[jcopy+1] <= 'Z')){
					joinword2[k++] = line[++jcopy];
				}
				joinword2[k] = '\0';
				dictresult = wordindict(joinword1, joinword2, dictionaryptr, numwords);
				switch (dictresult){
					case joinl:
						/*Combining the words has resulted in a real word. Merge them.*/
						line[j] = 'l';
						break;
					case addl:
						/*Add an l and move the rest of the line over*/
						line[j] = 'l';
						memmove(&(line[j+2]), &(line[j+1]),linesize-j);
						line[j+1] = ' ';
						linesize++;
						break;
					case nothing:
						break;
					default:
						break;

				}
			}	
		}

		fprintf(outputfile, "%s", line);
	}
	fclose(currentfile);
	fclose(outputfile);
	/*printf("Done: %s\n",Fname);*/
	return NULL;
}

/*Augments the given filename by adding 1 to the current file name*/
void nextfilename(char* s){
	/*s should be of the form: "index_split_XXX.html", we will increment it*/
	if(s[14] != '9'){
		s[14]++;
	}
	else{
		s[14] = '0';
		if(s[13] != '9'){
			s[13]++;
		}
		else{
			s[12]++;
			s[13] = '0';
		}
	}
	return;
}

int main(){

	/*The base name of the html guts of the ebook, all other files are increments*/
	char htmlfilename[] = "index_split_000.html";
	
	/*Variables for the dictionary*/
	char individualdictword[dictmaxwordlength];
	FILE* dictwords = NULL;
	char** dictionaryptr = malloc(dictwordcount * sizeof(char*));
	
	/*My dictionary list happens to be in a file called words.txt, with at most dictwordcount words, and isn't sorted*/
	dictwords = fopen("words.txt","r");

	unsigned int counter = 0;
	/*Reads from the file into the dictionary*/
	while(fgets(individualdictword, 100, dictwords)){
		/*The read includes the newline, we will replace it with a '\0'*/	
		dictionaryptr[counter] = malloc(sizeof(char)*dictmaxwordlength);
		individualdictword[strlen(individualdictword)-1] = '\0';
		strcpy(dictionaryptr[counter],individualdictword);
		counter++;
	}
	fclose(dictwords);
	quicksort(dictionaryptr, 0, counter-1);

	/*Creating one thread per file*/
	struct argsforanalysis args[1000];/*Probably don't need this many, but this is the max amount the naming scheme holds*/
	pthread_t threadids[999];/*Let us hope that there are much less than 999 threads...*/
	unsigned short int numthreads = 0;
	FILE* htmlfile = NULL;

	/*Arguments for the main thread*/
	strcpy(args[0].Fname, htmlfilename);
	args[0].dictionaryptr = dictionaryptr;
	args[0].numwords = counter;

	nextfilename(htmlfilename);	

	/*Creates threads while the filename is not empty*/
	while((htmlfile = fopen(htmlfilename,"r")) != NULL){
		fclose(htmlfile);/*Closing it here so the worker thread can use it*/
		numthreads++;
		strcpy(args[numthreads].Fname, htmlfilename);
		args[numthreads].dictionaryptr = dictionaryptr;
		args[numthreads].numwords = counter;
		pthread_create(&threadids[numthreads-1], NULL, analyzeFile, (void*) &args[numthreads]);
		nextfilename(htmlfilename);
	}
	
	/*And something for the main thread to do while the others finish*/
	analyzeFile((void*) &args[0]);
	
	/*Join all threads and free memory*/
	void* retval = NULL;
	for(int i = 0; i < numthreads; i++){
		pthread_join(threadids[i], &retval);
	}
	while(counter){
		free(dictionaryptr[--counter]);	
	}
	free(dictionaryptr);
	
	return 0;
}

