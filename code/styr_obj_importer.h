
#define MIN 50
#define MAX 8500000

char numberstr[15];
int maxcounter = 0;
int counter = 0;
int chflag2 = 0;
int pos = 0;
char chname[MIN];

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
	int v_count=0, vn_count=0, vt_count=0;
};

NameAndOrder* OSpace;

struct ObjectData {
	NameAndOrder* ObjectData;
	int count_of_objects;
};

void FreeFileMemory(void* Memory)
{
	if (Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

read_file_result ReadEntireFiles(char* Filename)
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
					FreeFileMemory(Result.Contents);
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

ObjectData CreateTheObject(char* FilePath);
void creatingvalues(char values[], datatype* f, NameAndOrder* obj);
void checkpos(int);
void stringtofloat(NameAndOrder* obj, char* numberstr);
void makepotency(float* potency, char* string);
void addnumbertoarray(NameAndOrder* PP, float num);
int markobjects(char* filedata);
void segregator(NameAndOrder* Space, int countofobj, char* filline);
void createfarray(char* str, int* pos, NameAndOrder* obj, int line);
void triangulate(char* str, int* pos, NameAndOrder* obj, int val_count);
retval GetVertexArray(NameAndOrder* obj,int line);
retval GetTextureArray(NameAndOrder* obj, int line);
retval GetNormalsArray(NameAndOrder* obj, int line);


//reading and working with data from file
ObjectData  CreateTheObject(char* FilePath) {
	char* filepath = FilePath;
	read_file_result result;
	result = ReadEntireFiles(filepath);
	char* fileline;
	fileline = (char*)result.Contents;
	ObjectData Obj = {};
	int countofobj = markobjects(fileline);
	Obj.count_of_objects = countofobj;
	Obj.ObjectData = (NameAndOrder*)VirtualAlloc(0, (countofobj * sizeof(NameAndOrder)), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	segregator(Obj.ObjectData, 0, fileline);
	return Obj;
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
			break;
		}
	}
	
	//continue after name
	for (i;data[i];i++) {
		int chflag = 0;
		if (data[i] == 'o' && data[i + 1] == ' ') {
			count++;
			segregator(Obj, count, &data[i]);
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
			createfarray(&data[i + 2], &fpos, &Obj[count], 0);
		}
		
	}
	
};

void createfarray(char* str, int* pos, NameAndOrder* obj, int line) {
	//counting of points 
	int val_counter = 0;
	
	
	for (int i = 0; str[i] != '\r' && str[i] != '\n' && str[i] != '\0';i++) {
		if (str[i] == '/') {
			val_counter++;
		}
	}
	val_counter += val_counter / 2;
	
	//controling the shape
	if (val_counter < 9) {
		printf("Error: Shape is less than triangle\n");
		return;
	}
	else if (val_counter == 9) {
		int i = 0;
		int num = 0;
		for (i; str[i] != '\r' && str[i] != '\n' && str[i] != '\0';i++) {
			
			if (isdigit(str[i])) {
				num = (num * 10) + (str[i] - '0');
			}
			else {
				obj->farray[*pos][line] = num;
				line++;
				num = 0;
			}
		}
		obj->farray[*pos][line] = num;
		*pos += 1;
	}
	else if (val_counter == 12) {
		int val_array[12] = {};
		
		int num = 0;
		int line = 0;
		for (int i = 0; str[i] != '\r' && str[i] != '\n' && str[i] != '\0';i++) {
			if (isdigit(str[i])) {
				num = (num * 10) + (str[i] - '0');
			}
			else {
				val_array[line] = num;
				line++;
				num = 0;
			}
		}
		val_array[line] = num;
		
		//first triangle
		obj->farray[*pos][0] = val_array[9];
		obj->farray[*pos][1] = val_array[10];
		obj->farray[*pos][2] = val_array[11];
		obj->farray[*pos][3] = val_array[0];
		obj->farray[*pos][4] = val_array[1];
		obj->farray[*pos][5] = val_array[2];
		obj->farray[*pos][6] = val_array[3];
		obj->farray[*pos][7] = val_array[4];
		obj->farray[*pos][8] = val_array[5];
		
		*pos += 1;
		
		//second triangle
		obj->farray[*pos][0] = val_array[3];
		obj->farray[*pos][1] = val_array[4];
		obj->farray[*pos][2] = val_array[5];
		obj->farray[*pos][3] = val_array[6];
		obj->farray[*pos][4] = val_array[7];
		obj->farray[*pos][5] = val_array[8];
		obj->farray[*pos][6] = val_array[9];
		obj->farray[*pos][7] = val_array[10];
		obj->farray[*pos][8] = val_array[11];
		
		*pos += 1;
	}
	else {
		triangulate(str, pos, obj, val_counter);
	}
	
	
	
	
	
	
}
//triangulating heavy shapes
void triangulate(char* str, int* pos, NameAndOrder* obj, int val_count) {
	
	//VERY IMPORTANT
	//VERTEXES MUST BE CLOCKVISE SHEDULED
	
	
	int x = 0;
	int y = 1;
	
	
	//initializing ingex array and givig space to it
	int* val_array;
	val_array = (int*)VirtualAlloc(0, (val_count * sizeof(int)), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	
	//finding and writing values o initialized array
	int num = 0;
	int line = 0;
	for (int i = 0; str[i] != '\r' && str[i] != '\n' && str[i] != '\0';i++) {
		if (isdigit(str[i])) {
			num = (num * 10) + (str[i] - '0');
		}
		else {
			val_array[line] = num;
			line++;
			num = 0;
		}
	}
	val_array[line] = num;
	//////////////////////////////////////////////////////////////////////////////
	//checking x and y if they values arent solid 
	//if x doest change using y-z else if y using x-z
	float x_check = 0;
	int counter = 1;
	float y_check = 0;
	//x check
	for (int i = 0;i < val_count; i += 3) {
		if (i == 0)
			x_check = obj->PPosition[val_array[i] - 1].v[x];
		else if (i > 0 && x_check == obj->PPosition[val_array[i] - 1].v[x])
			counter++;
	}
	if (counter == (val_count / 3)) {
		x = 1;
		y = 2;
	}
	else {//checking y if x is all right
		counter = 1;
		
		for (int i = 0;i < val_count; i += 3) {
			if (i == 0)
				y_check = obj->PPosition[val_array[i] - 1].v[y];
			else if (i > 0 && y_check == obj->PPosition[val_array[i] - 1].v[y])
				counter++;
		}
		if (counter == (val_count / 3)) {
			x = 0;
			y = 2;
		}
		
	}
	//CHECH IF VERTEX ARE IN CLOCKWISE OR CONTER-CLOCKWISE DIRECTION
	// the main formula A = 1/2*S(xi*yi+1 - xi+1*yi) + xN*y1 - x1*yN
	// Here we are looking for sign 
	// this formula counts the space that our figure takes and here we need only the sign of answer
	// if clockwise <0 else if conter-clockwise>0
	float clock_chech = 0;
	
	for (int count = 0; count <= (val_count - 3); count += 3) {
		if (count == (val_count - 3)) {
			clock_chech += (obj->PPosition[val_array[count] - 1].v[x] * obj->PPosition[val_array[0] - 1].v[y] -
							obj->PPosition[val_array[0]].v[x] * obj->PPosition[val_array[count] - 1].v[y]);
		}
		else {
			clock_chech += (obj->PPosition[val_array[count] - 1].v[x] * obj->PPosition[val_array[count + 3] - 1].v[y] -
							obj->PPosition[val_array[count + 3] - 1].v[x] * obj->PPosition[val_array[count] - 1].v[y]);
		}
	}
	
	
	
	//////////////////////////////////////////////////////////////////////////////
	//when array is filled and x with y are checked we are checking degrees beetween vectors
	/*for example we have 3 point 0 =(0,0), -1 = (-2,-2), 1 = (2, 0)		 0 ________1
	IMPORTANT vector a always comes from 0 to -1 and b from 0 to 1.			 /
	than we have 2 vectors a(-2,-2) and b(2, 0)								/
	and the formula is aswer = (- 2 * 0) - (-2 * 2) = 0-(-4)=4			-1 /
	IF ANSWER IS GREATER THAN 0 THAT MEANS THAT ANGLE IS LESS THAN 180
	IF ==0 THAT MEANS THAT ANGLE IS ==180 ant its not good as if it was LESS than 0
	AND IF ANGLE IS >180 WE Cant take this tiangle to triangulate*/
	
	//points in 2D after checking 
	v2 point_1 = {};
	v2 point0 = {};
	v2 point1 = {};
	v2 point_p = {};
	//2 vectors of triangle vector a = point_1 - point0    b = point1-point0
	v2 vectora = {};
	v2 vectorb = {};
	//index for deleting used verteces
	int index = val_count;
	
	for (int i = 0;;) {
		
		
		//bool check if angle is less than 180
		bool CrossVectorAngle = true;
		
		
		
		//check if index is more than 9 that means that there is no triangle, but i got through all figure and now have to re-zero  
		//till index wort == 9 what means tha all of poinst were triangulated
		if (i > index && index > 9)
			i = 0;
		
		
		
		
		//check if index is equal to 9 it means that there left onlu 1 triangle
		if (index <= 9) {
			line = 0;
			for (int i = 0;i < index;i++) {
				obj->farray[*pos][line++] = val_array[i];
			}
			*pos += 1;
			break;
		}
		
		
		//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
		//I IN THE BEGINNING
		//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
		if (i == 0) {
			//finding points
			point_1 = { obj->PPosition[val_array[index - 3] - 1].v[x], obj->PPosition[val_array[index - 3] - 1].v[y] };
			point0 = { obj->PPosition[val_array[i] - 1].v[x], obj->PPosition[val_array[i] - 1].v[y] };
			point1 = { obj->PPosition[val_array[i + 3] - 1].v[x], obj->PPosition[val_array[i + 3] - 1].v[y] };
			
			//find vector a and b
			vectora = { (point_1.x - point0.x),(point_1.y - point0.y) };
			vectorb = { (point1.x - point0.x),(point1.y - point0.y) };
			
			//checking angle and if conter-clockwise - changing the definition for angles 
			if ((vectora.x * vectorb.y - vectora.y * vectorb.x) <= 0 && clock_chech < 0) {
				CrossVectorAngle = false;
			}
			else if ((vectora.x * vectorb.y - vectora.y * vectorb.x) >= 0 && clock_chech > 0) {
				CrossVectorAngle = false;
			}
			if (CrossVectorAngle == true) {
				
				
				//checking all others poins if they are int array of triangle 
				//Point is in triangle if summ of w1 and w2 are <= 1 
				//there are special fomules for w1 and 2 that will be written here
				
				int point_counter = 0;
				
				for (int k = (i + 6);k < index - 3;k += 3) {//(point after p1; stop before p_1; step every 3 special for vertex)
					
					if (point_counter != 0) {
						i += 3;
						break;
					}
					
					point_p = { obj->PPosition[val_array[k] - 1].v[x], obj->PPosition[val_array[k] - 1].v[y] };
					float w1 = (point0.x * (point_1.y - point0.y) + (point_p.y - point0.y) * (point_1.x - point0.x) - point_p.x * (point_1.y - point0.y)) / ((point1.y - point0.y) * (point_1.x - point0.x) - (point1.x - point0.x) * (point_1.y - point0.y));
					
					float w2;
					//there are some problems with sqares, so i need ro use some fixes
					if (w1 >= 1 && (point_1.y - point0.y) == 0)
						w2 = 1.0;
					else if (w1 < 1 && w1 >= 0 && (point_1.y - point0.y) == 0)
						w2 = 0.0;
					else
						w2 = (point_p.y - point0.y - w1 * (point1.y - point0.y)) / (point_1.y - point0.y);//original formula
					
					//final check
					if (w1 < 0 || w2 < 0 || (w1 >= 0 && w2 >= 0 && (w1 + w2) > 1)) {
						
					}
					else {
						point_counter++;//leveling count of points in triangle
					}
					
				}
				
				
				
				if (point_counter == 0) {
					//writing the triangle to the list leveling pos and deleting point 0 from list
					obj->farray[*pos][0] = val_array[index - 3];
					obj->farray[*pos][1] = val_array[index - 2];
					obj->farray[*pos][2] = val_array[index - 1];
					obj->farray[*pos][3] = val_array[i];
					obj->farray[*pos][4] = val_array[i + 1];
					obj->farray[*pos][5] = val_array[i + 2];
					obj->farray[*pos][6] = val_array[i + 3];
					obj->farray[*pos][7] = val_array[i + 4];
					obj->farray[*pos][8] = val_array[i + 5];
					
					*pos += 1;
					
					//deleting the 0 point from array and marking last 3 symbols as 0
					for (int q = i; q < (index - 3); q++)
					{
						val_array[q] = val_array[q + 3];
					}
					for (int j = (index - 3);j < index;j++)
						val_array[j] = 0;
					index -= 3;
				}
				else {
					//leveling i if some of conditions are not true
					
				}
			}
			else {
				i += 3;
			}
			
		}
		//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
		//I IN THE END
		//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
		else if (i == (index - 3)) {
			point_1 = { obj->PPosition[val_array[i - 3] - 1].v[x], obj->PPosition[val_array[i - 3] - 1].v[y] };
			point0 = { obj->PPosition[val_array[i] - 1].v[x], obj->PPosition[val_array[i] - 1].v[y] };
			point1 = { obj->PPosition[val_array[0] - 1].v[x], obj->PPosition[val_array[0] - 1].v[y] };
			
			//find vector a and b
			vectora = { (point_1.x - point0.x),(point_1.y - point0.y) };
			vectorb = { (point1.x - point0.x),(point1.y - point0.y) };
			
			//checking angle and if conter-clockwise - changing the definition for angles 
			if ((vectora.x * vectorb.y - vectora.y * vectorb.x ) <= 0 && clock_chech < 0) {
				CrossVectorAngle = false;
			}
			else if ((vectora.x * vectorb.y - vectora.y * vectorb.x ) >= 0 && clock_chech > 0) {
				CrossVectorAngle = false;
			}
			if (CrossVectorAngle == true) {
				//checking all others poins if they are int array of triangle 
				//Point is in triangle if summ of w1 and w2 are <= 1 
				//there are special fomules for w1 and 2 that will be written here
				
				int point_counter = 0;
				
				for (int k = 3;k < (i - 3);k += 3) {//(point from the 0; stop before p_1; step every 3 special for vertex)
					
					if (point_counter != 0) {
						i += 3;
						break;
					}
					
					point_p = { obj->PPosition[val_array[k] - 1].v[x], obj->PPosition[val_array[k] - 1].v[y] };
					float w1 = (point0.x * (point_1.y - point0.y) + (point_p.y - point0.y) * (point_1.x - point0.x) - point_p.x * (point_1.y - point0.y)) / ((point1.y - point0.y) * (point_1.x - point0.x) - (point1.x - point0.x) * (point_1.y - point0.y));
					
					float w2;
					//there are some problems with sqares, so i need ro use some fixes
					if (w1 >= 1 && (point_1.y - point0.y) == 0)
						w2 = 1.0;
					else if (w1 < 1 && w1 >= 0 && (point_1.y - point0.y) == 0)
						w2 = 0.0;
					else
						w2 = (point_p.y - point0.y - w1 * (point1.y - point0.y)) / (point_1.y - point0.y);//original formula
					
					//final check
					if (w1 < 0 || w2 < 0 || (w1 >= 0 && w2 >= 0 && (w1 + w2) > 1)) {
						
					}
					else {
						point_counter++;//leveling count of points in triangle
					}
				}
				if (point_counter == 0) {
					//writing the triangle to the list leveling pos and deleting point 0 from list
					obj->farray[*pos][0] = val_array[i - 3];
					obj->farray[*pos][1] = val_array[i - 2];
					obj->farray[*pos][2] = val_array[i - 1];
					obj->farray[*pos][3] = val_array[i];
					obj->farray[*pos][4] = val_array[i + 1];
					obj->farray[*pos][5] = val_array[i + 2];
					obj->farray[*pos][6] = val_array[0];
					obj->farray[*pos][7] = val_array[1];
					obj->farray[*pos][8] = val_array[2];
					
					*pos += 1;
					
					//deleting the 0 point from array and marking last 3 symbols as 0
					for (int q = i; q < (index - 3); q++)
					{
						val_array[q] = val_array[q + 3];
					}
					for (int j = (index - 3);j < index;j++)
						val_array[j] = 0;
					index -= 3;
				}
				else {
					//leveling i if some of conditions are not true
					
				}
			}
			else {
				i += 3;
			}
			
			
		}
		//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
		//I IN THE MIDLE
		//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
		else {
			point_1 = { obj->PPosition[val_array[i - 3] - 1].v[x], obj->PPosition[val_array[i - 3] - 1].v[y] };
			point0 = { obj->PPosition[val_array[i] - 1].v[x], obj->PPosition[val_array[i] - 1].v[y] };
			point1 = { obj->PPosition[val_array[i + 3] - 1].v[x], obj->PPosition[val_array[i + 3] - 1].v[y] };
			
			//find vector a and b
			vectora = { (point_1.x - point0.x),(point_1.y - point0.y) };
			vectorb = { (point1.x - point0.x),(point1.y - point0.y) };
			
			//checking angle and if conter-clockwise - changing the definition for angles 
			if ((vectora.x * vectorb.y - vectora.y * vectorb.x) <= 0 && clock_chech < 0) {
				CrossVectorAngle = false;
			}
			else if ((vectora.x * vectorb.y - vectora.y * vectorb.x) >= 0 && clock_chech > 0) {
				CrossVectorAngle = false;
			}
			if (CrossVectorAngle == true) {
				//checking all others poins if they are int array of triangle 
				//Point is in triangle if summ of w1 and w2 are <= 1 
				//there are special fomules for w1 and 2 that will be written here
				
				int point_counter = 0;
				int k = 0;
				
				for (k;k < (i - 3);k += 3) {//(point in 0; stop before p_1; step every 3 special for vertex)
					
					if (point_counter != 0) {
						i += 3;
						break;
					}
					
					point_p = { obj->PPosition[val_array[k] - 1].v[x], obj->PPosition[val_array[k] - 1].v[y] };
					float w1 = (point0.x * (point_1.y - point0.y) + (point_p.y - point0.y) * (point_1.x - point0.x) - point_p.x * (point_1.y - point0.y)) / ((point1.y - point0.y) * (point_1.x - point0.x) - (point1.x - point0.x) * (point_1.y - point0.y));
					
					float w2;
					//there are some problems with sqares, so i need ro use some fixes
					if (w1 >= 1 && (point_1.y - point0.y) == 0)
						w2 = 1.0;
					else if (w1 < 1 && w1 >= 0 && (point_1.y - point0.y) == 0)
						w2 = 0.0;
					else
						w2 = (point_p.y - point0.y - w1 * (point1.y - point0.y)) / (point_1.y - point0.y);//original formula
					
					//final check
					if (w1 < 0 || w2 < 0 || (w1 >= 0 && w2 >= 0 && (w1 + w2) > 1)) {
						
					}
					else {
						point_counter++;//leveling count of points in triangle
					}
					
				}
				
				for (k = (i + 6);k <= (index - 3);k += 3) {//(point after p1; stop in the end of list; step every 3 special for vertex)
					
					if (point_counter != 0) {
						i += 3;
						break;
					}
					
					point_p = { obj->PPosition[val_array[k] - 1].v[x], obj->PPosition[val_array[k] - 1].v[y] };
					float w1 = (point0.x * (point_1.y - point0.y) + (point_p.y - point0.y) * (point_1.x - point0.x) - point_p.x * (point_1.y - point0.y)) / ((point1.y - point0.y) * (point_1.x - point0.x) - (point1.x - point0.x) * (point_1.y - point0.y));
					float w2 = (point_p.y - point0.y - w1 * (point1.y - point0.y)) / (point_1.y - point0.y);
					
					if (w1 < 0 || w2 < 0 || (w1 >= 0 && w2 >= 0 && (w1 + w2) > 1)) {
						
					}
					else {
						point_counter++;//leveling count of points in triangle
					}
				}
				if (point_counter == 0) {
					//writing the triangle to the list leveling pos and deleting point 0 from list
					obj->farray[*pos][0] = val_array[i - 3];
					obj->farray[*pos][1] = val_array[i - 2];
					obj->farray[*pos][2] = val_array[i - 1];
					obj->farray[*pos][3] = val_array[i];
					obj->farray[*pos][4] = val_array[i + 1];
					obj->farray[*pos][5] = val_array[i + 2];
					obj->farray[*pos][6] = val_array[i + 3];
					obj->farray[*pos][7] = val_array[i + 4];
					obj->farray[*pos][8] = val_array[i + 5];
					
					*pos += 1;
					
					//deleting the 0 point from array and marking last 3 symbols as 0
					for (int q = i; q < (index - 3); q++)
					{
						val_array[q] = val_array[q + 3];
					}
					for (int j = (index - 3);j < index;j++)
						val_array[j] = 0;
					index -= 3;
				}
				else {
					//leveling i if some of conditions are not true
					
				}
			}
			else {
				i += 3;
			}
			
			
		}
	}
}
//separating parts of ro strings to create values for array
void creatingvalues(char values[], datatype* f, NameAndOrder* obj) {
	int i, j;
	char number[50] = {};
	//initiluzing maxcounter
	if (flag.v == 1)
		maxcounter = 2;
	else if (flag.vn == 1)
		maxcounter = 2;
	else if (flag.vt == 1)
		maxcounter = 1;
	
	j = 0;
	
	//getting to number values of string
	for (i = 0;(values[i] != ' ') && (values[i] != '\r') && (values[i] != '\n') && (values[i] != '\0');++i) {
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
	float potency = 0.0;
	
	for (i = 0;numberstr[i] != '\0';++i) {
		if (numberstr[i] == '-')
			sign = -1;
		else if (numberstr[i] == '.')
			breakpoint = true;
		else if (breakpoint == true && isdigit(numberstr[i])) {
			numafterbreak *= 10;
			numafterbreak += (numberstr[i] - '0');
			delim *= 10;
		}
		else if (breakpoint == false && isdigit(numberstr[i])) {
			numbeforebreak *= 10;
			numbeforebreak += (numberstr[i] - '0');
		}
		else if (numberstr[i] == 'e') {
			makepotency(&potency, &numberstr[i + 1]);
			break;
		}
	}
	
	
	float fnum = 0;
	
	if (potency != 0 && potency > 0) {
		int coef = 1;
		for (int i = 1; i <= potency;i++) {
			coef *= 10;
		}
		fnum += (numbeforebreak + (numafterbreak / delim)) * sign * coef;
	}
	else if (potency != 0 && potency < 0) {
		int coef = 1;
		for (int i = -1; i >= potency;i--) {
			coef /= 10;
		}
		fnum += (numbeforebreak + (numafterbreak / delim)) * sign * coef;
	}
	else
		fnum += (numbeforebreak + (numafterbreak / delim)) * sign;
	
	addnumbertoarray(Object, fnum);
}
//if count has e the func makes it to his potency
void makepotency(float* potency, char* string) {
	int sign = 1;
	for (int i = 0;string[i] != '\0';i++) {
		if (string[i] == '-') {
			sign = -1;
		}
		else if (isdigit(string[i])) {
			*potency *= 10;
			*potency += (string[i] - '0');
		}
	}
	*potency *= sign;
}
//adding special count to special array
void addnumbertoarray(NameAndOrder* PP, float num) {
	if (flag.v == 1) {
		PP->PPosition[pos].v[counter] = num;
		PP->v_count++;
	}
	else if (flag.vn == 1) {
		PP->PPosition[pos].vn[counter] = num;
		PP->vn_count++;
	}
	else if (flag.vt == 1) {
		PP->PPosition[pos].vt[counter] = num;
		PP->vt_count++;
	}
	
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
retval GetVertexArray(NameAndOrder* obj, int line) {
	retval verteces;
	int count = 0;
	int position = 0;
	int minus_previous = 0;
	
	if (line > 0)
		for (int i = 0;i < line;i++)
		minus_previous += obj[i].v_count;
	
	minus_previous /= 3;
	
	for (int i = 0;obj[line].farray[i][0] != 0;i++) {
		count++;
	}
	verteces.count_o_vals = (count * 3 * 3);
	verteces.values = (float*)VirtualAlloc(0, (sizeof(float) * count * 3 * 3), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	for (int i = 0;i < count;i++) {
		for (int j = 0;j <= 8;j += 3)
			for (int k = 0;k <= 2;k++) {
			verteces.values[position] = obj[line].PPosition[(obj[line].farray[i][j]) - (1 + minus_previous)].v[k];//-1 because the number of array begins from 0
			position++;
		}
	}
	return verteces;
}
retval GetTextureArray(NameAndOrder* obj, int line) {
	retval verteces;
	int count = 0;
	int position = 0;
	int minus_previous = 0;
	
	if (line > 0)
		for (int i = 0;i < line;i++)
		minus_previous += obj[i].vt_count;
	
	minus_previous /= 2;
	
	
	for (int i = 0;obj[line].farray[i][0] != 0;i++) {
		count++;
	}
	verteces.count_o_vals = (count * 3 * 2);
	verteces.values = (float*)VirtualAlloc(0, (sizeof(float) * count * 3 * 2), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	for (int i = 0;i < count;i++) {
		for (int j = 1;j <= 8;j += 3)
			for (int k = 0;k <= 1;k++) {
			verteces.values[position] = obj[line].PPosition[(obj[line].farray[i][j]) - (1 + minus_previous)].vt[k];//-1 because the number of array begins from 0
			position++;
		}
	}
	return verteces;
}
retval GetNormalsArray(NameAndOrder* obj, int line) {
	retval verteces;
	int count = 0;
	int position = 0;
	int minus_previous = 0;
	
	if (line > 0)
		for (int i = 0;i < line;i++)
		minus_previous += obj[i].vn_count;
	
	minus_previous /= 3;
	
	
	for (int i = 0;obj[line].farray[i][0] != 0;i++) {
		count++;
	}
	verteces.count_o_vals = (count * 3 * 3);
	verteces.values = (float*)VirtualAlloc(0, (sizeof(float) * count * 3 * 3), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	for (int i = 0;i < count;i++) {
		for (int j = 2;j <= 9;j += 3)
			for (int k = 0;k <= 2;k++) {
			verteces.values[position] = obj[line].PPosition[(obj[line].farray[i][j]) - (1 + minus_previous)].vn[k];//-1 because the number of array begins from 0
			position++;
		}
	}
	return verteces;
}