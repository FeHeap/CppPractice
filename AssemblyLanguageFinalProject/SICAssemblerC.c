#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define opcodeFileName "opcode.txt"
#define sourceFileName "source.txt"
#define intermediateFileName "intermediate.txt"
#define LLSO "LLSO.txt"
#define objcodeFileName "objcode.txt"
#define symbolTableSize 100 //the number of the buckets in symbolTable



int mapSize; //the number of the buckets in opTable

FILE *input,*output;
struct opcUnit{
	char opc[10];
	char value[3];
	struct opcUnit* next;
}**opTable;

struct symbolUnit{
	char symbol[20];
	char position[5];
	struct symbolUnit* next;
}**symbolTable;

void setopTable();					//store the opcode in opTable(hash map, deal with the hash collision by linked list)
void freeOptable();					//to free the opTable
void freeOpUnit(struct opcUnit*);	//to free the links in the bucket(in opTable)
int hashcode(char*,int);			//Output hash code(not greater than the size of map)
void pushOP(char*,char*);			//push the opcode into opTable
char* getOPvalue(char*);			//according the opcode to search the opTable to get the value

void pass1();								//the 1st pass of the 2-pass assembler
void setsymbolTable(); 						//initialize the symbolTable(hash map, deal with the hash collision by linked list)
int pushSYM(); 								//push the symbol into the symbolTable
void freeSymbolTable(); 					//to free the symbol table
void freeSymbolUnit(struct symbolUnit*); 	//to free links in the bucket(in symbolTable)
int sXtoD(char*);							//transform the string(Hexadecimal) into the integer(Decimal)
int sDtoD(char*); 							//transform the string(Decimal) into the integer(Decimal)
char* DtosX(int); 							//transform the integer(Decimal) into the string(Hexadecimal)
int ADD(char*,char*); 						//according to the opc and varible_u deciding the size of the instruction
void clearBuf(char*,char*,char*); 			//to clear the stringBufs to ""
void showSymbles(); 						//to show the whole symbolTable

void pass2();								//the 2nd pass of the 2-pass assembler
void getAll(char*,char*,char*,char*,char*);	//get all the information of one line in intermediate file separately
char* getSP(char*);							//get the position of the symbol
void opcfprintfProcess(char*,char*,char*);	//according to varible_d, opc, varible_u to decide what the objcode the instruction will be

void addq(char*,char*,char*,char*);		//add the infromation to the queue for the output to objcode file
void deleteq();							//delete queue
void deleteAllq();						//free the whole queue

/*main function*/
int main(void){
	
	
	setopTable();
    pass1();
    showSymbles();
    
    pass2();
    
	freeOptable();
	freeSymbolTable();
	deleteAllq();
    system("PAUSE");
    return 0;
}

int hashcode(char* string, int Size){	//according to the input $string to decide the hashcode(not greater than the input $Size)
	int i;
	int hash = 0;
	int len = strlen(string)-1;
	for(i = 0; i < strlen(string); i++){
		hash += string[i]*31^(len-i);
	}
	hash = abs(hash)%Size;
	return hash;
}


//<------About opcode
void setopTable(){
	mapSize = 0; 
	char lineBuf[20];
	char chBuf;
	if((input = fopen(opcodeFileName,"r")) == NULL){
		printf("Fail to open opcode!");
		exit(0);
	}
	do{
    	fscanf(input,"%[^\n]",lineBuf);
    	mapSize++;
	}while((chBuf = fgetc(input)) != EOF);
	fclose(input);
	opTable = (struct opcUnit**)malloc(mapSize*sizeof(struct opcUnit*));
	int i;
	for(i = 0;i < mapSize;i++){
		*(opTable + i) = NULL;
	}
	input = fopen(opcodeFileName,"r");
    
    char blankBuf[10];
    char opc[10];
    char value[3];
	do{
    	fscanf(input,"%[^ \t]",opc);
    	fscanf(input,"%[ \t]",blankBuf);
    	fscanf(input,"%[^\n]",value);
    	pushOP(opc,value);
	}while((chBuf = fgetc(input)) != EOF);
	fclose(input);
}
void pushOP(char* opc,char* value){
	int index = hashcode(opc,mapSize);
	struct opcUnit* point;
	if(*(opTable + index) == NULL){
		*(opTable + index) = (struct opcUnit*)malloc(sizeof(struct opcUnit));
		point = *(opTable + index);
	}
	else{
		point = *(opTable + index);
		while(point->next != NULL){
			point = point->next;
		}
		point->next = (struct opcUnit*)malloc(sizeof(struct opcUnit));
		point = point->next;
	}
	strcpy(point->opc,opc);
	strcpy(point->value,value);
	point->next = NULL;
}

void freeOptable(){
	struct opcUnit* point;
	int i;
	for(i = 0;i<mapSize;i++){
		if(*(opTable + i) == NULL)
			continue;
		point = *(opTable + i);
		freeOpUnit(point);
	}
	free(opTable);
}
void freeOpUnit(struct opcUnit* point){
	if(point->next != NULL){
		freeOpUnit(point->next);
	}
	free(point);
}

void setsymbolTable(){
	
	symbolTable = (struct symbolUnit**)malloc(symbolTableSize*sizeof(struct symbolUnit*));
	int i;
	for(i = 0;i < symbolTableSize;i++){
		*(symbolTable + i) = NULL;
	}

}

int exist(char* opc){					//to detect whether the input $opc is valid, if is's valid return 1, else return 0
	int index = hashcode(opc,mapSize);
	struct opcUnit* point = *(opTable + index);
	while(point != NULL){
		if(!stricmp(opc,point->opc)){
			return 1;
		}
		point = point->next;
	}
	return 0;
}

char* getOPvalue(char* opcode){		//according the input $opcode to searching the opTable to return the value
	int index = hashcode(opcode,mapSize);
	struct opcUnit* point = *(opTable + index);
	while(1){
		if(!stricmp(point->opc,opcode)){
			return point->value;
		}
		point = point->next;
	}
}
//------>About opcode

//<------About symbol
int pushSYM(char* symbol,char* position){	//to push the varible into table, but if the varible has been declared before then return 1, else return 0;
	int index = hashcode(symbol,symbolTableSize);
	struct symbolUnit* point;
	if(*(symbolTable + index) == NULL){
		*(symbolTable + index) = (struct symbolUnit*)malloc(sizeof(struct symbolUnit));
		point = *(symbolTable + index);
	}
	else{
		point = *(symbolTable + index);
		if(!strcmp(symbol,point->symbol)){
			return 1;
		}
		while(point->next != NULL){
			if(!strcmp(symbol,point->symbol)){
				return 1;
			}
			point = point->next;
		}
		point->next = (struct symbolUnit*)malloc(sizeof(struct symbolUnit));
		point = point->next;
	}
	strcpy(point->symbol,symbol);
	strcpy(point->position,position);
	point->next = NULL;
	return 0;
}

void freeSymbolTable(){
	struct symbolUnit* point;
	int i;
	for(i = 0;i<symbolTableSize;i++){
		if(*(symbolTable + i) == NULL)
			continue;
		point = *(symbolTable + i);
		freeSymbolUnit(point);
	}
	free(symbolTable);
}
void freeSymbolUnit(struct symbolUnit* point){
	if(point->next != NULL){
		freeSymbolUnit(point->next);
	}
	free(point);
}
void showSymbles(){
	struct symbolUnit* point;
	int i;
	printf("symble table:\n\n");
	for(i = 0;i<symbolTableSize;i++){
		point = *(symbolTable + i);

		while(point != NULL){
			printf("%s\t%s\n",point->symbol,point->position);
			point = point->next;
		}
	}
}
char* getSP(char* symbol){
	int index = hashcode(symbol,symbolTableSize);
	struct symbolUnit* point = *(symbolTable + index);
	while(point != NULL){
		if(!stricmp(point->symbol,symbol)){
			return point->position;
		}
		point = point->next;
	}
	
	printf("the varible doesn't exist'!\n");
	freeOptable();
	freeSymbolTable();
	deleteAllq();
	exit(0);
}

//------>About symbol

//<------About translation
int sXtoD(char* sX){	//transform the string(Hexadecimal) into the integer(Decimal)
	int i;
	int num;
	for(num = 0,i = 0;i < strlen(sX);i++){
		num *= 16;
		num += (sX[i]>='0'&&sX[i]<='9')?sX[i]-'0':sX[i]+10-'A';
	}
	return num;
}
int sDtoD(char* sD){	//transform the string(Decimal) into the integer(Decimal)
	int i;
	int num;
	for(num = 0,i = 0;i < strlen(sD);i++){
		num *= 10;
		num += sD[i]-'0';
	}
	return num;
}

char* DtosX(int D){		//transform the integer(Decimal) into the string(Hexadecimal)
	char sX[5];
	int i;	
	for(i = 0;i < 4;i++){
		sX[3 - i] = (D%16 < 10)?'0'+D%16:'A'-10+D%16;
		D /= 16;
	}
	sX[4] = '\0';
	return sX;
}
//------>About translation

void clearBuf(char* varible_d,char* opc,char* varible_u){
	varible_d[0] = '\0';
	opc[0] = '\0';
	varible_u[0] = '\0';
}

char ReservedWordOfByteWord[][5] = {"BYTE","WORD","RESB","RESW"};

int ADD(char* opc,char* varible_u){	//according the input $opc, $varible_u to return the size of the instruction, but if the $opc is valid return -1
	if(exist(opc)){
		return 3;
	}
	else{
		int i;
		for(i = 0;i < 4;i++){
			if(!stricmp(ReservedWordOfByteWord[i],opc))
				break;
		}
		switch (i){
			case 0:
				if(varible_u[0] == 'X'||varible_u[0] == 'x'){
					return 1;
				}else if(varible_u[0] == 'C'||varible_u[0] == 'c'){
					return 3;
				}
			case 1:
				return 3;
			case 2:
				return sDtoD(varible_u);
			case 3:
				return 3*sDtoD(varible_u);
			default:
				return -1;
		}
	}	
}

char program[30] = {};	//the Name of the program
int start;				//the start position of the program
int programEnd;			//the end of the program
int programLength;		//the length of the program
int startflag = 1;		//if there is START line, startflag == 1, else startflag == 0

/*pass1*/
void pass1(){
	int LC;		//location counter
	
	setsymbolTable();	//initialize the symbolTable
	if((input = fopen(sourceFileName,"r")) == NULL){	//open the sourceFile with read mode
		printf("Fail to open source!");
		exit(0);
	}
	output = fopen(intermediateFileName,"w");			//open the sourceFile with write mode
	
	char Buf[50];			//to buffer the blanks and comments
	char chBuf;				//to buffer '\n'
	char varible_d[20];
    char opc[10];
    char varible_u[20];
    do{		//read the first valid line
    	fscanf(input,"%[^ \t;\n]",varible_d);
    	fscanf(input,"%[ \t]",Buf);
    	fscanf(input,"%[^ \t;\n]",opc);
    	fscanf(input,"%[ \t]",Buf);
    	fscanf(input,"%[^ \t;\n]",varible_u);
    	fscanf(input,"%[^\n]",Buf);
	}while((chBuf = fgetc(input)) != EOF&&strlen(opc) == 0);	//if it's blank line or comment line, read the next line 
	
	if(!stricmp(opc,"START")){	//if the START Location has been set in the program, set the $LC
		strcpy(program,varible_d);
		start = LC = sXtoD(varible_u);
		fprintf(output,"%04X\t%s\t%s\t%s\n",LC,varible_d,opc,varible_u);
	}
	else{						//if not, set the $LC to 0 and reopen the sourceFile
		LC = 0;
		startflag = 0;			//no start line
		fclose(input);
		input = fopen(sourceFileName,"r");
	}
	
	clearBuf(varible_d,opc,varible_u);	//clear buffer
	
	do{	//read every line
    	fscanf(input,"%[^ \t;\n]",varible_d);
    	fscanf(input,"%[ \t]",Buf);
    	fscanf(input,"%[^ \t;\n]",opc);
    	fscanf(input,"%[ \t]",Buf);
    	fscanf(input,"%[^ \t;\n]",varible_u);
    	fscanf(input,"%[^\n]",Buf);
		
    	if(!stricmp("END",opc)){
    		break;
		}
    	if(strlen(opc) != 0){					//if the line is valid, fprint in to intermediate file
    		fprintf(output,"%04X\t%s\t%s\t%s\n",LC,varible_d,opc,varible_u);
		}
		else{
			continue;
		}
		
		if(strlen(varible_d) != 0){
			if(pushSYM(varible_d,DtosX(LC))){	//push the varible into the symbolTable ,and detect the bug of duplicate varible declaration
				printf("The varible has already exist!\n");
				freeOptable();
				freeSymbolTable();
				exit(0);
			}
		}

		int add = ADD(opc,varible_u);			//According to the opc and the varible_u to decide the instruction size, which will add to the $LC
		if(add == -1){							//detect bug of invalid opcode
			printf("invalid instruction!\n");
			freeOptable();
			freeSymbolTable();
			exit(0);
		}
		LC += add;								//Location Counter update
		clearBuf(varible_d,opc,varible_u);		//clear the buffer
	}while((chBuf = fgetc(input)) != EOF);
	fprintf(output,"\t\t%s\t%s\n",opc,varible_u);	//the END Line
	
	programEnd = LC;						//store the end position of the program
	programLength = programEnd - start;		//store the length of the program
	
	fclose(input);		//close souse file
	fclose(output);		//close intermediate file
}

void getAll(char* instruction,char* location,char* varible_d,char* opc,char* varible_u){	//analyze one line instruction to assign the information to the buffer respectively
	if(instruction[4] == '\t'&&instruction[5] == '\t'){
		int i = 0,j = 0;
		for(;instruction[i] != '\t'&&instruction[i] != ' ';i++,j++){
			location[j] = instruction[i];
		}
		location[j] = '\0';
		varible_d[0] = '\0';
		i = 6,j = 0;
		for(;instruction[i]!='\t'&&instruction[i]!=' ';i++,j++){
			opc[j] = instruction[i];
		}
		opc[j] = '\0';
		for(;instruction[i] =='\t'||instruction[i]==' ';i++);
		j = 0;
		for(;instruction[i]!='\t'&&instruction[i]!=' '&&instruction[i]!='\0';i++,j++){
			varible_u[j] = instruction[i];
		}
		varible_u[j] = '\0';
	}
	else{
		int i = 0,j = 0;
		for(;i < 4;i++,j++){
			location[j] = instruction[i];
		}
		location[j] = '\0';
		
		i = 5,j = 0;
		for(;instruction[i]!='\t'&&instruction[i]!=' ';i++,j++){
			varible_d[j] = instruction[i];
		}
		varible_d[j] = '\0';
		
		for(;instruction[i] =='\t'||instruction[i]==' ';i++);
		
		j = 0;
		for(;instruction[i]!='\t'&&instruction[i]!=' ';i++,j++){
			opc[j] = instruction[i];
		}
		opc[j] = '\0';
		
		for(;instruction[i] =='\t'||instruction[i]==' ';i++);
		
		j = 0;
		for(;instruction[i]!='\t'&&instruction[i]!=' '&&instruction[i]!='\0';i++,j++){
			varible_u[j] = instruction[i];
		}
		varible_u[j] = '\0';
	}
}

//<------queue

struct link{							//to store the information of instruction for the output to objcode file
	int position;
	char varible_d[20]; //varible be declared
	char opc[10];		//opcode
	char varible_u[20];	//varible be used or imm
	struct link* next;
}*front,*rear;

void addq(char* location,char* varible_d,char* opc,char* varible_u){ //add the infromation to the queue for the output to objcode file
	if(front == NULL){
		front = rear = (struct link*)malloc(sizeof(struct link));
		rear->position = sXtoD(location);
		strcpy(rear->varible_d,varible_d);
		strcpy(rear->opc,opc);
		strcpy(rear->varible_u,varible_u);
		rear->next = NULL;
	}
	else{
		rear->next = (struct link*)malloc(sizeof(struct link));
		rear = rear->next;
		rear->position = sXtoD(location);
		strcpy(rear->varible_d,varible_d);
		strcpy(rear->opc,opc);
		strcpy(rear->varible_u,varible_u);
		rear->next = NULL;
	}
}

void deleteq(){		//delete queue
	struct link* temp = front;
	front = front->next;
	free(temp);
}

void deleteAllq(){	//free the whole queue
	if(front != NULL){
		deleteq();
	}
}
//------>queue

void opcfprintfProcess(char* varible_d,char* opc,char* varible_u){	//according to varible_d, opc, varible_u to decide the object code to output to the file
	if(exist(opc)){
		fprintf(output,"%s",getOPvalue(opc));
		if(strlen(varible_u) == 0){
			fprintf(output,"0000");
		}
		else if(varible_u[strlen(varible_u)-2] == ','&&(varible_u[strlen(varible_u)-1] == 'X'||varible_u[strlen(varible_u)-1] == 'x')){
			char varible_u_buf[20];
			strcpy(varible_u_buf,varible_u);
			varible_u_buf[strlen(varible_u_buf)-2] = '\0';
			char positionBuf[20];
			strcpy(positionBuf,getSP(varible_u_buf));
			int i;
			for(i = 0;i < 8;i++){
				if(positionBuf[0] <= '8'){
						positionBuf[0] += 1;
				}
				else if(positionBuf[0] == '9'){
					positionBuf[0] = 'A';
				}
				else{
					positionBuf[0] += 1;
					}
			}
			
			fprintf(output,"%s",positionBuf);
		}
		else{
			fprintf(output,"%s",getSP(varible_u));
		}
	}
	else{
		int i;
		for(i = 0;i < 4;i++){
			if(!stricmp(ReservedWordOfByteWord[i],opc))
				break;
		}
		switch (i){
			case 0:
				if(varible_u[0] == 'X'||varible_u[0] == 'x'){
					fprintf(output,"%c%c",varible_u[2],varible_u[3]);
				}else if(varible_u[0] == 'C'||varible_u[0] == 'c'){
					fprintf(output,"%02X%02X%02X",varible_u[2],varible_u[3],varible_u[4]);
				}
				break;
			case 1:
				fprintf(output,"%06X",sDtoD(varible_u));
				break;
			case 2:
			case 3:
			default:
				break;
		}
	}
	
}

/*pass2*/
void pass2(){
	//<------LLSO process
	input = fopen(intermediateFileName,"r");	//open the intermediate file with read mode
	output = fopen(LLSO,"w");					//open the LLSO file with write mode
	char location[5];
	char chBuf;
	char varible_d[20];
    char opc[10];
    char varible_u[20];
    char instruction[100];
    
    if(startflag){
	    fscanf(input,"%[^\n]",instruction); //read the START line
	   	fprintf(output,"%s\n",instruction);	//output the START line to the LLSO file
	   	instruction[0] = '\0';				//initialize the instruction buffer
	   	chBuf = fgetc(input);				//buffer the '\n'
	}
   	
   	front = NULL;	//initialize the front point to NULL
   	rear = NULL;	//initialize the rear point to NULL
   	
    do{
    	fscanf(input,"%[^\n]",instruction);	//read every line from the intermediate file
    	
    	getAll( instruction, location, varible_d, opc, varible_u);
    	
		if(instruction[0] == '\t')	//break, if read END line
    		break;
    		
    	if(strlen(varible_u) < 8){
    		fprintf(output,"%s\t\t",instruction);
		}
    	else{
    		fprintf(output,"%s\t",instruction);
		}
		
		if(stricmp(opc,"RESW")&&stricmp(opc,"RESB")){
			addq(location,varible_d,opc,varible_u);		//add the information of each instruction into queue except "RESW" "RESB"
		}
    	opcfprintfProcess(varible_d,opc,varible_u);		//fprintf the object code
    	
		fprintf(output,"\n");
    	instruction[0] = '\0';							//initialize the instruction buffer
	}while((chBuf = fgetc(input)) != EOF);
	
	fprintf(output,"%s\n",instruction);		//output the END line to the LLSO file
	
	fclose(input);				//close the intermediate file
	fclose(output);				//close the LLSO file
	//------>LLSO process
	
	//<------objcode process
	output = fopen(objcodeFileName,"w");							//open the objcode file with write mode
	fprintf(output,"H%s\t%06X%06X",program,start,programLength);	//fprint the Head line 
	int sum = 0;
	int len = 0;
	struct link* point = front;
	int ST = front->position;	//ST is to store the start of the position of each Text line
	fprintf(output,"\nT%06X",point->position);
	for(;point->next != NULL;point = point->next){
		sum = point->position - ST;
		if(sum > 27){
			fprintf(output,"%02X",len);
			while(front != point){
				opcfprintfProcess(front->varible_d,front->opc,front->varible_u);
				deleteq();
			}
			
			ST = front->position;
			point = front;
			len = 0;
			fprintf(output,"\nT%06X",point->position);
		}
		len += ADD(point->opc,point->varible_u);
	}
	len += ADD(point->opc,point->varible_u);
	fprintf(output,"%02X",len);
	while(front != NULL){
		opcfprintfProcess(front->varible_d,front->opc,front->varible_u);
		deleteq();
	}
	fprintf(output,"\nE%06X",start);	//fprint the End line 
	
	fclose(output);
	//------>objcode process
}
