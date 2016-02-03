#include <iostream>
#include <fstream>

#ifdef WIN32
#include <string>
#include <windows.h>
#define CLEAR_SCREEN "cls"
#define PAUSE "pause"
#else
#include <string.h>
#include <stdlib.h>
#define CLEAR_SCREEN "clear"
#define PAUSE "read pause"
#endif

using namespace std;

string save_comment(string line, string param, string new_value) {
	bool write_stat = false; // Переменная, для проверки статуса записи
	string banka; // Объявление переменной для сохранения комментария
	banka.clear(); // Очистка переменной
	int line_size = line.size(); // Для определения размера лини
	for (int i = 0; i <= line_size; i++) {
		if ((line[i - 1] != '/') && (line[i] == '#')) { // Если найден знак комментария и перед ним нет знака экранирования
			write_stat = true; // Разрешение записи комментария в переменную
			banka = " "; // Вставка Пробела перед знаком комментария
		}
		if (write_stat == true)
			banka += line[i]; // Запись комментария в переменную
	}
	return (param + " = "  + new_value + banka); // Возвращение целой строки
}

//

string cut_1(string line/* сам текст */, char sign /*До или после какого знака вернуть текст*/) // CUT ver: 3.0
{
    string banka = ""; // Временная переменная
    int dlinaLine = line.length(); // Запись дляны строчки
	for (int i = 0; i <= dlinaLine; i++) {
		if (((i + 1) < dlinaLine) && (line[i] == ' ') && (line[i + 1] == ' ')) // Если много пробелов
			continue; // Если да, то пропуск
		if ((i + 2) <= dlinaLine) { // Если не достигнут конец строки и остается ещё два символа
			if ((line[i + 1] == ' ') && (line[i + 2] == sign)) { // Определение есть ли пробел и знак препинания после него
				banka = banka + line[i]; break; } else // Если да, то остановка
			if ((line[i] == ' ') && (line[i + 1] == sign)) // Если текущий знак пробел, а следующий знак препинания
				{ break; }; // Если да, то остановка
		}
		if (!(banka[0]) && (line[i] == ' ')) continue; // Если перед переменной пробел
		if (line[i] == ' ') return "0"; // Если в середине переменной найден пробел, вернуть 0
		if (line[i] != sign) // Если не знак препинания
			banka = banka + line[i]; // Помещене знака в переменную
		else break; // Если знак препинания - остановиться
	}
	return banka;
}

//

int count_lines(string linkToconf) {
	string readText;
	int count = 0; // Счётчик кол-ва строк
	ifstream i(linkToconf.c_str()); // Открытие файла
	while(true) {
        getline(i, readText);
		if (i) { // Если не было ошибки считывания
			count++; // То инкримент счётчика
		} else break; // Если была ошибка, то остановка цикла
	}
	i.close(); // Закрытие файла
	return (count - 1); // Возврат результата на единицу меньше, так как массив начинается с нуля
}

void edit_conf(string linkToconf, string parametr, string new_value) {
	FILE *fp;
	if ((fp = fopen(linkToconf.c_str(), "r")) == NULL)
		exit(-1);
	fclose(fp);
	string readText;
	int count_l = 0, coord_line = -1, all_lines = count_lines(linkToconf);
	if (all_lines == -1) { // Если в конфигурационном файле пусто
		system(CLEAR_SCREEN);
		cout << "CAN'T READ CONFIGURATION FILE!!!" << endl;
		system(PAUSE);
		exit(0);
	}
	ifstream i(linkToconf.c_str()); // Открытие файла для чтения
	string *lines = new string[all_lines + 1]; // Создание массива для заполнения, с запасом в одну ячейку
	while(true) {
        getline(i, readText);
		if (i) {
			if(cut_1(readText, '=') == parametr) { // Если найдена искомая строчка
                coord_line = count_l; // То запоминается её координата
			}
			lines[count_l] = readText; // Заполнение массивва
		} else break;
		count_l++; // Инкримент счётчика линий (для массива)
	}
	i.close(); // Закрытие файла
	if (coord_line == -1) { cout << "ERROR" << endl; return; } // Если не был найден искомый параметр
	//lines[coord_line].clear();
	lines[coord_line] = save_comment(lines[coord_line], parametr, new_value); // Внесение нового значчения с сохранением комментария
	ofstream o(linkToconf.c_str()); // Открытие файла для записи
    for(int i = 0; i <= all_lines; i++) {
		o << lines[i].c_str() << endl; // Запись
    }
	o.close(); // Закрытие файла
	delete [] lines;
	return;
}
