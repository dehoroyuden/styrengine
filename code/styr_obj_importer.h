#include<iostream>
#include<string>
#include<fstream>
#include <cstdlib>
#include <Windows.h>

using namespace std;

typedef uint32_t u32;

#define MIN 50
#define MAX 2000000

char numberstr[15];
int maxcounter = 0;
int counter = 0;
int chflag2 = 0;
int pos = 0;
char chname[MIN];

struct FileData {
	NameAndOrder* Data;
	int count_of_objs;
};

struct retval {
	float* values;
	int count_o_vals;
};

//specia flag for counter
struct datatype {
	unsigned int v : 1;//ƒвоиточие после объ€влени€ означает, что
	unsigned int vn : 1;//следующа€ цифра определ€ет длину 
	unsigned int vt : 1;//выдел€емого места под значение
}flag{ 0,0,0 };

struct PBuffer {
	float v[3];
	float vt[2];
	float vn[3];
};

struct NameAndOrder {
	char* name;
	int farray[MAX][9];//чтобы сделать на квадраты надо помен€ть 9 на 12 и в функции createfstring увеличить лимит line до 12
	PBuffer PPosition[MAX];
};

struct read_file_result
{
	u32 ContentsSize;
	void* Contents;
};

//special functionf for openinf and reading the file
inline u32 SafeTruncateUInt64(uint64_t Value)
{
	// TODO(Denis): Defines for maximum values
	//Assert(Value <= 0xFFFFFFFF);
	u32 Result = (u32)Value;
	return(Result);
}

void PlatformFreeFileMemory(void* Memory)
{
	if (Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

read_file_result PlatformReadEntireFile(char* Filename)
{
	read_file_result Result = {};

	HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize))
		{
			u32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize.QuadPart, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents)
			{
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
					(FileSize32 == BytesRead))
				{
					// NOTE(Denis): File read successfully
					Result.ContentsSize = FileSize32;
				}
				else
				{
					PlatformFreeFileMemory(Result.Contents);
					Result.Contents = 0;
				}
			}
			else
			{
				// TODO(Denis): Logging
			}
		}
		else
		{
			// TODO(Denis): Logging
		}
		CloseHandle(FileHandle);
	}
	else
	{
		// TODO(Denis): Logging
	}

	return(Result);
}

FileData CreateTheObject(char* FilePath);
void creatingvalues(char values[], datatype* f, NameAndOrder* obj);
void checkpos(int);
void stringtofloat(NameAndOrder* obj, char* numberstr);
void addnumbertoarray(NameAndOrder* PP, float num);
int markobjects(char* filedata);
void segregator(NameAndOrder* Space, int countofobj, char* filline);
void createfarray(char* str, int pos, NameAndOrder* obj, int line);
retval GetVertexArray(NameAndOrder* obj, int num_of_obj);
retval GetTextureArray(NameAndOrder* obj, int num_of_obj);
retval GetNormalsArray(NameAndOrder* obj, int num_of_obj);
void CleanSpace(NameAndOrder* OSpace);

//reading and working with data from file
FileData CreateTheObject(char* filepath) {
	read_file_result result;
	result = PlatformReadEntireFile(filepath);
	char* fileline;
	fileline = (char*)result.Contents;
	int countofobj = markobjects(fileline);
	FileData OSpace = {};
	OSpace.Data = (NameAndOrder*)VirtualAlloc(0, (countofobj * sizeof(NameAndOrder)), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	segregator(OSpace.Data, 0, fileline);
	PlatformFreeFileMemory(fileline);
	return OSpace;
}
//returns the count of objects in file
int markobjects(char* filedata) {
	int i, j = 0;


	for (i = 0;filedata[i];i++) {
		if (filedata[i] == 'o' && filedata[i + 1] == ' ')
			j++;
	}
	return j;
}
//special func that controls and segregates data from file to special structures 
void segregator(NameAndOrder* Obj, int count, char* data) {
	int i, j, namesize;
	int fpos = 0;
	//finding and creating name
	for (i = 0;data[i]; i++) {
		if (data[i] == 'o' && data[i + 1] == ' ') {
			namesize = 0;
			while (data[i] != '\r' && data[i] != '\n') {
				namesize++;
				i++;
			}
			int z = 0;
			Obj[count].name = (char*)VirtualAlloc(0, (namesize * sizeof(char)), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			for (j = (i - namesize);j != (i);j++) {
				Obj[count].name[z++] = data[j];
			}
			//continue after name
			for (i;data[i];i++) {
				int chflag = 0;
				if (data[i] == 'o' && data[i + 1] == ' ') {
					break;
				}
				else if (data[i] == 'v' && data[i + 1] == ' ') {
					chflag = 2;
					checkpos(chflag);
					creatingvalues(&data[i + 2], &flag, &Obj[count]);
					counter = 0;
					pos++;
				}
				else if (data[i] == 'v' && data[i + 1] == 'n') {
					chflag = 4;
					checkpos(chflag);
					creatingvalues(&data[i + 3], &flag, &Obj[count]);
					counter = 0;
					pos++;
				}
				else if (data[i] == 'v' && data[i + 1] == 't') {
					chflag = 8;
					checkpos(chflag);
					creatingvalues(&data[i + 3], &flag, &Obj[count]);
					counter = 0;
					pos++;
				}
				else if (data[i] == 'f' && data[i + 1] == ' ' && isdigit(data[i + 2])) {
					createfarray(&data[i + 2], fpos, &Obj[count], 0);
					fpos++;
				}

			}
			count++;
		}


		for (i;data[i] != '\r'; i++) {
			if (data[i] == 'o' && data[i + 1] == ' ')
				segregator(Obj, count, &data[i]);
			break;
		}

	}

};
//fullfilling the array special for f for v
void createfarray(char* str, int pos, NameAndOrder* obj, int line) {
	int i = 0;
	int num = 0;
	for (i = 0; str[i] != '\r' && str[i] != '\n' && str[i] != '\0';i++) {
		if (line == 9) {
			return;
		}
		else if (isdigit(str[i])) {
			num = (num * 10) + (str[i] - '0');
		}
		else {
			obj->farray[pos][line] = num;
			line++;
			num = 0;
		}

	}
	obj->farray[pos][line] = num;

}
//separating parts of ro strings to create values for array
void creatingvalues(char values[], datatype* f, NameAndOrder* obj) {
	int i, j;
	char number[15] = {};
	//initiluzing maxcounter
	if (flag.v == 1)
		maxcounter = 2;
	else if (flag.vn == 1)
		maxcounter = 2;
	else if (flag.vt == 1)
		maxcounter = 1;

	j = 0;

	//getting to number values of string
	for (i = 0;(values[i] != ' ') && (values[i] != '\r');++i) {
		if (values[i] < 0)
			break;
		else if (values[i] == '\0')
			break;

		else {
			number[j++] = values[i];
		}
	};

	//making float values and adding them to array depends on counter
	if (counter <= maxcounter)
		stringtofloat(obj, number);


	//leveling counter
	counter++;

	//if line ist empty continue to create values
	if (counter <= maxcounter)
		creatingvalues(&values[i + 1], f, obj);
}
//turning data from file from tpe char to float
void stringtofloat(NameAndOrder* Object, char* numberstr) {
	int i;
	int sign = 1;
	bool breakpoint = false;
	float numbeforebreak = 0.0;
	float numafterbreak = 0.0;
	float delim = 1.0;

	for (i = 0;numberstr[i] != '\0';++i) {
		if (numberstr[i] == '-')
			sign = -1;
		else if (numberstr[i] == '.')
			breakpoint = true;
		else if (breakpoint == true) {
			numafterbreak *= 10;
			numafterbreak += (numberstr[i] - '0');
			delim *= 10;
		}
		else if (breakpoint == false) {
			numbeforebreak *= 10;
			numbeforebreak += (numberstr[i] - '0');
		}
	}

	float fnum = 0;

	fnum += (numbeforebreak + (numafterbreak / delim)) * sign;
	addnumbertoarray(Object, fnum);
}
//adding special count to special array
void addnumbertoarray(NameAndOrder* PP, float num) {
	if (flag.v == 1)
		PP->PPosition[pos].v[counter] = num;
	else if (flag.vn == 1)
		PP->PPosition[pos].vn[counter] = num;
	else if (flag.vt == 1)
		PP->PPosition[pos].vt[counter] = num;

}
//checking the type of data like v/vn/vt
void checkpos(int chflag) {
	if (chflag2 != chflag) {
		switch (chflag) {
		case 2: {
			pos = 0;
			maxcounter = 2;
			chflag2 = chflag;
			flag.v = 1;
			flag.vn = NULL;
			flag.vt = NULL;
			break;
		}
		case 4: {
			pos = 0;
			maxcounter = 2;
			chflag2 = chflag;
			flag.v = NULL;
			flag.vn = 1;
			flag.vt = NULL;
			break;
		}
		case 8: {
			pos = 0;
			maxcounter = 1;
			chflag2 = chflag;
			flag.v = NULL;
			flag.vn = NULL;
			flag.vt = 1;
			break;
		}
		}
	}
};
//returning the adress to array of floats of v/vt/vn;
retval GetVertexArray(NameAndOrder* obj, int num_of_obj) {
	retval verteces;
	int count = 0;
	int position = 0;

	for (int i = 0;obj[num_of_obj].farray[i][0] != 0;i++) {
		count++;
	}
	verteces.count_o_vals = (count * 3 * 3);
	verteces.values = (float*)VirtualAlloc(0, (sizeof(float) * count * 3 * 3), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	for (int i = 0;i < count;i++) {
		for (int j = 0;j <= 8;j += 3)
			for (int k = 0;k <= 2;k++) {
				verteces.values[position] = obj->PPosition[(obj[num_of_obj].farray[i][j]) - 1].v[k];//-1 because the number of array begins from 0
				position++;
			}
	}
	return verteces;
}
retval GetTextureArray(NameAndOrder* obj, int num_of_obj) {
	retval verteces;
	int count = 0;
	int position = 0;

	for (int i = 0;obj[num_of_obj].farray[i][0] != 0;i++) {
		count++;
	}
	verteces.count_o_vals = (count * 3 * 2);
	verteces.values = (float*)VirtualAlloc(0, (sizeof(float) * count * 3 * 2), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	for (int i = 0;i < count;i++) {
		for (int j = 1;j <= 8;j += 3)
			for (int k = 0;k <= 1;k++) {
				verteces.values[position] = obj->PPosition[(obj[num_of_obj].farray[i][j]) - 1].vt[k];//-1 because the number of array begins from 0
				position++;
			}
	}
	return verteces;
}
retval GetNormalsArray(NameAndOrder* obj, int num_of_obj) {
	retval verteces;
	int count = 0;
	int position = 0;

	for (int i = 0;obj[num_of_obj].farray[i][0] != 0;i++) {
		count++;
	}
	verteces.count_o_vals = (count * 3 * 3);
	verteces.values = (float*)VirtualAlloc(0, (sizeof(float) * count * 3 * 3), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	for (int i = 0;i < count;i++) {
		for (int j = 2;j <= 9;j += 3)
			for (int k = 0;k <= 2;k++) {
				verteces.values[position] = obj->PPosition[(obj[num_of_obj].farray[i][j]) - 1].vn[k];//-1 because the number of array begins from 0
				position++;
			}
	}
	return verteces;
}
//cleans the space given to OSpace
void CleanSpace(NameAndOrder* OSpace) {
	VirtualFree(OSpace, 0, MEM_RELEASE);
}